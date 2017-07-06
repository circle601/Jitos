#pragma once
#include "stdafx.h"
#include "ByteCode.h"





const int ClearedHeader = 0x1111111111111;

bool StartAllocator(void* Memory, size_t memorySise);
void StopAllocator();
Obj* AllocatePointerObject(size_t count);
Obj* AllocateObject(size_t sise);
Obj* AllocateObject(Obj* baseClass);
bool FreeObject(Obj* ptr);

void* ClearAllocate(int requested_bytes);
void* Allocate(int requested_bytes);
bool Free(void* ptr, size_t sise);
bool Free(void* ptr );

void Fullcheck();