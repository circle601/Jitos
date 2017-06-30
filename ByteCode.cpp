#include "stdafx.h"
#include "ByteCode.h"
#include "Compiler.h"
#include "Memory.h"
#include "JitThread.h"




void* GetDataPointer(Obj* object) {
	unsigned char langth = object->CallMax - object->CallMin;
	return (void*)((char*)object + langth * sizeof(CallTableElement));
}

void* GetItemPtr(Obj* object, CallTableElement* element) {
	if (element == NULL) return  NULL;
	if (object == NULL) return  NULL;

	unsigned char langth = object->CallMax - object->CallMin;
	return (void*)((char*)(&object->data) + (langth * sizeof(CallTableElement) + element->ofset));
}

void* GetItemPtr(Obj* object, char index) {
	CallTableElement* item = GetCallElementPtr(object, index);
	unsigned char langth = object->CallMax - object->CallMin;
	return object + langth * sizeof(CallTableElement) + item->ofset;
}


CallTableElement* GetCallElementPtr(Obj* object, char index) {
	unsigned char min = object->CallMin;
	unsigned char max = object->CallMax;
	if (index >= min && index < max) {
		char calloffset = index - min;
		return (CallTableElement*)(&(object->data) + calloffset * sizeof(CallTableElement)); //check this bit
	}
	else {
		return 0;
	};
}

void RunAddress(void* address) {
	int result = ((MyFunc)address)();
}

bool CompileIndex(Obj* object, char index) {
	CallTableElement* item = GetCallElementPtr(object, index);
	if (item == NULL) {
		return false;
	}
	void* ptr = GetItemPtr(object, item);
	if (ptr == NULL) return false;
	int type = item->type;
	if (type == CallTableBytecode) {
		BytecodeProgram* program = (BytecodeProgram*)ptr;
		//check if bitecode compiled
		int compiledAddr = (int)program->Compiled;
		if (compiledAddr != 0) {
			return (void*)compiledAddr;
		}
		//compile bitecode

		SetFunction(program, object);
		if (Compile()) {
			size_t length = ResultLength();
			char* place = (char*)Allocate(length);
			if (place == NULL) {
				return false;
			}
			program->Compiled = place;
			CopyOutput(place);
			return true;
		} else {
				return false;
		}
	}
	else if (type == CallTableProgram)
	{
		return true;
	}
	else {
		return false;
	}
}

void* GetExecuteAddr(Obj* object, char index) {
	CallTableElement* item = GetCallElementPtr(object, index);
	if (item == NULL) {
		if (object->Baseclass != NULL) {
			return GetExecuteAddr(object->Baseclass, index);
		}
		return 0;
	}
	void* ptr = GetItemPtr(object, item);
	if (ptr == NULL) return 0;
	int type = item->type;
	if (type == CallTableBytecode) {  //compile bitecode

		BytecodeProgram* program = (BytecodeProgram*)ptr;
		if (program->Compiled != NULL) {
			return program->Compiled;
		}
		else {
			EnqueueFunction(object, index, true);
		}
		
		if(program->Compiled != NULL){
			return program->Compiled;
		}
		else {
			return NULL;
		}

	}
	else if (type == CallTableProgram) // precompiled code
	{
		if (ptr == 0) {
			return NULL;
		}
		return ptr;
	}
	else 
	{
		return NULL;
	}
	return NULL;
	}




