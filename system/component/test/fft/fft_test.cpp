#include <cmath>
#include <cstring>

#include "system/component/inc/fftt.h"
#include "system/component/inc/frame_draw.h"
#include "system/component/inc/rcam.h"

#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")

#pragma comment(lib, "libfftw3-3.lib")

#include "windows.h"

class Stats : public ExecNode {
 public:
  Stats() {}
  ~Stats() {}

  void PrintStats() {
    printf("FFTT Time: %dms\n", fft_ms_);
  }

 private:
  void* Exec(void* data) override {
    fft_ms_ = FFTT::GetTag((Frame*)data)->ms;
    return data;
  }

  volatile int fft_ms_;
};

static const int WIN_X = 1280;
static const int WIN_Y = 720;
int main() {
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "TEST");

  Rcam camera;
  camera.SubWindow2Point(0, 0, 2047, 512);

  FrameDraw df(camera.GetConfig());
  FFTT fftt(camera.GetConfig());
  FFTTDraw dfftt(camera.GetConfig());
  Stats stats;

  df.AddProducer(&camera);
  fftt.AddProducer(&camera);
  dfftt.AddProducer(&fftt);
  stats.AddProducer(&fftt);

  df.setViewport(sf::FloatRect(0, 0, 0.5, 1));
  dfftt.setViewport(sf::FloatRect(0.5, 1, 0.5, 1));

  int64_t ms = Frame::SteadyClockTimeMs();
  int frames = 0;

  camera.StartStream();
  while (window.isOpen()) {
    if (Frame::SteadyClockTimeMs() - ms > 1000) {
      stats.PrintStats();
      ms = Frame::SteadyClockTimeMs();
    }
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
        printf("%c", (char)event.text.unicode);
      }
    }

    window.clear();
    window.draw(df);
    window.draw(dfftt);
    window.display();
  }

  camera.Close();
  return 0;
}
