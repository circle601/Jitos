#pragma once

#include "ByteCode.h"
#include "Memory.h"

Obj* JitAllocateObject(Obj* baseclass);
char JitAllocateArray(int Type,int amount);
void* JitGetCallPtr(Obj* object, int index);
void* JitGetItem(Obj* object, int index);