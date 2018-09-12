#pragma once

//#include <ctime>
//#include <cstdlib>
#include "runtime/umem.hh"

//#if __cplusplus <= 199711L
//#define thread_local __thread
//#define override
//#endif

//#ifndef _ISOC11_SOURCE
//#if !(_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)
//#error Need atleast posix_memalign!
//#endif
	  inline void* aligned_alloc(size_t alignment, size_t size)
	  {
		void* result;
		if (mythos::posix_memalign(&result, alignment, size) == 0) {
		  return result;
		} else {
		  return nullptr;
		}
	  }
//#endif


