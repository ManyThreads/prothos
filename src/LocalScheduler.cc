//#include "LocalScheduler.hh"

//#include <iostream>
//#include <atomic>
//#include <pthread.h>

//using namespace Prothos;

//void Prothos::ExitTask::execute(){
	//LocalScheduler::getLocalScheduler().scheduleTask(this);
	//pthread_exit(0);	
//}

//static ExitTask exitTask;

//Prothos::LocalScheduler &Prothos::LocalScheduler::getLocalScheduler(){
	//static LocalScheduler scheduler;
	//return scheduler;
//}

//Prothos::LocalScheduler::LocalScheduler()
//: readyTasks(), completedTasks(), openTasks(1) {
//}

//void Prothos::LocalScheduler::schedulerMain(){
	////Generate initial Tasks
	////decoder->expand(NULL);
	//while(openTasks != 0){
		//Task* completedTask = completedTasks.pop();
		//switch(completedTask->getState()){
			//case SuccessorsUnknown:{
					//completedTask->expandTask();
					//completedTasks.push(completedTask);
				//}
				//break;
			//case Expanded:{
					//completedTask->notifySuccessors();
					//for(auto successor : completedTask->getSuccessors()){
						//if(successor->isReady()){
							//readyTasks.push(successor);
							//openTasks.fetch_add(1);
						//}
					//}
					//openTasks.fetch_sub(1);
				//}
				//break;	
		//}
	//}
	//readyTasks.push(&exitTask);
//}

//Task* Prothos::LocalScheduler::getTask(){
	//return readyTasks.pop();
//}
	
//void Prothos::LocalScheduler::scheduleTask(Task* task){
	//readyTasks.push(task);
	//openTasks.fetch_add(1);
//}

//void Prothos::LocalScheduler::taskDone(Task* task){
	//completedTasks.push(task);
//}

//void Prothos::LocalScheduler::waitForAll(){
	//openTasks.fetch_sub(1);
//}

//void Prothos::thread_main(){
	//LocalScheduler &scheduler = LocalScheduler::getLocalScheduler();
	//while(true){
		//Task* nextTask = scheduler.getTask(); //blocks, if queue empty

		//nextTask->executeTask();

		//scheduler.taskDone(nextTask);
	//}
//}
