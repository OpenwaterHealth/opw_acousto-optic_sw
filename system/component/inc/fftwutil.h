#pragma once

// Utilities for transforming between the output of a FFTW
//   fft transform and traditional fourier space

#include <array>
#include <vector>

#include "frame.h"
#include "zoomable.h"

#include <vector>

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

// Transform between an fractional fft point and a location on an FFTW
//   transform
// @param x position of the point to transform
// @param y position of the point to transform
// @param width width of the original image
// @param height of the original image
int FFTWIndex(double x, double y, int width, int height);


// Transform between an absolute fft point and a location on an FFTW
//   transform
// @param x position of the point to transform
// @param y position of the point to transform
// @param width width of the original image
// @param height of the original image
int FFTWIndex(int x, int y, int width, int height);


// Class for drawing an FFTW shifted texture
class FFTWDraw : public Zoomable {
 public:
  FFTWDraw() {}

  ~FFTWDraw() {}

  // resize ROI to deal with different incoming frame sizes
  void Resize(int width, int height);
  void Resize(const Frame* fr);

  // Transform between an fractional fft point and a location on an FFTW
  //   transform
  // @param x position of the point to transform
  // @param y position of the point to transform
  int FFTWIndex(double x, double y);

  // Transform between an absolute fft point and a location on an FFTW
  //   transform
  // @param x position of the point to transform
  // @param y position of the point to transform
  int FFTWIndex(int x, int y);

 protected:
  // update drawing objects with pixel data
  void Update(const std::vector<uint32_t>& pixels);

 private:
  int width_ = 0;
  int height_ = 0;
  int fft_x_sz_ = 0;
  int fft_y_sz_ = 0;
  sf::Texture tx_;
  std::array<sf::Sprite, 4> sp_;
};

// Generate the indices of a circle
class FFTWCircle /*: public std::iterator<std::forward_iterator_tag, int>*/ {
 public:
  FFTWCircle() {}

  // Construct an iterator to emit indices of an FFTW corresponding
  //   to a cirle at (x,y) with radius r
  // @param x x-coordinate of the cirle
  // @param y y-coordinate of the circle
  // @param r radius of the circle
  // @param width width of the incoming frames
  // @param height height of the incoming frames
  FFTWCircle(double x, double y, double r, int width, int height);
  ~FFTWCircle() {}

  // Resize for different incoming frames
  // @param width width of the incoming frames
  // @param height height of the incoming frames
  void Resize(int width, int height);

  // Construct an iterator to emit indices of an FFTW corresponding
  //   to a cirle at (x,y) with radius r
  // @param x x-coordinate of the cirle
  // @param y y-coordinate of the circle
  // @param r radius of the circle
  void Set(double x, double y, double r);

  // iterator functions.
  // underlying iterator is not guaranteed to remain a
  // std::vector iterator
  std::vector<int>::iterator begin() { return idx_.begin(); }
  std::vector<int>::iterator end() { return idx_.end(); }

 private:
  int width_ = 0;
  int height_ = 0;
  std::vector<int> idx_;
};
