#include "Queue.h"
#include "JITCompilerThreading.h"
#include "ByteCode.h"
#include "Debug.h"
#include "Memory.h"

Queue* Toload;
Mutex QueueMutex;
Mutex CompileMutex;
ThreadEvent resetEvent = 0;
volatile bool running = false;
volatile bool Tasks = false;
void* queuememory;

struct CompileItem {
	int ThreadId;
	Obj* item;
	int index;
};


void StopCompilerThread() {
	if (running) {
		running = false;
		SetThreadEvent(resetEvent);
	}
}

static int GetID() {
	return (int)CreateThreadEvent();
}

static void BlockUntilDone(CompileItem Toadd) {
#ifdef _KERNAL	
	Note("blocking");
	WaitThreadEvent((ThreadEvent)Toadd.ThreadId);
	DestroyThreadEvent((ThreadEvent)Toadd.ThreadId);
	Note("Done blocking");
#else
	Note("blocking");
	WaitThreadEvent((ThreadEvent)Toadd.ThreadId);
	DestroyThreadEvent((ThreadEvent)Toadd.ThreadId);
	Note("Done blocking");
#endif

}
bool InitJitD() {
	if (running) return false;
	size_t sise = QueueGetsise(sizeof(CompileItem), 255);
	queuememory = Allocate(sise);
	if (queuememory == NULL) {
		Error("unable to allocate queue");
		return false;
	}
	Toload = (Queue*)queuememory;
	if (!CreateQueue(Toload, sizeof(CompileItem), 255)) {
		Error("unable to create queue");
		return false;
	}

	if (resetEvent != NULL) {
		DestroyThreadEvent(resetEvent);
	}
	resetEvent = CreateThreadEvent();

	if (resetEvent == NULL) {
		Error("Error Reset Event.\n");
		return false;
	}

	if (QueueMutex != NULL) {
		DestroyMutex(QueueMutex);
	}
	QueueMutex = MakeMutex();

	if (QueueMutex == NULL) {
		Error("Error creating mutex.\n");
		return false;
	}

	if (CompileMutex != NULL) {
		DestroyMutex(QueueMutex);
	}
	CompileMutex = MakeMutex();

	if (CompileMutex == NULL) {
		Error("Error creating mutex.\n");
		return false;
	}


	return true;




}

void CloseJitD() {
	StopCompilerThread();
	Free(queuememory);
}

void SignalError(int ThreadId) {
	Error("Failed to Compile Function");
}

static inline void CompileNow(CompileItem WorkItem) {
	if (!CompileProgramIndex(WorkItem.item, WorkItem.index)) {
		SignalError(WorkItem.ThreadId);
	}
}

static void DoCompilework(CompileItem WorkItem) {
	LockMutex(CompileMutex);
	CompileNow(WorkItem);
	FreeMutex(CompileMutex);
}



void EnqueueFunction(Obj* item,int index,bool Blocking) {
	Note("Enququeing function");
	CompileItem Toadd;
#ifdef _KERNAL	
	//todo this
#else
	if (Blocking) {
		Toadd.ThreadId = GetID();
	}
	else {
		Toadd.ThreadId = 0;
	}
#endif
	
	Toadd.item = item;
	Toadd.index = index;

	if (!running) {
		if (!Blocking) return;
		CompileNow(Toadd);
		return;
	}

	if (LockMutex(QueueMutex)) {
		QueuePush(Toload, &Toadd);
		Tasks = true;
		FreeMutex(QueueMutex);
	}
	SetThreadEvent(resetEvent);
	if (Blocking) {
		BlockUntilDone(Toadd);
	}
}






void CompilerThreadRun() {
	if (running) return;
	running = true;
	Note("init Thread");
	if (!running) return;
	CompileItem WorkItem;
	Note("Starting main loop");
	while (running) {
		if (LockMutex(QueueMutex)){
			CompileItem* ptr = 0;
			if (!QueueisEmpty(Toload)) {
				ptr = (CompileItem*)QueuePeek(Toload);
				if (ptr != NULL) {
					WorkItem = *ptr;
					QueuePop(Toload);
				}
				if (!QueueisEmpty(Toload)) {
					Tasks = true;
				}
				else {
					Tasks = false;
				}
			}
			else {
				Tasks = false;
			}
			FreeMutex(QueueMutex);
			if (ptr != NULL) {
				Note("work to do");
				DoCompilework(WorkItem);
		#ifdef _KERNAL

				//todo this
		#else
				SetThreadEvent((ThreadEvent)WorkItem.ThreadId);

		#endif
			}
		}
		if (!running) break;
		if (!Tasks) {
			WaitThreadEvent(resetEvent); //todo check if it acually waits
		}
	}
	DestroyMutex(QueueMutex);
	DestroyThreadEvent(resetEvent);
	Note("Ending");
}

