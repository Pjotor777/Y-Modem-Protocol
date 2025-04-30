//===================================================================================
// Name        : posixThread.hpp
// Author(s)   :
//			   : Craig Scratchley
//			   : Eton Kan
// Version     : September 2024
// Copyright   : Copyright 2020 - 2024, Craig Scratchley and Eton Kan
// Description : A derived class that allows creating threads with policy and priority.
//===================================================================================

#ifndef PTHREAD_POSIXTHREAD
#define PTHREAD_POSIXTHREAD

#include <iostream>
#include <cstring>
#include <thread>
#include <system_error>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>

inline static int initFunc() {
   // Pre-allocate some memory for the process.
   // Seems to make priorities work better, at least
   // when using gdb.
   // For some programs, the amount of memory allocated
   // here should perhaps be greater.
   void* ptr{malloc(5000)}; // check for errors
   free(ptr);

   /* Lock memory */
   int retVal{mlockall(MCL_CURRENT|MCL_FUTURE)};
   if(retVal) {
      throw std::system_error(errno, std::system_category());
   }
   return retVal;
}

inline static const int init = initFunc();

namespace pthreadSupport
{
	int getSchedParam(int *policy, sched_param *sch)
	;
	int getSchedParam(pthread_t th, int *policy, sched_param *sch)
	;
	int setSchedParam(int policy, sched_param sch)
	;
	int setSchedParam(pthread_t th, int policy, sched_param sch)
	;
   //function: get priority for the current thread
   int getSchedPrio()
   ;
   int setSchedPrio(int priority)
	;
	int setSchedPrio(pthread_t th, int priority)
	;
	int get_priority_max(int policy)
	;
	int get_priority_min(int policy)
	;

	//Set new thread's priority when it is higher than creating thread's priority
	template<typename _Callable, typename... _Args>
	void ctorHelperHigher(int policy, sched_param sch, sem_t &sem4, _Callable&& __f, _Args&&... __args){
		if (pthread_setschedparam(pthread_self(), policy, &sch)){
			sem_post(&sem4);
			throw std::system_error(errno, std::system_category());
			return;
		}
		sem_post(&sem4);
		__f(std::forward<_Args>(__args)...);
	}

	//Set new thread's priority when it is lower than creating thread's priority
	template<typename _Callable, typename... _Args>
	void ctorHelperLower(int policy, sched_param sch, sem_t &sem4, _Callable&& __f, _Args&&... __args){
		sem_post(&sem4);
		if (!setSchedParam(policy, sch)) {
			sched_yield(); // is this needed?
			__f(std::forward<_Args>(__args)...);
		}
	}

	class posixThread : public std::jthread
	{
	private:
		//Helper function for setting thread's policy and schedule parameter (including's priority (lower or higher))
		template<typename _Callable, typename... _Args>
		jthread beforeThread(int policy, sched_param sch, _Callable&& __f, _Args&&... __args) {
			sem_t sem4;
			jthread th;
			sched_param curSch;
			int curPolicy = -1;

			getSchedParam(&curPolicy, &curSch); //Getting creating thread info

			if(policy == -1){
				policy = curPolicy;
			}

			// do we need try/catch block here?  *wcs*
			sem_init(&sem4,0,0);
			if(curSch.sched_priority <= sch.sched_priority){
				th = std::jthread([&]{ctorHelperHigher(policy, sch, sem4, std::forward<_Callable>(__f), std::forward<_Args>(__args)...);});
				sched_yield(); // this may not be needed.
			}
			else{
				// don't need sem4 stuff in this case probably.
				th = std::jthread([&]{ctorHelperLower(policy, sch, sem4, std::forward<_Callable>(__f), std::forward<_Args>(__args)...);});
			}
			sem_wait(&sem4);
			sem_destroy(&sem4);
			return th;
		}

//		//Flag variable is used as a place holder so the code can distinguish if it is setting policy or priority
//		template<typename _Callable, typename... _Args>
//		thread beforeThread(int policy, ??? policy_flag, _Callable&& __f, _Args&&... __args) {
//	[...]
//		}

		template<typename _Callable, typename... _Args>
		jthread beforeThread(int policy, int prio, _Callable&& __f, _Args&&... __args) {
			sched_param curSch;
			int curPolicy = -1;

			getSchedParam(&curPolicy, &curSch); //Getting creating thread info
			curSch.sched_priority = prio;
			return beforeThread(policy, curSch, std::forward<_Callable>(__f), std::forward<_Args>(__args)...);
		}

	public:
		using jthread::jthread;

		posixThread() noexcept = default;

		//Start thread with custom priority and creating thread's policy
		template<typename _Callable, typename... _Args>
		explicit
		posixThread(int prio, _Callable&& __f, _Args&&... __args) :
			std::jthread(beforeThread(-1, prio, std::forward<_Callable>(__f), std::forward<_Args>(__args)...)){}

		//Start thread with custom policy and creating thread's parameters (including priority)
		// ??

		//Start thread with custom policy and custom priority
		template<typename _Callable, typename... _Args>
		explicit
		posixThread(int policy, int prio, _Callable&& __f, _Args&&... __args) :
			std::jthread(beforeThread(policy, prio, std::forward<_Callable>(__f), std::forward<_Args>(__args)...)){}

		//Start thread with custom parameters and creating thread's policy
		template<typename _Callable, typename... _Args>
		explicit
		posixThread(sched_param sch, _Callable&& __f, _Args&&... __args) :
			std::jthread(beforeThread(-1, sch, std::forward<_Callable>(__f), std::forward<_Args>(__args)...)){}

		//Start thread with custom parameter (including priority) and custom policy
		template<typename _Callable, typename... _Args>
		explicit
		posixThread(int policy, sched_param sch, _Callable&& __f, _Args&&... __args) :
			std::jthread(beforeThread(policy, sch, std::forward<_Callable>(__f), std::forward<_Args>(__args)...)){}

		void getSchedandPolicy(int *policy, sched_param *sch){
			getSchedParam(this->native_handle(), policy, sch);
		}

		int getPolicy(){
			sched_param sch;
			int policy = -1;
			getSchedParam(this->native_handle(), &policy, &sch);
			return policy;
		}

		sched_param getParam(){
			sched_param sch;
			int policy = -1;
			getSchedParam(this->native_handle(), &policy, &sch);
			return sch;
		}

		int getPriority(){
			sched_param sch;
			int policy = -1;
			getSchedParam(this->native_handle(), &policy, &sch);
			return sch.sched_priority;
		}

		int setPriority(int prio){
			return setSchedPrio(this->native_handle(), prio);
		}

		//Set schedule parameters and use the existing policy of this thread
		int setParam(sched_param sch){
			sched_param curSch;
			int curPolicy = -1;
			getSchedParam(this->native_handle(), &curPolicy, &curSch);
			return setSchedParam(this->native_handle(), curPolicy, sch);
		}

		int setParamAndPolicy(int policy, sched_param sch){
			return setSchedParam(this->native_handle(), policy, sch);
		}
	};
}

#endif
