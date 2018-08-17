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

	//FlowGraph::FunctionNode cfn([](FlowGraph::GenericMsg){
				//std::cout << "cruel ";
				//return FlowGraph::GenericMsg();
			//}
	//);


	//FlowGraph::FunctionNode wfn([](FlowGraph::GenericMsg){
				//std::cout << "world" << std::endl;
				//return FlowGraph::GenericMsg();
			//}
	//);

	//FlowGraph::JoinNode<2> jn;

	//FlowGraph::FunctionNode lfn([](FlowGraph::GenericMsg){
				//std::cout << "!!!!!1 elf" << std::endl;
				//return FlowGraph::GenericMsg();
			//}
	//);

	//FlowGraph::makeEdge(hfn, cfn);
	//FlowGraph::makeEdge(cfn, wfn);
	//FlowGraph::makeEdge(cfn, jn.getInPort(0));
	//FlowGraph::makeEdge(wfn, jn.getInPort(1));
	//FlowGraph::makeEdge(jn, lfn);

	//hfn.pushValue(FlowGraph::GenericMsg());

		
	new MsgTask("Hello World");

	(new MsgDagTask(0,"What's ")) -> addSucc(new MsgDagTask(1,"up?"));



	prothos_finalize();

	return 0;
}
