#include <cmath>
#include <cstring>

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

static const int WIN_X = 1280;
static const int WIN_Y = 720;
int main() {
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "TEST");

  Rcam* camera = new Rcam();

  FrameDraw df(camera->GetConfig());

  df.setViewport(sf::FloatRect(0.0, 0.0, 1.0, 1.0));

  camera->StartStream();
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
        printf("%c", (char)event.text.unicode);
        if ((char)event.text.unicode == 'r') {
          printf("RESET\n");
          camera->Reset();
          delete camera;
          camera = new Rcam();
          camera->StartStream();
          printf("New addr: %p\n", camera);
        }
      }
    }

    Frame* fr;
    if (fr = camera->GetFrame()) {
      df.Update(fr);
    }

    window.clear();
    window.draw(df);
    window.display();
  }

  camera->Close();
  delete camera;
  return 0;
}
