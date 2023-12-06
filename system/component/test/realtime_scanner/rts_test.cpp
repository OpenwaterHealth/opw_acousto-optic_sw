#include <cmath>
#include <cstring>

#define SFML_STATIC
#include "system/component/inc/fftt.h"
#include "system/component/inc/frame_draw.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/roi.h"
#include "system/component/inc/scandraw.h"

#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")

#pragma comment(lib, "libfftw3-3.lib")

#include "windows.h"

class FakeROI : public ExecNode {
 public:
  FakeROI() {}
  ~FakeROI() {}

  int i_ = 0;

  void* Exec(void* data) override {
    ++i_;
    roi_tag* r = ROI::GetTag((Frame*)data);
    r->roi = i_;
    r->rou = 1.0;
    return data;
  }
};

static const int WIN_X = 1280;
static const int WIN_Y = 720;
int main() {
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "TEST");

  Rcam camera;
  camera.Open();
  camera.SubWindow2Point(0, 0, 512, 512);

  FrameDraw df(camera.GetConfig());
  FFTT fftt(camera.GetConfig());
  ROI roi(camera.GetConfig());
  FakeROI froi;
  ScanDraw sd;
  sd.SetSize(10, 10, 10);

  df.AddProducer(&camera);
  fftt.AddProducer(&camera);
  roi.AddProducer(&fftt);
  froi.AddProducer(&roi);
  sd.AddProducer(&froi);

  camera.resize(50);

  roi.Set(0.5, 0.5, 0.25);

  df.setViewport(sf::FloatRect(0, 0, 0.5, 1));
  sd.setViewport(sf::FloatRect(0.5, 0, 0.5, 1));

  int64_t ms = Frame::SteadyClockTimeMs();
  int frames = 0;

  printf("Starting Camera\n");

  camera.SetStream(false);
  camera.Start();
  while (window.isOpen()) {
    if (Frame::SteadyClockTimeMs() - ms > 1000) {
      printf("%d %d\n", camera.DroppedFrames(), camera.DeviceFrames() - frames);
      frames = camera.DeviceFrames();
      ms = Frame::SteadyClockTimeMs();
    }
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
        char c = (char)event.text.unicode;
        printf("%c", c);
        if (c == '1') sd.SetViewRelative(-1);
        if (c == '2') sd.SetViewRelative(1);
      }
    }

    window.clear();
    window.draw(df);
    window.draw(sd);
    window.display();
  }

  camera.Close();
  return 0;
}
