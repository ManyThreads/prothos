#include "Prothos.hh"
#include "LocalScheduler.hh"

#include <pthread.h>
#include <thread>
#include <iostream>

void Prothos::prothos_init(){

	//Create a scheduler instance
	LocalScheduler &scheduler = LocalScheduler::getLocalScheduler();
	std::thread scheduler_thread(&LocalScheduler::schedulerMain, &scheduler);
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(0, &cpus);
	pthread_setaffinity_np(scheduler_thread.native_handle(), sizeof(cpu_set_t), &cpus);

	//Run this on core 1

	pthread_t self = pthread_self();
	CPU_ZERO(&cpus);
	CPU_SET(1, &cpus);
	pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpus);

	//Run everything else on cores 2-n
	int numCPU = std::thread::hardware_concurrency();
	std::cout << "Running on " << numCPU << " Cores" <<std::endl;
	std::vector<std::thread> threads(numCPU - 2);

	for(int i = 0; i < numCPU - 2; i++){
		threads[i] = std::thread(thread_main);

		CPU_ZERO(&cpus);
		CPU_SET(i + 2, &cpus);
		pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpus);
	}
}

