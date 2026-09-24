// button.hpp - Declares the immediate-mode Button widget (normal, hover,
// pressed, disabled).
#pragma once
#include <raylib.h>

class Button {
private:
  float x;
  float y;
  int width;
  int height;
  Texture2D image;
  Texture2D background;

public:
  Button(int x, int y, int width, int height, Texture2D image,
         Texture2D background);
  void draw();
  void event();
};
