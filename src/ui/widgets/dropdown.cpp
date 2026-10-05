#include "dropdown.hpp"
#include <optional>

Dropdown::Dropdown(Rectangle bounds, const Texture2D &background,
                   const Texture2D &hover_image, const Texture2D &armed_image,
                   ButtonContent content, std::vector<ButtonContent> dd_content)
    : background_(background), hover_image_(hover_image),
      armed_image_(armed_image), content_(content), dd_content_(dd_content),
      main_button_(bounds, background, hover_image, armed_image, content) {
  int idx = 0;
  for (auto cont : dd_content) {
    buttons_.emplace_back(Rectangle{bounds.x, bounds.y + idx * bounds.height,
                                    bounds.width, bounds.height},
                          background, hover_image, armed_image, cont);
    idx++;
  }
};

void Dropdown::draw() {
  if (!active_) {
    main_button_.draw();
    return;
  };
  for (int i = 0; i < dd_content_.size(); ++i) {
    buttons_[i].draw();
  }
};

std::optional<int> Dropdown::update(const Input &in) {
  if (!active_) {
    if (main_button_.update(in)) {
      active_ = true;
    }
    return std::nullopt;
  }
  std::optional<int> clicked_button = std::nullopt;
  for (int i = 0; i < dd_content_.size(); ++i) {
    if (buttons_[i].update(in)) {
      content_ = dd_content_[i];
      main_button_.set_content(dd_content_[i]);
      clicked_button = i;
      active_ = false;
    }
  }
  return clicked_button;
};
