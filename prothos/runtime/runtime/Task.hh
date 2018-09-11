#pragma once
#include "app/mlog.hh"

namespace Prothos{

enum TaskState{
	Dormant,
	Ready,
	Executed
};

class Task{
public:
	Task();
	virtual ~Task(){};
	void executeTask();
	TaskState getState();
	virtual void setState(TaskState state);
private:
	virtual void execute() = 0;
	TaskState state;

	friend class WorkstealingTask;
};

class WorkstealingTask : public Task{
	public:
		void setState(TaskState state);
};

class MsgTask : public WorkstealingTask{
	public:
		MsgTask(const char *str)
			: WorkstealingTask()
			, str(str)
		{
			setState(Ready);
		}

		void execute() override{
			MLOG_INFO(mlog::app, str);
		}

	private:
		const char *str;
};

} //Prothos
