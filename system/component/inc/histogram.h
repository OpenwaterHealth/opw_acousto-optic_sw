#pragma once

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

class Histogram {
 public:
  Histogram(int min, int max, int bucket_size = 1);
  ~Histogram();

  void Clear();
  void Add(int val);

  void Print();

  void SetWindow(int x_sz, int y_sz, int y_max, sf::Color color);
  const sf::Drawable & Draw();

  operator sf::Drawable & ();

 private:
  uint64_t* data_;
  sf::VertexArray* draw_;
  int min_, max_, bucket_size_, n_;
  float draw_scale_, max_y_;
};

