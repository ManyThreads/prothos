#pragma once

#include "Task.hh"
#include "app/mlog.hh"

#include <vector>

namespace Prothos{

class DagTask : public WorkstealingTask {
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
		if(getState() == Executed){
			task->decDeps();
		}
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

class MsgDagTask : public DagTask{
	public:
		MsgDagTask(int deps, const char *str)
			: DagTask(deps)
			, str(str)
		{
		}

		void body() override{
			MLOG_INFO(mlog::app, str);
		}

	private:
		const char *str;
};

} //Prothos
