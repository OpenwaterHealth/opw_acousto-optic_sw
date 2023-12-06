#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

// circular buffer class
// @param T type of data to be stored in this buffer
// Threadsafe between Push() and Pop() functions
template <typename T>
class CircularBuffer {
 public:
  CircularBuffer() {}

  // Construct a buffer with a given size
  // @param n number of elements in queue
  CircularBuffer(size_t n, T value = T());

  ~CircularBuffer() {}

  // Resize buffer
  // Do not call after the buffer is in use
  // @param n number of elements
  // @param value default value for new elements
  void resize(size_t n, T value = T());

  // Get next element to be pushed
  // @returns element to be pushed
  T& Next();

  // Push element from Next();
  // Example: 
  //   complex& c = foo.Next();
  //   c.real = 3;
  //   c.imag = 4;
  //   foo.Push();
  void Push();

  // Push element by reference
  // @param data single element to push
  void Push(const T& data);

  // Peek at the nth element to be popped
  // @param n (optional) how far to peek into the buffer
  // @returns the nth element to be popped
  T& Peek(size_t n = 0);

  // Pop multiple elements without looking at them
  // @param n number of elements to pop
  // Example:
  //   complex c0 = Peek(0);
  //   complex c1 = Peek(1);
  //   Pop(2);
  void Pop(size_t n);

  // Pop single element
  T& Pop();

  // Check number of elements available for push
  // @return number of elements available for push
  size_t PushAvailable();
  
  // Check the number of elements stored in the buffer
  // @return number of elements available for pop
  size_t PopAvailable();

  // Get the raw buffer
  // Needed for constructing / destructing heap elements
  std::vector<T>& Raw();

  size_t size();
  
 private:
  volatile int head_ = 0;
  volatile int tail_ = 0;
  volatile int pushed_ = 0;
  volatile int popped_ = 0;
  std::vector<T> data_;
};


template<typename T>
CircularBuffer<T>::CircularBuffer(size_t len, T value) {
  data_.resize(len, value);
}


template<typename T>
void CircularBuffer<T>::resize(size_t len, T value) {
  assert(pushed_ == popped_);
  data_.resize(len, value);
  head_ = 0;
  tail_ = 0;
  pushed_ = 0;
  popped_ = 0;
}


template<typename T>
void CircularBuffer<T>::Push(const T& data) {
  data_[head_] = data;
  head_ = head_ + 1 >= data_.size() ? 0 : head_ + 1;
  ++pushed_;
}


template<typename T>
T& CircularBuffer<T>::Next() {
  assert(data_.size() + popped_ - pushed_ >= 1);
  return data_[head_];
}


template<typename T>
void CircularBuffer<T>::Push() {
  assert(data_.size() + popped_ - pushed_ >= 1);
  head_ = head_ + 1 >= data_.size() ? 0 : head_ + 1;
  ++pushed_;
}


template<typename T>
T& CircularBuffer<T>::Pop() {
  assert(pushed_ - popped_ >= 1);
  int tail = tail_;
  tail_ = tail_ + 1 >= data_.size() ? 0 : tail_ + 1;
  ++popped_;
  return data_[tail];
}


template<typename T>
T& CircularBuffer<T>::Peek(size_t n) {
  assert(pushed_ - popped_ >= (n + 1));
  n = (tail_ + n) % data_.size();
  return data_[n];
}


template<typename T>
void CircularBuffer<T>::Pop(size_t n) {
  assert(pushed_ - popped_ >= n);
  tail_ = (tail_ + n) % data_.size();
  popped_ += n;
}

template<typename T>
size_t CircularBuffer<T>::size() {
  return data_.size();
}


template<typename T>
std::vector<T>& CircularBuffer<T>::Raw() {
  return data_;
}


template<typename T>
size_t CircularBuffer<T>::PushAvailable() {
  return data_.size() + popped_ - pushed_;
}


template<typename T>
size_t CircularBuffer<T>::PopAvailable() {
  return pushed_ - popped_;
}
