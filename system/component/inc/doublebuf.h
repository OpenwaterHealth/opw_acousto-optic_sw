#pragma once

#include <mutex>
#include <vector>


// threadsafe read/write double buffer
template <typename T>
class DoubleBuf {
 public:
  DoubleBuf() : doublebuf_(0) {}

  ~DoubleBuf() {}

  // resize the buffer.
  // DO NOT call once the buffer is in use, behavior is undefined
  // @param sz size to allocated
  // @param value default value for new array
  void resize(size_t sz, T value = T());

  // get the write buffer
  // @returns write buffer
  std::vector<T>& Write();

  // release the write buffer
  void WriteDone();

  // get the read buffer
  // @returns read buffer
  const std::vector<T>& Read();

  // release the read buffer
  void ReadDone();

 private:
  std::vector<T> data_[2];
  int doublebuf_;

  std::mutex read_, write_;
};


template <typename T>
void DoubleBuf<T>::resize(size_t sz, T value) {
  data_[0].resize(sz, value);
  data_[1].resize(sz, value);
}


template <typename T>
std::vector<T>& DoubleBuf<T>::Write() {
  write_.lock();

  return data_[doublebuf_];
}


template <typename T>
void DoubleBuf<T>::WriteDone() {
  // TODO(carsten): make this a proper reader/writer
  read_.lock();
  doublebuf_ ^= 1;
  read_.unlock();
  write_.unlock();
}


template <typename T>
const std::vector<T>& DoubleBuf<T>::Read() {
  read_.lock();
  return data_[doublebuf_ ^ 1];
}


template <typename T>
void DoubleBuf<T>::ReadDone() {
  read_.unlock();
}
