#pragma once

#include <array>
#include <mutex>
#include <vector>

#include "system/component/inc/execnode.h"
#include "system/component/inc/zoomable.h"


// Class for drawing a scan as it happens
class ScanDraw : public ExecNode, public Zoomable {
 public:
  ScanDraw() {}
  ~ScanDraw() {}

  // Set the size for the scan
  // Dimensions are in <i,j,k> tuples.
  // i is the first dimension scanned, j is the second, and k is the third.
  void SetSize(int i, int j, int k);

  // Set the index that will be next written to <i, j, k>
  void SetIndex(int i, int j, int k);

  // Set which plane (k index) is being displayed
  // Changes relative to the plane currently being displayed, ie
  // SetViewRelative(1) will move to the next plane, and SetViewRelative(-1)
  //   will move to the previous plane.
  // If this would move the plane out of currently written planes, goes to the
  //   most current plane.
  void SetViewRelative(int k);

 private:
  void* Exec(void* data) override;

  void UpdateDisplay();

  // voxel data
  std::vector<double> voxel_;

  // dimensions of the scan
  std::array<int, 3> dim_;

  volatile int display_plane_ = -1;
  volatile int max_plane_ = -1;
  volatile int write_idx_ = 0;

  // maximum value seen so far
  volatile double max_val_ = 1.0;

  // SFML drawable object
  sf::Texture tx_;
  sf::Sprite sp_;
  std::vector<uint32_t> px_;

  // pixel writer lock
  std::mutex wr_lock_;

  // index/pointer lock
  std::mutex p_lock_;
};
