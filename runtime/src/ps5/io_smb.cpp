#include <ps5rt/io_smb.hpp>

#include <ps5rt/io.hpp>

#include "smb_read_ahead.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
#endif
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <string_view>

namespace ps5rt {
namespace {

constexpr const char* kConfigPath =
    "/data/NATIVE-EMUS-PS5/network/smb.ini";
constexpr int kDefaultPort = 445;
constexpr int kDefaultTimeoutSeconds = 10;
constexpr std::uint32_t kFallbackReadSize = 512u * 1024u;
constexpr std::size_t kDefaultReadAheadBytes = 512u * 1024u;
constexpr std::size_t kMaxReadAheadBytes = 8u * 1024u * 1024u;

struct SmbConfig {
  bool enabled{true};
  std::string server{};
  int port{kDefaultPort};
  std::string share{};
  std::string username{};
  std::string password{};
  std::string domain{"WORKGROUP"};
  int timeout_seconds{kDefaultTimeoutSeconds};
  std::size_t read_ahead_bytes{kDefaultReadAheadBytes};
};

struct ParsedSmbUri {
  std::string server{};
  int port{kDefaultPort};
  std::string share{};
  std::string path{};
};

std::mutex g_registration_mutex;
bool g_registered{};

std::string trim(std::string value) {
  std::size_t first = 0;
  while (first < value.size() &&
         std::isspace(static_cast<unsigned char>(value[first])))
    ++first;

  std::size_t last = value.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(value[last - 1])))
    --last;

  return value.substr(first, last - first);
}

bool equal_ascii_ci(std::string_view a, std::string_view b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const auto ca = static_cast<unsigned char>(a[i]);
    const auto cb = static_cast<unsigned char>(b[i]);
    if (std::tolower(ca) != std::tolower(cb))
      return false;
  }
  return true;
}

bool parse_bool(std::string_view value, bool fallback) noexcept {
  if (equal_ascii_ci(value, "1") ||
      equal_ascii_ci(value, "true") ||
      equal_ascii_ci(value, "yes") ||
      equal_ascii_ci(value, "on"))
    return true;
  if (equal_ascii_ci(value, "0") ||
      equal_ascii_ci(value, "false") ||
      equal_ascii_ci(value, "no") ||
      equal_ascii_ci(value, "off"))
    return false;
  return fallback;
}

void normalize_relative(std::string& path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  while (!path.empty() && path.front() == '/')
    path.erase(path.begin());
}

bool load_config(SmbConfig& config) noexcept {
  std::FILE* file = std::fopen(kConfigPath, "rb");
  if (!file)
    return false;

  char line[1024];
  while (std::fgets(line, sizeof(line), file)) {
    std::string entry = trim(line);
    if (entry.empty() || entry[0] == '#' || entry[0] == ';' ||
        entry[0] == '[')
      continue;

    const auto equals = entry.find('=');
    if (equals == std::string::npos)
      continue;

    const std::string key = trim(entry.substr(0, equals));
    const std::string value = trim(entry.substr(equals + 1));

    if (equal_ascii_ci(key, "enabled")) {
      config.enabled = parse_bool(value, config.enabled);
    } else if (equal_ascii_ci(key, "server")) {
      config.server = value;
    } else if (equal_ascii_ci(key, "port")) {
      config.port = std::clamp(std::atoi(value.c_str()), 1, 65535);
    } else if (equal_ascii_ci(key, "share")) {
      config.share = value;
    } else if (equal_ascii_ci(key, "username") ||
               equal_ascii_ci(key, "user")) {
      config.username = value;
    } else if (equal_ascii_ci(key, "password") ||
               equal_ascii_ci(key, "pass")) {
      config.password = value;
    } else if (equal_ascii_ci(key, "domain") ||
               equal_ascii_ci(key, "workgroup")) {
      config.domain = value;
    } else if (equal_ascii_ci(key, "timeout_seconds")) {
      config.timeout_seconds =
          std::clamp(std::atoi(value.c_str()), 1, 120);
    } else if (equal_ascii_ci(key, "read_ahead_kib")) {
      const auto kib =
          std::clamp(std::atoi(value.c_str()), 0, 8192);
      config.read_ahead_bytes =
          static_cast<std::size_t>(kib) * 1024u;
    }
  }
  std::fclose(file);

  if (config.server.rfind("smb://", 0) == 0)
    config.server.erase(0, 6);

  while (!config.server.empty() && config.server.back() == '/')
    config.server.pop_back();
  while (!config.share.empty() && config.share.front() == '/')
    config.share.erase(config.share.begin());
  while (!config.share.empty() && config.share.back() == '/')
    config.share.pop_back();

  return config.enabled &&
         !config.server.empty() &&
         !config.share.empty();
}

bool parse_uri(std::string_view uri, ParsedSmbUri& out) {
  constexpr std::string_view prefix = "smb://";
  if (!uri.starts_with(prefix))
    return false;

  const auto host_begin = prefix.size();
  const auto slash = uri.find('/', host_begin);
  if (slash == std::string_view::npos || slash == host_begin)
    return false;

  std::string host(uri.substr(host_begin, slash - host_begin));
  const auto colon = host.rfind(':');
  if (colon != std::string::npos && colon + 1 < host.size()) {
    const auto port_text = host.substr(colon + 1);
    const bool digits =
        !port_text.empty() &&
        std::all_of(
            port_text.begin(), port_text.end(),
            [](char c) { return c >= '0' && c <= '9'; });
    if (digits) {
      out.port = std::clamp(std::atoi(port_text.c_str()), 1, 65535);
      host.resize(colon);
    }
  }
  out.server = std::move(host);

  const auto share_end = uri.find('/', slash + 1);
  if (share_end == std::string_view::npos) {
    out.share = std::string(uri.substr(slash + 1));
  } else {
    out.share =
        std::string(uri.substr(slash + 1, share_end - slash - 1));
    out.path = std::string(uri.substr(share_end + 1));
  }

  normalize_relative(out.path);
  return !out.server.empty() &&
         !out.share.empty() &&
         !out.path.empty();
}

bool same_endpoint(
    const SmbConfig& config,
    const ParsedSmbUri& uri) noexcept {
  return equal_ascii_ci(config.server, uri.server) &&
         equal_ascii_ci(config.share, uri.share) &&
         config.port == uri.port;
}

smb2_context* connect_share(const SmbConfig& config) noexcept {
  smb2_context* ctx = smb2_init_context();
  if (!ctx)
    return nullptr;

  smb2_set_timeout(ctx, config.timeout_seconds);
  smb2_set_security_mode(ctx, 0);
  smb2_set_user(
      ctx, config.username.empty() ? "" : config.username.c_str());
  smb2_set_domain(ctx, config.domain.c_str());
  smb2_set_password(
      ctx,
      (config.username.empty() && config.password.empty())
          ? nullptr
          : config.password.c_str());

  std::string server = config.server;
  if (config.port != kDefaultPort)
    server += ":" + std::to_string(config.port);

  if (smb2_connect_share(
          ctx, server.c_str(), config.share.c_str(), nullptr) != 0) {
    smb2_destroy_context(ctx);
    return nullptr;
  }

  return ctx;
}

class SmbReader final : public RandomAccessReader {
public:
  SmbReader(SmbConfig config, std::string path) noexcept
      : config_(std::move(config)),
        path_(std::move(path)),
        read_ahead_(std::min(
            config_.read_ahead_bytes,
            kMaxReadAheadBytes)) {}

  ~SmbReader() override {
    std::scoped_lock lock(mutex_);
    close_locked();
  }

  bool open() noexcept {
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
    out_read = 0;
    if (destination.empty())
      return Result::success();

    std::scoped_lock lock(mutex_);
    if (!ctx_ || !fh_)
      return {ErrorCode::io_error, 0, "SMB file is not open"};

    if (offset >= size_)
      return Result::success();

    const auto wanted = static_cast<std::size_t>(
        std::min<std::uint64_t>(
            destination.size(),
            size_ - offset));

    if (read_ahead_.copy(
            offset,
            destination,
            wanted,
            out_read)) {
      return Result::success();
    }

    const auto remaining = static_cast<std::size_t>(
        std::min<std::uint64_t>(
            size_ - offset,
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max())));
    const auto fetch_size =
        read_ahead_.fetch_size(wanted, remaining);

    if (fetch_size <= wanted) {
      read_ahead_.clear();
      return read_remote_locked(
          offset,
          destination.first(wanted),
          out_read);
    }

    std::vector<std::byte> fetched_bytes(
        fetch_size,
        std::byte{});
    std::size_t fetched = 0;
    auto result = read_remote_locked(
        offset,
        std::span<std::byte>{
            fetched_bytes.data(),
            fetched_bytes.size()},
        fetched);
    if (!result) {
      read_ahead_.clear();
      return result;
    }

    read_ahead_.store(
        offset,
        std::move(fetched_bytes),
        fetched);

    if (!read_ahead_.copy(
            offset,
            destination,
            wanted,
            out_read)) {
      read_ahead_.clear();
      return {
          ErrorCode::io_error,
          0,
          "SMB read-ahead cache fill mismatch"};
    }

    return Result::success();
  }

private:
  Result read_remote_locked(
      std::uint64_t offset,
      std::span<std::byte> destination,
      std::size_t& out_read) noexcept {
    out_read = 0;
    auto* dst =
        reinterpret_cast<std::uint8_t*>(
            destination.data());
    std::size_t done = 0;
    bool retried = false;

    while (done < destination.size()) {
      std::uint32_t max_read =
          smb2_get_max_read_size(ctx_);
      if (max_read == 0)
        max_read = kFallbackReadSize;

      const auto request =
          static_cast<std::uint32_t>(
              std::min<std::size_t>(
                  destination.size() - done,
                  max_read));

      const int rc = smb2_pread(
          ctx_,
          fh_,
          dst + done,
          request,
          offset + done);

      if (rc < 0) {
        if (!retried && reconnect_locked()) {
          retried = true;
          continue;
        }
        return {
            ErrorCode::io_error,
            rc,
            "SMB pread failed"};
      }

      if (rc == 0)
        break;

      done += static_cast<std::size_t>(rc);
      retried = false;
    }

    out_read = done;
    return Result::success();
  }

  bool open_locked() noexcept {
    ctx_ = connect_share(config_);
    if (!ctx_)
      return false;

    fh_ = smb2_open(ctx_, path_.c_str(), O_RDONLY);
    if (!fh_) {
      close_locked();
      return false;
    }

    smb2_stat_64 stat{};
    if (smb2_fstat(ctx_, fh_, &stat) != 0) {
      close_locked();
      return false;
    }

    size_ = static_cast<std::uint64_t>(stat.smb2_size);
    return size_ != 0;
  }

  void close_locked() noexcept {
    read_ahead_.clear();
    if (ctx_ && fh_)
      (void)smb2_close(ctx_, fh_);
    fh_ = nullptr;

    if (ctx_)
      smb2_destroy_context(ctx_);
    ctx_ = nullptr;
  }

  bool reconnect_locked() noexcept {
    close_locked();
    return open_locked();
  }

  SmbConfig config_{};
  std::string path_{};
  detail::SmbReadAheadCache read_ahead_{0};
  smb2_context* ctx_{};
  smb2fh* fh_{};
  std::uint64_t size_{};
  mutable std::mutex mutex_{};
};

Result open_smb(
    std::string_view uri,
    OpenMode mode,
    RandomAccessReaderPtr& out) noexcept {
  out.reset();

  if (mode != OpenMode::read_only)
    return {ErrorCode::unsupported, 0, "SMB backend is read-only"};

  ParsedSmbUri parsed{};
  if (!parse_uri(uri, parsed))
    return {ErrorCode::invalid_argument, 0, "invalid SMB URI"};

  SmbConfig config{};
  if (!load_config(config))
    return {ErrorCode::permission_denied, 0, "SMB backend not configured"};

  if (!same_endpoint(config, parsed))
    return {
        ErrorCode::permission_denied, 0,
        "SMB URI endpoint is not authorized by smb.ini"};

  auto reader = std::make_unique<SmbReader>(
      std::move(config), std::move(parsed.path));
  if (!reader->open())
    return {ErrorCode::io_error, 0, "SMB open failed"};

  out = std::move(reader);
  return Result::success();
}

} // namespace

Result initialize_smb_backend() noexcept {
  std::scoped_lock lock(g_registration_mutex);
  if (g_registered)
    return Result::success();

  const auto result =
      register_random_access_backend("smb", &open_smb);
  if (result)
    g_registered = true;
  return result;
}

void shutdown_smb_backend() noexcept {
  std::scoped_lock lock(g_registration_mutex);
  if (!g_registered)
    return;

  (void)unregister_random_access_backend("smb", &open_smb);
  g_registered = false;
}

} // namespace ps5rt
