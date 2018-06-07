#include "Prothos.hh"
#include "LocalScheduler.hh"

#include <pthread.h>
#include <thread>
#include <iostream>


static std::vector<std::thread> threads;

void Prothos::prothos_init(){
	int numCPU = std::thread::hardware_concurrency();
	if(numCPU < 3){
		std::cerr << "Not enough HW-threads" << std::endl;
		return;
	}

	threads.reserve(numCPU - 1);

	//Create a scheduler instance
	LocalScheduler &scheduler = LocalScheduler::getLocalScheduler();
	threads[0] = std::thread(&LocalScheduler::schedulerMain, &scheduler);
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(0, &cpus);
	pthread_setaffinity_np(threads[0].native_handle(), sizeof(cpu_set_t), &cpus);

	//Run this on last core

	pthread_t self = pthread_self();
	CPU_ZERO(&cpus);
	CPU_SET(numCPU - 1, &cpus);
	pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpus);

	//Run everything else on cores 2-n
	std::cout << "Running on " << numCPU << " Cores" <<std::endl;

	for(int i = 1; i < numCPU - 1; i++){
		threads[i] = std::thread(thread_main);

		CPU_ZERO(&cpus);
		CPU_SET(i, &cpus);
		pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpus);
	}
}

void Prothos::prothos_finalize(){
	for(auto &t : threads) t.join();
}
