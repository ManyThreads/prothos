#include "Prothos.hh"

#include <iostream>
#include <benchmark/benchmark.h>

using namespace Prothos;

static void TaskBench(benchmark::State& state){
	for (auto _ : state){
		prothos_init();
		
		/* Testing Task */	
		new MsgTask("Hello World");

		prothos_finalize();
	}
}

static void DagBench(benchmark::State& state){
	for (auto _ : state){
		prothos_init();
		
		/* Testing DAG Task Graph */	
		(new MsgDagTask(0,"What's ")) -> addSucc(new MsgDagTask(1,"up?"));

		prothos_finalize();
	}
}


static void FlowGraphBench(benchmark::State& state){
	for (auto _ : state){
		prothos_init();
		
		/* Testing FlowGraph */	
		FlowGraph::FunctionNode hfn([](FlowGraph::GenericMsg){
					std::cout << "Hello ";
					return FlowGraph::GenericMsg();
				}
		);

		FlowGraph::FunctionNode cfn([](FlowGraph::GenericMsg){
					std::cout << "cruel ";
					return FlowGraph::GenericMsg();
				}
		);


		FlowGraph::FunctionNode wfn([](FlowGraph::GenericMsg){
					std::cout << "world" << std::endl;
					return FlowGraph::GenericMsg();
				}
		);

		FlowGraph::JoinNode<2> jn;

		FlowGraph::FunctionNode lfn([](FlowGraph::GenericMsg){
					std::cout << "!!!!!" << std::endl;
					return FlowGraph::GenericMsg();
				}
		);

		FlowGraph::makeEdge(hfn, cfn);
		FlowGraph::makeEdge(cfn, wfn);
		FlowGraph::makeEdge(cfn, jn.getInPort(0));
		FlowGraph::makeEdge(wfn, jn.getInPort(1));
		FlowGraph::makeEdge(jn, lfn);

		hfn.pushValue(FlowGraph::GenericMsg());


		prothos_finalize();
	}
}

//int main(){
	BENCHMARK(TaskBench);
	BENCHMARK(DagBench);
	BENCHMARK(FlowGraphBench);
	
	BENCHMARK_MAIN();
	//return 0;
//}
