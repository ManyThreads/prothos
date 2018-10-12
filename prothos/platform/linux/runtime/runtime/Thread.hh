#pragma once

#include "runtime/mlog.hh"
#include <cstddef>
#include <thread>
#include <array>

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

  void start(){
	MLOG_INFO(mlog::app, __func__, DVAR(&t));
	t = std::thread(&Thread::startup, this);
  }

  void join(){
	t.join();
  }

  void startup(){
	localThread = this;
	run();
  }

  virtual void run() = 0;
private:
  std::thread t;
};

template<size_t SIZE, class T>
class ThreadGroup
{
public:
  static constexpr const size_t num_threads = SIZE;

  ThreadGroup() {};

  void finalize(){
	for(auto &t : threads){
		t.join();
	}
  }

  void start()
  {
	  for(auto &t : threads){
		t.start();
	  }
  }
protected:
  std::array<T, SIZE> threads;
};

} //Prothos

