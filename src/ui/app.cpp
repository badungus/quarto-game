#include "app.hpp"
#include "input.hpp"
#include <raylib.h>

Gui::Gui() {}

void Gui::run() {
  // Uncapped, the loop renders thousands of frames per second, which hung the
  // GPU driver on some machines.
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN);
  InitWindow(0, 0, "Quarto"); // 0, 0 = monitor resolution
  // Borderless rather than exclusive fullscreen: no display mode switch.
  ToggleBorderlessWindowed();
  // A window covering exactly the monitor is taken over by the GPU driver as
  // fullscreen, which blanks the screen for a frame on startup. One pixel
  // taller keeps it a normal window; the extra row is off screen.
  const int monitor = GetCurrentMonitor();
  SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor) + 1);
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
    // Created hidden and shown only once a frame is drawn: until then the
    // window shows black.
    if (IsWindowHidden())
      ClearWindowState(FLAG_WINDOW_HIDDEN);
  }

  assets_.unload();

  CloseWindow();
}
