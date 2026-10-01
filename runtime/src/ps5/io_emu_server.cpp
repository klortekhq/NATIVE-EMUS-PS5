#include <ps5rt/emu_server.hpp>
#include <ps5rt/io.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>

extern "C" {
int sceNetInit();
int sceNetPoolCreate(const char*, int, int);
int sceNetPoolDestroy(int);
int sceSslInit(std::size_t);
int sceSslTerm(int);
int sceHttpInit(int, int, std::size_t);
int sceHttpTerm(int);
int sceHttpCreateTemplate(int, const char*, int, int);
int sceHttpDeleteTemplate(int);
int sceHttpSetResponseHeaderMaxSize(int, std::size_t);
int sceHttpCreateConnectionWithURL(int, const char*, int);
int sceHttpDeleteConnection(int);
int sceHttpCreateRequestWithURL(int, int, const char*, std::uint64_t);
int sceHttpDeleteRequest(int);
int sceHttpAddRequestHeader(int, const char*, const char*, int);
int sceHttpSendRequest(int, const void*, std::size_t);
int sceHttpGetStatusCode(int, int*);
int sceHttpGetResponseContentLength(int, int*, std::uint64_t*);
int sceHttpGetAllResponseHeaders(int, char**, std::size_t*);
int sceHttpReadData(int, void*, std::size_t);
}

namespace ps5rt {
namespace {

constexpr int kHttpGet = 0;
constexpr int kHttpHead = 2;
constexpr int kHeaderOverwrite = 0;

struct HttpRuntime {
  std::mutex mutex{};
  int net_pool{-1};
  int ssl{-1};
  int http{-1};
  int tmpl{-1};
  bool registered{};
  std::string token{};
  std::string user_agent{"NATIVE-EMUS-PS5/1"};
};

HttpRuntime g_http{};


bool starts_with_ci(std::string_view text, std::string_view prefix) noexcept {
  if (text.size() < prefix.size())
    return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const auto a = static_cast<unsigned char>(text[i]);
    const auto b = static_cast<unsigned char>(prefix[i]);
    if (std::tolower(a) != std::tolower(b))
      return false;
  }
  return true;
}

std::string header_value(
    const char* headers,
    std::size_t size,
    std::string_view name) {
  if (!headers || !size)
    return {};

  std::string_view block(headers, size);
  std::size_t pos = 0;
  while (pos < block.size()) {
    const auto end = block.find('\\n', pos);
    auto line = block.substr(
        pos, end == std::string_view::npos ? block.size() - pos : end - pos);
    if (!line.empty() && line.back() == '\\r')
      line.remove_suffix(1);

    if (starts_with_ci(line, name) &&
        line.size() > name.size() &&
        line[name.size()] == ':') {
      auto value = line.substr(name.size() + 1);
      while (!value.empty() && (value.front() == ' ' || value.front() == '\\t'))
        value.remove_prefix(1);
      return std::string(value);
    }

    if (end == std::string_view::npos)
      break;
    pos = end + 1;
  }
  return {};
}

bool emus_to_http(std::string_view uri, std::string& out) {
  constexpr std::string_view prefix = "emus://";
  if (!uri.starts_with(prefix))
    return false;

  const auto rest = uri.substr(prefix.size());
  const auto slash = rest.find('/');
  if (slash == std::string_view::npos || slash == 0 || slash + 1 >= rest.size())
    return false;

  const auto authority = rest.substr(0, slash);
  const auto id = rest.substr(slash + 1);

  // Catalog IDs are SHA-256 hex today. Keep the transport contract strict so
  // a malformed URI cannot be turned into a different HTTP path.
  if (id.size() != 64)
    return false;
  for (const char ch : id) {
    const bool hex =
        (ch >= '0' && ch <= '9') ||
        (ch >= 'a' && ch <= 'f') ||
        (ch >= 'A' && ch <= 'F');
    if (!hex)
      return false;
  }

  if (authority.find_first_of(" \\t\\r\\n?#") != std::string_view::npos)
    return false;

  out = "http://";
  out.append(authority);
  out.append("/api/v1/files/");
  out.append(id);
  return true;
}

Result add_common_headers(int req) noexcept {
  if (!g_http.token.empty()) {
    std::string auth = "Bearer ";
    auth += g_http.token;
    const int rc = sceHttpAddRequestHeader(
        req, "Authorization", auth.c_str(), kHeaderOverwrite);
    if (rc < 0)
      return {ErrorCode::system_error, rc, "sceHttpAddRequestHeader auth failed"};
  }
  return Result::success();
}

class EmuServerReader final : public RandomAccessReader {
public:
  explicit EmuServerReader(std::string url) : url_(std::move(url)) {}

  ~EmuServerReader() override {
    std::scoped_lock lock(mutex_);
    close_connection();
  }

  Result open() noexcept {
    std::scoped_lock lock(mutex_);
    return open_locked();
  }

  Result size(std::uint64_t& out_bytes) const noexcept override {
    out_bytes = size_;
    return Result::success();
  }

  Result read_at(
      std::uint64_t offset,
      std::span<std::byte> destination,
      std::size_t& out_read) noexcept override {
    std::scoped_lock lock(mutex_);
    out_read = 0;

    if (offset > size_)
      return {ErrorCode::invalid_argument, 0, "server read offset past EOF"};
    if (destination.empty() || offset == size_)
      return Result::success();
    if (conn_ < 0)
      return {ErrorCode::system_error, 0, "server connection is closed"};

    const auto remaining = size_ - offset;
    const auto wanted64 = std::min<std::uint64_t>(
        remaining, static_cast<std::uint64_t>(destination.size()));
    if (wanted64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
      return {ErrorCode::invalid_argument, 0, "server read size overflow"};
    const auto wanted = static_cast<std::size_t>(wanted64);
    const auto end = offset + wanted64 - 1;

    char range[96]{};
    const int n = std::snprintf(
        range, sizeof(range), "bytes=%llu-%llu",
        static_cast<unsigned long long>(offset),
        static_cast<unsigned long long>(end));
    if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(range))
      return {ErrorCode::invalid_argument, 0, "server Range header overflow"};

    const int req = sceHttpCreateRequestWithURL(
        conn_, kHttpGet, url_.c_str(), 0);
    if (req < 0)
      return {ErrorCode::system_error, req, "sceHttpCreateRequestWithURL failed"};

    const auto delete_req = [&]() noexcept { (void)sceHttpDeleteRequest(req); };

    auto result = add_common_headers(req);
    if (!result) {
      delete_req();
      return result;
    }

    int rc = sceHttpAddRequestHeader(
        req, "Range", range, kHeaderOverwrite);
    if (rc < 0) {
      delete_req();
      return {ErrorCode::system_error, rc, "sceHttpAddRequestHeader Range failed"};
    }

    if (!etag_.empty()) {
      rc = sceHttpAddRequestHeader(
          req, "If-Range", etag_.c_str(), kHeaderOverwrite);
      if (rc < 0) {
        delete_req();
        return {ErrorCode::system_error, rc, "sceHttpAddRequestHeader If-Range failed"};
      }
    }

    rc = sceHttpSendRequest(req, nullptr, 0);
    if (rc < 0) {
      delete_req();
      return {ErrorCode::io_error, rc, "sceHttpSendRequest failed"};
    }

    int status = 0;
    rc = sceHttpGetStatusCode(req, &status);
    if (rc < 0) {
      delete_req();
      return {ErrorCode::io_error, rc, "sceHttpGetStatusCode failed"};
    }
    if (status != 206) {
      delete_req();
      return {ErrorCode::io_error, status, "server did not honor byte range"};
    }

    while (out_read < wanted) {
      const auto chunk = wanted - out_read;
      rc = sceHttpReadData(
          req,
          destination.data() + out_read,
          chunk);
      if (rc < 0) {
        delete_req();
        return {ErrorCode::io_error, rc, "sceHttpReadData failed"};
      }
      if (rc == 0)
        break;
      out_read += static_cast<std::size_t>(rc);
    }

    delete_req();
    if (out_read != wanted)
      return {ErrorCode::io_error, 0, "short server range response"};
    return Result::success();
  }

private:
  Result open_locked() noexcept {
    conn_ = sceHttpCreateConnectionWithURL(
        g_http.tmpl, url_.c_str(), 1);
    if (conn_ < 0)
      return {ErrorCode::system_error, conn_, "sceHttpCreateConnectionWithURL failed"};

    const int req = sceHttpCreateRequestWithURL(
        conn_, kHttpHead, url_.c_str(), 0);
    if (req < 0) {
      close_connection();
      return {ErrorCode::system_error, req, "server HEAD request creation failed"};
    }

    const auto delete_req = [&]() noexcept { (void)sceHttpDeleteRequest(req); };
    auto result = add_common_headers(req);
    if (!result) {
      delete_req();
      close_connection();
      return result;
    }

    int rc = sceHttpSendRequest(req, nullptr, 0);
    if (rc < 0) {
      delete_req();
      close_connection();
      return {ErrorCode::io_error, rc, "server HEAD send failed"};
    }

    int status = 0;
    rc = sceHttpGetStatusCode(req, &status);
    if (rc < 0 || status != 200) {
      delete_req();
      close_connection();
      return {
          ErrorCode::io_error,
          rc < 0 ? rc : status,
          "server HEAD failed"};
    }

    int length_type = 0;
    std::uint64_t length = 0;
    rc = sceHttpGetResponseContentLength(req, &length_type, &length);
    if (rc < 0) {
      delete_req();
      close_connection();
      return {ErrorCode::io_error, rc, "server content length unavailable"};
    }
    size_ = length;

    char* headers = nullptr;
    std::size_t headers_size = 0;
    if (sceHttpGetAllResponseHeaders(req, &headers, &headers_size) >= 0)
      etag_ = header_value(headers, headers_size, "ETag");

    delete_req();
    return Result::success();
  }

  void close_connection() noexcept {
    if (conn_ >= 0) {
      (void)sceHttpDeleteConnection(conn_);
      conn_ = -1;
    }
  }

  mutable std::mutex mutex_{};
  std::string url_{};
  std::string etag_{};
  std::uint64_t size_{};
  int conn_{-1};
};

Result open_emu_server(
    std::string_view uri,
    OpenMode mode,
    RandomAccessReaderPtr& out) noexcept {
  out.reset();
  if (mode != OpenMode::read_only)
    return {ErrorCode::unsupported, 0, "EMUS backend is read-only"};

  std::string url;
  if (!emus_to_http(uri, url))
    return {ErrorCode::invalid_argument, 0, "invalid emus:// URI"};

  auto reader = std::make_unique<EmuServerReader>(std::move(url));
  auto result = reader->open();
  if (!result)
    return result;

  out = std::move(reader);
  return Result::success();
}

void cleanup_after_failed_init() noexcept {
  if (g_http.tmpl >= 0) {
    (void)sceHttpDeleteTemplate(g_http.tmpl);
    g_http.tmpl = -1;
  }
  if (g_http.http >= 0) {
    (void)sceHttpTerm(g_http.http);
    g_http.http = -1;
  }
  if (g_http.ssl >= 0) {
    (void)sceSslTerm(g_http.ssl);
    g_http.ssl = -1;
  }
  if (g_http.net_pool >= 0) {
    (void)sceNetPoolDestroy(g_http.net_pool);
    g_http.net_pool = -1;
  }
  g_http.token.clear();
}

} // namespace

Result initialize_emu_server_backend(const EmuServerConfig& config) noexcept {
  std::scoped_lock lock(g_http.mutex);
  if (g_http.registered)
    return Result::success();

  const int net = sceNetInit();
  if (net < 0)
    return {ErrorCode::system_error, net, "sceNetInit failed"};

  g_http.net_pool = sceNetPoolCreate("ps5rt-emus", 256 * 1024, 0);
  if (g_http.net_pool < 0) {
    const int rc = g_http.net_pool;
    cleanup_after_failed_init();
    return {ErrorCode::system_error, rc, "sceNetPoolCreate failed"};
  }

  g_http.ssl = sceSslInit(256 * 1024);
  if (g_http.ssl < 0) {
    const int rc = g_http.ssl;
    cleanup_after_failed_init();
    return {ErrorCode::system_error, rc, "sceSslInit failed"};
  }

  g_http.http = sceHttpInit(g_http.net_pool, g_http.ssl, 256 * 1024);
  if (g_http.http < 0) {
    const int rc = g_http.http;
    cleanup_after_failed_init();
    return {ErrorCode::system_error, rc, "sceHttpInit failed"};
  }

  g_http.user_agent =
      config.user_agent.empty()
          ? std::string("NATIVE-EMUS-PS5/1")
          : std::string(config.user_agent);
  g_http.token.assign(config.bearer_token);

  g_http.tmpl = sceHttpCreateTemplate(
      g_http.http, g_http.user_agent.c_str(), 2, 0);
  if (g_http.tmpl < 0) {
    const int rc = g_http.tmpl;
    cleanup_after_failed_init();
    return {ErrorCode::system_error, rc, "sceHttpCreateTemplate failed"};
  }

  (void)sceHttpSetResponseHeaderMaxSize(g_http.tmpl, 16 * 1024);

  auto registered =
      register_random_access_backend("emus", open_emu_server);
  if (!registered) {
    cleanup_after_failed_init();
    return registered;
  }

  g_http.registered = true;
  return Result::success();
}

void shutdown_emu_server_backend() noexcept {
  std::scoped_lock lock(g_http.mutex);

  if (g_http.registered) {
    (void)unregister_random_access_backend("emus", open_emu_server);
    g_http.registered = false;
  }

  cleanup_after_failed_init();
}

} // namespace ps5rt
