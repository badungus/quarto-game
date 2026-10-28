// text_field.cpp - Implements TextField focus and digit input.
#include "text_field.hpp"
#include <cctype>

TextField::TextField(Rectangle bounds, const Texture2D &panel,
                     std::string label, std::string value)
    : button_(bounds, panel, colors::SECONDARY_BUTTON, ""), label_(label),
      value_(value) {}

void TextField::update(const Input &in) {
  if (button_.update(in))
    focused_ = true;
  else if (in.pressed || IsKeyPressed(KEY_ENTER))
    focused_ = false;

  if (focused_) {
    // At most 9 digits, so the engine can read it as an int.
    for (int c = GetCharPressed(); c != 0; c = GetCharPressed())
      if (std::isdigit(c) && value_.size() < 9)
        value_ += (char)c;
    if (IsKeyPressed(KEY_BACKSPACE) && !value_.empty())
      value_.pop_back();
  }
  button_.set_content(label_ + " " + value_ + (focused_ ? "_" : ""));
}
