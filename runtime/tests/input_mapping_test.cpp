#include <ps5rt/input.hpp>

#include <cassert>
#include <cstdint>

int main() {
  ps5rt::InputSnapshot input{};
  input.controllers[0].connected = true;
  input.controllers[0].user_id = 10;
  input.controllers[0].buttons = static_cast<std::uint32_t>(ps5rt::Button::cross);
  input.controllers[1].connected = true;
  input.controllers[1].user_id = 20;
  input.controllers[1].buttons = static_cast<std::uint32_t>(ps5rt::Button::circle);

  ps5rt::LocalPlayerMap map{};
  map.source_for_player = {1, 0, -1, -1};
  ps5rt::LocalPlayerSnapshot players{};
  assert(ps5rt::map_local_players(input, map, players));
  assert(players.players[0].connected && players.players[0].user_id == 20);
  assert(players.players[0].buttons == static_cast<std::uint32_t>(ps5rt::Button::circle));
  assert(players.players[1].connected && players.players[1].user_id == 10);
  assert(!players.players[2].connected && !players.players[3].connected);

  // A disconnected source stays a disconnected player without disturbing others.
  input.controllers[1] = {};
  assert(ps5rt::map_local_players(input, map, players));
  assert(!players.players[0].connected);
  assert(players.players[1].connected && players.players[1].user_id == 10);

  // Duplicate and out-of-range devices are rejected atomically.
  map.source_for_player = {0, 0, -1, -1};
  assert(!ps5rt::map_local_players(input, map, players));
  assert(!players.players[0].connected && !players.players[1].connected);

  map.source_for_player = {0, 4, -1, -1};
  assert(!ps5rt::validate_local_player_map(map, input.controllers.size()));
  map.source_for_player = {0, -2, -1, -1};
  assert(!ps5rt::validate_local_player_map(map, input.controllers.size()));

  // Local mapping is pure and requires no network/session object.
  map.source_for_player = {0, -1, -1, -1};
  assert(ps5rt::map_local_players(input, map, players));
  assert(players.players[0].connected && !players.players[1].connected);
  return 0;
}
