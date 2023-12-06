#pragma once

#include "system/component/inc/circular_buffer.h"
#include "system/component/inc/execnode.h"


// Synchronize between nodes flowing through ExecNodes()
//   and user space.
// Example:
//   ExecNode e1;
//   ExecNode e2;
//   SyncNode s;
//   e2.AddProducer(&e1);
//   s.AddProducer(&e2);
//   e1.Consume(data_in);
//   void* data_out = s.Wait();
// data_out will now contain whatever was produced by data_in flowing
//   through e1 and e2.
class SyncNode : public ExecNode {
 public:
  SyncNode();
  ~SyncNode() {}

  // resize for a number of possible stored nodes
  // @param n number of stored nodes
  void resize(size_t n) override;

  // return a pointer to a data structure, or NULL if none available
  // data structure will remain valid until Get() is called next.
  void* Get();

  // Wait until a new node is received
  // @param seconds seconds to wait, if blank or negative wait indefinitely
  void* Wait(double seconds = -1);

 private:
  void* Exec(void* data) override;

  // whether a data structure is in use by a user
  //   ie, is the last result of Get() not NULL.
  volatile bool user_ = false;
  // keep track of the number of data structures this node has seen
  volatile int pushed_ = 0;
  volatile int popped_ = 0;
  CircularBuffer<void*> buf_;
  std::mutex mutex_;
};
