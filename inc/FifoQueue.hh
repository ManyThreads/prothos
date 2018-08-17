#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template <typename T>
class FifoQueue
{
 public:

  T pop() {
    std::unique_lock<std::mutex> mlock(mutex);
    while (queue.empty()) {
      cond.wait(mlock);
    }
    auto item = queue.front();
    queue.pop();
    return item;
  }

  void push(const T& item) {
    std::unique_lock<std::mutex> mlock(mutex);
    queue.push(item);
    mlock.unlock();
    cond.notify_one();
  }

  void push(T&& item) {
    std::unique_lock<std::mutex> mlock(mutex);
    queue.push(std::move(item));
    mlock.unlock();
    cond.notify_one();
  }

  int size(){
    return queue.size();
  }

 private:
  std::queue<T> queue;
  std::mutex mutex;
  std::condition_variable cond;
};
