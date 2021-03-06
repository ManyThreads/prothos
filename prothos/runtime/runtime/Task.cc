#include "Task.hh"
#include "Worker.hh"

using namespace Prothos;

Prothos::Task::Task()
	: state(Dormant)
{
}

void Prothos::Task::setState(TaskState state){
	this->state = state;
}

TaskState Prothos::Task::getState(){
	return state;
}

void Prothos::WorkstealingTask::setState(TaskState state){
	this->state = state;
	if(state == Ready){
		Prothos::Worker::getLocalWorker()->pushWsTask(this);
	}
}

void Prothos::Task::executeTask(){
	execute();
	state = Executed;
}
