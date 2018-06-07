#pragma once
#include <list>

#include "FifoQueue.hh"
#include "Task.hh"

namespace Prothos{

class LocalScheduler{
public:
	static LocalScheduler &getLocalScheduler();
	Task* getTask();
	void taskDone(Task* task);

	void schedulerMain();

private:
	LocalScheduler();

	FifoQueue<Task*> readyTasks;
	FifoQueue<Task*> completedTasks;
};

void thread_main();


} //Prothos
