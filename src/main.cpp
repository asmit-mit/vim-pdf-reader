#include "SFML/Graphics.hpp"
#include <print>

int main(int argc, char *argv[]) {
  const int res_x = 640;
  const int res_y = 480;
  const int fps = 60;

  sf::RenderWindow window(sf::VideoMode({res_x, res_y}), "Hello World!",
                          (sf::Style::Resize + sf::Style::Close));
  window.setFramerateLimit(fps);

  sf::View view;

  sf::Font font("/home/asmitpaul/.local/share/fonts/"
                "JetBrainsMonoNerdFont-BoldItalic.ttf");
  sf::Text text(font, "Frontend!", 30);
  text.setFillColor(sf::Color::Green);

  sf::CircleShape circle(100);
  circle.setFillColor(sf::Color::Red);

  while (window.isOpen()) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>())
        window.close();

      if (event->is<sf::Event::Resized>()) {
        auto window_size = window.getSize();
        view.setSize({(float)window_size.x, (float)window_size.y});
        auto view_size = view.getSize();
        view.setCenter({view_size.x / 2, view_size.y / 2});
      }
    }

    auto text_bounds = text.getLocalBounds();
    text.setOrigin(text_bounds.getCenter());
    text.setPosition(view.getCenter());

    auto circle_bounds = circle.getLocalBounds();
    circle.setOrigin(circle_bounds.getCenter());
    circle.setPosition(view.getCenter());

    window.setView(view);
    window.clear();
    window.draw(circle);
    window.draw(text);
    window.display();
  }

  return 0;
}
