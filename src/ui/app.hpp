#pragma once

#include "screens/screen.hpp"
#include "theme/assets.hpp"

class Gui {
public:
  Gui();
  void run();

private:
  Assets assets_; // declared before screen_, which refers to it
  Screen screen_;
};
