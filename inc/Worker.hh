#pragma once
#include "FifoQueue.hh"
#include "hlmsDeque.hh"
#include "Task.hh"
#include "WorkerMap.hh"

#include <unistd.h>
#include <iostream>
#include <pthread.h>

namespace Prothos{

class Worker;

static const unsigned StealAttempts = 10;

class Worker{
	public:
		Worker()
			: isIdle(false)
			, running(true)
		{}

		void run(){
			while(running){
				Task *t;
				// execute all tasks on work-stealing queue
				while((t = wsQueue.pop_bottom())){
					isIdle = false;
					t->executeTask();
				}

				// execute a task from low priotiy queue
				if(lpQueue.size() > 0){
					t = lpQueue.pop();
					t->executeTask();
					if(!running){
						//std::cerr << "Tschau!" << std::endl;
						return;
					}
					continue;
				}

				Worker *victim = GlobalWorkerMap::getMap()->getRandomWorker();
				t = victim->tryStealTask();
				if(t){
					isIdle = false;
					//std::cerr << "Task stolen!" << std::endl;
					t->executeTask();
				}else{
					usleep(100);
				}
			}
			//std::cerr << "Tschau!" << std::endl;
		}

		// spawn local task
		void pushWsTask(Task *t){
			wsQueue.push_bottom(t);
		}

		// push task from external worker/thread (low priority)
		void pushTask(Task *t){
			lpQueue.push(t);	
		}

	
		Task *tryStealTask(){
			auto t = wsQueue.pop_top();
			return t.second;
		}
	private:
		hlms::deque<Task> wsQueue; // Work-Stealing Queue	
		FifoQueue<Task*> lpQueue; // Low priority Queue

		bool isIdle;
		bool running;

		friend class TerminationMarkerTask;
		friend class TerminationTask;
};

class TerminationTask : public Task{
	public:
		TerminationTask()
		{
			Worker* curr = GlobalWorkerMap::getMap()->getLocalWorker();
			first = curr;
			//curr->running = false;	
			setState(Ready);
		}

		void execute() override {
			Worker* curr = GlobalWorkerMap::getMap()->getLocalWorker();
			curr->running = false;
			Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
			if(curr != first){
				next->pushTask(this);
			}

		}
	private:
		Worker* first;
};

class TerminationMarkerTask : public Task{
	public:
		TerminationMarkerTask()
			: w(nullptr)
			, cycle(0)
		{
			setState(Ready);
		}

		void execute() override {
			Worker* curr = GlobalWorkerMap::getMap()->getLocalWorker();
			if((w == nullptr) || (!curr->isIdle)){
				curr->isIdle = true;
				if((w == nullptr) || (cycle > 0)){
					w = curr;
					cycle = 0;
				}
				Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
				next->pushTask(this);
			}else if(w == curr){
				cycle++;
				if(cycle >= 2){
					Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
					next->pushTask(new TerminationTask);
				}else{
					Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
					next->pushTask(this);
				}
			}else{
				Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
				next->pushTask(this);
			}
		}	
	private:
	volatile Worker* w;
	volatile unsigned cycle;
};

} //Prothos
