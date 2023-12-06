#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "circular_buffer.h"


// Class to chain execution of processing on data
// Execution on each piece of data is handled in background threads managed
//   by a threadmanager common to all ExecNodes
// Users must first set up the desired chain using AddProducer().
// Thereafter, if a node Produce()s a piece of data, it will automatically
//   send it downstream to all consumers.
// Each ExecNode() will run Exec() on each piece of incoming data, and
//   send the returned value downstream to all consumers
// The function AtExit() will run after all consumers have finished with
//   the data, and any required cleanup can be performed.
class ExecNode {
 public:
  // Basic constructor.  Allows for up to 10 simultaneous threads
  //   executing for a single node.
  // @param threaded whether this node should spawn a new thread per
  //   piece of incoming data
  ExecNode();

  virtual ~ExecNode() {}

  // Add a producer this FrameNode will consume frames from
  // By default, the FrameNode will wait for all producer nodes to
  //   emit the SAME frame. once this node has receieved the same
  //   frame from each of its producers, it will call Exec() on that
  //   frame and then Produce() to all frame nodes listening to this node.
  // If sync is false, it will Exec() and Produce() on all frames incoming.
  void AddProducer(ExecNode* en);
  void AddConsumer(ExecNode* en);
  static void Join(ExecNode* producer, ExecNode* consumer);


  // Manually send data to this ExecNode, as if a producer had
  //   sent the data to it.
  // @note WILL run this ExecNode's Exec() function
  // @param data data to consumer
  void Consume(void* data);

  // Manually send data to all consumers of this ExecNode, as if this
  //   execnode had produced the data itself
  // All consumer nodes will execute in a separate thread
  // @note will NOT run this ExecNodes Exec() function
  //   before sending downstream to consumers
  // @param data data to produce.
  void Produce(void* data);

  // Check if this node is the end of the chain (leaf in the tree)
  // @returns true if there are no ExecNodes consuming from this producer
  bool IsLeaf();

  // Check if this node is the start of the chain (root in the tree)
  // @returns true if there are no ExecNodes producing to this consumer
  bool IsRoot();

  // Set if the node will wait for the same frame on all producers
  void SetSync(bool sync);

  // Check if this ExecNode has any data waiting to be processed
  bool IsExecDone();

  // Set the maximum number of threads on this system
  static void SetNumberThreads(size_t threads);

  // Resize to queue up to n operations
  // If a derived class overloads this function, be sure to call
  //   ExecNode::resize to make sure all elements in a chain have the
  //   same queue size
  virtual void resize(size_t n);

 protected:
  // Do the work this node is expected to on this frame
  // @param data data to operate on
  // @returns pointer to result from this node
  virtual void* Exec(void* data) { return data; }

  // Perform any cleanup needed after the produced threads finish
  // @param return value from the Exec() of this node
  virtual void AtExit(void* data) {}

 private:
  // Run a nodes Exec(), potentially call downstream nodes
  //   Exec or upstream nodes AtExit() depending on producers/
  //   consumers.
  void Execute(void* data);

  // Check if a downstream node should be run
  bool Schedule(void* data, ExecNode* producer);

  // Run upstream node cleanup
  void CleanUp(void* data, ExecNode* consumer);

  // Run scheduled ExecNodes based on run_list
  void Run(const std::vector<bool>& run_list, void* data);


  // Class used by ExecNodes to manage thread execution
  class ThreadManager {
   public:
    ThreadManager();
    ~ThreadManager();

    void Schedule(ExecNode* en, void* data);

    void Thread();

    void SetNumberThreads(size_t threads);

   private:
    // number of concurrent threads handling ExecNode execution
    static const int THREADS = 16;
    // maximum backlog of data waiting for execution.  Overflow triggers a
    // program exit
    static const int MAX_BACKLOG = 2000;

    std::vector<std::thread> threads_;
    bool stop_ = false;

    CircularBuffer<std::pair<ExecNode*, void*>> scheduled_nodes_;
    std::mutex wr_mutex_;
    std::mutex rd_mutex_;
    // Semaphore to alert sleeping threads to wake up, either due to
    //   new data in the queue or shutting down.
    std::condition_variable wake_thread_;
  };
  static ThreadManager thread_manager_;

  std::vector<ExecNode*> producers_;
  std::vector<ExecNode*> consumers_;

  CircularBuffer<void*> down_queue_;
  std::vector<int> down_;

  CircularBuffer<void*> up_queue_;
  std::vector<int> up_;

  std::mutex mutex_;
  std::mutex cleanup_;
  // Semaphore to alert sleeping ExecNode to check if it is next in line
  //   to be run.
  std::condition_variable order_;
  bool sync_ = true;
};
