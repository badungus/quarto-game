// text_field.hpp - Declares TextField: a Button that takes digits while it is
// focused. Click it to focus, click elsewhere or press Enter to leave.
#pragma once
#include "button.hpp"
#include "input.hpp"
#include <raylib.h>
#include <string>

class TextField {
private:
  Button button_;
  std::string label_;
  std::string value_;
  bool focused_ = false;

public:
  TextField(Rectangle bounds, const Texture2D &background,
            const Texture2D &hover_image, const Texture2D &armed_image,
            std::string label, std::string value);
  void update(const Input &in);
  void draw() const { button_.draw(); }
  const std::string &value() const { return value_; }
  void set_value(std::string value) { value_ = std::move(value); }
};
