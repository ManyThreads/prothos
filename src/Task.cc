#include "Task.hh"
#include "Worker.hh"
#include <iostream>

using namespace Prothos;

Prothos::Task::Task()
	: state(Dormant)
{
}

void Prothos::Task::setState(TaskState state){
	this->state = state;
	if(state == Ready){
		Prothos::LocalWorker::getInstance()->pushTask(this);
	}
}

TaskState Prothos::Task::getState(){
	return state;
}

void Prothos::Task::executeTask(){
	execute();
	state = Executed;
}
