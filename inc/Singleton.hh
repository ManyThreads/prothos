#pragma once

#include <thread>
#include <map>

namespace Prothos{

template<typename T>
class PerThreadSingleton{
	public:
		static T* getInstance(){  
			//CritSectLock lock(m_cs.GetInnerCritSect()); 
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
			std::thread::id id = std::this_thread::get_id(); 
			T* retVal;  
			if(map.find(id) != map.end())  
			{  
				retVal = map[id];  
				map.erase(id);  
				delete retVal;   
			} 
		
		}

	private:
		PerThreadSingleton() {}
		PerThreadSingleton(const PerThreadSingleton&);
		PerThreadSingleton & operator = (const PerThreadSingleton &);
		static std::map<std::thread::id, T*> map;

};

template<typename T> std::map<std::thread::id, T*> PerThreadSingleton<T>::map;


} //Prothos
