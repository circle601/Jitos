#include "Debug.h"
#include "stdafx.h"

Naked void PrintStack(void) { //todo finish this
	__asm {

	}
	for (int i = -8; i < 8; i++)
	{
		int value = 0;
		__asm {
			mov eax, i
			mov eax, [esp + eax * 4]
			mov value, eax
		}
		//NumNote(value);
	}
	__asm {
		ret
	}
}

#ifdef _KERNAL


void Error(const char* errorcode) {
	PrintError(errorcode);
}

static inline void Note(const char* errorcode) {}
static inline void Ping() {}
static inline void Warn(const char* errorcode) {}

#else
using namespace std;
void PrintError(const char* errorcode) {
	cout << "Error:" << errorcode << "\n";
}


void Note(const char* errorcode) {
	cout << errorcode << "\n";
}

void Error(const char* errorcode) {
	cout << "Error:" << errorcode << "\n";
}

void Warn(const char* errorcode) {
	cout << "warn:" << errorcode << "\n";
}

#endif


