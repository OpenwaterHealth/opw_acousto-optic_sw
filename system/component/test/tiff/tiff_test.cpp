#include "frame.h"

#include <cstring>

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#pragma comment (lib, "sfml-graphics-s.lib")
#pragma comment (lib, "sfml-window-s.lib")
#pragma comment (lib, "sfml-system-s.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "freetype.lib")

#include "windows.h"

static const int IMG_X = 2712;
static const int IMG_Y = 2080;
int main() {
  Frame* fr = new Frame;
  fr->data = new uint16_t[IMG_X * IMG_Y];
  
  fr->width = IMG_X;
  fr->height = IMG_Y;
  fr->bits = 10;
  for (int col = 0; col < IMG_Y; ++col) {
    for (int row = 0; row < IMG_X; ++row) {
      fr->data[row + col * IMG_X] = (int)((double)(row + col) * (1 << fr->bits) / (IMG_X + IMG_Y)) % (1 << fr->bits);
    }
  }

  fr->TimeStamp();
  printf("%d\n %x\n\n", fr->timestamp[0], fr->timestamp[1]);
  fr->Write("test.tiff");
  fr->TimeStamp();
  printf("%d\n %x\n\n", fr->timestamp[0], fr->timestamp[1]);

  delete fr;

  fr = new Frame("test.tiff");
  if (fr->data == NULL) {
    printf("ERR: could not read TIFF\n");
    return -1;
  }
  
  sf::RenderWindow window(sf::VideoMode(1024, 768), "CMOS");

  DrawFrame drawframe(fr);
  drawframe.SetWindow(1024, 768);
  
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      }
      else if (event.type == sf::Event::TextEntered) {
        
      }
    }

    window.draw(drawframe);
    window.display();
  }


  return 0;
}