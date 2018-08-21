#pragma once
#include "hlmsDeque.hh"
#include "Task.hh"
#include "Singleton.hh"

#include <iostream>

namespace Prothos{

class Worker;
typedef PerThreadSingleton<Worker> LocalWorker;

static const unsigned StealAttempts = 30;

class Worker{
	public:
		Worker()
			: running(true)
		{}

		void run(){
			while(running){
				Task *t;
				while((t = taskQueue.pop_bottom())){
					t->executeTask();
				}
				for(unsigned attempt = 0; attempt < StealAttempts; attempt++){
					Worker *victim = LocalWorker::getRandomInstance();
					Task *t = victim->tryStealTask();
					if(t){
						std::cout << "Task stolen!" << std::endl;
						t->executeTask();
						break;
					}
				}
				//std::cout << "I'm bored to death :(" << std::endl;
				return;
			}
		}

		void stop(){
			running = false;
		}

		void pushTask(Task *t){
			taskQueue.push_bottom(t);
		}
	
		Task *tryStealTask(){
			auto t = taskQueue.pop_top();
			return t.second;
		}

		bool running;
		deque<Task> taskQueue;	
};

} //Prothos
