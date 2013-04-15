#ifndef THREAD_H
#define THREAD_H

#ifndef __THREAD_ID
	#define BAD_ID 0xFFFFFFFF
	typedef unsigned int genericID;
	typedef genericID threadID;
	typedef genericID lockID;
	#define __THREAD_ID
#endif

//Thread initialization
void thread_initialize();
void thread_deinitialize();

//Global critical sections
void         thread_enter_critical_section();
void         thread_leave_critical_section();

//Threading
threadID     thread_create(void* funcPtr, void* userData);
threadID     thread_getcurrentID();
int          thread_waitfor(threadID ID);
void         thread_kill(threadID ID);
void         thread_sleep(double time);

//Processor information
int          thread_numprocessors();

//Lock API
lockID       lock_create();
void         lock_destroy(lockID lockID);
lockID       lock_enter(lockID lockID);
void         lock_leave(lockID lockID);
void         lock_waitfor(lockID lockID);

#endif