/* =============================================================================
 * OpenGBH threading system based on the GLFW source code.
 *
 * GLFW 2.7 (win32_thread.c and related files)
 * http://glfw.sourceforge.net
 *
 * Retained header and license information follows:
 * -----------------------------------------------------------------------------
 * Copyright (c) 2002-2006 Camilla Berglund
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 * ===========================================================================*/
#include "threading.h"
#ifdef WIN32
#  include "windows.h"
#else
#  include <stdlib.h>
#  include <pthread.h>
#  include <sched.h>
#  include <signal.h>
#  include <sys/time.h>
#  include <unistd.h>
#if APL
#    include <sys/sysctl.h>
#endif
#endif

/*******************************************************************************
 * Internal data structures and typedefs
 ******************************************************************************/
typedef struct _thread_struct _thread;
typedef void thread_function(void*);
struct _thread_struct
{
	_thread*    Previous, *Next;  //Linked list implementation
	threadID    ID;               //Thread ID
	void*       Function;         //Function to call

	//System-specific handles
#ifdef WIN32
	HANDLE      Handle;
	DWORD       WinID;
#else
	pthread_t   PosixID;
#endif
};

threadID thread_next_id = 1;
_thread thread_main;

//Get thread pointer
_thread* _thread_getpointer(threadID ID)
{
	_thread* t;
	for (t = &thread_main; t != NULL; t = t->Next) {
		if (t->ID == ID) break;
	}
	return t;
}

//Remove thread
void _thread_remove(_thread* t)
{
	if (t->Previous != NULL)
	{
		t->Previous->Next = t->Next;
	}
	if (t->Next != NULL)
	{
		t->Next->Previous = t->Previous;
	}
	//Destroy lua state
	//lua_close(t->L);
	free((void*)t);
}




/*******************************************************************************
 * Windows specific code
 ******************************************************************************/
#ifdef WIN32
CRITICAL_SECTION thread_critical_section;
void thread_enter_critical_section()
{
	EnterCriticalSection(&thread_critical_section);
}
void thread_leave_critical_section()
{
	LeaveCriticalSection(&thread_critical_section);
}


//Wrapper for the newly created thread
DWORD WINAPI _thread_new(LPVOID lpParam)
{
	thread_function* threadfun;
	_thread*   t;

	//Get pointer to thread information for current thread
	t = _thread_getpointer(thread_getcurrentID());
	if (t == NULL) return 0;

	//Get user thread function pointer
	threadfun = t->Function;

	//Call the user thread function
	threadfun((void*)lpParam);

	//Kill thread
	//log_writem("Thread died [%p] (threadID[0x%x])",t->Function,t->ID);

	//Remove thread from thread list
	thread_enter_critical_section();
	_thread_remove(t);
	thread_leave_critical_section();

	//When the thread function returns, the thread will die...
	return 0;
}


//Thread management code
threadID thread_create(void* funcPtr, void* userData)
{
	threadID ID;
	_thread* t, *t_tmp;
	HANDLE      hThread;
	DWORD       dwThreadId;

	//Enter critical section
	thread_enter_critical_section();

	//Create a new thread information memory area
	t = (_thread*)malloc(sizeof(_thread));
	if (t == NULL)
	{
		thread_leave_critical_section();
		return -1;
	}

	//Get a new unique thread id
	ID = thread_next_id++;

	//Store thread information in the thread list
	t->Function = funcPtr;
	t->ID       = ID;

	//Create lua script state
	//script_initialize(&t->L);

	//Create thread
	hThread = CreateThread(
	              NULL,              //Default security attributes
	              0,                 //Default stack size (1 MB)
	              _thread_new,       //Thread function (a wrapper function)
	              (LPVOID)userData,  //Argument to thread is the user argument
	              0,                 //Default creation flags
	              &dwThreadId        //Returned thread identifier
	         );

	//Did the thread creation fail?
	if (hThread == NULL)
	{
		free((void*)t);
		thread_leave_critical_section();
		return -1;
	}

	//Store more thread information in the thread list
	t->Handle = hThread;
	t->WinID  = dwThreadId;

	//Append thread to thread list
	t_tmp = &thread_main;
	while (t_tmp->Next != NULL)
	{
		t_tmp = t_tmp->Next;
	}
	t_tmp->Next = t;
	t->Previous = t_tmp;
	t->Next     = NULL;

	//Leave critical section
	thread_leave_critical_section();

	//Return the thread ID
	//log_writem("Spawned thread [%p] (threadID[0x%x])",funcPtr,ID);
	return ID;
}

threadID thread_getcurrentID()
{
	_thread*  t;
	threadID ID = -1;
	DWORD    WinID;

	//Get Windows thread ID
	WinID = GetCurrentThreadId();

	//Enter critical section (to avoid an inconsistent thread list)
	thread_enter_critical_section();

	//Loop through entire list of threads to find the matching Windows thread ID
	for (t = &thread_main; t != NULL; t = t->Next)
	{
		if (t->WinID == WinID)
		{
			ID = t->ID;
			break;
		}
	}

	//Leave critical section
	thread_leave_critical_section();
	return ID;
}

int thread_waitfor(threadID ID)
{
	DWORD       result;
	HANDLE      hThread;
	_thread* t;

	//Enter critical section
	thread_enter_critical_section();

	//Get thread information pointer
	t = _thread_getpointer(ID);

	//Is the thread already dead?
	if (t == NULL)
	{
		thread_leave_critical_section();
		return 1;
	}

	//Get thread handle
	hThread = t->Handle;

	//Leave critical section
	thread_leave_critical_section();

	//Wait for thread to die
	//if(waitmode == GLFW_WAIT) {
	result = WaitForSingleObject(hThread, INFINITE);
	//} else if(waitmode == GLFW_NOWAIT) {
	//result = WaitForSingleObject(hThread, 0);
	//} else {
	//return 0;
	//}

	//Did we have a time-out?
	if (result == WAIT_TIMEOUT) return 0;
	return 1;
}

void thread_kill(threadID ID)
{
	_thread* t;

	//Enter critical section
	thread_enter_critical_section();

	//Get thread information pointer
	t = _thread_getpointer(ID);
	if (t == NULL)
	{
		thread_leave_critical_section();
		return;
	}

	//Message
	//log_writem("Killed thread [%p] (threadID[0x%x])",t->Function,t->ID);

	//Simply murder the process, no mercy!
	if (TerminateThread(t->Handle, 0))
	{
		//Close thread handle
		CloseHandle(t->Handle);
		//Remove thread from thread list
		_thread_remove(t);
	}

	//Leave critical section
	thread_leave_critical_section();
}


//Thread sleeping
void thread_sleep(double time)
{
	DWORD t;

	if (time <= 1e-3)
	{
		t = 1;
	}
	else if (time > 2147483647.0)
	{
		t = 2147483647;
	}
	else
	{
		t = (DWORD)(time*1000.0 + 0.5);
	}
	Sleep(t);
}


//Thread get number of processors
int thread_numprocessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return (int)si.dwNumberOfProcessors;
}


//Locks
lockID lock_create()
{
	CRITICAL_SECTION* lock;

	//Allocate memory for lock
	lock = (CRITICAL_SECTION*)malloc(sizeof(thread_critical_section));
	if (!lock)
	{
		return BAD_ID;
	}
	if ((lockID)lock == BAD_ID)
	{
		return BAD_ID;
	}

	//Initialize lock
	InitializeCriticalSection(lock);

	//Cast to lock ID and return
	return (lockID)lock;
}

void lock_destroy(lockID lockID)
{
	DeleteCriticalSection((CRITICAL_SECTION*)lockID);
	free((void*)lockID);

}

lockID lock_enter(lockID lockID)
{
	if (lockID != BAD_ID)
	{
		//Wait for lock to be released
		EnterCriticalSection((CRITICAL_SECTION*)lockID);
	}
	return lockID;
}

void lock_leave(lockID lockID)
{
	if (lockID != BAD_ID)
	{
		//Wait for lock to be released
		LeaveCriticalSection((CRITICAL_SECTION*)lockID);
	}
}
void lock_waitfor(lockID lockID)
{
	lock_leave(lock_enter(lockID));
}


//Initialize
void thread_initialize()
{
	//Initialize critical section handle
	InitializeCriticalSection(&thread_critical_section);

	//The first thread (the main thread) has ID 0
	thread_next_id = 0;

	//Fill out information about the main thread (this thread)
	thread_main.ID       = thread_next_id++;
	thread_main.Function = 0;
	thread_main.Handle   = GetCurrentThread();
	thread_main.WinID    = GetCurrentThreadId();
	thread_main.Previous = 0;
	thread_main.Next     = 0;
}


//Deinitialize
void thread_deinitialize()
{
	_thread* t, *t_next;

	//Enter critical section
	thread_enter_critical_section();

	//Kill all threads
	t = thread_main.Next;
	while (t != NULL)
	{
		//Get pointer to next thread
		t_next = t->Next;

		//Simply murder the process, no mercy!
		if (TerminateThread(t->Handle, 0))
		{
			//Close thread handle
			CloseHandle(t->Handle);

			//Free memory allocated for this thread
			free((void*) t);
		}

		//Select next thread in list
		t = t_next;
	}

	//Leave critical section
	thread_leave_critical_section();

	//Delete critical section handle
	DeleteCriticalSection(&thread_critical_section);
}




#else
/*******************************************************************************
 * POSIX threading code
 ******************************************************************************/



pthread_mutex_t thread_critical_section;
void thread_enter_critical_section()
{
	pthread_mutex_lock(&thread_critical_section);
}
void thread_leave_critical_section()
{
	pthread_mutex_unlock(&thread_critical_section);
}

void* _thread_new(void *arg)
{
	thread_function* threadfunc;
	_thread *t;
	pthread_t posixID;

	//Get current thread ID
	posixID = pthread_self();

	//Enter critical section
	thread_enter_critical_section();

	//Loop through entire list of threads to find the matching POSIX thread ID
	for(t = &thread_main; t != NULL; t = t->Next)
	{
		if(t->PosixID == posixID) break;
	}

	if(t == NULL)
	{
		thread_leave_critical_section();
		return NULL;
	}

	//Get user thread function pointer
	threadfunc = t->Function;

	//Leave critical section
	thread_leave_critical_section();

	//Call the user thread function
	threadfunc(arg);

	//Remove thread from thread list
	thread_enter_critical_section();
	_thread_remove(t);
	thread_leave_critical_section();

	//When the thread function returns, the thread will die...
	return NULL;
}


threadID thread_create(void* funcPtr, void* userData)
{
	threadID ID;
	_thread *t, *t_tmp;
	int result;

	//Enter critical section
	thread_enter_critical_section();

	//Create a new thread information memory area
	t = (_thread*)malloc(sizeof(_thread));
	if (t == NULL)
	{
		thread_leave_critical_section();
		return BAD_ID;
	}

	//Get a new unique thread id
	ID = thread_next_id++;

	//Store thread information in the thread list
	t->Function = funcPtr;
	t->ID       = ID;

	//Create thread
	result = pthread_create(
		&t->PosixID,      //Thread handle
		NULL,             //Default thread attributes
		_thread_new,      //Thread function (a wrapper function)
		(void*)userData   //Argument to thread is user argument
   	);

	//Did the thread creation fail?
	if(result != 0)
	{
		free((void*)t);
		thread_leave_critical_section();
		return BAD_ID;
	}

	//Append thread to thread list
	t_tmp = &thread_main;
	while (t_tmp->Next != NULL)
	{
		t_tmp = t_tmp->Next;
	}
	t_tmp->Next = t;
	t->Previous = t_tmp;
	t->Next     = NULL;

	//Leave critical section
	thread_leave_critical_section();
	return ID;
}

threadID thread_getcurrentID()
{
	_thread *t;
	threadID ID = -1;
	pthread_t posixID;

	//Get current thread ID
	posixID = pthread_self();

	//Enter critical section
	thread_enter_critical_section();

	//Loop through entire list of threads to find the matching POSIX thread ID
	for(t = &thread_main; t != NULL; t = t->Next)
	{
		if(t->PosixID == posixID)
		{
			ID = t->ID;
			break;
		}
	}

	//Leave critical section
	thread_leave_critical_section();
	return ID;
}


int thread_waitfor(threadID ID)
{
	pthread_t thread;
	_thread *t;

	//Enter critical section
	thread_enter_critical_section();

	//Get thread information pointer
	t = _thread_getpointer(ID);

	//Is the thread already dead?
	if(t == NULL)
	{
		thread_leave_critical_section();
		return 1;
	}

	//If got this far, the thread is alive => polling returns FALSE
	//if(waitmode == GLFW_NOWAIT)
	//{
		//thread_leave_critical_section();
		//return 0;
	//}

	//Get thread handle
	thread = t->PosixID;

	//Leave critical section
	thread_leave_critical_section();

	//Wait for thread to die
	(void)pthread_join(thread, NULL);

	return 1;
}


void thread_kill(threadID ID)
{
	_thread *t;

	//Enter critical section
	thread_enter_critical_section();

	//Get thread information pointer
	t = _thread_getpointer(ID);
	if(t == NULL)
	{
		thread_leave_critical_section();
		return;
	}

	//Simply murder the process, no mercy!
	pthread_kill(t->PosixID, SIGKILL);

	//Remove thread from thread list
	_thread_remove(t);

	//Leave critical section
	thread_leave_critical_section();
}


void thread_sleep(double time)
{
	struct timeval  currenttime;
	struct timespec wait;
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
	long dt_sec, dt_usec;

	if (time < 1e-3) {
//#ifdef _GLFW_HAS_SCHED_YIELD
		sched_yield();
//#endif
		return;
	}

	//Not all pthread implementations have a pthread_sleep() function. We
	//do it the portable way, using a timed wait for a condition that we
	//will never signal. NOTE: The unistd functions sleep/usleep suspends
	//the entire PROCESS, not a single thread, which is why we can not
	//use them to implement glfwSleep.

	//Set timeout time, relatvie to current time
	gettimeofday( &currenttime, NULL );
	dt_sec  = (long)time;
	dt_usec = (long)((time - (double)dt_sec) * 1000000.0);
	wait.tv_nsec = (currenttime.tv_usec + dt_usec) * 1000L;
	if(wait.tv_nsec > 1000000000L)
	{
		wait.tv_nsec -= 1000000000L;
		dt_sec++;
	}
	wait.tv_sec  = currenttime.tv_sec + dt_sec;

	//Initialize condition and mutex objects
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	//Do a timed wait
	pthread_mutex_lock(&mutex);
	pthread_cond_timedwait(&cond, &mutex, &wait);
	pthread_mutex_unlock(&mutex);

	//Destroy condition and mutex objects
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}


int thread_numprocessors()
{
#if APL
    int mib[2], ncpu, n;
    size_t len = 1;
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    n = 1;
    if( sysctl( mib, 2, &ncpu, &len, NULL, 0 ) != -1 )
    {
        if( len > 0 )
        {
            n = ncpu;
        }
    }
	return n;
#else
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}


//Locks
lockID lock_create()
{
	pthread_mutex_t *mutex;

	//Allocate memory for mutex
	mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	if(!mutex) return BAD_ID;

	//Initialise a mutex object
	(void)pthread_mutex_init(mutex, NULL);
	return (lockID)mutex;
}

void lock_destroy(lockID lockID)
{
	//Destroy the mutex object
	pthread_mutex_destroy((pthread_mutex_t*)lockID);
	//Free memory for mutex object
	free((void*)lockID);
}


lockID lock_enter(lockID lockID)
{
	//Wait for mutex to be released
	(void)pthread_mutex_lock((pthread_mutex_t*)lockID);
	return lockID;
}


void lock_leave(lockID lockID)
{
	//Release mutex
	pthread_mutex_unlock((pthread_mutex_t*)lockID);
}

void lock_waitfor(lockID lockID)
{
	lock_leave(lock_enter(lockID));
}


void thread_initialize()
{
	// Initialize critical section handle
	(void) pthread_mutex_init(&thread_critical_section, NULL );

	// The first thread (the main thread) has ID 0
	thread_next_id = 0;

	// Fill out information about the main thread (this thread)
	thread_main.ID       = thread_next_id++;
	thread_main.Function = NULL;
	thread_main.Previous = NULL;
	thread_main.Next     = NULL;
	thread_main.PosixID  = pthread_self();
}


void thread_deinitialize()
{
	_thread *t, *t_next;

	//Enter critical section
	thread_enter_critical_section();

	//Kill all threads (NOTE: THE USER SHOULD WAIT FOR ALL THREADS TO
	//DIE, _BEFORE_ CALLING glfwTerminate()!!!)
	t = thread_main.Next;
	while (t != NULL)
	{
		//Get pointer to next thread
		t_next = t->Next;

		//Simply murder the process, no mercy!
		pthread_kill(t->PosixID, SIGKILL);

		//Free memory allocated for this thread
		free((void*)t);

		//Select next thread in list
		t = t_next;
	}

	//Leave critical section
	thread_leave_critical_section();

	//Delete critical section handle
	pthread_mutex_destroy(&thread_critical_section);
}

#endif
