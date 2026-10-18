#pragma once

#include "screen.hpp"
#include "assets.hpp"
#include <optional>

class Gui {
public:
  Gui();
  void run();

private:
  Assets assets_; // declared before screen_, which refers to it
  // Built in run() once the window exists: widget positions depend on the
  // screen size.
  std::optional<Screen> screen_;
};
