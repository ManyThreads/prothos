#pragma once
#include "runtime/mlog.hh"
#include "utils/FifoQueue.hh"
#include <atomic>

namespace Prothos{

enum TaskState{
	Dormant,
	Ready,
	Executed
};

class Task { 
public:
	Task();
	virtual ~Task(){};
	void executeTask();
	TaskState getState();
	virtual void setState(TaskState state);
private:
	virtual void execute() = 0;
	TaskState state;

	std::atomic<Task*> next;

	friend class WorkstealingTask;
	friend class FifoQueue<Task>;
};

class WorkstealingTask : public Task{
	public:
		void setState(TaskState state);
};

class TaskBody{
	public:
		virtual void run() = 0;
};

template<typename Body>
class TaskBodyImpl : public TaskBody {
	public:
		TaskBodyImpl(Body &body)
			: body(body)
		{}
		
		void run() override {
			body();
		}

	private:
		Body body;	
};

class UserTask : public Task {
	public:
		template<typename Body>
		UserTask(Body body)
			: myBody(new TaskBodyImpl<Body>(body))
		{}

		void execute() override {
			myBody->run();
		}
	private:
		TaskBody* myBody;
};

class MsgTask : public WorkstealingTask{
	public:
		MsgTask(const char *str)
			: str(str)
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
