#pragma once

#include "runtime/mlog.hh"
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
			MLOG_INFO(mlog::app, __func__, DVAR(e), DVAR(this));
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
				MLOG_INFO(mlog::app, __func__, DVAR(tail), DVAR(this));
				ret = tail;
				tail = tail->next;
			}
			return ret;
			
		}

	private:
			std::atomic<T* > head;
			T* tail; 
};

