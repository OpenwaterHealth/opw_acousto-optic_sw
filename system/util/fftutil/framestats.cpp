#include "framestats.h"

#include "system/component/inc/fftt.h"
#include "system/component/inc/roi.h"
#include "system/component/inc/stddev.h"

#include "windows.h"

// currently computing 7:
// mean pixel value
// % pixels saturated
// ROI energy
// ROI / ROU (double)
// Max FFT power
// Min FFT power
// Speckle contrast
static const int N_STATS = 7;


static inline double abs2(const fftw_complex c) {
  return c[0] * c[0] + c[1] * c[1];
}


void FrameStats::Init(const Frame* fr) {
  text_.resize(N_STATS);
  string_.resize(N_STATS);

  font_.loadFromFile("c:/windows/fonts/cour.ttf");
  float pos = 0;
  for (sf::Text& text : text_) {
    text.setFont(font_);
    text.setCharacterSize(CHAR_SIZE);
    text.setFillColor(sf::Color::White);
    text.setString("");
    text.setPosition(0, pos);
    text.setStyle(sf::Text::Bold);
    pos += CHAR_SIZE;
  }
  text_[0].setString("No Frame Received");

  millis_ = GetTickCount();
}

void* FrameStats::Exec(void* data) {
  if (GetTickCount() < millis_ + 500) return data;
  millis_ = GetTickCount();

  Frame* fr = (Frame*)data;
  int64_t sum = 0;
  int saturated = 0;
  double fr_width = fr->width;
  double fr_height = fr->height;

  int satval = (1 << fr->bits) - 1;
  for (int i = 0; i < fr->width * fr->height; ++i) {
    sum += fr->data[i];
    if (fr->data[i] == satval) ++saturated;
  }
  double mean = static_cast<double>(sum) / (fr_width * fr_height);
  double pct_saturated = (double)saturated \
    / (fr_width * fr_height) \
    * 100.0;

  double roi_rou = ROI::GetTag(fr)->roi / ROI::GetTag(fr)->rou;
  double fft_max = 0;
  double fft_min = std::numeric_limits<double>::infinity();

  double* fft = FFTT::GetTag(fr)->fft;
  for (int i = 0; i < (fr->width / 2 + 1) * fr->height; ++i) {
    double a = fft[i];
    if (a > fft_max) fft_max = a;
    if (a < fft_min) fft_min = a;
  }

  double speckleContrast = StdDev::GetTag(fr)->stddev / StdDev::GetTag(fr)->mean;

  std::lock_guard<std::mutex> lock(mutex_);
  char buf[80];
  snprintf(buf, 80, "Mean:      %.2lf", mean);
  string_[0] = std::string(buf);
  snprintf(buf, 80, "Saturated: %.1lf%%", pct_saturated);
  string_[1] = std::string(buf);
  snprintf(buf, 80, "ROI:       %.3le", ROI::GetTag(fr)->roi);
  string_[2] = std::string(buf);
  snprintf(buf, 80, "ROI/ROU:   %.2lf", roi_rou);
  string_[3] = std::string(buf);
  snprintf(buf, 80, "Max FFT:   %.3le", fft_max);
  string_[4] = std::string(buf);
  snprintf(buf, 80, "Min FFT:   %.3le", fft_min);
  string_[5] = std::string(buf);
  snprintf(buf, 80, "Speckle Contrast:   %.3le", speckleContrast);
  string_[6] = std::string(buf);

  for (int i = 0; i < text_.size(); ++i) {
    text_[i].setString(string_[i]);
  }


  return data;
}


void FrameStats::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  std::lock_guard<std::mutex> lock(mutex_);

  for (const sf::Text& text : text_) {
    target.draw(text, states);
  }
}
