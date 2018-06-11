#include "FlowGraph.hh"
#include "Prothos.hh"

#include <iostream>

using namespace Prothos;

int main(){
	std::cout << "main started" << std::endl;

	prothos_init();

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

	FlowGraph::makeEdge(hfn, cfn);
	FlowGraph::makeEdge(cfn, wfn);

	Task *t = hfn.putTask(FlowGraph::GenericMsg());

	prothos_schedule_task(t);
	prothos_finalize();

	return 0;
}
