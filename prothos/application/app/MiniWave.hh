#pragma once

#include "runtime/mlog.hh"
#include "runtime/Task.hh"
#include "runtime/FlowGraph.hh"

using namespace Prothos;


int miniwaveMain() {

    Task* fgt = new UserTask( []() {
		MLOG_INFO(mlog::app, "UserTask miniwave start");

        FlowGraph::Graph g;

		auto node1 = new FlowGraph::ContinueNode<FlowGraph::ContinueMsg>( g, 
				[] ( FlowGraph::ContinueMsg ) {
						MLOG_INFO(mlog::app, "node1");
						return FlowGraph::ContinueMsg();
				});


		auto node2 = new FlowGraph::ContinueNode<FlowGraph::ContinueMsg>( g, 
				[] ( FlowGraph::ContinueMsg ) {
						MLOG_INFO(mlog::app, "node2");
						return FlowGraph::ContinueMsg();
				});

		auto node3 = new FlowGraph::ContinueNode<FlowGraph::ContinueMsg>( g, 
				[] ( FlowGraph::ContinueMsg ) {
						MLOG_INFO(mlog::app, "node3");
						return FlowGraph::ContinueMsg();
				});

        makeEdge( *node1, *node2 );
        makeEdge( *node2, *node3 );

		

        auto trigger = new FlowGraph::SourceNode<FlowGraph::ContinueMsg>(g,
        []( FlowGraph::ContinueMsg& ) {
		    static bool triggered = false;
		    MLOG_INFO(mlog::app, "triggered");
			if (!triggered) {
				triggered = true;
				return true;
				} 
		    return false;
        });

        makeEdge( *trigger, *node1 );

        Visitor visitor;
//        NodeDescription nd;
        visitor.traverse(*trigger);        

        trigger->activate();
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
	wg->finalize();
	MLOG_INFO(mlog::app, "Good Bye!");
    return 0;

} // miniwaveMain
