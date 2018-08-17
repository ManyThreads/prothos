#pragma once
#include "FifoQueue.hh"
#include "Task.hh"
#include "Singleton.hh"

#include <iostream>

namespace Prothos{

class Worker;
typedef PerThreadSingleton<Worker> LocalWorker;

class Worker{
	public:
		void run(){
			while(taskQueue.size() > 0){
				Task* nextTask = taskQueue.pop();
				nextTask->executeTask();
				
			}
		}

		FifoQueue<Task*> taskQueue;	
};

} //Prothos
