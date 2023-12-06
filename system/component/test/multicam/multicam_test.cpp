#include <cstdio>


#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "system/component/inc/frame_draw.h"
#include "system/component/inc/rcam.h"
#include "windows.h"

static const int WIN_X = 1280;
static const int WIN_Y = 720;

int main() {
  CCyUSBDevice usb;
  usb.Reset();


  printf("Devices: %d\n", Rcam::NumCameras());
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "CMOS");

  std::vector<Rcam> cameras(Rcam::NumCameras());
  for (int i = 0; i < cameras.size(); ++i) {
    cameras[i].Open(i);
    printf("Serial number %d: %d\n", i, cameras[i].SerialNumber());
  }

  //cameras[0].Reset();
  //cameras[0].Start();

  std::vector<FrameDraw> fd(cameras.size());
  for (int i = 0; i < cameras.size(); ++i) {
    fd[i].Init(cameras[i].GetConfig());
    fd[i].AddProducer(&cameras[i]);
    fd[i].setViewport(sf::FloatRect((float)i / (float)cameras.size(), 0,
                                    1.0f / (float)cameras.size(), 1));
  }

  for (Rcam& camera : cameras) {
    camera.SetStream(true);
    camera.Start();
  }

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
      }
    }

    window.clear();
    for (const FrameDraw& f : fd) {
      window.draw(f);
    }
    window.display();
  }

  for (Rcam& camera : cameras) {
    camera.Close();
  }

  system("pause");
}