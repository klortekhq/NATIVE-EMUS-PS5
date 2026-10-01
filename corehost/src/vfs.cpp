#include <corehost/vfs.hpp>

#include <ps5rt/io.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>

namespace corehost {
namespace {

struct ReadOnlyHandle {
  std::string path{};
  ps5rt::RandomAccessReaderPtr reader{};
  std::uint64_t size{};
  std::uint64_t position{};
};

ReadOnlyHandle* impl(lr::VfsFileHandle* handle) noexcept {
  return reinterpret_cast<ReadOnlyHandle*>(handle);
}

const ReadOnlyHandle* impl(const lr::VfsFileHandle* handle) noexcept {
  return reinterpret_cast<const ReadOnlyHandle*>(handle);
}

const char* get_path(lr::VfsFileHandle* handle) {
  auto* h = impl(handle);
  return h ? h->path.c_str() : nullptr;
}

lr::VfsFileHandle* open(
    const char* path, unsigned mode, unsigned) {
  if (!path)
    return nullptr;

  // Do not claim any write semantics until ps5rt has an explicit writable
  // backend. This is enough for Beetle's disc/CHD/PBP reads.
  if ((mode & lr::vfs_file_access_read) == 0 ||
      (mode & lr::vfs_file_access_write) != 0)
    return nullptr;

  ps5rt::RandomAccessReaderPtr reader;
  if (!ps5rt::open_random_access(
          path, ps5rt::OpenMode::read_only, reader))
    return nullptr;

  std::uint64_t size = 0;
  if (!reader || !reader->size(size))
    return nullptr;

  auto h = std::make_unique<ReadOnlyHandle>();
  h->path = path;
  h->reader = std::move(reader);
  h->size = size;
  h->position = 0;
  return reinterpret_cast<lr::VfsFileHandle*>(h.release());
}

int close(lr::VfsFileHandle* handle) {
  delete impl(handle);
  return handle ? 0 : -1;
}

std::int64_t size(lr::VfsFileHandle* handle) {
  const auto* h = impl(handle);
  if (!h || h->size >
      static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
    return -1;
  return static_cast<std::int64_t>(h->size);
}

std::int64_t tell(lr::VfsFileHandle* handle) {
  const auto* h = impl(handle);
  if (!h || h->position >
      static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
    return -1;
  return static_cast<std::int64_t>(h->position);
}

bool add_signed(
    std::uint64_t base,
    std::int64_t delta,
    std::uint64_t& out) noexcept {
  if (delta >= 0) {
    const auto value = static_cast<std::uint64_t>(delta);
    if (value > std::numeric_limits<std::uint64_t>::max() - base)
      return false;
    out = base + value;
    return true;
  }

  // Avoid negating INT64_MIN in signed arithmetic.
  const auto magnitude =
      static_cast<std::uint64_t>(-(delta + 1)) + 1u;
  if (magnitude > base)
    return false;
  out = base - magnitude;
  return true;
}

std::int64_t seek(
    lr::VfsFileHandle* handle,
    std::int64_t offset,
    int whence) {
  auto* h = impl(handle);
  if (!h)
    return -1;

  std::uint64_t next = 0;
  switch (whence) {
    case lr::vfs_seek_start:
      if (offset < 0)
        return -1;
      next = static_cast<std::uint64_t>(offset);
      break;

    case lr::vfs_seek_current:
      if (!add_signed(h->position, offset, next))
        return -1;
      break;

    case lr::vfs_seek_end:
      if (offset > 0 || !add_signed(h->size, offset, next))
        return -1;
      break;

    default:
      return -1;
  }

  h->position = next;
  // libretro-common's implementation returns 0 on success, not the new
  // position. tell() reports the cursor separately.
  return 0;
}

std::int64_t read(
    lr::VfsFileHandle* handle,
    void* destination,
    std::uint64_t length) {
  auto* h = impl(handle);
  if (!h || (!destination && length != 0) ||
      length > static_cast<std::uint64_t>(
          std::numeric_limits<std::size_t>::max()))
    return -1;

  if (length == 0)
    return 0;

  if (h->position >= h->size)
    return 0;

  const auto available = h->size - h->position;
  const auto request = static_cast<std::size_t>(
      std::min<std::uint64_t>(length, available));

  std::size_t got = 0;
  const auto result = h->reader->read_at(
      h->position,
      std::span<std::byte>(
          static_cast<std::byte*>(destination), request),
      got);
  if (!result)
    return -1;

  h->position += got;
  return static_cast<std::int64_t>(got);
}

std::int64_t write(
    lr::VfsFileHandle*, const void*, std::uint64_t) {
  return -1;
}

int flush(lr::VfsFileHandle* handle) {
  return handle ? 0 : -1;
}

int remove(const char*) {
  return -1;
}

int rename(const char*, const char*) {
  return -1;
}

lr::VfsInterface g_vfs{
    get_path,
    open,
    close,
    size,
    tell,
    seek,
    read,
    write,
    flush,
    remove,
    rename,
};

} // namespace

lr::VfsInterface* vfs_interface_v1() noexcept {
  return &g_vfs;
}

} // namespace corehost
