#include "Prothos.hh"

#include <iostream>
#include <benchmark/benchmark.h>

using namespace Prothos;

static void TaskBench(benchmark::State& state){
	for (auto _ : state){
		prothos_init();
		
		new MsgTask("Hello World");

		prothos_finalize();
	}
}

static void DagBench(benchmark::State& state){
	for (auto _ : state){
		prothos_init();
		
		(new MsgDagTask(0,"What's ")) -> addSucc(new MsgDagTask(1,"up?"));

		prothos_finalize();
	}
}

static void FlowGraphBench(benchmark::State& state){
	for (auto _ : state){
		prothos_init();
	
		FlowGraph::Graph g;
	
		int counter = 10;
		FlowGraph::SourceNode sn(g, [&](FlowGraph::GenericMsg &m) -> bool{ 
				std::cout << "Source Node " << counter << std::endl;
				m.ptr = new int(counter);
				if(0 < counter--) 
					return true;
				else 
					return false;
		});

		FlowGraph::FunctionNode hfn(g, [](FlowGraph::GenericMsg m){
					std::cout << "Hello " << *static_cast<int*>(m.ptr);
					return FlowGraph::GenericMsg();
				}
		);

		FlowGraph::FunctionNode cfn(g, [](FlowGraph::GenericMsg){
					std::cout << "cruel ";
					return FlowGraph::GenericMsg();
				}
		);


		FlowGraph::FunctionNode wfn(g, [](FlowGraph::GenericMsg){
					std::cout << "world" << std::endl;
					return FlowGraph::GenericMsg();
				}
		);

		FlowGraph::JoinNode<2> jn(g);

		FlowGraph::FunctionNode lfn(g, [](FlowGraph::GenericMsg){
					std::cout << "!!!!!" << std::endl;
					return FlowGraph::GenericMsg();
				}
		);

		FlowGraph::makeEdge(sn, hfn);
		FlowGraph::makeEdge(hfn, cfn);
		FlowGraph::makeEdge(cfn, wfn);
		//FlowGraph::makeEdge(cfn, jn.getInPort(0));
		//FlowGraph::makeEdge(wfn, jn.getInPort(1));
		//FlowGraph::makeEdge(jn, lfn);

		//hfn.pushValue(FlowGraph::GenericMsg());


		sn.activate();
		//g.waitForAll();
		prothos_finalize();
	}
}

	//BENCHMARK(TaskBench);
	//BENCHMARK(DagBench);
	BENCHMARK(FlowGraphBench);
	
	BENCHMARK_MAIN();
