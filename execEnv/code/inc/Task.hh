#pragma once
#include <list>
#include <vector>

namespace Prothos{

enum TaskState{
	SuccessorsUnknown,
	Expanded,
	Zombie
};

class Task{
public:
	Task(TaskState state, int dependencies);
	virtual ~Task(){};
	void executeTask();
	void expandTask();
	void addChild(Task* task);
	void addParent(Task* task);
	void doneExpanding(); //Called when all successor tasks have been generated
	void notifySuccessors();
	std::vector<Task*> getSuccessors();
	bool isReady();
	TaskState getState();
private:
	virtual void execute() = 0;
	virtual void expand() = 0;
	TaskState state;
	int dependencyCounter;
	std::vector<Task*> predecessors;
	std::vector<Task*> successors;
	bool isExecuted;
};

} //Prothos
