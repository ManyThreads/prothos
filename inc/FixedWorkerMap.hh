#pragma once

#include <thread>
#include <map>
#include <mutex>

#include "Worker.hh"
#include "WorkerMap.hh"

namespace Prothos{

class FixedWorkerMap : public WorkerMap{
	public:
		FixedWorkerMap(int NumWorkers)
			: NumWorkers(NumWorkers)
		{
			assert(!(NumWorkers < 0));
			std::lock_guard<std::mutex> mlock(mutex);
			myWorkers.resize(NumWorkers);
			myThreads.resize(NumWorkers);

			for(auto &w : myWorkers){
				w = new Worker;
			}

			for(auto &t : myThreads){
				t = std::pair<int, std::thread::id>(-1, std::thread::id());
			}
		}

		Worker* getLocalWorker(){  
			//todo: reader/writer lock
			std::lock_guard<std::mutex> mlock(mutex);
			std::thread::id id = std::this_thread::get_id(); 
			int i = getIndexByThread(id);
			if(i < 0){
				return registerWorker(id);	
			}

			return getWorkerByIndex(i);
		}


		Worker* getRandomWorker(){
			std::lock_guard<std::mutex> mlock(mutex);
			int i = rand() % NumWorkers;
			return myWorkers[i];
		}

		Worker* getNextWorker(){
			//todo: reader/writer lock
			std::lock_guard<std::mutex> mlock(mutex);
			std::thread::id id = std::this_thread::get_id(); 
			int i = getIndexByThread(id);
			if(i < 0){
				registerWorker(id);	
				i = getIndexByThread(id);
			}

			return getWorkerByIndex( ( i + 1 ) % NumWorkers );		
		}

	private:
		Worker* registerWorker(std::thread::id id){
			for( int i = 0; i < NumWorkers; i++){
				if(myThreads[i].first < 0){
					myThreads[i].first = i;
					myThreads[i].second = id;
					return myWorkers[i];
				}
			}
			return nullptr;
		}

		int getIndexByThread(std::thread::id id){
			for(auto t : myThreads){
				if(t.second == id){
					return t.first;
				}
			}
			return (-1);
		}

		Worker* getWorkerByIndex (int index){
			assert(index >= 0 && index < NumWorkers);
			return myWorkers[index];	
		}

		const int NumWorkers;
		std::vector<std::pair<int, std::thread::id> > myThreads;
		std::vector<Worker*> myWorkers;
		std::mutex mutex;

};

} //Prothos
