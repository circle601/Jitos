#include "JITCompilerThreading.h"



#ifdef _KERNAL




#include "Memory.h"
//todo all this
Mutex MakeMutex() {
	return (Mutex)Allocate(sizeof(MutexInternal));
	return NULL; //TODO THIS
}


void ThreadAwaitPointer(bool* locker) {

}

bool LockMutex(Mutex mutex) {
	Mutex address = mutex;
	bool Locked = false;
	while (!Locked) {
		__asm {
			mov eax, 0;
			mov ebx, 1;
			lock CMPXCHG[address], ebx;
			SETZ Locked;
		};
		if (!Locked) {
			ThreadAwaitPointer((bool*)mutex);
		}
	}
	return Locked;
}

bool FreeMutex(Mutex mutex) {
	Mutex address = mutex;
	__asm{
	lock mov [address], 0
	};
}

void DestroyMutex(Mutex mutex)
{
	Free(mutex);
}

ThreadEvent CreateThreadEvent() {
	ThreadEvent eve = (ThreadEvent)Allocate(sizeof(MutexInternal));
	(*(int*)eve) = 1;
	return eve;
}

void SetThreadEvent(ThreadEvent thread) {
	(*(int*)thread) = 0;
}

void WaitThreadEvent(ThreadEvent thread) {
	ThreadEvent address = thread;
	bool Locked = false;
	while (!Locked) {
		__asm {
			mov eax, 0;
			mov ebx, 1;
			lock CMPXCHG[address], ebx;
			SETZ Locked;
		};
		if (!Locked) {
			ThreadAwaitPointer((bool*)address);
		}
	}
	return;
}

void DestroyThreadEvent(ThreadEvent thread) {
	Free(thread);
}


#else
#include <windows.h>
#include <process.h>

Mutex MakeMutex() {
	return CreateMutex(NULL, FALSE, NULL);

}

bool LockMutex(Mutex mutex) {
	DWORD dwWaitResult = WaitForSingleObject(mutex, 1000L);
	if (dwWaitResult == WAIT_FAILED) {
		DWORD result = GetLastError();
		
	}
	if (dwWaitResult == WAIT_OBJECT_0)
	{
		return true;
	}
	return false;
}

bool FreeMutex(Mutex mutex) {
	if (!ReleaseMutex(mutex))
	{
		//todo  Deal with error.
		return false;
	}
	return true;
}

void DestroyMutex(Mutex mutex)
{
	CloseHandle(mutex);
}


ThreadEvent CreateThreadEvent() {
	return CreateEvent(NULL, TRUE, FALSE, NULL); //should not be used, but makes the design fit closer to jitos
}

void SetThreadEvent(ThreadEvent thread) {
	PulseEvent(thread);
}

void WaitThreadEvent(ThreadEvent thread) {
	DWORD result = 0;
		do
		{
			result = WaitForSingleObjectEx(thread, 10000, FALSE);
		} while (result != WAIT_FAILED && result != WAIT_ABANDONED && result != WAIT_OBJECT_0);
}

void DestroyThreadEvent(ThreadEvent thread) {
	CloseHandle(thread);
}


#endif