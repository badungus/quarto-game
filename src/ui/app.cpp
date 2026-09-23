#include "app.hpp"

#include <raylib.h>

void Gui::run() {
  InitWindow(800, 600, "Quarto");

  while (!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawText("Quarto", 300, 250, 40, BLACK);

    EndDrawing();
  }

  CloseWindow();
}
