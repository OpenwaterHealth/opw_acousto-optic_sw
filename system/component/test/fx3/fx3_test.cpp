#include <conio.h>
#include <cstring>
#include <fstream>

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

#include "system/component/inc/fx3.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/frame_draw.h"


int main() {
  sf::RenderWindow window(sf::VideoMode(1920, 1080), "TEST");

  printf("Gumsticks: %d\n", Rcam::NumCameras());

  Rcam camera;
  camera.Open();
  camera.SubWindow2Point(0, 0, 2048, 512);
  printf("Serial Number: %d\n", camera.SerialNumber());

  camera.SetStream(true);
  
  FrameDraw fd(camera.GetConfig());

  fd.AddProducer(&camera);

  camera.Start();
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
        char c = (char)event.text.unicode;
        printf("%c", c);
        switch (c) {
          case 'r': {
            camera.Reset();
            camera.SetStream(true);
            camera.Start();
          } break;
          case 'y': {
            camera.SetStream(true);
          } break;
          case 'n': {
            camera.SetStream(false);
          } break;
          case 'h': {
            camera.Close();
            camera.Open();
            camera.SubWindow2Point(0, 0, 2048, 512);
            camera.SetStream(true);
            camera.Start();
          } break;
        }
      }
    }

    window.clear();
    window.draw(fd);
    window.display();

  }

  camera.Close();
  return 0;
}
