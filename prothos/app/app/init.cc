/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MIT License -- MyThOS: The Many-Threads Operating System
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */

#include "mythos/init.hh"
#include "mythos/invocation.hh"
#include "mythos/protocol/CpuDriverKNC.hh"
#include "mythos/PciMsgQueueMPSC.hh"
#include "cpu/hwthread_pause.hh"
#include "runtime/Portal.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/Example.hh"
#include "runtime/PageMap.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "app/mlog.hh"
#include <cstdint>
#include "util/optional.hh"

mythos::InvocationBuf* msg_ptr asm("msg_ptr");
int main() asm("main");

constexpr uint64_t stacksize = 4*4096;
char initstack[stacksize];
char* initstack_top = initstack+stacksize;

mythos::Portal portal(mythos::init::PORTAL, msg_ptr);
mythos::CapMap myCS(mythos::init::CSPACE);
mythos::PageMap myAS(mythos::init::PML4);
mythos::KernelMemory kmem(mythos::init::KM);
mythos::SimpleCapAllocDel capAlloc(portal, myCS, mythos::init::APP_CAP_START,
                                  mythos::init::SIZE-mythos::init::APP_CAP_START);

char threadstack[stacksize];
char* thread1stack_top = threadstack+stacksize/2;
char* thread2stack_top = threadstack+stacksize;

/**
 * Centralized barrier based on:
 * - Mellor-CrummeyScott1991a - Algorithms for Scalable Synchronization on Shared-memory Multiprocessors
 */
class SenseBarrier {
public:
  SenseBarrier(size_t count) : _count(count), _counter(count) {}

  typedef unsigned char sense_t;

  static void init(sense_t& local_sense) { local_sense = false; }
  void wait(sense_t& local_sense)
  {
    local_sense = !local_sense;
    if (_counter.fetch_sub(1) == 1) {
      _counter.store(_count);
      _global_sense.store(local_sense);
    } else {
      while (_global_sense.fetch_or(0) != local_sense) {
        mythos::hwthread_pollpause();
      }
    }
  }

private:
  size_t _count;
  std::atomic<size_t> _counter{0};
  std::atomic<sense_t> _global_sense{false};
};

template<class D>
class TcbBase {
public:
  typedef D tcb_t;
private:
  tcb_t* _this;
public:
  TcbBase() : _this(static_cast<tcb_t*>(this)) {};

  uintptr_t gs()
  {
    return uintptr_t(&_this);
  }

  static tcb_t& local() {
      tcb_t* ptr;
      static_assert(sizeof(ptr) == 8, "Expected different pointer size.");
      asm("movq %%gs:0,%0" : "=r" (ptr));
      return *ptr;
  };
};

template<class T>
class ManualLifetime {
public:

  template<class... ARGS>
  T& create(ARGS&&... args) {
    new(&_storage->_obj) T(std::forward<ARGS>(args)...);
    return _storage._obj;
  }

  void destroy() {_storage->_obj.~T(); }

  T& operator*() { return _storage._obj; }
  T const& operator*() const { return _storage._obj; }

  T* operator->() { return &_storage._obj; }
  T const* operator->() const { return &_storage._obj; }

private:
  union Storage {
    Storage() {};
    ~Storage() {};
    T _obj;
  } _storage;
};

class Thread : public TcbBase<Thread> {
public:
  static constexpr size_t STACK_SIZE = 4096;
  // parameters for starting
  size_t rank = 0;
  size_t pages = 0;
  // created by main thread
  ManualLifetime<mythos::Portal> portal;
  ManualLifetime<mythos::Frame> ib;
  size_t ib_offset;
  ManualLifetime<mythos::ExecutionContext> ec;
  ManualLifetime<mythos::CapMap> cs;
  mythos::CapPtr sc = mythos::null_cap;
  mythos::CapPtr pml4 = mythos::null_cap;
  // created by this thread
  mythos::CapPtr pml3cc = mythos::null_cap;
  mythos::CapPtr pml3nc = mythos::null_cap;
  SenseBarrier* barrier;
  SenseBarrier::sense_t local_sense;

  uint8_t stack[STACK_SIZE];

  mythos::optional<void> start(mythos::PortalLock& pl);

  static void* startup(void* ctx) { static_cast<Thread*>(ctx)->startup(); return nullptr; }
  void startup();
};

mythos::optional<void> Thread::start(mythos::PortalLock& pl)
{
  { // create portal
    auto res = portal->create(pl, kmem).wait();
    if (!res) return res;
  }
  { // create EC
    auto res = ec->create(pl, kmem, myAS, myCS, sc,
        &stack[STACK_SIZE], &startup, this).wait();
    if (!res) return res;
  }
  { // bind portal
    auto res = portal->bind(pl, *ib, ib_offset, ec->cap()).wait();
    if (!res) return res;
  }
  mythos::syscall_notify(ec->cap());
}

  void Thread::startup()
  {
    // init without portal
    barrier->init(local_sense);
    // wait for signal ... this is not save unless an additional flag is checked
    mythos::ISysretHandler::handle(mythos::syscall_wait());
    mythos::PortalLock pl(*portal);
    { // set up access to Thread via Thread::local()
      auto res = ec->setFSGS(pl, 0u, gs()).wait();
      ASSERT(res);
    }
    barrier->wait(local_sense);
    // set up local cap map
    //cs->create(pl, kmem, 6, 4, 0); // <- global cap map
    {
      auto res = cs->create(pl, kmem, 6, 16, 0);
      ASSERT(res);
    }
    // - reference caps from init CS
    // insert it into global CS
    // create 2 PML2 tables (CC+NC)
    // create 2 PML3 tables (CC+NC)
    barrier->wait(local_sense);
    // create PML4 table
    // - 1. PML3 from init AS
    // - own CC PML3
    // - other PML3 as NC
    // switch to new cap map
    // switch to new address space
    barrier->wait(local_sense);
    // todo: do sth. productive
  }



template<size_t SIZE>
class ThreadGroup {
public:
  ThreadGroup(mythos::CapPtr cap);
  mythos::optional<void> start(mythos::PortalLock& pl);
private:
  static constexpr const size_t num_threads = SIZE;
  Thread thread[SIZE];
  mythos::CapMap cs;
};

template<size_t SIZE>
ThreadGroup<SIZE>::ThreadGroup(mythos::CapPtr cap)
  : cs(cap)
{
}

template<size_t SIZE>
mythos::optional<void> ThreadGroup<SIZE>::start(mythos::PortalLock& pl)
{
  { // set up new global cspace
    auto res = cs.create(pl, kmem, 6, 4, 0);
    ASSERT(res);
  }
  for (size_t i = 0; i < num_threads; ++i) {
  }
}


void* thread_main(void* ctx)
{
  MLOG_INFO(mlog::app, "hello thread!", DVAR(ctx));
  mythos::ISysretHandler::handle(mythos::syscall_wait());
  MLOG_INFO(mlog::app, "thread resumed from wait", DVAR(ctx), DVAR(&Thread::local()));
  return 0;
}

int main()
{
  MLOG_ERROR(mlog::app, "application is starting blablub :)", DVARhex(msg_ptr), DVARhex(initstack_top));
  ThreadGroup<4> threads(mythos::null_cap);
  return 0;
}

