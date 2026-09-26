#include "app.hpp"
#include "input.hpp"
#include <raylib.h>

Gui::Gui() : screen_(assets_) {}

void Gui::run() {
  InitWindow(0, 0, "Quarto"); // 0, 0 = monitor resolution
  SetWindowState(FLAG_FULLSCREEN_MODE);
  assets_.load();

  while (!WindowShouldClose()) {
    screen_.update(read_input());

    BeginDrawing();
    ClearBackground(RAYWHITE);
    screen_.draw();
    EndDrawing();
  }

  assets_.unload();

  CloseWindow();
}
