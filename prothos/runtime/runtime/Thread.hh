#pragma once

#include "mythos/init.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/Portal.hh"
#include "app/mlog.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/PageMap.hh"
#include "runtime/CapMap.hh"
#include "mythos/invocation.hh"

extern mythos::Portal portal;
extern mythos::KernelMemory kmem;
extern mythos::PageMap myAS;
extern mythos::CapMap myCS;
extern mythos::SimpleCapAllocDel capAlloc;

void* myBody(void*){
  MLOG_INFO(mlog::app, "hello body");
	return nullptr;
}

class Thread{
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
  
  mythos::optional<void> start(size_t rank, mythos::Frame &f, uintptr_t vaddr);
};


mythos::optional<void> Thread::start(size_t rank, mythos::Frame &f, uintptr_t vaddr)
{
  this->rank = rank;
  mythos::PortalLock pl(portal);
  MLOG_INFO(mlog::app, __func__);
  ec.create(pl, kmem, myAS, myCS, mythos::init::SCHEDULERS_START+rank, &stack[STACK_SIZE], myBody, nullptr).wait();

  p.setbuf(reinterpret_cast<mythos::InvocationBuf*>(vaddr + rank * sizeof(mythos::InvocationBuf)));
  p.create(pl, kmem).wait();
  p.bind(pl, f, rank * sizeof(mythos::InvocationBuf) ,ec.cap()).wait();
}

template<size_t SIZE>
class ThreadGroup {
public:
  static constexpr const size_t num_threads = SIZE;

  ThreadGroup(){};
  
  void start(){
  mythos::Frame f(capAlloc());
  uintptr_t vaddr = 26*1024*1024;
  {
	  mythos::PortalLock pl(portal);
	// todo: round up size
	f.create(pl, kmem,2 * 1024 * 1024 /*  sizeof(mythos::InvocationBuf) * num_threads */, 4*1024).wait();
	  myAS.mmap(pl, f, vaddr, 2 * 1024 * 1024 /* sizeof(mythos::InvocationBuf) * num_threads */, 0x1).wait();
  }
	  for (size_t i = 0; i < num_threads; ++i) {
		  thread[i].start(i, f, vaddr);
	  }
  }

  Thread thread[num_threads];
};
