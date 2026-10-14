#pragma once
#include "command.hpp"
#include <optional>

enum class ScreenId { Menu, Game, Result };

struct Action {
  std::optional<Command> cmd;
  ScreenId screen;
};
