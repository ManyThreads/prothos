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
#include "runtime/Portal.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/Example.hh"
#include "runtime/PageMap.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "runtime/tls.hh"
#include "runtime/mlog.hh"
#include <cstdint>
#include "util/optional.hh"
#include "runtime/umem.hh"

#include "runtime/Task.hh"
#include "runtime/DAG.hh"
#include "runtime/FlowGraph.hh"
#include "runtime/Worker.hh"
#include "runtime/Thread.hh"

#include <vector>

using namespace Prothos;

mythos::InvocationBuf* msg_ptr asm("msg_ptr");
int main() asm("main");

constexpr uint64_t stacksize = 4*4096;
char initstack[stacksize];
char stack[stacksize];

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

void* thread_main(void* ctx)
{
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
    typedef mythos::PCIeRingChannel<128,8> CtrlChannel;
    CtrlChannel ctrlToHost;
    CtrlChannel ctrlFromHost;
};

mythos::PCIeRingProducer<HostChannel::CtrlChannel> app2host;
mythos::PCIeRingConsumer<HostChannel::CtrlChannel> host2app;

void* body(void*) {
    MLOG_INFO(mlog::app, __func__);
    return nullptr;
}


const int M=1000, const N=1000;   // matrix size
const int B = 100;                // block size calculated by each node
const int MB = (M/B) + (M%B>0);   // number of blocks in dim M
const int NB = (M/B) + (M%B>0);   // number of block sin dim N

double **value;                   // value matrix pointer

// matrix calculation method
inline double calc( double v0, double v1 ) {
    if ( v0 == v1 )
        return 2*v0;
    else
        return std::max(v0,v1);
}

// contrinue node matrix
ContinueNode<FlowGraph::ContinueMsg> ***nodes;


int main()
{
    char const str[] = "hello world!";
    char const end[] = "bye, cruel world!";
    mythos::syscall_debug(str, sizeof(str)-1);
    MLOG_ERROR(mlog::app, "application is starting :)", DVARhex(msg_ptr), DVARhex(initstack_top));
    {
        mythos::PortalLock pl(portal);

        /* init heap */
        uintptr_t vaddr = 28*1024*1024; // choose address different from invokation buffer
        auto size = 512*1024*1024; // 512 MB
        auto align = 2*1024*1024; // 2 MB
        // allocate a 2MiB frame
        mythos::Frame f(capAlloc());
        auto res2 = f.create(pl, kmem, size, align).wait();
        auto res3 = myAS.mmap(pl, f, vaddr, size, 0x1).wait();
        mythos::heap.addRange(vaddr, size);
    }

    Task* fgt = new UserTask( []() {
        FlowGraph::Graph g;

        // setup node graph
        value[M-1][N-1] = 0;
        for( int i=MB; --i>=0; )
            for( int j=NB; --j>=0; ) {
                nodes[i][j] =
                    new ContinueNode<FlowGraph::ContinueMsg>( g,
                        [=]( FlowGraph::ContinueMsg ) {

                            int start_i = i*B;
                            int end_i = (i*B+B > M) ? M : i*B+B;
                            int start_j = j*B;
                            int end_j = (j*B+B > N) ? N : j*B+B;

                            for ( int ii = start_i; ii < end_i; ++ii ) {
                                for ( int jj = start_j; jj < end_j; ++jj ) {
                                    double v0 = ii == 0 ? 0 : value[ii-1][jj];
                                    double v1 = jj == 0 ? 0 : value[ii][jj-1];
                                    value[ii][jj] = ii==0 && jj==0 ? 1 : calc(v0,v1);
                                }
                            }
                            return FlowGraph::ContinueMsg(); 
                        }
                );

                if ( i + 1 < MB ) makeEdge( **nodes[i][j], **nodes[i+1][j] );
                if ( j + 1 < NB ) makeEdge( **nodes[i][j], **nodes[i][j+1] );

            }
            
        auto trigger = new FlowGraph::SourceNode<FlowGraph::ContinueMsg>(g, 
            []( FlowGraph::ContinueMsg& ) {
            return false;
        });

        makeEdge( *trigger, **nodes[0][0] );

        trigger->activate();
        g.wait_for_all();
        MLOG_INFO(mlog::app, "calculation result is: ", value[M-1][N-1]);

    });

    UserTask* t0 = new UserTask([]() {
        MLOG_INFO(mlog::app, "UserTask t0 start");
        for(int i = 0; i < 5; i++) {
            (new MsgDagTask(0,"Hello"))->addSucc(new MsgDagTask(1," World"));
        }
    });
    WorkerGroup* wg = new FixedWorkerGroup<3>;
    wg->start();

    wg->pushTask(t0);
    wg->pushTask(fgt);
    wg->pushTask(new TerminationMarkerTask());

    mythos::syscall_debug(end, sizeof(end)-1);
    //while(1);
    return 0;
}

