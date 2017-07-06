#pragma once
#include "ByteCode.h"
void CompilerThreadRun();
void EnqueueFunction(Obj* item, int index, bool Blocking);
void StopCompilerThread();
bool InitJitD();
void CloseJitD();