#pragma once
#include "utils/FifoQueue.hh"
#include "utils/hlmsDeque.hh"
#include "runtime/Task.hh"
#include "runtime/Thread.hh"
//#include "WorkerMap.hh"
//#include "runtime/mlog.hh"

//#include <unistd.h>
#include <vector>

namespace Prothos{

class Worker;

class WorkerGroup{
	public:
	  virtual void pushTask(Task* t) = 0;
	  virtual void start() = 0;
	private:
	  virtual Worker* getRandomWorker() = 0;
      virtual Worker* getNextWorker(Worker* self) = 0;

	  friend class Worker;
};

class GroupWorker{
	public:
		void setGroup(WorkerGroup* wg){
			this->wg = wg;
		}
		WorkerGroup* getGroup(){
			return wg;
		}
	private:
		WorkerGroup* wg;
};

static const unsigned StealAttempts = 10;


template<size_t SIZE>
class FixedWorkerGroup : public WorkerGroup, public ThreadGroup<SIZE, Worker> {
	public:
      FixedWorkerGroup(){
		for(size_t i = 0; i < SIZE; i++){
			ThreadGroup<SIZE, Worker>::threads[i].setGroup(this);
		}
	  }

	  Worker* getRandomWorker() override {
		static size_t w = 0;
		w = (w+1)%SIZE;
		//MLOG_INFO(mlog::app, __func__, DVAR(w));
		return &ThreadGroup<SIZE, Worker>::threads[w];
	  };

      Worker* getNextWorker(Worker* self) override {
		for(size_t i = 0; i < SIZE; i++){
			if(&ThreadGroup<SIZE, Worker>::threads[i] == self){
				return &ThreadGroup<SIZE, Worker>::threads[(i+1)%SIZE];
			}
		}
		ASSERT(false);
		return nullptr;
	  }

	  void pushTask(Task* t) override {
		ThreadGroup<SIZE, Worker>::threads[0].pushTask(t);
	  }

	  void start() override { 
		ThreadGroup<SIZE, Worker>::start();
	  }
};

class Worker : public GroupWorker, public Thread{
	public:
		Worker()
			: isIdle(false)
			, running(true)
		{}

		static Worker* getLocalWorker(){
			return static_cast<Worker*>(getLocalThread());
		}

		static Worker* getNextWorker(){
			Worker* self = static_cast<Worker*>(getLocalThread());
			return self->getGroup()->getNextWorker(self);
		}

		void run(){
			//MLOG_INFO(mlog::app, __func__);
    //MLOG_INFO(mlog::app, "worker ", DVARhex(getLocalThread()));
			int cycle = 0;
			while(running){
				//cycle = (cycle+1)%100;
				//if(cycle == 0){
					////MLOG_INFO(mlog::app, "worker cycle");
				//}
				Task *t;
				// execute all tasks on work-stealing queue
				while((t = wsQueue.pop_bottom())){
					//MLOG_INFO(mlog::app, "got task");
					isIdle = false;
					t->executeTask();
				}
				// execute a task from low priotiy queue
				if((t = lpQueue.pop())){
					//MLOG_INFO(mlog::app, "got LP task");
					t->executeTask();
					if(!running){
						break;
					}
					continue;
				}

				Worker *victim = getGroup()->getRandomWorker();
				t = victim->tryStealTask();
				if(t){
					//MLOG_INFO(mlog::app, "Task stolen");
					isIdle = false;
					t->executeTask();
				}else{
					//usleep(100);
				}
			}
			//MLOG_INFO(mlog::app, "worker out! *drops mic*");
		}

		// spawn local task
		void pushWsTask(Task *t){
			//MLOG_INFO(mlog::app, __func__, DVAR(t), DVAR(this));
			wsQueue.push_bottom(t);
			//MLOG_INFO(mlog::app, "pushed");
		}

		// push task from external worker/thread (low priority)
		void pushTask(Task *t){
			//MLOG_INFO(mlog::app, __func__, DVAR(t), DVAR(this));
			lpQueue.push(t);
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

class TerminationTask : public Task{
	public:
		TerminationTask()
		{
			Worker* curr = Worker::getLocalWorker();
			first = curr;
			//curr->running = false;	
			setState(Ready);
		}

		void execute() override {
			Worker* curr = Worker::getLocalWorker();
			curr->running = false;
			Worker* next = Worker::getNextWorker();
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
			//MLOG_INFO(mlog::app, __func__);
			Worker* curr = Worker::getLocalWorker();
			if((w == nullptr) || (!curr->isIdle)){
				curr->isIdle = true;
				if((w == nullptr) || (cycle > 0)){
					w = curr;
					cycle = 0;
				}
				Worker* next = Worker::getNextWorker();
				next->pushTask(this);
			}else if(w == curr){
				cycle++;
				if(cycle >= 2){
					Worker* next = Worker::getNextWorker();
					next->pushTask(new TerminationTask);
				}else{
					Worker* next = Worker::getNextWorker();
					next->pushTask(this);
				}
			}else{
				Worker* next = Worker::getNextWorker();
				next->pushTask(this);
			}
		}	
	private:
	volatile Worker* w;
	volatile unsigned cycle;
};

} //Prothos
