// main_menu_screen.cpp - Implements MainMenuScreen.

#include "main_menu_screen.hpp"
#include "layout.hpp"
#include <raylib.h>

MainMenu::MainMenu(Game &game, MatchController &match_controller,
                   const Assets &assets)
    : game_(game) {}

void MainMenu::update(const Input &in) {}

void MainMenu::draw_menu_buttons() {}
