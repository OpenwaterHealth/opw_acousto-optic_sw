#include "system/component/inc/frame_draw.h"

#include "system/component/inc/colormap.h"

#ifdef _MSC_VER
#pragma comment (lib, "sfml-graphics-s.lib")
#pragma comment (lib, "sfml-window-s.lib")
#pragma comment (lib, "sfml-system-s.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "freetype.lib")
#endif

FrameDraw::FrameDraw(const Frame* fr) {
  Init(fr->width, fr->height);
}


FrameDraw::FrameDraw(int width, int height) {
  Init(width, height);
}


FrameDraw::~FrameDraw() {
}


void FrameDraw::Init(const Frame* fr) {
  Init(fr->width, fr->height);
}


void FrameDraw::Init(int width, int height) {
  width_ = width;
  height_ = height;

  px_.resize(width * height, 0xFF000000);

  tx_.create(width_, height_);
  sp_.setTexture(tx_);

  Zoomable::SetDrawable(width, height, std::vector<sf::Drawable*>({&sp_}));
}


void FrameDraw::Update(const Frame* fr) {
  Exec((void*)fr);
}


void* FrameDraw::Exec(void* data) {
  Frame* fr = (Frame*)data;
  assert(fr->height == height_);
  assert(fr->width == width_);

  // only display frames as fast as we can update the display object
  if (!wr_lock_.try_lock()) return data;
  for (int i = 0; i < height_ * width_; ++i) {
    px_[i] = COLORMAP_GREY[fr->data[i]];
  }
  std::lock_guard<std::mutex> rdlock(Zoomable::lock_);
  tx_.update((uint8_t*)px_.data());

  wr_lock_.unlock();
  return data;
}
