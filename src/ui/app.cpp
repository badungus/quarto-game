#include "app.hpp"
#include "input.hpp"
#include <raylib.h>

Gui::Gui() {}

void Gui::run() {
  // Uncapped, the loop renders thousands of frames per second, which hung the
  // GPU driver on some machines.
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(0, 0, "Quarto"); // 0, 0 = monitor resolution
  // Borderless rather than exclusive fullscreen: no display mode switch.
  ToggleBorderlessWindowed();
  SetTargetFPS(60); // in case the driver ignores vsync
  assets_.load();
  screen_.emplace(assets_);

  while (!WindowShouldClose()) {
    // Minimized, the screen size is 0, so the layout would divide by zero.
    if (IsWindowMinimized()) {
      BeginDrawing();
      EndDrawing();
      continue;
    }
    screen_->update(read_input());
    BeginDrawing();
    ClearBackground(GetColor(0x32393BFF));
    screen_->draw();
    EndDrawing();
  }

  assets_.unload();

  CloseWindow();
}
