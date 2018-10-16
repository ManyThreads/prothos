#pragma once

#include "runtime/mlog.hh"
#include "runtime/Task.hh"
#include "runtime/FlowGraph.hh"

using namespace Prothos;

int userMain(){

	MLOG_INFO(mlog::app, "Hello World! ");
  Task* fgt = new UserTask( [&](){
		FlowGraph::Graph g;

		struct numGenF{
			numGenF() : counter(0) {}

			int counter;

			bool operator()(int &i){
				i = counter;
				//MLOG_INFO(mlog::app, "number: ", counter);
				if(counter < 10000){
					counter++;
					return true;
				}
				return false;
			}
		};

		auto numGen = new FlowGraph::SourceNode<int>(g, numGenF());

		auto primeCheck = new FlowGraph::ConditionalNode<int, int, 1>(g, [](int num, int &out){
			for(int i = 2; i < num; i++){
				if(num % i == 0) return (-1);
			}		
			//MLOG_INFO(mlog::app, "Found prime number: ", num);
			out = num;
			return 0;
		});

		//auto join = new FlowGraph::JoinNode<std::tuple<int, int> >(g);

		//auto tuple  = new FlowGraph::FunctionNode<std::tuple<int,int>, int>(g, [](std::tuple<int, int> t){
			//MLOG_INFO(mlog::app, "Tuple ", std::get<0>(t), std::get<1>(t));
			//return 0;
		//});

		auto msg = new FlowGraph::FunctionNode<int, int>(g, [](int num){
			MLOG_INFO(mlog::app,"Found prime number: ", num);		
			return num;
		});

	  FlowGraph::makeEdge(*numGen, *primeCheck);
	  FlowGraph::makeEdge(primeCheck->get(0), *msg);
	  //FlowGraph::makeEdge(*numGen, join->getInPort<0>());
	  //FlowGraph::makeEdge(*primeCheck, join->getInPort<1>());
	  //FlowGraph::makeEdge(*join, *tuple);
	  numGen->activate();
  });

  UserTask* t0 = new UserTask([](){
	MLOG_INFO(mlog::app, "UserTask t0 start");
	for(int i = 0; i < 5; i++){
		(new MsgDagTask(0,"Hello"))->addSucc(new MsgDagTask(1," World")); 
		//new MsgTask("Hello World :D");
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
}
