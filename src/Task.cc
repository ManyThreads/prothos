#include "Task.hh"
#include "LocalScheduler.hh"
#include <iostream>

using namespace Prothos;

Prothos::Task::Task(TaskState state, int dependencies)
	: state(state)
	, dependencyCounter(dependencies)
	, isExecuted(false)
{
}

TaskState Prothos::Task::getState(){
	return state;
}

void Prothos::Task::executeTask(){
	execute();
	isExecuted = true;
}

void Prothos::Task::expandTask(){
	state = Expanded;
	expand();
}

void Prothos::Task::addChild(Task* task){
	successors.push_back(task);
	task->addParent(this);
	if(state == Zombie){
		task->dependencyCounter--;
		if(task->isReady()){
			LocalScheduler::getLocalScheduler().scheduleTask(task);
		}
	}
}

void Prothos::Task::addParent(Task* task){
	predecessors.push_back(task);
}

void Prothos::Task::doneExpanding(){
	state = TaskState::Expanded;
}

void Prothos::Task::notifySuccessors(){
	for(auto successor : successors){
		successor->dependencyCounter--;
	}
	state = Zombie;
}

std::vector<Task*> Prothos::Task::getSuccessors(){
	return successors;
}

bool Prothos::Task::isReady(){
	return dependencyCounter == 0;
}
