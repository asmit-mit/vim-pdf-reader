#pragma once

#include <SFML/Graphics.hpp>

#include "pdf/pdf_renderer.h"
#include "ui/widget.h"

namespace ui {

class PageView : public Widget {
public:
  PageView(const sf::Texture &dummy, std::vector<sf::RectangleShape> &boxes);

  void draw(sf::RenderTarget &window) const override;
  void update() override {};

  sf::Sprite &getSprite();

  void setPosition(const sf::Vector2f &position);
  sf::Vector2f getPosition() const;
  sf::FloatRect getGlobalBounds() const;

  void setRotation(int rot);
  void setScale(const sf::Vector2f &size);
  sf::Vector2u getTextureSize() const;

  void setKey(const pdf::PDFRenderKey &key);
  pdf::PDFRenderKey getKey() const;

  void setTexture(const sf::Texture &texture);
  void reset();

  void showSelectionBoxes();
  void hideSelectionBoxes();

  bool hasTexture() const;
  bool isInView(const sf::Vector2f &window_size) const;

private:
  std::vector<sf::RectangleShape> &selection_boxes_;
  sf::Sprite sprite_;
  pdf::PDFRenderKey key_;

  bool active_;
  bool show_selection_boxes_;
};

} // namespace ui
