#include "ui/page_view.h"

namespace ui {

PageView::PageView(const sf::Texture &dummy, std::vector<sf::RectangleShape> &boxes)
    : selection_boxes_(boxes), sprite_(dummy) {
  active_ = false;
  show_selection_boxes_ = false;
};

void PageView::draw(sf::RenderTarget &window) const {
  if (active_)
    window.draw(sprite_);

  if (!show_selection_boxes_)
    return;

  for (const auto &shape : selection_boxes_)
    window.draw(shape);
}

sf::Sprite &PageView::getSprite() {
  return sprite_;
}

void PageView::setPosition(const sf::Vector2f &position) {
  sprite_.setPosition(position);
}

sf::Vector2f PageView::getPosition() const {
  return sprite_.getPosition();
}

sf::FloatRect PageView::getGlobalBounds() const {
  return sprite_.getGlobalBounds();
}

void PageView::setRotation(int rot) {
  sprite_.setRotation(sf::degrees(rot * 90.f));
}

void PageView::setScale(const sf::Vector2f &scale) {
  sprite_.setScale(scale);
}

sf::Vector2u PageView::getTextureSize() const {
  return sprite_.getTexture().getSize();
}

void PageView::setKey(const pdf::PDFRenderKey &key) {
  key_ = key;
}

pdf::PDFRenderKey PageView::getKey() const {
  return key_;
}

void PageView::setTexture(const sf::Texture &texture) {
  sprite_.setTexture(texture, true);
  const auto bounds = sprite_.getLocalBounds();
  sprite_.setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
  active_ = true;
}

void PageView::reset() {
  active_ = false;
}

void PageView::showSelectionBoxes() {
  show_selection_boxes_ = true;
}

void PageView::hideSelectionBoxes() {
  show_selection_boxes_ = false;
}

bool PageView::hasTexture() const {
  return active_;
}

bool PageView::isInView(const sf::Vector2f &window_size) const {
  const auto bounds = sprite_.getGlobalBounds();

  float top = bounds.position.y;
  float bottom = bounds.position.y + bounds.size.y;

  return !(top > window_size.y || bottom < 0.f);
}

} // namespace ui
