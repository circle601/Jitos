#pragma once
#include "ByteCode.h"
#include "tdefs.h"

#ifndef _KERNAL
#define assert(x)
#else
#define assert(x)
#endif // DEBUG




#define HI_UINT16(a) ((((unsigned int)a) >> 8) & 0xFF)
#define LO_UINT16(a) (((unsigned int)a) & 0xFF)

#define a_UINT16(a) (((unsigned int)a) & 0xFF)
#define b_UINT16(a) ((((unsigned int)a) >> 8) & 0xFF)
#define c_UINT16(a) ((((unsigned int)a) >> 16) & 0xFF)
#define d_UINT16(a) ((((unsigned int)a) >> 24) & 0xFF)

#define Split2(a) LO_UINT16(a),HI_UINT16(a)
#define Split4(a) a_UINT16(a),b_UINT16(a),c_UINT16(a),d_UINT16(a)



typedef unsigned char Register;



#define FlagCarry 0
#define FlagZero  1
#define FlagNotZero  2
#define FlagSign  3
#define FlagNotSign  3


#define regEax 1
#define regEBx 2
#define regECx 3
#define regEDx 4
#define regFLAGS 5
#define regEDI 6
#define regESI 7
#define regEsp 8 

struct ScopeLevel {
	int varables;
	int startpoint;
	int endjumps;
};

struct Varable
{
	int type;
	int id;
	char StackStorage;
	Register RegisterStorage;
	bool init;
	bool Created;
};


static unsigned char PeakByte();
static unsigned char ReadByte();
static short ReadShort();
static unsigned short ReadUShort();
static int ReadInt();
static unsigned int ReadUInt();
static void Move(Varable* source, Varable* Dest);
static void Move(Varable* source, int Dest);
static bool MoveREG(Varable* source, Register Dest);
static bool ForceREG(Varable* source, Register Dest);
static void Cast();
static void SendHome(Register Dest);
//Interface


void SetPackage(PackageProgram* package);
void CopyOutput(char* result);
bool Compile();
void SetFunction(BytecodeProgram* function, Obj* parentClass);
void SetCompilerWorkingMemory(char* ptr, size_t sise);
void Error(const char*);
int ResultLength();