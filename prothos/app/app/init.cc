#include <cstddef>
#include <cstdint>

#include "mythos/PciMsgQueueMPSC.hh"
#include "mythos/protocol/CpuDriverKNC.hh"
#include "runtime/mlog.hh"

// Prothos runtime
#include "os/OS.hh"
#include "thread/Thread.hh"

using namespace prothos;

// hack for compiling/linking std::shared_ptr
extern "C" int sched_yield(void) {
  return 0;
}

using byte = uint8_t;

constexpr size_t STACK_SIZE = 1024 * 1024;
byte STACK[STACK_SIZE];

// externally required variables
byte* initstack_top = STACK + STACK_SIZE;
mythos::InvocationBuf* msg_ptr;

// fixed addresses
const uintptr_t PORTAL_POOL_ADDR = 26 * 1024 * 1024;
const uintptr_t HEAP_ADDR = 28 * 1024 * 1024;

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

extern "C" int main() {
  MLOG_INFO(mlog::app, "app main");
  OS::init_all();

  auto thread_main = []() {
    MLOG_INFO(mlog::app, "thread spawned");

    volatile uintptr_t counter = 0;
    while (counter < 1'000'000) counter++;

    auto& thread = prothos::thread::current_thread();
    MLOG_INFO(mlog::app, "thread done", DVAR(thread.id), DVAR(counter));
  };

  prothos::Thread t1(thread_main);
  // prothos::Thread t2(thread_main);
  // prothos::Thread t3(thread_main);

  MLOG_INFO(mlog::app, "waiting for join...");
  t1.join();
  MLOG_INFO(mlog::app, "thread has joined...");

  return 0;
}