#include <cmath>
#include <cstring>

#include "system/component/inc/filterdev.h"
#include "system/component/inc/frame_draw.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/stddev.h"
#include "system/component/inc/tagsave.h"


#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")

#pragma comment(lib, "libfftw3-3.lib")

#include "windows.h"


template <typename T>
double Mean(const std::vector<T>& data) {
  T sum = 0;
  for (const T& elt : data) {
    sum += elt;
  }
  return (double)sum / (double)(data.size());
}

template <typename T>
double Sigma(const std::vector<T>& data) {
  double mean = Mean(data);
  double sv = 0;
  for (const T& elt : data) {
    double v = elt - mean; 
    sv += v * v;
  }

  return sqrt(sv / (double)(data.size()));
}

class FrameScale : public ExecNode {
 public:
  FrameScale(int sf) : sf_(sf) {}
  ~FrameScale() {}

private:
 void* Exec(void* data) {
   Frame* fr = (Frame*)data;

   for (int i = 0; i < fr->width * fr->height; ++i) {
     fr->data[i] *= sf_;
   }
   return data;
 }

 int sf_ = 0;
};

class PixelDev : public ExecNode {
 public:
  PixelDev(int x, int y, int n) : x_(x), y_(y) { pixels_.reserve(n); }
  ~PixelDev() {}

  double Mean() {
    std::lock_guard<std::mutex> lock(mutex_);
    return ::Mean<uint16_t>(pixels_);
  }

  double Sigma() {
    std::lock_guard<std::mutex> lock(mutex_);
    return ::Sigma<uint16_t>(pixels_);
  }

 private:
  void* Exec(void* data) override {
    std::lock_guard<std::mutex> lock(mutex_);
    Frame* fr = (Frame*)data;
    pixels_.push_back(fr->data[y_ * fr->width + x_]);
    return data;
  }

  int x_ = 0;
  int y_ = 0;
  std::mutex mutex_;
  std::vector<uint16_t> pixels_;
};

//static const int PIXEL_X = 1158;
//static const int PIXEL_Y = 50;
static const int PIXEL_X = 1157;
static const int PIXEL_Y = 50;
static const int N_FRAMES = 100;
static const int WIN_X = 1280;
static const int WIN_Y = 720;
int main() {
  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "TEST");

  Rcam camera;
  camera.Open();
  camera.SubWindow2Point(0, 0, 2048, 2048);

  FilterDev fd(camera.GetConfig());
  FrameDraw df(camera.GetConfig());
  FrameScale fs(1);
  PixelDev pd(PIXEL_X, PIXEL_Y, N_FRAMES);
  StdDev sd;
  TagSave ts;

  fd.LoadPixelData("dead_pixels.cfg", camera.SerialNumber(), 10000);

  fd.AddProducer(&camera);
  sd.AddProducer(&fd);
  pd.AddProducer(&sd);
  ts.AddProducer(&pd);
  fs.AddProducer(&ts);
  df.AddProducer(&fs);
  
  printf("Exposure: %.3e\n", camera.GetExposure());
  camera.resize(30);

  //camera.WriteCFG(0x2481, 0, 1);
  camera.SetBLC(false);
  //camera.SetGain(1.0);

  camera.SetStream(true);
  camera.Start();
  while (window.isOpen()) {
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
    window.display();
    if (camera.GetFrameCount() > N_FRAMES) window.close();
  }

  camera.Close();
  while (!camera.IsExecDone()) {}

  printf("Pixel (%d, %d) u: %.3e\n", PIXEL_X, PIXEL_Y, pd.Mean());
  printf("Pixel (%d, %d) o: %.3e\n", PIXEL_X, PIXEL_Y, pd.Sigma());
  

  system("pause");
  return 0;
}
