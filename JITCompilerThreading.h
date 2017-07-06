#pragma once

#include "stdafx.h"

#ifdef _KERNAL

struct MutexInternal
{
	volatile int Locked;
};

typedef MutexInternal* Mutex;
typedef void* Thread;
typedef void* ThreadEvent;

#else


#include <thread>        
#include <mutex>
#include <windows.h>
#include <process.h>

typedef HANDLE Mutex;
typedef HANDLE Thread;
typedef HANDLE ThreadEvent;

#endif

Mutex MakeMutex();
bool LockMutex(Mutex mutex);
bool FreeMutex(Mutex mutex);
void DestroyMutex(Mutex mutex);
bool CheckMutex(Mutex mutex);

ThreadEvent CreateThreadEvent();
void SetThreadEvent(ThreadEvent thread);
void WaitThreadEvent(ThreadEvent thread);
void DestroyThreadEvent(ThreadEvent thread);
void SetThreadEvent(ThreadEvent resetEvent);