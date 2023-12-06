#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "system/component/inc/execnode.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/time.h"

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

// compute and display stats about a frame
class FrameStats : public ExecNode, public sf::Drawable {
 public:
  FrameStats() {}
  ~FrameStats() {}

  // Initialize the Stats object for frames with width/height/bits from fr
  // @param fr example frame
  void Init(const Frame* fr);

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

 private:
  // size of text
  static const int CHAR_SIZE = 24;

  // compute stats about the incoming frame
  void* Exec(void* data) override;

  sf::Font font_;
  std::vector<sf::Text> text_;
  std::vector<std::string> string_;
  mutable std::mutex mutex_;

  time_t millis_ = 0;
};
