//#pragma once
//#include <list>
//#include <atomic>

//#include "FifoQueue.hh"
//#include "Task.hh"

//namespace Prothos{

//class ExitTask : public Task{
	//public:
		//ExitTask()
			//: Task(Expanded, 0)
		//{}
		//~ExitTask(){}
		//void expand(){}
		//void execute();
//};

//class LocalScheduler{
//public:
	//static LocalScheduler &getLocalScheduler();
	//Task* getTask();
	//void scheduleTask(Task* task);
	//void taskDone(Task* task);

	//void schedulerMain();
	//void waitForAll();

//private:
	//LocalScheduler();

	//FifoQueue<Task*> readyTasks;
	//FifoQueue<Task*> completedTasks;
	//std::atomic<size_t> openTasks;
//};

//void thread_main();


//} //Prothos
