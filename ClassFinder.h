#pragma once
#include "bytecode.h"
void StartClassStore();
void CleanUpClassStore();
void AddJitClass(const char* name, Obj* object);
Obj* ResolveClass(const char* name);