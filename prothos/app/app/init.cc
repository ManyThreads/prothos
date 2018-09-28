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
#include "runtime/Thread.hpp"

#include <cstddef>
#include <cstdint>

#include <vector>

using namespace prothos;

using byte = uint8_t;

constexpr size_t STACK_SIZE = 1024 * 1024;
byte STACK[STACK_SIZE];

// externally required variables
byte *initstack_top = STACK + STACK_SIZE;
mythos::InvocationBuf* msg_ptr;

mythos::Portal PORTAL(mythos::init::PORTAL, msg_ptr);
mythos::CapMap CAPABILITY_SPACE(mythos::init::CSPACE);
mythos::PageMap ADDRESS_SPACE(mythos::init::PML4);
mythos::KernelMemory KERNEL_MEM(mythos::init::KM);
mythos::SimpleCapAllocDel CAP_ALLOC(
    PORTAL,
    CAPABILITY_SPACE,
    mythos::init::APP_CAP_START,
    mythos::init::SIZE - mythos::init::APP_CAP_START
);

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

void* thread_main(void* context) {
    MLOG_INFO(mlog::app, "thread main...");
    return nullptr;
}

void init_heap();

extern "C" int main() {
    const char str[] = "hello world!";
    mythos::syscall_debug(str, sizeof(str) - 1);

    init_heap();

    prothos::thread::Thread t0(thread_main);

    while (true) {}

    return 0;
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

/*mythos::InvocationBuf *msg_ptr asm("msg_ptr");
int main() asm("main");

constexpr uint64_t stacksize = 4 * 4096;
char initstack[stacksize];
char stack[stacksize];

char *initstack_top = initstack + stacksize;

mythos::Portal portal(mythos::init::PORTAL, msg_ptr);
mythos::CapMap myCS(mythos::init::CSPACE);
mythos::PageMap myAS(mythos::init::PML4);
mythos::KernelMemory kmem(mythos::init::KM);
mythos::SimpleCapAllocDel capAlloc(portal, myCS, mythos::init::APP_CAP_START,
                                   mythos::init::SIZE -
                                       mythos::init::APP_CAP_START);

char threadstack[stacksize];
char *thread1stack_top = threadstack + stacksize / 2;
char *thread2stack_top = threadstack + stacksize;

void *thread_main(void *ctx) {
  MLOG_INFO(mlog::app, "hello thread!", DVAR(ctx));
  mythos::ISysretHandler::handle(mythos::syscall_wait());
  MLOG_INFO(mlog::app, "thread resumed from wait", DVAR(ctx));
  return 0;
}

struct HostChannel {
  void init() {
    ctrlToHost.init();
    ctrlFromHost.init();
  }
  typedef mythos::PCIeRingChannel<128, 8> CtrlChannel;
  CtrlChannel ctrlToHost;
  CtrlChannel ctrlFromHost;
};

mythos::PCIeRingProducer<HostChannel::CtrlChannel> app2host;
mythos::PCIeRingConsumer<HostChannel::CtrlChannel> host2app;

void *body(void *) {
  MLOG_INFO(mlog::app, __func__);
  return nullptr;
}

int main() {
  char const str[] = "hello world!";
  char const end[] = "bye, cruel world!";
  mythos::syscall_debug(str, sizeof(str) - 1);
  MLOG_ERROR(mlog::app, "application is starting :)", DVARhex(msg_ptr),
             DVARhex(initstack_top));
  {
    mythos::PortalLock pl(portal);

    uintptr_t vaddr =
        28 * 1024 * 1024; // choose address different from invokation buffer
    auto size = 512 * 1024 * 1024; // 512 MB
    auto align = 2 * 1024 * 1024;  // 2 MB
    // allocate a 2MiB frame
    mythos::Frame f(capAlloc());
    auto res2 = f.create(pl, kmem, size, align).wait();
    auto res3 = myAS.mmap(pl, f, vaddr, size, 0x1).wait();
    mythos::heap.addRange(vaddr, size);
  }

  Task *fgt = new UserTask([]() {
    FlowGraph::Graph g;
    int num = 0;
    auto numGen = new FlowGraph::SourceNode<int>(g, [&](int &i) {
      i = num;
      if (num < 100) {
        num++;
        return true;
      }
      return false;
    });

    auto primeCheck = new FlowGraph::FunctionNode<int, int>(g, [](int num) {
      for (int i = 2; i < num; i++) {
        if (num % i == 0)
          return num;
      }
      MLOG_INFO(mlog::app, "Found prime number: ", num);
      return 0;
    });

    auto join = new FlowGraph::JoinNode<std::tuple<int, int>>(g);

    auto tuple = new FlowGraph::FunctionNode<std::tuple<int, int>, int>(
        g, [](std::tuple<int, int> t) {
          MLOG_INFO(mlog::app, "Tuple ", std::get<0>(t), std::get<1>(t));
          return 0;
        });

    FlowGraph::makeEdge(*numGen, *primeCheck);
    FlowGraph::makeEdge(*numGen, join->getInPort<0>());
    FlowGraph::makeEdge(*primeCheck, join->getInPort<1>());
    FlowGraph::makeEdge(*join, *tuple);
    numGen->activate();
  });

  UserTask *t0 = new UserTask([]() {
    MLOG_INFO(mlog::app, "UserTask t0 start");
    for (int i = 0; i < 5; i++) {
      (new MsgDagTask(0, "Hello"))->addSucc(new MsgDagTask(1, " World"));
      // new MsgTask("Hello World :D");
    }
  });
  WorkerGroup *wg = new FixedWorkerGroup<3>;
  wg->start();

  wg->pushTask(t0);
  wg->pushTask(fgt);
  wg->pushTask(new TerminationMarkerTask());

  mythos::syscall_debug(end, sizeof(end) - 1);
  // while(1);
  return 0;
}*/
