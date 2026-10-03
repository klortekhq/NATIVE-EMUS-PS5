#include <ps5rt/vfs.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main() {
  std::string normalized;

  assert(ps5rt::normalize_host_path("/data//EMU-PS5/./NES/../SNES", normalized));
  assert(normalized == "/data/EMU-PS5/SNES");

  assert(ps5rt::normalize_host_path("relative/../roms/game.chd", normalized));
  assert(normalized == "roms/game.chd");

  assert(ps5rt::normalize_host_path("../../outside", normalized));
  assert(normalized == "../../outside");

  assert(!ps5rt::normalize_host_path("", normalized));

  const auto root =
      std::filesystem::temp_directory_path() / "ps5rt-vfs-host-test";
  std::error_code ec;
  std::filesystem::remove_all(root, ec);

  const auto nested = root / "a" / "b" / "c";
  assert(ps5rt::ensure_directory(nested.string()));
  assert(std::filesystem::is_directory(nested));

  // Idempotent creation must stay successful.
  assert(ps5rt::ensure_directory(nested.string()));

  const auto file = root / "regular-file";
  {
    std::ofstream out(file);
    out << "data";
  }
  // A regular file cannot be reused as a directory component.
  assert(!ps5rt::ensure_directory((file / "child").string()));

  std::filesystem::remove_all(root, ec);
  return 0;
}
