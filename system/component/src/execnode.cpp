#include "system/component/inc/execnode.h"

template <typename T>
int idx(std::vector<T>& v, const T& elt) {
  for (int i = 0; i < v.size(); ++i) {
    if (v[i] == elt) return i;
  }
  return -1;
}

ExecNode::ThreadManager::ThreadManager() {
  scheduled_nodes_.resize(MAX_BACKLOG);
  threads_.resize(THREADS);
  for (std::thread& t : threads_) {
    t = std::thread(&ThreadManager::Thread, this);
  }
}

ExecNode::ThreadManager::~ThreadManager() {
  stop_ = true;
  wake_thread_.notify_all();
  for (std::thread& t : threads_) {
    t.join();
  }
}

void ExecNode::ThreadManager::Schedule(ExecNode* en, void* data) {
  std::lock_guard<std::mutex> lock(wr_mutex_);
  if (!scheduled_nodes_.PushAvailable()) {
    printf("ExecNode maximum backlog reached\n");
    assert(0);
    exit(-1);
  }
  scheduled_nodes_.Push(std::pair<ExecNode*, void*>(en, data));
  wake_thread_.notify_one();
}

void ExecNode::ThreadManager::Thread() {
  while (true) {
    std::pair<ExecNode*, void*> next;
    {
      std::unique_lock<std::mutex> lk(rd_mutex_);
      wake_thread_.wait(
          lk, [&] { return scheduled_nodes_.PopAvailable() || stop_; });
      if (stop_) return;
      next = scheduled_nodes_.Pop();
    }
    next.first->Execute(next.second);
  }
}

void ExecNode::ThreadManager::SetNumberThreads(size_t threads) {
  stop_ = true;
  wake_thread_.notify_all();
  for (std::thread& t : threads_) {
    t.join();
  }
  stop_ = false;
  threads_.resize(threads);
  for (std::thread& t : threads_) {
    t = std::thread(&ThreadManager::Thread, this);
  }
}

ExecNode::ThreadManager ExecNode::thread_manager_;

static const int BUFLEN = 10;

ExecNode::ExecNode() {
  down_queue_.resize(BUFLEN);
  up_queue_.resize(BUFLEN);
}

void ExecNode::Join(ExecNode* producer, ExecNode* consumer) {
  producer->consumers_.push_back(consumer);
  producer->up_.push_back(0);

  consumer->producers_.push_back(producer);
  consumer->down_.push_back(0);

  if (producer->down_queue_.size() > consumer->down_queue_.size()) {
    consumer->resize(producer->down_queue_.size());
  } else {
    producer->resize(consumer->down_queue_.size());
  }
}

void ExecNode::AddProducer(ExecNode* en) { Join(en, this); }
void ExecNode::AddConsumer(ExecNode* en) { Join(this, en); }

void ExecNode::resize(size_t n) {
  if (down_queue_.size() == n) return;
  down_queue_.resize(n);
  up_queue_.resize(n);

  for (ExecNode* en : consumers_) {
    en->resize(n);
  }
}

void ExecNode::SetNumberThreads(size_t threads) {
  thread_manager_.SetNumberThreads(threads);
}

bool ExecNode::IsLeaf() { return consumers_.size() == 0; }

bool ExecNode::IsRoot() { return producers_.size() == 0; }

void ExecNode::Produce(void* data) {
  assert(IsRoot());
  mutex_.lock();
  up_queue_.Push(data);
  std::vector<bool> run_list;
  for (ExecNode* consumer : consumers_) {
    run_list.push_back(consumer->Schedule(data, this));
  }
  mutex_.unlock();
  for (int i = 0; i < run_list.size(); ++i) {
    if (run_list[i]) {
      thread_manager_.Schedule(consumers_[i], data);
    }
  }
}

void ExecNode::Consume(void* data) {
  assert(IsRoot());
  mutex_.lock();
  down_queue_.Push(data);
  up_queue_.Push(data);
  mutex_.unlock();
  thread_manager_.Schedule(this, data);
}

bool ExecNode::IsExecDone() { return !up_queue_.PopAvailable(); }

bool ExecNode::Schedule(void* data, ExecNode* producer) {
  std::lock_guard<std::mutex> lock(mutex_);

  // check if we need to synchronize between multiple producers
  if (sync_ && down_.size() > 1) {
    ++down_[idx(producers_, producer)];
    for (int& i : down_) {
      if (i == 0) {
        return false;
      }
    }

    for (int& i : down_) --i;
  }
  assert(down_queue_.PushAvailable());
  down_queue_.Push(data);
  up_queue_.Push(data);
  return true;
}

void ExecNode::Execute(void* data) {
  void* rv = NULL;
  if (data) rv = Exec(data);

  // Wait until first data in queue is done
  std::unique_lock<std::mutex> lck(mutex_);
  if (down_queue_.Peek() != data) {
    order_.wait(lck, [&] { return down_queue_.Peek() == data; });
  }

  std::vector<bool> run_list;

  if (IsLeaf()) {
    if (rv) AtExit(rv);
    rv = up_queue_.Pop();
    for (ExecNode* producer : producers_) {
      producer->CleanUp(rv, this);
    }
    assert(down_queue_.PopAvailable());
    down_queue_.Pop();
    lck.unlock();
    order_.notify_all();

  } else {
    for (ExecNode* consumer : consumers_) {
      run_list.push_back(consumer->Schedule(rv, this));
    }
    assert(down_queue_.PopAvailable());
    down_queue_.Pop();
    lck.unlock();
    order_.notify_all();

    Run(run_list, rv);
  }

  
}

void ExecNode::CleanUp(void* data, ExecNode* consumer) {
  // Always need to wait until all downstream nodes are done
  //   with the current set of data (otherwise we could free
  //   data still being used by a consumer)
  std::lock_guard<std::mutex> lock(cleanup_);
  if (up_.size() > 1) {
    ++up_[idx(consumers_, consumer)];
    for (int i : up_) {
      if (i == 0) {
        return;
      }
    }

    for (int& i : up_) --i;
  }

  if (data) AtExit(data);
  assert(up_queue_.PopAvailable());
  void* up_data = up_queue_.Pop();

  for (ExecNode* producer : producers_) {
    producer->CleanUp(up_data, this);
  }
}

void ExecNode::Run(const std::vector<bool>& run_list, void* data) {
  // Minor optimization for single chain of ExecNodes - rather than
  //   using the scheduler to schedule then run the next node, just
  //   run it automatically, and log any additional nodes to be run.
  ExecNode* en = NULL;
  for (int i = 0; i < run_list.size(); ++i) {
    if (run_list[i]) {
      if (en == NULL) {
        en = consumers_[i];
      } else {
        thread_manager_.Schedule(consumers_[i], data);
      }
    }
  }
  if (en) en->Execute(data);
}

void ExecNode::SetSync(bool sync) { sync_ = sync; }
