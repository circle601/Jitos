// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define NULL 0

#ifdef _KERNAL


#include "tdefs.h";
#include "size_t.h";
extern void* memcpy(void* destination, const void* source, size_t num);
extern size_t strlen(const char* str);
extern char* strcpy(char* destination, const char* source);
extern int strcmp(const char* str1, const char* str2);
extern void *memset(void *dest, char val, size_t count);
#else



#define NOCOMM 
#define WIN32_LEAN_AND_MEAN

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
#include <fstream>      
#include <iostream>
#include <string>
#include <iomanip>

#endif // DEBUG






// TODO: reference additional headers your program requires here
