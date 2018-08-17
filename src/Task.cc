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
		Prothos::LocalWorker::getInstance()->taskQueue.push(this);
	}
}

TaskState Prothos::Task::getState(){
	return state;
}

void Prothos::Task::executeTask(){
	execute();
	state = Executed;
}

//void Prothos::Task::expandTask(){
	//state = Expanded;
	//expand();
//}

//void Prothos::Task::addChild(Task* task){
	//successors.push_back(task);
	//task->addParent(this);
	//if(state == Zombie){
		//task->dependencyCounter--;
		//if(task->isReady()){
			//LocalScheduler::getLocalScheduler().scheduleTask(task);
		//}
	//}
//}

//void Prothos::Task::addParent(Task* task){
	//predecessors.push_back(task);
//}

//void Prothos::Task::doneExpanding(){
	//state = TaskState::Expanded;
//}

//void Prothos::Task::notifySuccessors(){
	//for(auto successor : successors){
		//successor->dependencyCounter--;
	//}
	//state = Zombie;
//}

//std::vector<Task*> Prothos::Task::getSuccessors(){
	//return successors;
//}
