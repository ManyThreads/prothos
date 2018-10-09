#pragma once

#include "util/assert.hh"
#include <atomic>

// single reader, multiple writer fifo
template <typename T>
class FifoQueue
{
	public:
		FifoQueue()
			: head(nullptr)
			, tail(nullptr)
		{}

		void push(T* e){
			ASSERT(e);
			auto h =head.load();
			do{
				e->next.store(h);
			}while(!head.compare_exchange_weak(h,e));
		}

		T* pop(){
			T* ret = nullptr;
			if(tail == nullptr){
				auto h = head.exchange(nullptr);

				//invert list
				while(h != nullptr){
					auto tmp = h->next.load();
					h->next = tail;
					tail = h;
					h = tmp;
				}
			}
				
			if(tail != nullptr){
				ret = tail;
				tail = tail->next;
			}
			return ret;
			
		}

		bool empty(){
			return (head.load() == nullptr) && (tail == nullptr);
		}

	private:
			std::atomic<T* > head;
			T* tail; 
};

