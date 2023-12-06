#include <array>
#include <cmath>
#include <cstring>
#include <vector>

#include "fftt.h"
#include "frame.h"
#include "rcam.h"
#include "roi.h"

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")

#pragma comment(lib, "libfftw3-3.lib")

#include "windows.h"

static const int WIN_X = 500;
static const int WIN_Y = 500;
static const int IMG_X = 200;
static const int IMG_Y = 200;
static const int N_IMG = 3;

int main() {
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "TEST");

  std::array<Frame, N_IMG> frame;
  std::array<FrameDraw, N_IMG> framedraw;


  for (int i = 0; i < N_IMG; ++i) {
    frame[i].width = IMG_X;
    frame[i].height = IMG_Y;
    frame[i].bits = 5;
    frame[i].data = new uint16_t[IMG_X * IMG_Y];
    for (int y = 0; y < IMG_Y; ++y) {
      for (int x = 0; x < IMG_X; ++x) {
        if (i == 0) {
          if (((x / 10) % 2) ^ ((y / 10) % 2)) {
            frame[i].data[y * IMG_X + x] = x + y;
          } else {
            frame[i].data[y * IMG_X + x] = 0;
          }
        } else if (i == 1) {
          if (abs(x - IMG_X / 2) < 10 && abs(y - IMG_Y / 2) < 10) {
            frame[i].data[y * IMG_X + x] = (1 << frame[i].bits) - 1;
          } else {
            frame[i].data[y * IMG_X + x] = 0;
          }
        } else {
          frame[i].data[y * IMG_X + x] = x + y;
        }
      }
    }
    framedraw[i].Init(&frame[i]);
    framedraw[i].setViewport(sf::FloatRect(i * 1.0/N_IMG, 0, 1.0/N_IMG, 1.0));
  }
  

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
        printf("%c", (char)event.text.unicode);
      } else if (event.type == sf::Event::MouseWheelScrolled) {
        printf("%d, %d, %f\n",
          event.mouseWheelScroll.x,
          event.mouseWheelScroll.y,
          event.mouseWheelScroll.delta);
        int img =
            event.mouseWheelScroll.x / ((double)window.getSize().x / N_IMG);
        float x_rel = (event.mouseWheelScroll.x %
                      (int)((double)window.getSize().x / N_IMG)) /
                      ((double)window.getSize().x / N_IMG);
        float y_rel = event.mouseWheelScroll.y / ((double)window.getSize().y);

        framedraw[img].Zoom(x_rel,
                            y_rel,
                            event.mouseWheelScroll.delta > 0 ? 1.5 : .75);
      }
    }

    window.clear();
    for (int i = 0; i < N_IMG; ++i) {
      framedraw[i].Update(&frame[i]);
      window.draw(framedraw[i]);
    }
    window.display();
  }

  return 0;
}
