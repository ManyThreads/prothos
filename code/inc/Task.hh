#pragma once
#include <list>
#include <vector>

namespace Prothos{

enum TaskState{
	AllSuccessorsKnown,
	SuccessorsUnknown
};

class Task{
public:
	Task(TaskState state);
	virtual ~Task(){};
	virtual void execute() = 0;
	void addChild(Task* task);
	void addParent(Task* task);
	void doneExpanding(); //Called when all successor tasks have been generated
	bool notifySuccessors();
	std::vector<Task*> getSuccessors();
	bool isReady();
protected:
	int dependencyCounter;
private:
	TaskState expansionState;
	std::vector<Task*> predecessors;
	std::vector<Task*> successors;
};

} //Prothos
