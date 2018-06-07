#include "FlowGraph.hh"
#include "Prothos.hh"

#include <iostream>

using namespace Prothos;

int main(){
	std::cout << "main started" << std::endl;

	prothos_init();

	//FlowGraph::FunctionNode hfn([](FlowGraph::GenericMsg){
				//std::cout << "Hello ";
				//return FlowGraph::GenericMsg();
			//}
	//);

	//FlowGraph::FunctionNode wfn([](FlowGraph::GenericMsg){
				//std::cout << "world" << std::endl;
				//return FlowGraph::GenericMsg();
			//}
	//);

	//FlowGraph::makeEdge(hfn, wfn);

	//hfn.putTask(FlowGraph::GenericMsg());

	return 0;
}
