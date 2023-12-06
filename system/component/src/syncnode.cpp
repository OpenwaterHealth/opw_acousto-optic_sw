#include "system/component/inc/syncnode.h"


SyncNode::SyncNode() {
  resize(10);
}


void SyncNode::resize(size_t n) {
  buf_.resize(n);
  ExecNode::resize(n);
}


void* SyncNode::Get() {
  if (user_) {
    buf_.Pop();
    ++popped_;
    user_ = false;
  }

  if (buf_.PopAvailable()) {
    user_ = true;
    return buf_.Peek();
  }
  return NULL;
}


void* SyncNode::Wait(double seconds) {
  using std::chrono::steady_clock;

  steady_clock::time_point end =
      steady_clock::now() + std::chrono::milliseconds((int)(seconds * 1000));
  void* d;
  while ((d = Get()) == NULL) {
    if (seconds > 0 && steady_clock::now() > end) {
      return NULL;
    }
  }
  return d;
}


void* SyncNode::Exec(void* data) {
  mutex_.lock();
  int wait = ++pushed_;
  buf_.Push(data);
  mutex_.unlock();

  while (wait - popped_ > 0) {
    std::this_thread::yield();
  }
  return data;
}
