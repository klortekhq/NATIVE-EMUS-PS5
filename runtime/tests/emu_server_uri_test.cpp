#include "../src/ps5/emu_server_uri.hpp"

#include <cassert>
#include <string>

int main() {
  const std::string id(64, 'a');
  std::string out;

  assert(ps5rt::detail::emus_to_http(
      "emus://192.168.1.50:8787/" + id,
      out));
  assert(out ==
      "http://192.168.1.50:8787/api/v1/files/" + id);

  assert(ps5rt::detail::emus_to_http(
      "emus://192.168.1.50:8787/" + id + "/Game Disc.cue",
      out));
  assert(out ==
      "http://192.168.1.50:8787/api/v1/files/" + id +
      "?path=Game%20Disc.cue");

  assert(ps5rt::detail::emus_to_http(
      "emus://192.168.1.50:8787/" + id + "/../tracks/Track 01.bin",
      out));
  assert(out ==
      "http://192.168.1.50:8787/api/v1/files/" + id +
      "?path=..%2Ftracks%2FTrack%2001.bin");

  assert(ps5rt::detail::emus_to_http(
      "emus://[fd00::50]:8787/" + id + "/disc#1.chd",
      out));
  assert(out ==
      "http://[fd00::50]:8787/api/v1/files/" + id +
      "?path=disc%231.chd");

  assert(!ps5rt::detail::emus_to_http(
      "http://192.168.1.50:8787/" + id + "/game.chd",
      out));
  assert(!ps5rt::detail::emus_to_http(
      "emus://192.168.1.50:8787/not-a-valid-id/game.chd",
      out));
  assert(!ps5rt::detail::emus_to_http(
      "emus://bad host:8787/" + id + "/game.chd",
      out));
  assert(!ps5rt::detail::emus_to_http(
      "emus://192.168.1.50:8787/" + id + "/bad\nname.chd",
      out));

  return 0;
}
