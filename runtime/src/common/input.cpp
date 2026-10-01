#include <ps5rt/input.hpp>

namespace ps5rt {

static_assert(max_local_players <= InputSnapshot::max_controllers,
              "local player map cannot exceed polled controller sources");

Result validate_local_player_map(const LocalPlayerMap& map,
                                 std::size_t source_count) noexcept {
  std::array<bool, InputSnapshot::max_controllers> used{};
  for (const auto source : map.source_for_player) {
    if (source == -1) continue;
    if (source < -1)
      return {ErrorCode::invalid_argument, 0, "disabled controller source must be -1"};
    const auto index = static_cast<std::size_t>(source);
    if (index >= source_count || index >= used.size())
      return {ErrorCode::invalid_argument, 0, "controller source is out of range"};
    if (used[index])
      return {ErrorCode::invalid_argument, 0, "controller source assigned more than once"};
    used[index] = true;
  }
  return Result::success();
}

Result map_local_players(const InputSnapshot& input, const LocalPlayerMap& map,
                         LocalPlayerSnapshot& out) noexcept {
  const auto valid = validate_local_player_map(map, input.controllers.size());
  if (!valid) {
    out = {};
    return valid;
  }

  LocalPlayerSnapshot mapped{};
  for (std::size_t player = 0; player < map.source_for_player.size(); ++player) {
    const auto source = map.source_for_player[player];
    if (source < 0) continue;
    mapped.players[player] = input.controllers[static_cast<std::size_t>(source)];
  }
  out = mapped;
  return Result::success();
}

} // namespace ps5rt
