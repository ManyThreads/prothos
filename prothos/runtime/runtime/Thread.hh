#pragma once

#include "mythos/init.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "runtime/Portal.hh"
#include "runtime/mlog.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/PageMap.hh"
#include "runtime/CapMap.hh"
#include "mythos/invocation.hh"
#include "runtime/tls.hh"

extern mythos::Portal portal;
extern mythos::KernelMemory kmem;
extern mythos::PageMap myAS;
extern mythos::CapMap myCS;
extern mythos::SimpleCapAllocDel capAlloc;

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
    : ec(capAlloc())
    , p(capAlloc(), nullptr)
  {}

  static constexpr size_t STACK_SIZE = 4096;

  size_t rank;
  mythos::ExecutionContext ec;
  mythos::Portal p;
  uint8_t stack[STACK_SIZE];

  virtual void run() = 0;
  static void* startup(void* ctx) { 
	  localThread = static_cast<Thread*>(ctx);
	  static_cast<Thread*>(ctx)->run(); 
	  return nullptr; 
  }

  void start(size_t rank, mythos::Frame &f, uintptr_t vaddr)
  {
    this->rank = rank;
    mythos::PortalLock pl(portal);
    MLOG_INFO(mlog::app, __func__);
	/* create execution context */
    auto tls = mythos::setupNewTLS();
    ec.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START+rank+1)
    .prepareStack(&stack[STACK_SIZE]).startFun(&startup, this)
    .suspended(false).fs(tls)
    .invokeVia(pl).wait();

	/* create portal */
	p.setbuf(reinterpret_cast<mythos::InvocationBuf*>(vaddr + rank * sizeof(mythos::InvocationBuf)));
	p.create(pl, kmem).wait();
	p.bind(pl, f, rank * sizeof(mythos::InvocationBuf),ec.cap()).wait();
    MLOG_INFO(mlog::app, "this ", DVARhex(this));
  }
};

template<size_t SIZE, class T>
class ThreadGroup
{
public:
  static constexpr const size_t num_threads = SIZE;

  ThreadGroup() {};

  static_assert(
    std::is_base_of<Thread, T>::value,
    "T must be a descendant of Thread"
  );

  void start()
  {
	/* allocate frame used as buffer for the portal of every thread */
    mythos::Frame f(capAlloc());
    uintptr_t vaddr = 26 * 1024 * 1024;
    {
      mythos::PortalLock pl(portal);
      // todo: round up size
      f.create(pl, kmem,2 * 1024 * 1024 /*  sizeof(mythos::InvocationBuf) * num_threads */, 4*1024).wait();
      myAS.mmap(pl, f, vaddr, 2 * 1024 * 1024 /* sizeof(mythos::InvocationBuf) * num_threads */, 0x1).wait();
    }
    for (size_t i = 0; i < num_threads; ++i)
    {
      threads[i].start(i, f, vaddr);
    }
  }
protected:
  T threads[num_threads];
};

} //Prothos

