#include "LocalScheduler.hh"

#include <iostream>

using namespace Prothos;

Prothos::LocalScheduler &Prothos::LocalScheduler::getLocalScheduler(){
	static LocalScheduler scheduler;
	return scheduler;
}

Prothos::LocalScheduler::LocalScheduler()
: readyTasks(), completedTasks() {
}

void Prothos::LocalScheduler::schedulerMain(){
	//Generate initial Tasks
	//decoder->expand(NULL);
	while(true){
		Task* completedTask = completedTasks.pop();
		if (!completedTask->notifySuccessors()){
			//Child tasks not known
			//decoder->expand(completedTask)
			completedTasks.push(completedTask);
		}
		else{
			for(auto successor : completedTask->getSuccessors()){
				if(successor->isReady()){
					readyTasks.push(successor);
				}
			}
		}

	}
}

Task* Prothos::LocalScheduler::getTask(){
	return readyTasks.pop();
}

void Prothos::LocalScheduler::taskDone(Task* task){
	completedTasks.push(task);
}

void Prothos::thread_main(){
	LocalScheduler &scheduler = LocalScheduler::getLocalScheduler();

	while(true){
		Task* nextTask = scheduler.getTask(); //blocks, if queue empty

	    nextTask->execute();

	    scheduler.taskDone(nextTask);
	}
}
