#pragma once

#include <thread>
#include <map>
#include <mutex>

namespace Prothos{

template<typename T>
class Singleton{
	public:
		static T* getInstance(){
			if(instance == nullptr){
				instance = new T;
			}
			return instance;
		}
	private:
		Singleton() {}
		Singleton(const Singleton &);
		Singleton & operator = (const Singleton &);

		static T* instance;
};

} //Prothos
