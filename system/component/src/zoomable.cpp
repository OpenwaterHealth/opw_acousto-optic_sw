#include "system/component/inc/zoomable.h"


void Zoomable::setViewport(const sf::FloatRect& viewport) {
  view_.setViewport(viewport);
}


void Zoomable::Zoom(float x, float y, float zoom_factor) {
  sf::Vector2f c = view_.getCenter();
  zoom_ *= zoom_factor;
  if (zoom_ < 1) zoom_ = 1;

  // set new center based on requested (x,y)
  c.x += (x - 0.5f) * width_ / zoom_;
  c.y += (y - 0.5f) * height_ / zoom_;

  // clip the edge
  if (c.x + width_ / zoom_ / 2 > width_) c.x = width_ - width_ / zoom_ / 2;
  if (c.x - width_ / zoom_ / 2 < 0) c.x = width_ / zoom_ / 2;
  if (c.y + height_ / zoom_ / 2 > height_) c.y = height_ - height_ / zoom_ / 2;
  if (c.y - height_ / zoom_ / 2 < 0) c.y = height_ / zoom_ / 2;

  view_.setSize(width_ / zoom_, height_ / zoom_);
  view_.setCenter(c);
}


void Zoomable::Zoom(const sf::Vector2f& rel, float zoom_factor) {
  Zoom(rel.x, rel.y, zoom_factor);
}


sf::Vector2i Zoomable::PixelPosition(sf::Vector2f rel) {
  sf::Vector2f c = view_.getCenter();
  c.x += (rel.x - 0.5f) * width_ / zoom_;
  c.y += (rel.y - 0.5f) * height_ / zoom_;
  return sf::Vector2i(int(c.x), int(c.y));
}


sf::Vector2f Zoomable::FractionalPosition(sf::Vector2f rel) {
  sf::Vector2f c = view_.getCenter();
  c.x += (rel.x - 0.5f) * width_ / zoom_;
  c.y += (rel.y - 0.5f) * height_ / zoom_;
  c.x /= width_;
  c.y /= height_;
  return c;
}


void Zoomable::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  std::lock_guard<std::mutex> lock(lock_);
  target.setView(view_);
  for (sf::Drawable* dr : objs_) {
    target.draw(*dr, states);
  }
  target.setView(target.getDefaultView());
}


void Zoomable::SetDrawable(int width, int height,
                           const std::vector<sf::Drawable*>& objs) {
  width_ = width;
  height_ = height;
  objs_ = objs;
  view_.reset(sf::FloatRect(0, 0, float(width_), float(height_)));
}


void Zoomable::SetDrawable(const std::vector<sf::Drawable*>& objs) {
  objs_ = objs;
}
