#pragma once
#include "ByteCode.h"
void LoaderThreadRun();
void EnqueueFunction(Obj* item, int index, bool Blocking);
void StopLoaderThread();
bool InitJitD();
void CloseJitD();