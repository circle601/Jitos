#include "stdafx.h"
#include "Compiler.h"
#include "JitInterface.h"

#define NewError

#include "Debug.h"
#undef NewError

//todo need to make a multythreaded compiler


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
static char* ReadString();






bool ErrorCode = false;
#ifndef _KERNAL

using namespace std;



static void Error(const char* errorcode) {
	cout << "Error:" << errorcode << "\n";
	ErrorCode = true;
}

#else

static inline void Error(const char* errorcode) {
	PrintError(errorcode);
	ErrorCode = true;
}

#endif // DEBUG

#define AssertClear() if(ErrorCode) return false;;



template<class First = char, class ...Rest>
void CC(First first, Rest... rest) {
	WorkPlace[iOut++] = first;
	CC(rest...);
}

template<class First = char>
void CC(First first) {
	WorkPlace[iOut++] = first;
	if (iOut > MaxWorkPlaceSise) Error("workplace overflow");
}



#define RegisterCount 5






#pragma region  varables

#define SCOPES 16

//also errvor varable uptop
static PackageProgram* Package;
static Obj* ParentClass;
static int VariableTop = 0;
static unsigned int ScopeTop = 0;
static Byte* WorkPlace; // place to store program being created;
static size_t MaxWorkPlaceSise;
static ScopeLevel Scope[SCOPES];
static Byte VariablesinScope[SCOPES];  //stack of varables in scope
static Varable Variables[SCOPES];
static int endJumps[SCOPES];
static Byte parameters[SCOPES];
static int Registers[RegisterCount]; //abcdflags
static Byte* program; // bytecode
static int Codelength;
static int iP = 0; // place in input
static int iOut = 0; // place in output
static int ReturnType;
static int VarId; // number of initvarables;
static int StackBias;
static int EndJumpIid;

static int Flagswanted = 0;
static int Flagvar = 0;
static bool clearFlags = true;
#pragma endregion




static bool AddVar(Byte id, int Type) {
	if (VarId == SCOPES) {
		return false;
	}
	Varable var;
	var.id = id;
	var.type = Type;
	var.init = false;
	var.RegisterStorage = 0;
	var.Created = true;
    Variables[id] = var;
	VariablesinScope[VarId] = id;
	Scope[ScopeTop].varables++;
	VarId++;
	var.StackStorage = 0;
	return true;
}

static void UnloadVar(Varable var) {
	if (var.init) {
		CC(0x58);
		StackBias--;
		(&Variables[var.id])->init = false;
	}
}

static void ReturnCleanUp()
{
	for (size_t i = 0; i < VarId; i++)
	{
		Varable var = Variables[VariablesinScope[i]];
		if (var.init) {
			ForceREG(&var, regEDx);
			CC(0x5A);
		}
	}
	return;
}

static bool AddScope() {
	ScopeTop++;
	ScopeLevel level;
	level.startpoint = iOut;
	level.varables = 0;
	level.endjumps = 0;
	Scope[ScopeTop]= level;
	Assert(ScopeTop > 0);
	return true;
}

static void SetFlags(Varable* var,Byte Target) {
	Flagvar =var->id;
	Flagswanted = Target;
	clearFlags = false;
}

void SetFunction(BytecodeProgram* function, Obj* parentClass) {

	ParentClass = parentClass;
	program = (Byte*)&function->data;
	Codelength = function->length;
	ReturnType = function->ReturnType;
	VariableTop = 0;
	ScopeTop = 0;
	iP = function->ParamCount *sizeof(int);
	iOut = 0;
	StackBias = 0;
	AddScope();
	for (size_t i = 0; i < RegisterCount; i++)
	{
		Registers[i] = 0;
	}
	for (size_t i = 0; i < SCOPES; i++)
	{
		Variables[i].id = i;
		Variables[i].Created = false;
	}
	if (function->ParamCount > 0) {
		int count = function->ParamCount;
		int param = 1;
		for (size_t i = 0; i < count; i++)
		{
			Varable* var = &Variables[param++];
			var->Created = true;
			var->type = ((int*)program)[i];
			var->RegisterStorage = 0;
			var->init = true;
			var->StackStorage = i - count;
		}
	}
	Assert(ScopeTop == 1);
}







static void UnloadScope() {
	if (ScopeTop == 0) {
		Error("No scope to unload");
		return;
	}
	int end = iOut;
	int endcount = Scope[ScopeTop].endjumps;
	for (size_t i = 0; i < endcount; i++)
	{
		int id = endJumps[ --EndJumpIid];
		int endpos = end  - id - 3;
		WorkPlace[id] = (char)(endpos & 0xff);
		WorkPlace[id + 1] = (char)((endpos >>8) & 0xff);
		WorkPlace[id + 2] = (char)((endpos >> 16) & 0xff);
		WorkPlace[id + 3] = (char)((endpos >> 24) &  0xff);
	}

	int count = Scope[ScopeTop].varables;
	for (size_t i = 0; i < count; i++)
	{
		UnloadVar(Variables[VariablesinScope[--VarId]]);
	}
	Scope[ScopeTop].varables = 0;
	ScopeTop--;
}

static int memoryOfset(int stackid) {
	if (stackid > StackBias) {
		Warn("invalid varable offset");
	//	return 0;
	}
	return (StackBias - stackid) * 4;
}


static int GenToStart() {
	return  Scope[ScopeTop].startpoint - iOut;
}
static int GenToEnd() {
	endJumps[EndJumpIid++] = iOut;
	Scope[ScopeTop].endjumps++;
	return 0;
}




static int RegisterForVarable(Register reg) {
	switch (reg)
	{
	case(regEax): return 000;
	case(regECx): return 1;
	case(regEDx): return 2;
	case(regEBx): return 3;
	case(regEsp): return 4;
	//case(ebp): return 5;
	case(regESI): return 6;
	case(regEDI): return 7;
	default:
		Error("unknow register");
		return 0;
	}
}


static inline char SIB(int Scale,Register Index, Register Base) {
	Byte SIB = 0;
	SIB |= Scale << 6;
	SIB |= (RegisterForVarable(Index) & 0x7) << 3;
	SIB |= (RegisterForVarable(Base) & 0x7);
	return SIB;
}

static inline char ModRM(int mod, Register Reg, Register RM) {
	Byte modrm = 0;
	modrm |= mod << 6;
	modrm |= (RegisterForVarable(Reg) & 0x7) << 3;
	modrm |= (RegisterForVarable(RM) & 0x7);
	return modrm;
}
static inline char ModRMSingle(int mod, int part2, Register RM) {
	Byte modrm = 0;
	modrm |= mod << 6;
	modrm |= (part2 & 0x7) << 3;
	modrm |= (RegisterForVarable(RM) & 0x7);
	return modrm;
}

static void SendAllHome() {
	SendHome(regEax);
	SendHome(regEBx);
	SendHome(regECx);
	SendHome(regEDx);
}
static void SendAllHomeExcept(Register reg) {
	for (Register i = 0; i < 4; i++)
	{
		if (i != reg) {
			SendHome(i);
		}
	}
}


static inline void CCNumber(int number) {
	unsigned int num = (unsigned int)number;
	if (number > 127) {
		CC(Split4(num));
	}
	else {
		CC((char)num);
	}
}

static void CheckInit(Varable* A) {
	if ((!A->init && A->RegisterStorage == 0) || !A->Created ) {
		Error("varable not initalised");
		return;
	}
}



static void Instruction(char code, Varable* A, Varable* B) {
	int Areg = A->RegisterStorage;
	int Breg = B->RegisterStorage;
	int Reg1 = 0;
	int Reg2 = 0;;
	bool Register = false;
	CheckInit(A);
	CheckInit(B);
	if (A->init && Areg == 0 && B->init && Breg == 0) {
		MoveREG(A, regEax);
		Areg = A->RegisterStorage;
		Breg = B->RegisterStorage;
	}
	int ofset = 0;
	if (A->init && Breg != 0 ) {
		int Aofset = memoryOfset(A->StackStorage);
		ofset = Aofset;
		Reg1 = Breg;
	}
	else if (B->init && Areg != 0) {
		int Bofset = memoryOfset(B->StackStorage);
		ofset = Bofset;
		Reg1 = Areg;
	}
	else {
		Register = true;
		Reg1 = Areg;
		Reg2 = Breg;
	}
		CC(code);
		if (Register) {
			//todo fix it
			Error("not done");
		}else {
			CC(ModRM((ofset > 127) ? 2 : 1, Reg1, regEsp));
			CC(SIB(0, regEsp, regEsp));
			CCNumber(ofset);
		}
}

static void InstructionSingle(char code, char Part2, Varable* A) {
	CheckInit(A);
	int Areg = A->RegisterStorage;
	if (A->init && Areg != 0 ) {
		CC(code);
		CC(ModRMSingle(3, Part2, Areg));
	}
	else {
		int ofset = memoryOfset(A->StackStorage);
		CC(code);
		CC(ModRMSingle((ofset > 127) ? 2 : 1, Part2, regEsp));
		CC(SIB(0, regEsp, regEsp));
		CCNumber(ofset);
	}
}

static  void BreakPoint() {
	CC(0xCC);
}
static unsigned int ResolveIndex(int index) {
	if (index < 256) {
		return index;
	}
	else {
		int id = index - 256;
		if (id < Package->redirectionTableCount) {
			void* result = (void*)Package->redirectionTable[id].ptr;
			return (int)result;
		}
		else {
			Error("invalid redirect index");
		}
		
	}
	return 0;
}

static void NewStackFrame() {
	CC(0x55, 0x89, 0xE5);
	//push    ebp
	//mov     ebp, esp
}
static void RestoreStackFrame() {
	CC(0x5d);
}
static void PushConstant(int constat) {

	if (constat < 128) {
		CC(0x6A, (char)constat);
	}
	else {
		CC(0x68, Split4(constat));
	}
	StackBias++;
}
static void PushArgument(Varable* var) {
	
//	MoveREG(var, regEax);
	int reg = var->RegisterStorage;
	if (reg == regEax) {
		CC(0x50);
	}else	if (reg == regEBx) {
		CC(0x53);
	}
	else	if (reg == regEDx) {
		CC(52);
	}
	else {
		InstructionSingle(0xFF, 6, var); //todo Fix
	}
	StackBias++;
}
static void ClearArgument() {
	StackBias--;
	CC(0x5a);//  pop    edx 
}
static void BuiltinCall(void* address) {
	CC(0xb8, Split4((unsigned int)address)); // mov    eax, 0x400
	CC(0xff, 0xd0);//call eax
}


static void SetVarable(Varable* var,int Value) {

	if (var->init) {
		int Register = var->RegisterStorage;
		if (Register == 0) {//varable is in memory location
			int ofset = memoryOfset(var->StackStorage);
			if (ofset < 128) {
				CC(0xC7, 0x44, 0x24, (char)ofset, Split4(Value));
			}
			else {
				CC(0xC7, 0x84, 0x24, Split4(ofset), Split4(Value));
			}
		}
		else if (Register == regEax) { //varable is in Register
			CC(0xb8, Split4(Value)); // mov eax, 4
		}
		else {
			SendHome(Register);
			SetVarable(var, Value);
		}
	}
	else {
		int type = var->type;
		if (isNnumber(type)) {
			PushConstant(Value);
		}
		else if (isObject(type)) {
			PushConstant(Value);
		}
		else {
			Error("unfinised");
		}
		var->init = true;
		var->StackStorage = StackBias;
	}
}

void Stacktrace() {
	Error("Stack trace Disabled");
	//SendAllHome();
	//BuiltinCall(PrintStack);
}

bool Compile() {

	Assert(ScopeTop == 1);
	while(iP < Codelength)
	{
		Byte Opcode = ReadByte();

		switch (Opcode)
		{
		case(0x0): //nop
			//CC(0xFF);
			break;


	//___________________________0x1X debugging
		case(0x10): //TODO breakpoint
			CC(0xCC); ///INT 3  (but a special debugging opcode for it)
			break;
		case(0x11): //TODO throw
			break;
#ifdef NOTKERNAL
		case(0x12): //PrintNum Debug
		{
		Byte varable = ReadByte();
			Varable* var = &Variables[varable];
			PushArgument(var);
			SendHome(regEax);
			SendHome(regEDx);
			BuiltinCall(PrintNumDebug);
			ClearArgument();
		}
		break;
#endif
	//___________________________0x2X Function
		case(0x20): 
			ReturnCleanUp();
			CC(0xC3); // ret
			break;
		case(0x21): //TODO ReturnObject
		{
			Byte varable = ReadByte();
			CheckInit(&Variables[varable]);
			if (Variables[varable].type != ReturnType && !(ReturnType == CallTableObject && isObject(Variables[varable].type))) {
				Error("invalid Type");
				return false;
			}
			MoveREG(&Variables[varable], regEax);
			ReturnCleanUp();
			CC(0xC3);  // ret
			break;
		}
		case(0x22): //TODO Call
		{
			//0x22 Call // length 3: opcode, var,returnvar, elementid , param count, parameters
			
			Byte	varable = ReadByte();
			Byte	returnvarable = ReadByte();
			Byte	elementid = ReadByte();
			Byte	paramCount = ReadByte();

			Varable* thisvar = &Variables[varable];
			Varable* Returnvarable = &Variables[returnvarable];
			CheckInit(thisvar);
			
			if (thisvar->RegisterStorage != 0 && thisvar->init) {
				SendAllHomeExcept(thisvar->RegisterStorage);
				PushConstant((unsigned int)elementid);
				PushArgument(thisvar);
				SendHome(thisvar->RegisterStorage);
			}
			else {
				SendAllHome();
				PushConstant((unsigned int)elementid);
				PushArgument(thisvar);
			}
		
			BuiltinCall(JitGetCallPtr);
			ClearArgument();
			ClearArgument();
			PushArgument(thisvar);
			for (size_t i = 0; i < paramCount; i++)
			{
				Varable* var = &Variables[ReadByte()];
				CheckInit(var);
				PushArgument(var);
			}

			CC(0xff, 0xd0);//call eax;
			ClearArgument();
			for (size_t i = 0; i < paramCount; i++)
			{
				ClearArgument();
			}
			if (returnvarable != 255) {
				ForceREG(Returnvarable, regEax);
			}
			break;
		
		}
		case(0x23): //TODO CallObject
			Error("notdone CallObject"); break;
		case(0x24): //TODO Constant
			{
			Byte varable;
			Byte Type;
				varable = ReadByte();
				Type = Variables[varable].type;
					
					//case(CallTableSystemReserved):
					//case(CallTableBool):
					//case(CallTableArray):
					//case(CallTableString):
						
					if(isNnumber(Type))
					{
					unsigned int Value = ReadUInt();
					SetVarable(&Variables[varable], Value);

					}
					else if (isFloat(Type)) {
						Error(" float not supported");
					}
					else if(CallTableString == Type){
					//todo Need to have way of freeing string
						char* stringdata = ReadString();
						int length = strlen(stringdata) + 1;
						char* stringmemory = (char*)Allocate(length);
						memcpy(stringmemory, stringdata, length);
						SetVarable(&Variables[varable], (int)stringmemory);
					}
					else {
						Error("unsupported type");
					}
					
				break;
			}
		case(0x25): //TODO ConstantPool
			Error("notdone"); break;
		case(0x26):// Call(dynamic) // length2: opcode, object,idvarable
		{
			Error("notdone"); break;
		}
		case(0x27):// returnConstant // length3+: opcode,varable,Data todo
		{
			Error("notdone"); break;
		}
		case(0x28):// Call(compiletime) // length 3: opcode, object,output elementid,params  // calling a static method
		{
			unsigned int object = ReadUInt();
			Byte output = ReadByte();
			Byte elementid = ReadByte();
			Byte paramCount = ReadByte();

			
			
			if (object < 255) Error("Static method on primative");
			unsigned int Objectptr = ResolveIndex(object);
			if (object < 255) Error("Static method on primative");
			if (Objectptr == 0) Error("class not found");
			
			SendAllHome();

			PushConstant((unsigned int)elementid);
			PushConstant((unsigned int)Objectptr);
		
			BuiltinCall(JitGetCallPtr);
			ClearArgument();
			ClearArgument();

			for (size_t i = 0; i < paramCount; i++)
			{
				Varable* var = &Variables[ReadByte()];
				CheckInit(var);
				PushArgument(var);
			}
			CC(0xff, 0xd0);//call eax;
			if (output != 255) {
				Varable* Returnvarable = &Variables[output];
				ForceREG(Returnvarable, regEax);
			}
		
			for (size_t i = 0; i < paramCount; i++)
			{
				ClearArgument();
			}
		}
		break;
		case(0x29):// Call(self compiletime) // length 3: opcode, elementid,param count, params  // calling a static method in same class
		{
			SendAllHome();
			Byte elementid = ReadByte();
			Byte paramCount = ReadByte();
			if (paramCount > 0) {
				Error("unfinished paramcount");
			}
			for (size_t i = 0; i < paramCount; i++)
			{
				Varable* var = &Variables[ReadByte()];
				CheckInit(var);
				PushArgument(var);
			}
			PushConstant((unsigned int)elementid);
			PushConstant((unsigned int)ParentClass);
		
			BuiltinCall(JitGetCallPtr);
			ClearArgument();
			ClearArgument();
			CC(0xff, 0xd0);//call eax;
			for (size_t i = 0; i < paramCount; i++)
			{
				ClearArgument();
			}
			
		}
			//___________________________0x3X Scope
			case(0x30): // scopeStart
				AddScope();
				break;
			case(0x31): //ScopeElse
				Error("unfinished");
				break;
			case(0x32): //ScopeEnd
				UnloadScope();
				break;

	//___________________________0x5X branch


					case(0x40): // ConditionalToStart
					{
						Error("unfinished");
						break;
					}
					case(0x41): // ConditionalToEnd
					case(0x42): // ConditionalToExit
					{
						int varable = ReadByte();
						if (varable == Flagvar) {
							if (Flagswanted == FlagZero) {
								CC(0x0F, 0x84); // JE
							}
							else if (Flagswanted == FlagNotZero) {
								CC(0x0F, 0x85); // JNE
							}
							else {
								Error("unfinished");
							}

						}
						else {
							Error("unfinished");
						}
						
						
						int gen = GenToEnd();
						CC(Split4(gen));
						break;
					}
						
					case(0x43): // ToStart
					{
						int gen = GenToStart() - 5;
						CC(0xE9, Split4(gen));
						break;
					}
					case(0x44): // ToEnd
					{
						CC(0xE9);
						int gen = GenToEnd() ;
						CC(Split4(gen));
						break;
					}
					case(0x45): // ToExit exits all blocks in else trail
						Error("unfinished"); break;
	//___________________________0x5X Varables
			case(0x50):// CreateVar
			{
				int varable = ReadByte();
				int Type = ReadUInt();
				if (!AddVar(varable, Type)) {
					Error("unable to create var");
					return false;
				}
				break;
			}
			case(0x52):// SetVar
			{
				int varableA = ReadByte();
				int varableB = ReadByte();
				Move(&Variables[varableA], &Variables[varableB]);
				break;
			}
			case(0x53):// Null
			{
				Error("notdone");
				int varableA = ReadByte();
			}
			case(0x54):// New(dynamic)
			{
				Error("notdone");
			}break;
			case(0x55):// New(static)
			{

				int AType = ReadUInt();
				int varableBResult = ReadByte();

				if (AType == CallTableArray) {

				} else if(AType == CallTableString) {
					Error("notdone");
				}else if (AType < 256 && AType != CallTableObject) {
					Error("notdone");
				}
				else {
					Varable* var = &Variables[varableBResult];
					if (!(var->type == CallTableObject || var->type == AType)) { //todo inheritance
						Error("incompatible types");
					}
					SendAllHome();
					PushConstant(ResolveIndex(AType));
					BuiltinCall(JitAllocateObject);
					ClearArgument();

					var->init = true;
					var->StackStorage = ++StackBias;
					ForceREG(var, regEax);
					CC(0x50);// push eax
					
				}

			}break;
			case(0x56):// GetType
				Error("notdone"); break;
			case(0x57):// GetElementstatic (compile time)

			case(0x58):// SetElementstatic (compile time)
			case(0x59):// Typecast
			case(0x5a):// Typecast(dynamic)
				Error("notdone"); break;
			case(0x5b):// GetElement (compile time)
			{
				Byte VarableA = ReadByte();
				Byte Index = ReadByte();
				Byte VarableB = ReadByte();

				

				
				Varable* InputVar = &Variables[VarableA];
				Varable* OuputVar = &Variables[VarableB];

				CheckInit(InputVar);
				SendAllHome();
				if (!isObject(InputVar->type)) {
					Error("Object Expected");
				}
				//todo need to do type check
				PushConstant(Index);
				PushArgument(InputVar);
				BuiltinCall(JitGetItem);
				ClearArgument();
				ClearArgument();
				if (!OuputVar->init) {
					SetVarable(OuputVar,0);
				}
				if (OuputVar->RegisterStorage == 0) {
					CheckInit(OuputVar);
					CC(0x8b, 0x00);//  mov    eax, DWORD PTR[eax]
					
					int ofset = memoryOfset(OuputVar->StackStorage);
					if (ofset > 127) {
						CC(0x89, 0x84, 0x24, Split4(ofset));
					}
					else {
						CC(0x89, 0x44, 0x24, (Byte)ofset);
					}
				}else{
					Error("varable on stack expected Expected");
				}
				break;
			}
			case(0x5c):// SetElement (compile time)
			{
				Byte VarableA = ReadByte();
				Byte Index = ReadByte();
				Byte VarableB = ReadByte();
				SendAllHome();
				Varable* InputVar = &Variables[VarableA];
				Varable* OuputVar = &Variables[VarableB];
				if (!isObject(InputVar->type)) {
					Error("Object Expected");
				}
				//todo need to do type check
				PushConstant(Index);
				PushArgument(InputVar);
				BuiltinCall(JitGetItem);
				ClearArgument();
				ClearArgument();

				break;
			}
	//___________________________0x7X math
			case(0x70): //Div
			case(0x71):// Mul
				Error("notdone"); break;
			case(0x72): //Add
			{
				int varableA = ReadByte();
				int varableB = ReadByte();
				int Result = ReadByte();
				int vartypeA = Variables[varableA].type;
				int vartypeB = Variables[varableB].type;
				if (vartypeA == vartypeB && isNnumber(vartypeA))
				{
					Instruction(0x03, &Variables[varableA], &Variables[varableB]); // add
					ForceREG((&Variables[Result]), regEax);
				}
				else {
					Error("unsupported type");
				}
				break;
			}
			case(0x73):// Sub
			{
				int varableA = ReadByte();
				int varableB = ReadByte();
				int Result = ReadByte();
				int vartypeA = Variables[varableA].type;
				int vartypeB = Variables[varableB].type;
				if (vartypeA == vartypeB && isNnumber(vartypeA))
				{
					Instruction(0x2B, &Variables[varableA], &Variables[varableB]); // sub
					ForceREG((&Variables[Result]), regEax);
				}
				else {
					Error("unsupported type");
				}
				break;
			}
			case(0x74):// TODO Negate
			case(0x75):// TODO Remainder
				Error("notdone"); break;
			case(0x76):// Inc
			{
				int varableA = ReadByte();
				int vartype = Variables[varableA].type;
				if (isNnumber(vartype))
				{
					int reg = Variables[varableA].RegisterStorage;
					if (reg == 0) {
						if (Variables[varableA].init) {
							int ofset = memoryOfset(Variables[varableA].StackStorage);
							if (ofset < 128) {
								CC(0xFF, 0x44, 0x24, (char)ofset); // inc DWORD PTR[esp + 4]
							}
							else {
								CC(0xFF, 0x84, 0x24, Split4(ofset)); // inc DWORD PTR[esp + 1000]
							}
						}
						else {
							Error("uninitalised varable");
						}
					}
					else if (reg == regEax) {
						CC(0x40); //  inc eax 
					}
					else {
						Error("unsupported register");
					}
				}
				break;
			}
			case(0x77):// Dec
			{
				int varableA = ReadByte();
				int vartype = Variables[varableA].type;
				if (isNnumber(vartype ))
				{
					int reg = Variables[varableA].RegisterStorage;
					if (reg == 0) {
						if (Variables[varableA].init) {
							int ofset = memoryOfset(Variables[varableA].StackStorage);
							if (ofset < 128) {
								CC(0xFF, 0x4C, 0x24, (char)ofset); // dec DWORD PTR[esp + 4]
							}
							else {
								CC(0xFF, 0x8C, 0x24, Split4(ofset)); // dec DWORD PTR[esp + 1000]
							}
						}
						else {
							Error("uninitalised varable");
						}
					}
					else if (reg == regEax) {
						CC(0x48); //  inc eax 
					}
					else {
						Error("unsupported register");
					}
				}
				break;
			}
			case(0x78):// Add Constant
			{
				int varable = ReadByte();
				int vartype = Variables[varable].type;
				if (isNnumber(vartype))
				{
					unsigned int number = ReadInt();
					int Register = Variables[varable].RegisterStorage;
					 if (Register == 0 && Variables[varable].init) {//varable is in memory location
						int ofset = memoryOfset(Variables[varable].StackStorage);
						if (ofset < 128) {
							if (number < 128) {
								CC(0x83, 0x44, 0x24, (char)ofset, (char)number); //add DWORD PTR[esp + 4], 4
							}
							else {
								CC(0x81, 0x84, 0x24, (char)ofset, Split4(number));// add DWORD PTR[esp + 1026], 1024
							}
						}
						else {
							if (number < 128) {
								CC(0x83, 0x84, 0x24, Split4(ofset), (char)number); //add DWORD PTR[esp + 1026],4
							}
							else {
								CC(0x81, 0x84, 0x24, Split4(ofset), Split4(number)); //add DWORD PTR[esp + 1026],1024
							}
						}
					}
					else if (Register == regEax) { //varable is in Register
							CC(05, Split4(number)); //add eax,1025
					}
					else {
						Error("invalid register");
					}
				}
				else {
					Error("invalid type");
				}
				break;
			}
			case(0x79):// Sub Constant
			{
				int varable = ReadByte();
				int vartype = Variables[varable].type;
				if (isNnumber(vartype))
				{
					unsigned int number = ReadInt();
					int Register = Variables[varable].RegisterStorage;
					if (Register == 0 && Variables[varable].init) {//varable is in memory location
						int ofset = memoryOfset(Variables[varable].StackStorage);
						if (ofset < 128) {
							if (number < 128) {
								CC(0x83, 0x6C, 0x24, (char)ofset, (char)number); //add DWORD PTR[esp + 4], 4
							}
							else {
								CC(0x81, 0xAC, 0x24, (char)ofset, Split4(number));// add DWORD PTR[esp + 4], 1024
							}
						}
						else {
							if (number < 128) {
								CC(0x83, 0x84, 0x24, Split4(ofset), (char)number); //add DWORD PTR[esp + 1026],4
							}
							else {
								CC(0x81, 0x84, 0x24, Split4(ofset), Split4(number)); //add DWORD PTR[esp + 1026],1024
							}
						}
					}
					else if (Register == regEax) { //varable is in Register
						CC(05, Split4(number)); //add eax,1025
					}
					else {
						Error("invalid register");
					}
				}
				else {
					Error("invalid type");
				}
				break;
			}
				
	//___________________________0x8X bitwise
			case(0x80): //OR
			case(0x81): //Rshift
			case(0x82): //LShift
			case(0x83): //Xor
			case(0x84): //and
				Error("notdone"); break;
			case(0x86): //not
			{
				int varable = ReadByte();
				Varable* var = &Variables[varable];
				if (isNnumber(var->type) ){
					InstructionSingle(0xf7, 2, var);
				}
				else {
					Error("unfinihsed");
				}
			}
			break;
	//___________________________0x9X comparision

			case(0x91): // RefEqual

				Error("notdone"); break;

			case(0x92): // Equal		SETE 
			{
				int varableA = ReadByte();
				int varableB = ReadByte();
				int Result = ReadByte();
				Varable* varA = &Variables[varableA];
				Varable* varB = &Variables[varableB];
				int vartypeA = varA->type;
				int vartypeB = varB->type;
				if (vartypeA == vartypeB && (isNnumber(vartypeA) || isObject(vartypeA)))
				{
					MoveREG(varA, regEBx);
					CC(0x31, 0xC0);// xor eax,eax
					Instruction(0x3b, varA, varB);
					CC(0x0F, 0x94, 0xC0);//sete   al
					Varable* result =  &Variables[Result];
					ForceREG(result, regEax);
					SetFlags(result,FlagZero);
				}
				else {
					Error("unsupported type");
				}
				break;
			}
			case(0x93): // NotEqual		SETNE 
		//	case(0x94): // Compare     
			case(0x95): // GtThan
			case(0x96): // LsThan      
			case(0x97): //IsNull
			case(0x98): // NotNull
			case(0x99): // Zero			SETZ
			case(0x9a): // nZero        SETNZ    
			case(0x9b): // GEThan		SETAE 
			case(0x9c): // LEThan		SETBE 
				Error("notdone"); break;
			default:
				Error("illigal opcode"); break;
		}
		if (clearFlags) {
			Flagswanted = 0;
			Flagvar = 0;
		}
		else {
			clearFlags = true;
		}
		AssertClear();
	}
	AssertClear();
	UnloadScope();
	if (ScopeTop != 0) {
		Error("end of scope expected");
		return false;
	}
	CC(0xC3, 0x90);
	return true;
}


#pragma region Read




static inline unsigned char PeakByte() {
	return program[iP];
}

static inline unsigned char ReadByte() {
	return program[iP++];
}

static inline short ReadShort() {
	short FirstPart = program[iP++] ;
	return (program[iP++] & 0xFF) << 8 | FirstPart;
}

static inline unsigned short ReadUShort() {
	unsigned short FirstPart = program[iP++];
	return ((unsigned short)program[iP++] & 0xFF) << 8 | FirstPart;
}

static inline int ReadInt() {
	int output = ((int)program[iP++] & 0xFF);
	output |= (((int)program[iP++] & 0xFF) << 8);
	output |= (((int)program[iP++] & 0xFF) << 16);
	output |= (((int)program[iP++] & 0xFF) << 24);
	return output;
}

static inline unsigned int ReadUInt() {
	unsigned int output = ((unsigned int)program[iP++] & 0xFF);
	output |= (((unsigned int)program[iP++] & 0xFF) << 8);
	output |= (((unsigned int)program[iP++] & 0xFF) << 16);
	output |= (((unsigned int)program[iP++] & 0xFF) << 24);
	return output;
}


static char* ReadString()
{
	char* result;
	Byte length = ReadByte() + 1;
	result = (char*)(iP + (int)program);
	iP += length;
	return result;
}


#pragma endregion



static bool ForceREG(Varable* source, Register Dest) {
	if (Registers[Dest] != 0) {
		Varable*  regid = &Variables[Registers[Dest] - 1];
		regid->RegisterStorage = 0;
	}
	Registers[Dest] = source->id + 1;
	source->RegisterStorage = Dest;
	return true;
}


static void SendHome(Register Dest) {
	if (Registers[Dest] != 0) {
		Varable* var = &Variables[Registers[Dest] - 1];
		if (!var->init) {

			Warn("varable not in stack");
			PushArgument(var);
			var->init = true;
			var->StackStorage = StackBias;
		
		}
		else {
			CheckInit(var);
			int clearofset = memoryOfset(var->StackStorage);
			CC(0x89);
			CC(ModRM((clearofset > 127) ? 2 : 1, Dest, regEsp));
			CC(SIB(0, regEsp, regEsp));
			CCNumber(clearofset);
		}
		if (Registers[Dest] != 0) {
			Varable*  regid = &Variables[Registers[Dest] - 1];
			regid->RegisterStorage = 0;
		}
		Registers[Dest] = 0;
		
	}
}

static bool MoveREG(Varable* source, Register Dest) {
	int Register = source->RegisterStorage;
	if (!source->init) {
		return false;
	}
	
	if (Register == Dest) {
		return true;
	}


	if (Registers[Dest] != 0) {
		SendHome(Dest);
	}
	ForceREG(source, Dest);

	int ofset = memoryOfset(source->StackStorage);
	CC(0x8B);
	CC(ModRM((ofset > 127) ? 2 : 1, Dest, regEsp));
	CC(SIB(0, regEsp, regEsp));
	CCNumber(ofset);
}





static void Move(Varable* source, Varable* Dest) {
	int sourceType = source->type;
	CheckInit(source);
	if (!Dest->init) {
		SetVarable(Dest, 0);
	}
	
	if (source->type == Dest->type) {
	
			if (sourceType == CallTableNull) {

			}else
			if (isObject(sourceType) || isNnumber(sourceType)) {
				//these ones are standard ints; just normal move; 
				//TODO make it work
				int destreg = Dest->RegisterStorage;
				if (destreg == 0) {

				}
				else {
					MoveREG(source, destreg);
				}
			}
			else if (isFloat(sourceType)) {

			}
			else if (isUnmoveable(sourceType)){
				Error("attempted to move the moveable");
				return;
			}
			else {
				Error("unsupported move type");
			}
				
	}
	else {
		//TODO Typecast cast
	}

}



#pragma region WorkPlace
void SetPackage(PackageProgram* package) {
	Package = package;
}


void CopyOutput(char* result) {
	memcpy(result, WorkPlace, iOut);

#ifdef NOTKERNAL

	for (int i = 0; i < ResultLength(); i++) {
		cout << setfill('0') << setw(2) << hex << (unsigned int)(unsigned char)((char*)WorkPlace)[i];
		cout << " ";
	}
	cout << "Done! \n";
	cout.unsetf(ios::hex);
#endif
}

int ResultLength() {
	return iOut;
}

void SetCompilerWorkingMemory(char* ptr, size_t sise) {
	WorkPlace = (Byte*)ptr;
	MaxWorkPlaceSise = sise;
}
#pragma endregion