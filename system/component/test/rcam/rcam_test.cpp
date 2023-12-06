#include <cmath>
#include <cstring>

#include "system/component/inc/frame_draw.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/stddev.h"
#include "system/component/inc/time.h"


#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")

#pragma comment(lib, "libfftw3-3.lib")

#include "windows.h"

class SyncStuff : public ExecNode {
 public:
  SyncStuff() {}
  ~SyncStuff() {}

  int GetSeq() { return seq_; }

 private:
  int seq_;
  std::mutex mutex_;

  void* Exec(void* data) override {
    Frame* fr = (Frame*)data;
    std::lock_guard<std::mutex> lock(mutex_);
    seq_ = fr->seq;
    return data;
  }
};

static const int WIN_X = 1280;
static const int WIN_Y = 720;
int main() {
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "TEST");

  Rcam camera;
  camera.Open(0);
  camera.SubWindow2Point(0, 0, 512, 512);

  FrameDraw df(camera.GetConfig());
  StdDev sd;
  SyncStuff ss;

  df.AddProducer(&camera);
  sd.AddProducer(&camera);
  ss.AddProducer(&sd);


  camera.resize(30);

  int64_t ms = Component::SteadyClockTimeMs();
  int frames = 0;

  camera.SetStream(true);
  camera.Start();
  while (window.isOpen()) {
    if (Component::SteadyClockTimeMs() - ms > 1000) {
      printf("%d\n", camera.GetFrameCount() - frames);
      frames = camera.GetFrameCount();
      ms = Component::SteadyClockTimeMs();
      printf("Seq: %d\n", ss.GetSeq());
    }
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::TextEntered) {
        char c = (char)event.text.unicode;
        printf("%c\n", c);
        if (c == 'n') {
          camera.SetFrameCount(0);
        }
      }
    }

    window.clear();
    window.draw(df);
    window.display();
  }

  camera.Close();
  return 0;
}
