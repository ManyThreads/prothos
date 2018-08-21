#pragma once

#include <thread>
#include <map>
#include <mutex>

namespace Prothos{

template<typename T>
class Singleton{
	public:
		T* getInstance(){
			if(instance == nullptr){
				instance = new T;
			}
			return instance;
		}
	private:
		Singleton() {}
		Singleton(const Singleton &);
		Singleton & operator = (const Singleton &);

		T* instance;
};

template<typename T>
class PerThreadSingleton{
	public:
		static T* getInstance(){  
			//CritSectLock lock(m_cs.GetInnerCritSect()); 
			std::unique_lock<std::mutex> mlock(mutex);
			std::thread::id id = std::this_thread::get_id(); 
		   	T* retVal;  
			if(map.find(id) != map.end())  
			{  
				retVal = map[id];  
			}  
			else  
			{  
				retVal = new T();  
				map[id] = retVal;  
			}  
			return retVal;  
		}

		static void deleteInstance(){
			std::unique_lock<std::mutex> mlock(mutex);
			std::thread::id id = std::this_thread::get_id(); 
			T* retVal;  
			if(map.find(id) != map.end())  
			{  
				retVal = map[id];  
				map.erase(id);  
				delete retVal;   
			} 
		
		}

		static T* getRandomInstance(){
			std::unique_lock<std::mutex> mlock(mutex);
			auto item = map.begin();
			int random_index = rand() % map.size();
			std::advance(item, random_index);
			return item->second;
		}

	private:
		PerThreadSingleton() {}
		PerThreadSingleton(const PerThreadSingleton&);
		PerThreadSingleton & operator = (const PerThreadSingleton &);
		static std::map<std::thread::id, T*> map;
		static std::mutex mutex;

};

template<typename T> std::map<std::thread::id, T*> PerThreadSingleton<T>::map;
template<typename T> std::mutex PerThreadSingleton<T>::mutex;


} //Prothos
