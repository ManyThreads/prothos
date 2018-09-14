#pragma once
#include "utils/FifoQueue.hh"
#include "utils/hlmsDeque.hh"
#include "runtime/Task.hh"
#include "runtime/Thread.hh"
//#include "WorkerMap.hh"
#include "runtime/mlog.hh"

//#include <unistd.h>
#include <vector>

namespace Prothos{

class Worker;

static const unsigned StealAttempts = 10;

class Worker : public Thread{
	public:
		Worker()
			: isIdle(false)
			, running(true)
		{}

		static Worker* getLocalWorker(){
			return static_cast<Worker*>(getLocalThread());
		}

		void run(){
			MLOG_INFO(mlog::app, __func__);
    MLOG_INFO(mlog::app, "worker ", DVARhex(getLocalThread()));
			while(running){
			//MLOG_INFO(mlog::app, "worker cycle");
				Task *t;
				// execute all tasks on work-stealing queue
				while((t = wsQueue.pop_bottom())){
					MLOG_INFO(mlog::app, "got task");
					isIdle = false;
					t->executeTask();
				}
				// execute a task from low priotiy queue
				if((t = lpQueue.pop())){
					MLOG_INFO(mlog::app, "got LP task");
					t->executeTask();
					if(!running){
						return;
					}
					continue;
				}

				//Worker *victim = GlobalWorkerMap::getMap()->getRandomWorker();
				//t = victim->tryStealTask();
				//if(t){
					//isIdle = false;
					//t->executeTask();
				//}else{
					//usleep(100);
				//}
			}
			MLOG_INFO(mlog::app, "worker out! *drops mic*");
		}

		// spawn local task
		void pushWsTask(Task *t){
			MLOG_INFO(mlog::app, __func__);
			wsQueue.push_bottom(t);
		}

		// push task from external worker/thread (low priority)
		void pushTask(Task *t){
			MLOG_INFO(mlog::app, __func__);
			wsQueue.push_bottom(t);
		}

	
		Task *tryStealTask(){
			auto t = wsQueue.pop_top();
			return t.second;
		}
	private:
		hlms::deque<Task> wsQueue; // Work-Stealing Queue	
		FifoQueue<Task> lpQueue; // Low priority Queue

		bool isIdle;
		bool running;

		friend class TerminationMarkerTask;
		friend class TerminationTask;
};

//class TerminationTask : public Task{
	//public:
		//TerminationTask()
		//{
			//Worker* curr = GlobalWorkerMap::getMap()->getLocalWorker();
			//first = curr;
			////curr->running = false;	
			//setState(Ready);
		//}

		//void execute() override {
			//Worker* curr = GlobalWorkerMap::getMap()->getLocalWorker();
			//curr->running = false;
			//Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
			//if(curr != first){
				//next->pushTask(this);
			//}

		//}
	//private:
		//Worker* first;
//};

//class TerminationMarkerTask : public Task{
	//public:
		//TerminationMarkerTask()
			//: w(nullptr)
			//, cycle(0)
		//{
			//setState(Ready);
		//}

		//void execute() override {
			//Worker* curr = GlobalWorkerMap::getMap()->getLocalWorker();
			//if((w == nullptr) || (!curr->isIdle)){
				//curr->isIdle = true;
				//if((w == nullptr) || (cycle > 0)){
					//w = curr;
					//cycle = 0;
				//}
				//Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
				//next->pushTask(this);
			//}else if(w == curr){
				//cycle++;
				//if(cycle >= 2){
					//Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
					//next->pushTask(new TerminationTask);
				//}else{
					//Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
					//next->pushTask(this);
				//}
			//}else{
				//Worker* next = GlobalWorkerMap::getMap()->getNextWorker();
				//next->pushTask(this);
			//}
		//}	
	//private:
	//volatile Worker* w;
	//volatile unsigned cycle;
//};

} //Prothos
