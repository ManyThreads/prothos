
#include "runtime/mlog.hh"
#include "runtime/Task.hh"
#include "runtime/FlowGraph.hh"

#include <iostream>

using namespace Prothos;

int main()
{
	std::cout << "Hello World" << std::endl;

  Task* fgt = new UserTask( [](){
	  FlowGraph::Graph g;
		int num = 0;
		auto numGen = new FlowGraph::SourceNode<int>(g, [&](int &i){
				i = num;
				if(num < 100){
					num++;
					return true;
				}
				return false;
		});

		auto primeCheck = new FlowGraph::FunctionNode<int, int>(g, [](int num){
			for(int i = 2; i < num; i++){
				if(num % i == 0) return num;
			}		
			MLOG_INFO(mlog::app, "Found prime number: ", num);
			return 0;
		});

		auto join = new FlowGraph::JoinNode<std::tuple<int, int> >(g);

		auto tuple  = new FlowGraph::FunctionNode<std::tuple<int,int>, int>(g, [](std::tuple<int, int> t){
			//MLOG_INFO(mlog::app, "Tuple ", std::get<0>(t), std::get<1>(t));
			return 0;
		});

	  FlowGraph::makeEdge(*numGen, *primeCheck);
	  FlowGraph::makeEdge(*numGen, join->getInPort<0>());
	  FlowGraph::makeEdge(*primeCheck, join->getInPort<1>());
	  FlowGraph::makeEdge(*join, *tuple);
	  //numGen->activate();
  });

	fgt->executeTask();
	return 0;
}

