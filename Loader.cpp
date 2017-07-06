#include "stdafx.h"
#include "ByteCode.h"
#include "Loader.h"
#include "Memory.h"
#include "Debug.h"
#include "ClassFinder.h"
static char Stringbuffer[256];
static char* buffer;
static int length;
static int iP;
Byte Compressionmode;
static const unsigned int magicNumber = 0x1EAB11CA;

static Byte inline ReadByte() {
	if (iP < length) {
		return (Byte)buffer[iP++];
	}
	return 0;

}

static unsigned int ReadUInt()
{
	 unsigned int result = 0;
	if (iP + 3 > length) {
		return false;
	}

	result |= (unsigned int)((unsigned char)ReadByte() & 0xFF);
	result |= (unsigned int)(((unsigned char)ReadByte() & 0xFF) << 8);
	result |= (unsigned int)(((unsigned char)ReadByte() & 0xFF) << 16);
	result |= (unsigned int)(((unsigned char)ReadByte() & 0xFF) << 24);
	return result;
}

static bool CheckSection(const char* section) {
	if (Compressionmode == 0) {
		if (iP + 3 > length) {
			return false;
		}
		return section[0] == buffer[iP++] && section[1] == buffer[iP++] && section[2] == buffer[iP++] && section[3] == buffer[iP++];
	}
	else {
		return section[0] == ReadByte();
	}
}


static char* ReadString()
{
	char* result;
	Byte length = ReadByte() + 1;
	result = (char*)(iP + (int)buffer);
	iP += length;
	return result;
}


void FreeProgram(PackageProgram* program)
{
	Note("freeing program");
	if (program->classes != NULL) {
		for (size_t i = 0; i < program->classesCount; i++)
		{
			FreeObject(program->classes[i]);
		}
	}
	if (program->redirectionTable != NULL) {
		for (size_t i = 0; i < program->redirectionTableCount; i++)
		{
			Free(program->redirectionTable[i].name);
		}
	}
	Free(program->startpoint);
	Free(program->redirectionTable);
	Free(program->classes);
	Free(program->name);
}

static void ClearupProgramLoader(PackageProgram* program)
 {
	 FreeProgram(program);
 }


static Obj* LoadProgramObject(int length) {
	Note("Loading object");
	int i = 0;

	const int baselength = (sizeof(int) * 3 + sizeof(Byte) * 4);
	if (length < baselength) {
		Error("invalid length");
		return NULL;
	}
	

	int Objlength = ReadUInt(); //todo GEt accuarate length
	Obj* result = (Obj*)Allocate(Objlength);

	if (result == NULL) {
		Error("failed to allocate memory for class");
		return NULL;
	}
	ReadUInt(); // space for program referance
	result->length = Objlength;

	result->Referances = 0; ReadByte();
	result->Baseclass = 0; ReadUInt();
	result->Flags = ReadByte();
	result->CallMin = ReadByte();
	result->CallMax = ReadByte();
	if (result->CallMin > result->CallMax) {
		Error("invalid Call table");
		return NULL;
	}
	if (result->length >= length) {
		Error("invalid length ");
		return NULL;
	}
	//load callTable
	int numItems = (result->CallMax - result->CallMin);
	CallTableElement* calltable = (CallTableElement*)&(result->data);
	for (size_t i = 0; i < numItems; i++)
	{
		int location = ReadUInt();
		int itemtype = ReadUInt();
		calltable[i].ofset= location;
		calltable[i].type = itemtype;
	}

	char* data = (char*)&(result->data) + sizeof(CallTableElement) * numItems;
	

	unsigned int ofset =0;
	for (size_t i = 0; i < numItems; i++)
	{
	unsigned	int itemlen = ReadUInt() -4 ;
		if (itemlen > Objlength) {
			Error("invalid item length ");
			return NULL;
		}
		calltable[i].ofset = ofset;
		if (calltable[i].type == CallTableBytecode) {
			//__declspec(align(4)) struct BytecodeProgram {
			//	unsigned int Compiled;
			//	unsigned int ReturnType;
			//	unsigned int length;
			//	char data;
			//};
			BytecodeProgram* bcp = (BytecodeProgram* )(data + ofset);
			bcp->Compiled = 0; ReadUInt(); //Compiled
			bcp->ReturnType = ReadUInt(); //ReturnType
			bcp->ParamCount =  ReadUInt(); // ParamCount
			unsigned int codelength = ReadUInt() - 4;;
			ofset += sizeof(int) * 4;
			bcp->length = codelength;
			for (size_t copyx = 0; copyx < codelength; copyx++)
			{
				((Byte*)&bcp->data)[copyx] = ReadByte();
				ofset++;
			}
		}
		else {
			for (size_t copyx = 0; copyx < itemlen; copyx++)
			{
				data[ofset++] = ReadByte();

			}
		}


	}


	return result;
}





static PackageProgram* LoadProgramFileInternal() {
	
	   Note("Loading program file");
		PackageProgram* ouput = (PackageProgram*)Allocate(sizeof(PackageProgram));
		ouput->name = NULL;
		ouput->redirectionTable = NULL;
		ouput->classesCount = 0;
		ouput->classes = NULL;
		ouput->redirectionTableCount = 0;

		int i = 0;
		int magic = ReadUInt();
		if (magic != magicNumber) {
			Error("invalid magic number");
			ClearupProgramLoader(ouput);
			return NULL;
		}

		Byte filevertion = ReadUInt(); // file vertion
		 Compressionmode = ReadByte(); //0 means uncompressed
	
		 Note("reading header");


		ReadUInt(); // reserved
		if (!CheckSection("HEDR")) {
			Error("header expected");
			ClearupProgramLoader(ouput);
			return NULL;
		}
		char* name = ReadString();
		ouput->name = (char*)Allocate(strlen(name));
		if (ouput->name == NULL) {
			Error("unable to allocate space for name");
			ClearupProgramLoader(ouput);
			return NULL;
		}
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

		strcpy(ouput->name, name);

#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
		int redirectTable = ReadUInt();
		unsigned int classcount = ReadUInt();
		unsigned int totalclasslength = ReadUInt();

		ouput->classesCount = classcount;
		ouput->classes = (Obj**)Allocate(sizeof(Obj*) * classcount);
		if (ouput->classes == NULL) {
			Error("unable to allocate space for classes");
			ClearupProgramLoader(ouput);
			return NULL;
		}
		for (size_t i = 0; i < classcount; i++)
		{
			ouput->classes[i] = NULL;
		}

		name = ReadString();
		ouput->startpoint = (char*)Allocate(strlen(name));
		if (ouput->name == NULL) {
			Error("unable to allocate space for startpoint");
			ClearupProgramLoader(ouput);
			return NULL;
		}
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

		strcpy(ouput->startpoint, name);

#ifdef _MSC_VER
#pragma warning(pop)
#endif
		
		ouput->startnumber = ReadUInt();
		

		int usedLenght = 0;
		if (!CheckSection("PROG")) {
			Error("PROG expected");
			ClearupProgramLoader(ouput);
			return NULL;
		}
		for (size_t i = 0; i < classcount; i++)
		{
			if (iP > length) {
				Error("out of space");
				ClearupProgramLoader(ouput);
				return NULL;
			}
			char* ClassName = ReadString();
#ifndef _KERNAL
			std::cout << "------Loading Class "  << ouput->name << ClassName << " ... \n";
#endif
			Obj* objclass = LoadProgramObject(totalclasslength - usedLenght);
			if (objclass == NULL) {
				Error("failed to load class");
				ClearupProgramLoader(ouput);
				return NULL;
			}
			ouput->classes[i] = objclass;
		}
		
		if (!CheckSection("DATA")) {
			Error("DATA expected");
			ClearupProgramLoader(ouput);
			return NULL;
		}

		unsigned int datacount = ReadUInt();
		for (size_t i = 0; i < datacount; i++)
		{
			if (iP > length) {
				Error("out of space");
				ClearupProgramLoader(ouput);
				return NULL;
			}
		}

		Note("reading Redirection");

		if (!CheckSection("REDI")) {
			Error("REDI expected");
			ClearupProgramLoader(ouput);
			return NULL;
		}
		unsigned int redirectcount = ReadUInt();

		ouput->redirectionTableCount = redirectcount;
		ouput->redirectionTable = (RedirectionTableItem*)Allocate(sizeof(RedirectionTableItem) *redirectcount);
		if (ouput->redirectionTable == NULL) {
			Error("unable to allocate redirection");
			ClearupProgramLoader(ouput);
			return NULL;
		}
		for (size_t i = 0; i < classcount; i++)
		{
			ouput->redirectionTable->name = NULL;
		}



		for (size_t i = 0; i < redirectcount; i++)
		{
			if (iP > length) {
				Error("out of space");
				ClearupProgramLoader(ouput);
				return NULL;
			}

			char* redname = ReadString();
			RedirectionTableItem* Item = &(ouput->redirectionTable[i]);
			Item->name = (char*)Allocate(strlen(redname));
			Item->ptr = NULL;
			if (Item->name == NULL) {
				Error("unable to allocate name");
				ClearupProgramLoader(ouput);
				return NULL;
			}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

			strcpy(Item->name, redname);

#ifdef _MSC_VER
#pragma warning(pop)
#endif


			
			Item->id = ReadUInt();

			char charnum = ReadByte();
			if (charnum == -1) {
				Item->ptr = ResolveClass(Item->name);
			}else if ( charnum <  ouput->classesCount) {
					Item->ptr = ouput->classes[charnum];
				}
		}

		if (filevertion > 2) {
			if (!CheckSection("ENDP")) {
				Error("ENDP expected");
				ClearupProgramLoader(ouput);
				return NULL;
			}

		}
		else {

			magic = ReadUInt();
			if (magic != magicNumber) {
				Error("invalid magic number");
				ClearupProgramLoader(ouput);
				return NULL;
			}
		}
		
		
		return ouput;
		//
		//ouput->redirectionTable
		//ouput->classes[0]->data
	
}






#ifdef NOTKERNAL
PackageProgram* LoadProgramFile(const char* path) {
	std::ifstream is("D:\\ouput.erf", std::ifstream::binary);
	if (is) {
		// get length of file:
		is.seekg(0, is.end);
		length = is.tellg();
		is.seekg(0, is.beg);

		buffer = (char*)Allocate(length);
		if (buffer == NULL) {
			Error("unable to allocate buffer");
			return NULL;
		}
		// read data as a block:


		is.read(buffer, length);
		Fullcheck();
		if (is) {
			Note("all characters read successfully.");
		}
		else {
			Error("unable to read file");
			return false;
		}
		is.close();
		PackageProgram* result = LoadProgramFileInternal();
		Free(buffer);
		return result;
	}
	else {
		return NULL;
	}
}
#endif



PackageProgram* LoadProgramFile(char* data, size_t Count) {
	buffer = data;
	length = Count;
	PackageProgram* result = LoadProgramFileInternal();
	return result;
}