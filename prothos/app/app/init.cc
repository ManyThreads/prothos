#include "mythos/init.hh"
#include "mythos/PciMsgQueueMPSC.hh"
#include "mythos/invocation.hh"
#include "mythos/protocol/CpuDriverKNC.hh"
#include "runtime/CapMap.hh"
#include "runtime/Example.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/PageMap.hh"
#include "runtime/Portal.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "runtime/mlog.hh"
#include "runtime/tls.hh"
#include "runtime/umem.hh"
#include "util/optional.hh"

// Prothos Runtime
#include "thread/Thread.hh"
using namespace prothos;

// hack for compiling/linking std::shared_ptr
extern "C" int sched_yield(void) {
  return 0;
}

#include <cstddef>
#include <cstdint>

#include <vector>

using byte = uint8_t;

constexpr size_t STACK_SIZE = 1024 * 1024;
byte STACK[STACK_SIZE];

// externally required variables
byte* initstack_top = STACK + STACK_SIZE;
mythos::InvocationBuf* msg_ptr;

// fixed addresses
const uintptr_t PORTAL_POOL_ADDR = 26 * 1024 * 1024;
const uintptr_t HEAP_ADDR = 28 * 1024 * 1024;

// OS related global variables
mythos::Portal PORTAL(mythos::init::PORTAL, msg_ptr);
mythos::CapMap CAPABILITY_SPACE(mythos::init::CSPACE);
mythos::PageMap ADDRESS_SPACE(mythos::init::PML4);
mythos::KernelMemory KERNEL_MEM(mythos::init::KM);
mythos::SimpleCapAllocDel
    CAP_ALLOC(PORTAL, CAPABILITY_SPACE, mythos::init::APP_CAP_START,
              mythos::init::SIZE - mythos::init::APP_CAP_START);

class HostChannel {
public:
  using CtrlChannel = mythos::PCIeRingChannel<128, 8>;
  void init() {
    ctrl_to_host.init();
    ctrl_from_host.init();
  }

  CtrlChannel ctrl_to_host;
  CtrlChannel ctrl_from_host;
};

mythos::PCIeRingProducer<HostChannel::CtrlChannel> APP_2_HOST;
mythos::PCIeRingProducer<HostChannel::CtrlChannel> HOST_2_APP;

void init_portal_pool();
void init_heap();

extern "C" int main() {
  init_portal_pool();
  init_heap();
  MLOG_INFO(mlog::app, "heap initialized...");

  auto thread_main = []() {
    MLOG_INFO(mlog::app, "thread spawned");

    volatile uintptr_t counter = 0;
    while (counter <= 1'000'000) counter++;

    auto& thread = prothos::current_thread();
    MLOG_INFO(mlog::app, "thread done", DVAR(thread.id), DVAR(counter));
  };

  prothos::Thread t1(thread_main);

  MLOG_INFO(mlog::app, "waiting for join...");
  t1.join();
  MLOG_INFO(mlog::app, "thread has joined...");

  return 0;
}

void init_portal_pool() {
  constexpr size_t PORTAL_POOL_SIZE = 2 * 1024 * 1024;
  constexpr uintptr_t PORTAL_POOL_ADDR = 26 * 1024 * 1024;

  mythos::PortalLock lock(PORTAL);
  mythos::Frame frame(CAP_ALLOC());

  frame.create(lock, KERNEL_MEM, PORTAL_POOL_SIZE, 4096).wait();
  ADDRESS_SPACE.mmap(lock, frame, PORTAL_POOL_ADDR, PORTAL_POOL_SIZE, 0x1)
      .wait();
}

void init_heap() {
  constexpr size_t HEAP_SIZE = 512 * 1024 * 1024;
  constexpr size_t HEAP_ALIGN = 2 * 1024 * 1024;
  constexpr uintptr_t HEAP_ADDR = 28 * 1024 * 1024;

  mythos::PortalLock lock(PORTAL);
  mythos::Frame frame(CAP_ALLOC());

  frame.create(lock, KERNEL_MEM, HEAP_SIZE, HEAP_ALIGN).wait();
  ADDRESS_SPACE.mmap(lock, frame, HEAP_ADDR, HEAP_SIZE, 0x1).wait();

  mythos::heap.addRange(HEAP_ADDR, HEAP_SIZE);
}