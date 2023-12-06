#include "system/component/inc/scandraw.h"

#include "system/component/inc/colormap.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/roi.h"

void ScanDraw::SetSize(int i, int j, int k) {
  dim_[0] = i;
  dim_[1] = j;
  dim_[2] = k;

  display_plane_ = -1;
  write_idx_ = 0;
  max_plane_ = -1;
  max_val_ = 1.0;

  voxel_.resize(i * j * k, 0);

  px_.resize(i * j, COLORMAP_JET[0]);

  tx_.create(i, j);
  sp_.setTexture(tx_);

  Zoomable::SetDrawable(i, j, std::vector<sf::Drawable*>({&sp_}));
}

void ScanDraw::SetIndex(int i, int j, int k) {
  write_idx_ = i + j * dim_[0] + k * dim_[0] * dim_[1];
}

void ScanDraw::SetViewRelative(int k) {
  if (max_plane_ < 0) return;

  p_lock_.lock();

  display_plane_ += k;
  if (display_plane_ >= max_plane_) display_plane_ = max_plane_;
  if (display_plane_ < 0) display_plane_ = 0;

  p_lock_.unlock();

  std::lock_guard<std::mutex> lock2(wr_lock_);
  UpdateDisplay();
}

void* ScanDraw::Exec(void* data) {
  p_lock_.lock();

  roi_tag* roi = ROI::GetTag((Frame*)data);
  assert(roi);
  assert(write_idx_ < dim_[0] * dim_[1] * dim_[2]);
  voxel_[write_idx_] = roi->roi;
  if (voxel_[write_idx_] > max_val_) max_val_ = voxel_[write_idx_];

  if (write_idx_ % (dim_[0] * dim_[1]) == 0) {
    if (display_plane_ == max_plane_) ++display_plane_;
    ++max_plane_;
  }
  ++write_idx_;

  p_lock_.unlock();

  if (display_plane_ == max_plane_ && wr_lock_.try_lock()) {
    UpdateDisplay();
    wr_lock_.unlock();
  }

  return data;
}

void ScanDraw::UpdateDisplay() {
  int plane_size = dim_[0] * dim_[1];
  double scale = (double)(COLORMAP_LEN - 1) / max_val_;
  double* plane = &voxel_[display_plane_ * plane_size];
  for (int i = 0; i < plane_size; ++i) {
    int idx = (int)(plane[i] * scale);
    if (idx >= COLORMAP_LEN) idx = COLORMAP_LEN - 1;
    assert(idx >= 0);
    assert(idx < COLORMAP_LEN);
    px_[i] = COLORMAP_JET[idx];
  }
  std::lock_guard<std::mutex> lock(Zoomable::lock_);
  tx_.update((uint8_t*)px_.data());
}
