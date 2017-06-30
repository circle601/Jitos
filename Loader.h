#pragma once


#include "tdefs.h"

#include "ByteCode.h"



void FreeProgram(PackageProgram* program);
PackageProgram* LoadProgramFile(char* data, size_t Count);
