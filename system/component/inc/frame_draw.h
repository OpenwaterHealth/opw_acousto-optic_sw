#pragma once

#include "system/component/inc/execnode.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/zoomable.h"

// needed for SFML calls to compile properly
#define SFML_STATIC
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

// Class for drawing frame objects in SFML
class FrameDraw : public Zoomable, public ExecNode {
 public:
  // Create default frame drawing object
  // will need to call Init() to resize to a given frame
  FrameDraw() {}

  // Create frame drawing object based on existing frame.
  FrameDraw(const Frame* fr);

  // Create a frame drawing object based on pixel width and height
  FrameDraw(int width, int height);

  // Initialize frame drawing to match a given frame
  // @param fr frame parameters (height/width/bits) to draw
  void Init(const Frame* fr);

  // Initialize a frame drawing to match a frame's height/width
  // @param width width of a frame
  // @param height height of a frame
  void Init(int width, int height);

  ~FrameDraw();

  // Update the frame drawing object with new frame data
  // Frame data height and width must match the call to the constructor
  void Update(const Frame* fr);

 private:
  void* Exec(void* data);

  std::vector<uint32_t> px_;
  int width_ = 0;
  int height_ = 0;

  std::mutex wr_lock_;
  sf::Texture tx_;
  sf::Sprite sp_;
};
