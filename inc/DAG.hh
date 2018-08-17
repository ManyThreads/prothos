#pragma once

#include "Task.hh"

#include <list>
#include <vector>
#include <string>
#include <iostream>

namespace Prothos{

class DagTask : public Task {
public:
	DagTask(int dependencies)
		: deps(dependencies)
	{
		if(deps == 0){
			setState(Ready);
		}
	}

	virtual ~DagTask(){};

	void execute(){
		body();
		notifySuccessors();	
	};

	void addSucc(DagTask* task){
		succ.push_back(task);
	}
private:
	virtual void body() = 0;

	void notifySuccessors(){
		for(auto s : succ){
			s->decDeps();
		}
	}
	
	void decDeps(){
		deps--;
		if(deps == 0) setState(Ready);
	}

	std::vector<DagTask*> succ;
	int deps;
};

//class Task{
//public:
	//Task(TaskState state, int dependencies);
	//virtual ~Task(){};
	//void executeTask();
	//void expandTask();
	//void addChild(Task* task);
	//void addParent(Task* task);
	//void doneExpanding(); //Called when all successor tasks have been generated
	//void notifySuccessors();
	//std::vector<Task*> getSuccessors();
	//bool isReady();
	//TaskState getState();
//private:
	//virtual void execute() = 0;
	//virtual void expand() = 0;
	//TaskState state;
	//int dependencyCounter;
	//bool isExecuted;
//};

class MsgDagTask : public DagTask{
	public:
		MsgDagTask(int deps, std::string str)
			: DagTask(deps)
			, str(str)
		{
		}

		void body() override{
			std::cout << str << std::endl;
		}

	private:
		std::string str;
};

} //Prothos
