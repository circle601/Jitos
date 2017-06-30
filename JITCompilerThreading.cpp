#include "JITCompilerThreading.h"




#ifdef _KERNAL
//todo all this
Mutex MakeMutex() {
	return NULL; //TODO THIS
}

bool LockMutex(Mutex mutex) {
	return false;
}

bool FreeMutex(Mutex mutex) {
	return false;
}

void DestroyMutex(Mutex mutex)
{
	
}

ThreadEvent CreateThreadEvent() {
	return NULL; //TODO THIS
}

void SetThreadEvent(ThreadEvent thread) {

}

void WaitThreadEvent(ThreadEvent thread) {

}

void DestroyThreadEvent(ThreadEvent thread) {
	
}

void SetThreadEvent(ThreadEvent resetEvent)
{
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