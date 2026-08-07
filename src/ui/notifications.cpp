#include "ui/notifications.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Notifications::Notifications(
    const sf::Font &font_normal,
    const sf::Font &font_bold,
    core::HistoryManager &notification_history,
    core::EventBus &event_bus
)
    : event_bus_(event_bus), history_(notification_history), font_normal_(font_normal),
      font_bold_(font_bold), display_header_(font_bold, "Notification", utils::char_size + 2),
      display_msg_(font_normal_, "", utils::char_size) {
  visible_ = false;
  hovered_ = false;

  display_msg_.setFillColor(utils::hexToRGB(settings::fg_));
  display_header_.setFillColor(utils::hexToRGB(settings::fg_));

  display_area_.setFillColor(utils::hexToRGB(settings::bg_));
  display_area_.setOutlineThickness(2.f);
  display_area_.setOutlineColor(utils::hexToRGB(settings::notification_outline_bg_));

  idle_color_ = utils::hexToRGB(settings::bg_);
  hover_color_ = utils::hexToRGB(settings::notification_hover_bg_);

  event_bus_.subscribe<const char *>("notification.msg", [this](const char *msg) { show(msg); });
}

void Notifications::update() {
  if (!visible_)
    return;

  if (display_timer_.getElapsedTime().asSeconds() >= timeout_) {
    visible_ = false;
    hovered_ = false;
  }

  if (hovered_)
    display_area_.setFillColor(hover_color_);
  else
    display_area_.setFillColor(idle_color_);
}

void Notifications::draw(sf::RenderTarget &window) const {
  if (!visible_)
    return;

  window.draw(display_area_);
  window.draw(display_header_);
  window.draw(display_msg_);
}

void Notifications::handleEvent(const sf::Event &event) {
  if (!visible_)
    return;

  if (const auto *mouse = event.getIf<sf::Event::MouseMoved>())
    hovered_ = display_area_.getGlobalBounds().contains(sf::Vector2f(mouse->position));

  if (const auto *mouse = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (mouse->button == sf::Mouse::Button::Left &&
        display_area_.getGlobalBounds().contains(sf::Vector2f(mouse->position))) {
      visible_ = false;
    }
  }
}

void Notifications::onResize(const sf::Vector2f &size) {
  window_size_ = size;
  layout();
}

std::string Notifications::wrapText(
    const std::string &raw, const sf::Font &font, unsigned int char_size, float max_width
) {
  std::istringstream words(raw);
  std::string word, line, result;
  sf::Text probe(font, "", char_size);

  auto line_width = [&](const std::string &s) -> float {
    probe.setString(s);
    return probe.getLocalBounds().size.x;
  };

  auto flushLine = [&](const std::string &l) { result += l + '\n'; };

  auto pushWord = [&](const std::string &w) {
    std::string remaining = w;
    while (!remaining.empty()) {
      std::string candidate = line.empty() ? remaining : line + ' ' + remaining;
      if (line_width(candidate) <= max_width) {
        line = candidate;
        return;
      }

      std::string chunk = line.empty() ? "" : line + ' ';
      std::string seg;
      for (char c : remaining) {
        std::string with_dash = chunk + seg + c + '-';
        std::string without = chunk + seg + c;
        if (line_width(with_dash) > max_width) {
          flushLine(chunk + seg + '-');
          chunk = "";
          line = "";
          seg = std::string(1, c);
        } else
          seg += c;
      }

      remaining = seg;
      if (!line.empty() && !remaining.empty()) {
        std::string try_append = line + ' ' + remaining;
        if (line_width(try_append) <= max_width) {
          line = try_append;
          return;
        }
        flushLine(line);
        line = "";
      }
      line = remaining;
      return;
    }
  };

  while (words >> word) {
    std::string candidate = line.empty() ? word : line + ' ' + word;
    if (!line.empty() && line_width(candidate) > max_width) {
      flushLine(line);
      line = "";
      pushWord(word);
    } else
      line = candidate;
  }

  if (!line.empty())
    result += line;

  return result;
}

void Notifications::show(const char *msg) {
  history_.add(msg);

  const float inner_width = max_width_ - 2.f * padding_;
  std::string wrapped = wrapText(msg, font_normal_, utils::char_size, inner_width);

  display_msg_.setString(wrapped);
  display_timer_.restart();

  visible_ = true;
  layout();
}

void Notifications::layout() {
  if (window_size_.x == 0.f && window_size_.y == 0.f)
    return;

  sf::FloatRect hb = display_header_.getLocalBounds();
  float header_h = hb.size.y;

  sf::FloatRect tb = display_msg_.getLocalBounds();
  float text_w = tb.size.x;
  float text_h = tb.size.y;

  float box_w = std::min(std::max(text_w, hb.size.x) + 2.f * padding_, max_width_);
  float box_h = padding_ + header_h + padding_ * 1.5f + text_h + padding_ * 1.5f;

  const float margin = 8.f;
  float box_x = window_size_.x - box_w - margin;
  float box_y = margin;

  display_area_.setSize({box_w, box_h});
  display_area_.setPosition({box_x, box_y});

  display_header_.setPosition({box_x + padding_, box_y + padding_});
  display_msg_.setPosition({box_x + padding_, box_y + padding_ + header_h + padding_ * 1.5f});
}

} // namespace ui
