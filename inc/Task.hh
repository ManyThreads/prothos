#pragma once
#include <list>
#include <vector>
#include <string>
#include <iostream>

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
	void setState(TaskState state);
private:
	virtual void execute() = 0;
	TaskState state;
};

class MsgTask : public Task{
	public:
		MsgTask(std::string str)
			: Task()
			, str(str)
		{
			setState(Ready);
		}

		void execute() override{
			std::cout << str << std::endl;
		}

	private:
		std::string str;
};

} //Prothos
