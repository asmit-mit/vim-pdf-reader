#include "ui/page_view.h"

namespace ui {

PageView::PageView(const sf::Texture &dummy) : sprite_(dummy) {
  active_ = false;
  show_search_boxes_ = false;
  first_texture_set_ = false;
};

void PageView::draw(sf::RenderTarget &window) const {
  if (!active_)
    return;

  window.draw(sprite_);
  // window.draw(original_shape_);

  if (!show_search_boxes_)
    return;

  window.draw(search_boxes_, sf::RenderStates(original_shape_.getTransform()));
}

void PageView::update() {
  if (show_search_boxes_ && update_search_boxes_) {
    search_boxes_.clear();

    const sf::Color color{255, 255, 0, 80};
    for (std::size_t i = 0; i < search_result_.local_rects.size(); i++) {
      const auto &rect = search_result_.local_rects[i];
      sf::Vector2f tl = {rect.x0, rect.y0};
      sf::Vector2f tr = {rect.x1, rect.y0};
      sf::Vector2f br = {rect.x1, rect.y1};
      sf::Vector2f bl = {rect.x0, rect.y1};

      search_boxes_.append({tl, color});
      search_boxes_.append({tr, color});
      search_boxes_.append({br, color});
      search_boxes_.append({tl, color});
      search_boxes_.append({br, color});
      search_boxes_.append({bl, color});
    }

    update_search_boxes_ = false;
  }
}

sf::Sprite &PageView::getSprite() {
  return sprite_;
}

void PageView::move(const sf::Vector2f &delta) {
  sprite_.move(delta);
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

  if (!first_texture_set_) {
    original_shape_.setSize(bounds.size);
    original_shape_.setOrigin(sprite_.getOrigin());
    original_shape_.setFillColor(sf::Color::Black);
    first_texture_set_ = true;
  }

  active_ = true;
}

void PageView::reset() {
  active_ = false;
}

void PageView::setSearchResults(PDFSearchResult search_results) {
  clearSearchResults();
  search_result_ = std::move(search_results);
  search_boxes_.resize(search_result_.local_rects.size());
}

void PageView::clearSearchResults() {
  search_result_.local_rects.clear();
  search_boxes_.clear();
}

void PageView::showSearchResults() {
  show_search_boxes_ = true;
}

void PageView::hideSearchResults() {
  show_search_boxes_ = false;
}

void PageView::syncPageShapePos() {
  original_shape_.setPosition(sprite_.getPosition());
  update_search_boxes_ = true;
}

void PageView::syncPageShape(int rot) {
  original_shape_.setRotation(sf::degrees(rot * 90.f));

  const auto &page_size_raw = sprite_.getGlobalBounds().size;
  const bool swapped = (rot == 1 || rot == 3);
  const sf::Vector2f page_size = swapped ? sf::Vector2f{page_size_raw.y, page_size_raw.x}
                                         : page_size_raw;

  const auto shape_size = original_shape_.getSize();
  original_shape_.setScale({page_size.x / shape_size.x, page_size.y / shape_size.y});

  update_search_boxes_ = true;
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
