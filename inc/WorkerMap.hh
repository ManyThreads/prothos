#pragma once

#include <assert.h>

namespace Prothos{

class Worker;

class WorkerMap{
	public:
		WorkerMap(){
		}

		virtual ~WorkerMap(){
		}

		virtual Worker* getLocalWorker() = 0;
		virtual Worker* getRandomWorker() = 0;
		virtual Worker* getNextWorker() = 0;
};

class GlobalWorkerMap{
	public: 
		static WorkerMap* getMap(){
			assert(myMap != nullptr);
			return myMap;
		};  

		static void setInstance(WorkerMap* map){
			assert(myMap == nullptr);
			myMap = map;
		}

		static void removeInstance(){
			//delete myMap;
			myMap = nullptr;
		}

	private:
		GlobalWorkerMap() {}
		GlobalWorkerMap(const GlobalWorkerMap&);
		GlobalWorkerMap & operator = (const GlobalWorkerMap &);

		static WorkerMap* myMap;
};


} //Prothos
