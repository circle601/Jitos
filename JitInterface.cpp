#include "stdafx.h"
#include "JitInterface.h"


Obj* JitAllocateObject(Obj* baseclass) {
	return AllocateObject(baseclass);
}
Obj* JitAllocateArray(Obj* baseclass) {
	return NULL;
}
void* JitGetCallPtr(Obj* object, int index) {
	return GetExecuteAddr(object, index);
}