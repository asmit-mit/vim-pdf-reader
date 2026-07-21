#include "ui/completions.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Completions::Completions(const sf::Font &cmd_font, const sf::Font &desc_font, int font_size)
    : cmd_font_(cmd_font), desc_font_(desc_font) {
  list_dirty_ = false;
  first_visible_ = 0;
  selected_ = 0;
  display_area_.setFillColor(utils::hexToRGB(settings::status_bg_));
  selected_cmd_area_.setFillColor(utils::hexToRGB(settings::completion_highlight_bg_));
  selected_desc_area_.setFillColor(utils::hexToRGB(settings::completion_highlight_bg_));
  for (std::size_t i = 0; i < max_list_items_; i++) {
    display_cmd_list_.emplace_back(sf::Text(cmd_font_, "", font_size));
    display_desc_list_.emplace_back(sf::Text(desc_font_, "", font_size));
  }
}

void Completions::draw(sf::RenderTarget &window) const {
  if (!visible_)
    return;
  window.draw(display_area_);
  window.draw(selected_cmd_area_);
  window.draw(selected_desc_area_);

  const std::size_t visible = std::min(max_list_items_, completions_.size() - first_visible_);
  for (std::size_t row = 0; row < visible; ++row) {
    window.draw(display_cmd_list_[row]);
    window.draw(display_desc_list_[row]);
  }
}

void Completions::update() {
  if (!visible_)
    return;

  display_area_.setPosition(
      {0, window_size_.y - display_area_.getSize().y - 2 * utils::cmdline_height_ + 1.f}
  );

  if (list_dirty_) {
    const std::size_t last = std::min(first_visible_ + max_list_items_, completions_.size());

    std::size_t row = 0;
    for (std::size_t i = first_visible_; i < last; ++i, ++row) {
      display_cmd_list_[row].setString(completions_[i].first);
      display_cmd_list_[row].setPosition(
          {utils::padding, display_area_.getPosition().y + row * utils::cmdline_height_}
      );
      display_cmd_list_[row].setFillColor(cmd_fg_color_);

      display_desc_list_[row].setString(completions_[i].second);
      display_desc_list_[row].setPosition(
          {window_size_.x - display_desc_list_[row].getGlobalBounds().size.x - utils::padding,
           display_area_.getPosition().y + row * utils::cmdline_height_}
      );
      display_desc_list_[row].setFillColor(desc_fg_color_);

      if (i == selected_) {
        selected_cmd_area_.setPosition(
            {utils::padding - 2.f, display_cmd_list_[row].getPosition().y}
        );
        selected_cmd_area_.setSize(
            {display_cmd_list_[row].getGlobalBounds().size.x + 6.f, utils::cmdline_height_}
        );

        selected_desc_area_.setPosition(
            {window_size_.x - display_desc_list_[row].getGlobalBounds().size.x - utils::padding -
                 4.f,
             display_desc_list_[row].getPosition().y}
        );
        selected_desc_area_.setSize(
            {display_desc_list_[row].getGlobalBounds().size.x + utils::padding,
             utils::cmdline_height_}
        );
      }
    }

    list_dirty_ = false;
  }
}

void Completions::handleEvent(const sf::Event &event) {}

void Completions::onResize(const sf::Vector2f &size) {
  window_size_ = size;
}

void Completions::setFillColor(const sf::Color &color) {
  display_area_.setFillColor(color);
}

void Completions::setCmdColor(const sf::Color &color) {
  cmd_fg_color_ = color;
}

void Completions::setDescColor(const sf::Color &color) {
  desc_fg_color_ = color;
}

void Completions::setPosition(const sf::Vector2f &pos) {
  display_area_.setPosition(pos);
}

void Completions::setCompletionList(const std::vector<std::pair<std::string, std::string>> &list) {
  clear();
  completions_ = list;

  float height = std::min(max_list_items_, list.size()) * utils::cmdline_height_;
  display_area_.setSize({window_size_.x, height});

  list_dirty_ = true;
}

void Completions::clear() {
  first_visible_ = 0;
  selected_ = 0;
  completions_.clear();
}

void Completions::moveUp() {
  if (completions_.empty())
    return;

  if (selected_ == 0) {
    selected_ = completions_.size() - 1;
    if (completions_.size() > max_list_items_)
      first_visible_ = completions_.size() - max_list_items_;
    else
      first_visible_ = 0;

    list_dirty_ = true;
    return;
  }

  selected_--;
  if (selected_ < first_visible_)
    first_visible_--;

  list_dirty_ = true;
}

void Completions::moveDown() {
  if (completions_.empty())
    return;

  if (selected_ + 1 >= completions_.size()) {
    first_visible_ = 0;
    selected_ = 0;
    list_dirty_ = true;
    return;
  }

  selected_++;
  if (selected_ >= first_visible_ + max_list_items_)
    first_visible_++;

  list_dirty_ = true;
}

bool Completions::isVisible() {
  return visible_;
}

void Completions::show() {
  visible_ = true;
}

void Completions::hide() {
  visible_ = false;
}

std::string &Completions::getSelectedText() {
  return completions_[selected_].first;
}

} // namespace ui
