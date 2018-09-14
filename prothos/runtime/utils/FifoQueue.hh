#pragma once

#include <queue>
#include "runtime/Mutex.hh"

template <typename T>
class FifoQueue
{
 public:

  T* pop() {
	mythos::Mutex::Lock l(mutex);
	if(queue.empty()) return nullptr;
    auto item = queue.front();
    queue.pop();
    return item;
  }

  void push(const T* item) {
	mythos::Mutex::Lock l(mutex);
    queue.push(item);
  }

  int size(){
	mythos::Mutex::Lock l(mutex);
    return queue.size();
  }

 private:
  std::queue<T*> queue;
  mythos::Mutex mutex;
};
