#include <cstring>
#include <cmath>

#include "roi.h"

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

#include "windows.h"



static const int WIN_X = 401;
static const int WIN_Y = 201;
int main() {
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "TEST");

  ROIDraw roi_d(WIN_X, WIN_Y);

  roi_d.Set(0, 50, 25);
  roi_d.setViewport(sf::FloatRect(0, 0, 1, 1));

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
        printf("%c", (char)event.text.unicode);
      } else if (event.type == sf::Event::MouseWheelScrolled) {
        roi_d.Zoom(event.mouseWheelScroll.x / (double)window.getSize().x,
                   event.mouseWheelScroll.y / (double)window.getSize().y,
                   event.mouseWheelScroll.delta > 0 ? 1.5 : .66);
      }
    }

    window.clear();

    roi_d.Set(50 * cos(GetTickCount() / 1000.),
              50 * sin(GetTickCount() / 1000.),
              25);
    window.draw(roi_d);

    window.display();
  }

  return 0;
}
