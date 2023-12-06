#pragma once

#include <cassert>
#include <mutex>
#include <vector>


// Class to allocate a fixed pool of data
// Works similarly to alloc() and free(), except no
// memory is dynamically allocated.  Instead, a pool of 
// data structures are allocated at construction, and each
// portioned out by calling Alloc() and returned to the pool
// with Free()
template <typename T>
class Pool {
 public:
  // Constructor
  Pool() {}

  ~Pool() {}

  // Resize the pool
  // @param size number of elements to store
  // @param value optional base value.  requires assignement operator
  void resize(size_t size, T value = T());

  // Get a free element from the pool
  // @returns element ready to be written
  T& Alloc();

  // Check for a free element in the pool
  // @returns pointer to element to be written, NULL if none available
  T* TryAlloc();

  // Return an element to the free pool
  // @param elt pointer to element to free
  void Free(T* elt);

  // Raw data storage
  // useful for startup allocation
  std::vector<T>& Raw();

  // Get the size of the pool
  size_t size();

 private:
  std::vector<T> data_;
  std::vector<bool> alloc_;
  std::mutex mutex_;
};


template <typename T>
void Pool<T>::resize(size_t size, T value) {
  data_.resize(size, value);
  alloc_.resize(size, false);
}


template <typename T>
T& Pool<T>::Alloc() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (int i = 0; i < data_.size(); ++i) {
    if (!alloc_[i]) {
      alloc_[i] = true;
      return data_[i];
    }
  }
  assert(false);
  return data_[0];
}


template <typename T>
T* Pool<T>::TryAlloc() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (int i = 0; i < data_.size(); ++i) {
    if (!alloc_[i]) {
      alloc_[i] = true;
      return &data_[i];
    }
  }
  return NULL;
}


template <typename T>
void Pool<T>::Free(T* elt) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (int i = 0 ; i < data_.size() ; ++i) {
    if (elt == &data_[i]) {
      assert(alloc_[i] == true);
      alloc_[i] = false;
      return;
    }
  }
}


template <typename T>
std::vector<T>& Pool<T>::Raw() {
  return data_;
}

template <typename T>
size_t Pool<T>::size() {
  return data_.size();
}
