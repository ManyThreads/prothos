#include "Task.hh"
#include "LocalScheduler.hh"

using namespace Prothos;

Prothos::Task::Task(TaskState state)
	: expansionState(state)
	, dependencyCounter(0)
{}

void Prothos::Task::addChild(Task* task){
	successors.push_back(task);
	task->addParent(this);
}

void Prothos::Task::addParent(Task* task){
	predecessors.push_back(task);
	dependencyCounter++;
}

void Prothos::Task::doneExpanding(){
	expansionState = TaskState::AllSuccessorsKnown;
}

bool Prothos::Task::notifySuccessors(){
	if(expansionState == TaskState::AllSuccessorsKnown){
		for(auto successor : successors){
			successor->dependencyCounter--;
		}
		return true;
	}
	return false;
}

std::vector<Task*> Prothos::Task::getSuccessors(){
	return successors;
}

bool Prothos::Task::isReady(){
	return dependencyCounter == 0;
}
