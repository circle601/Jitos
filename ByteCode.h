
#pragma once


#include "tdefs.h"


#define ByteCodeNop 0



typedef volatile unsigned char AtomicByte;


typedef int (*MyFunc)(void);

//CallTableElement types
#define CallTableNull 0 
#define CallTableBytecode 1
#define CallTableProgram 2
#define CallTableSystemReserved 3
#define CallTableProgramPointer 4

#define CallTableBool 5
#define CallTableType 7
#define CallTableArray 8
#define CallTableString 9

#define CallTableByte 10
#define CallTableInt 12
#define CallTableUInt 13
#define CallTableFloat 14
#define CallTableDouble 15

#define CallTableObject 16

__declspec(align(4)) struct CallTableElement {
	unsigned int type;
	int ofset;
};

__declspec(align(4)) struct Array {
	unsigned int Length;
	char data;
};

typedef Array String;

__declspec(align(4)) struct BytecodeProgram {
	void* Compiled;
	unsigned int ReturnType;
	unsigned int ParamCount;
	unsigned int length;
	char data;
};

__declspec(align(4)) struct Obj {
	int length;
	AtomicByte Referances;
	Obj* Baseclass;
	Byte Flags;
	char CallMin;
	char CallMax;
	char data;
	//calltable
	//data
};

__declspec(align(4)) struct RedirectionTableItem {
	char* name;
	unsigned int id;
	Obj* ptr;
};

__declspec(align(4)) struct PackageProgram {
	int classesCount;
	int redirectionTableCount;
	char* name;
	char* startpoint;
	int startnumber;
	Obj** classes;
	RedirectionTableItem* redirectionTable;
};


void* GetDataPointer(Obj* object);

CallTableElement* GetCallElementPtr(Obj* object, char index);
void* GetItemPtr(Obj* object,CallTableElement* element);
void* GetItem(Obj* object, char index);



bool CompileProgramIndex(Obj* object, char index);
void* GetExecuteAddr(Obj* object, char index);




inline bool isNnumber(int type) {
	return (type == CallTableByte || type == CallTableInt || type == CallTableUInt || type == CallTableBool);
}

inline bool isObject(int type) {
	return (type == CallTableObject || type == CallTableArray || type == CallTableProgramPointer || type == CallTableString || type == CallTableType || type > 255);
}

inline bool isUnmoveable(int type) {
	return (type == CallTableBytecode || type == CallTableSystemReserved || type == CallTableProgram || type == CallTableNull);
}

inline bool isFloat(int type) {
	return (type == CallTableFloat || type == CallTableDouble);
}


inline bool isString(int type) {
	return (type == CallTableString || type == CallTableDouble);
}
