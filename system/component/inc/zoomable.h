#pragma once

#include <mutex>
#include <vector>

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

// Base class for objects that can be drawn and zoomed
// Derived objects must at minimum call SetDrawable() to determine
//   what will be drawn.
// It is recommended to lock Zoomable::lock_ while updating the underlying
//   drawable objects, otherwise screen tear may occur.
class Zoomable : public sf::Drawable {
 public:
  Zoomable() {}
  ~Zoomable() {}

  // Set the viewport (portion of the window to draw this on)
  // See
  // https://www.sfml-dev.org/documentation/2.5.1/classsf_1_1View.php#a8eaec46b7d332fe834f016d0187d4b4a
  void setViewport(const sf::FloatRect& viewport);

  // Zoom into/out of an (x,y) position by a factor
  // factors > 1 zoom in (make the image bigger), less than 1 zoom out
  // Zoom will attempt to redraw the scene with (x,y) as the new center,
  //   but if the requested center would put the edge of the scene in the
  //   drawing area, it will re-center to align the edge of the scene to
  //   the edge of the screen.
  // @param x position between 0 (left) and 1 (right)
  // @param y position between 0 (top) and 1 (bottom)
  // @param zoom_factor factor by which to zoom
  void Zoom(float x, float y, float zoom_factor);

  // Zoom as above, but with an (x,y) vector2f
  // @param rel position relative to the edges of the view
  // @param zoom factor factor by which to zoom
  void Zoom(const sf::Vector2f& rel, float zoom_factor);

  // render current textures
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // get an absolute pixel location from a relative (x,y)
  // @param rel (x,y) position relative to the edge of the view
  //   both x,y should be [0..1]
  // @returns vector with (x,y) corresponding to the absolute pixel loation
  sf::Vector2i PixelPosition(sf::Vector2f rel);

  // get an absolute fractional location from a relative (x,y)
  // this is equavilant to PxPosition() / (width, height)
  // @param rel (x,y) position relative to the edge of the view
  //   both x,y should be [0..1]
  // @returns vector with (x,y) corresponding to the absolute fractional loation
  sf::Vector2f FractionalPosition(sf::Vector2f rel);

 protected:
  // Set the total drawing area, and define which objects to draw
  // @param width of the objects to draw in pixels
  // @param height of the objects to draw in pixels
  // @param objs objects to draw when draw() is called
  void SetDrawable(int width, int height, const std::vector<sf::Drawable*>& objs);

  // Update the drawing objects to a new list
  // @param objs objects to draw when draw() is called
  void SetDrawable(const std::vector<sf::Drawable*>& objs);

  // Lock to synchronoize between updating the drawable object and
  //   actually drwaing to the screen.
  mutable std::mutex lock_;

 private:
  int width_ = 0;
  int height_ = 0;
  std::vector<sf::Drawable*> objs_;
  sf::View view_;
  float zoom_ = 1.0;
};
