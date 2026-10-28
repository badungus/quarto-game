#pragma once
#include "button.hpp"
#include "input.hpp"
#include "raylib.h"
#include <optional>
#include <vector>

class Dropdown {
private:
  Rectangle bounds_;
  const Texture2D &panel_;
  ButtonContent content_;
  std::vector<ButtonContent> dd_content_;
  Button main_button_;
  std::vector<Button> buttons_;
  bool active_ = false;

public:
  Dropdown(Rectangle bounds, const Texture2D &panel, ButtonContent content,
           std::vector<ButtonContent> dd_content);
  void draw();
  std::optional<int> update(const Input &in);
  // True while the list is open, covering the rows below the dropdown.
  bool open() const { return active_; }
  // Shows content on the closed dropdown, as if it had been picked.
  void set_content(ButtonContent content) {
    content_ = content;
    main_button_.set_content(content);
  }
};
