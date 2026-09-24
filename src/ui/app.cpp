#include "app.hpp"
#include "screens/game_screen.hpp"
#include <raylib.h>

Gui::Gui(Game &game) : game_(game) { ; }
void Gui::run() {
  InitWindow(1920, 1080, "Quarto");
  SetWindowState(FLAG_FULLSCREEN_MODE);
  GameScreen game_screen(game_);

  while (!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    game_screen.draw_piece_pool();

    DrawText("Hello world", 300, 250, 40, BLACK);

    EndDrawing();
  }

  CloseWindow();
}
