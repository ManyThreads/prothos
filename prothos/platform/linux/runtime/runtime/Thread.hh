#pragma once

#include <cstddef>
#include <thread>

namespace Prothos{
	class Thread;
}

extern thread_local Prothos::Thread* localThread;

namespace Prothos{



static Thread* getLocalThread(){
	return localThread;
}

class Thread {
public:

  Thread()
  {}


  virtual void run() = 0;

};

template<size_t SIZE, class T>
class ThreadGroup
{
public:
  static constexpr const size_t num_threads = SIZE;

  ThreadGroup() {};

  //static_assert(
    //std::is_base_of<Thread, T>::value,
    //"T must be a descendant of Thread"
  //);

  void start()
  {
  }
protected:
  T threads[num_threads];
};

} //Prothos

