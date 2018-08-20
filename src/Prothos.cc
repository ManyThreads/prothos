#include "Prothos.hh"
//#include "LocalScheduler.hh"

#include <pthread.h>
#include <thread>
#include <iostream>

static std::vector<std::thread> threads;

static void runWorker(){
	Prothos::LocalWorker::getInstance()->run();
	Prothos::LocalWorker::deleteInstance();
}

void Prothos::prothos_init(){
	int numCPU = std::thread::hardware_concurrency();
	std::cout << "Running on " << numCPU << " cores." << std::endl;

	//threads.resize(numCPU - 1);

	////Create a scheduler instance
	//LocalScheduler &scheduler = LocalScheduler::getLocalScheduler();
	//threads[0] = std::thread(&LocalScheduler::schedulerMain, &scheduler);
	//cpu_set_t cpus;
	//CPU_ZERO(&cpus);
	//CPU_SET(0, &cpus);
	//pthread_setaffinity_np(threads[0].native_handle(), sizeof(cpu_set_t), &cpus);

	////Run this on last core

	//pthread_t self = pthread_self();
	//CPU_ZERO(&cpus);
	//CPU_SET(numCPU - 1, &cpus);
	//pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpus);

	////Run everything else on cores 2-n
	//std::cout << "Running on " << numCPU << " Cores" <<std::endl;

	//for(int i = 1; i < numCPU - 1; i++){
		//threads[i] = std::thread(thread_main);

		//CPU_ZERO(&cpus);
		//CPU_SET(i, &cpus);
		//pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpus);
	//}
}

void Prothos::prothos_push_task(Task *t){
	Prothos::LocalWorker::getInstance()->taskQueue.push(t);
}

void Prothos::prothos_finalize(){
	////LocalScheduler::getLocalScheduler().waitForAll();
	//for(auto w : workers) {
		//w->taskQueue.push(ExitTask());
	//}
	//for(auto &t : threads) t.join();
	//std::cout << __func__ << std::endl;
	//Prothos::LocalWorker::getInstance()->run();
	runWorker();
}
