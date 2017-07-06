#include "Debug.h"
#include "stdafx.h"

#define Naked   __declspec( naked )  

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

#ifndef NewError
void Error(const char* errorcode) {
	PrintError(errorcode);
}
#endif
void Note(const char* errorcode) {
	PrintError(errorcode);
}

void Ping() {}
void Warn(const char* errorcode) {}

#else
using namespace std;
void PrintError(const char* errorcode) {
	cout << "Error:" << errorcode << "\n";
}


void Note(const char* errorcode) {
	cout << errorcode << "\n";
}
#ifndef NewError
void Error(const char* errorcode) {
	cout << "Error:" << errorcode << "\n";
}
#endif
void Warn(const char* errorcode) {
	cout << "warn:" << errorcode << "\n";
}



#endif


