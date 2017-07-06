#include "stdafx.h"
#include "Memory.h"
#include "Util.h"
#include "Debug.h" 
char* memory;

const int QueueSise = 255;
const int Overhead = 2 * sizeof(int);

const int _base_block_size = 16;


struct MemoryLevel
{
	int freeCount;
	size_t Blocksize;
	void* freeHeader[QueueSise]; //cycillic queue
	int Head;
	int Tail;
};


size_t MemorySise = 0;
int Levels = 0;
MemoryLevel* Freelists;





bool ready = false;




bool AddBlock(int Level, void* ptr) {
	void** blocks = Freelists[Level].freeHeader;
	int head = Freelists[Level].Head;
	int tail = Freelists[Level].Tail;

	if (head < tail && (head + 1) == tail) {
		Error("queue overflow");
		return false;
	}
	if (head >= QueueSise) {
		head = 0;
		if (tail == 0) {
			Error("queue overflow");
			return false;
		}
	}
	blocks[head] = ptr;
	head = head + 1;
	Freelists[Level].Head = head;
	Freelists[Level].freeCount++;
	return true;
}

void* popBlock(int Level) {
	void** blocks = Freelists[Level].freeHeader;
	int head = Freelists[Level].Head;
	int tail = Freelists[Level].Tail;
	if (tail == head) {
		Error("queue underflow");
		return (void*)-1;
	}
	if (tail >= QueueSise) {
		tail = 0;
		if (head == 0) {
			Error("queue underflow");
			return (void*)-1;
		}
	}
	void* result = blocks[tail];
	tail = tail + 1;
	Freelists[Level].freeCount--;
	Freelists[Level].Tail = tail;
	return result;
}

bool RequestMemory(int min) {
	return false;
}

void AddBorders(void* block, int level) {
	int blocksise = Freelists[level].Blocksize;
	*((int*)(char*)block) = blocksise;
	*((int*)((char*)block + blocksise - sizeof(int))) = blocksise;
}

bool inline Check(void* block, int level) {
#ifndef _KERNAL
	if (block == NULL) return true;
	int blocksise = Freelists[level].Blocksize;
	if (*((int*)(char*)block) != blocksise) {
		Error("invalid header");
			return false;
	};
	if (*((int*)((char*)block + blocksise - sizeof(int))) != blocksise) {
		Error("invalid footer");
		return false;
	}
#endif // DEBUG
	return true;
}


void Fullcheck() {
#ifndef _KERNAL
	int total = 0;
	int faluures = 0;
	for (size_t level = 0; level < Levels; level++)
	{
		for (size_t i = Freelists[level].Tail; i < Freelists[level].Head; i++)
		{
			total++;
			if (!Check(Freelists[level].freeHeader[i], level)) {
				faluures++;
			
			}
		}
	}
	if (faluures > 0) {
		Error("failed full check");
		std::cout << "failed full check: " << faluures << "/" << total << "\n";
	}
#endif // DEBUG
}



bool ProcessMemory(void* block,size_t size) {
	if (size < (32 + _base_block_size)) {
		Error("too small");
		return false;
	}
	size_t avalableSise = size ;
	if (block == NULL || (unsigned int)block == -1) return false;
	unsigned int ptr = (((unsigned int)block + 15) & ~(unsigned int)0x0F);
	if ((ptr - 8) < (unsigned int)block) {
		ptr += 16;
		avalableSise -= 16;
	}
	
	int blocks = (avalableSise - (_base_block_size / 2)) / _base_block_size; //todo i think this wastes memory
	avalableSise = blocks * _base_block_size;
	if (avalableSise > size) {
		Error("memory caculation error");
		return false;
	}


	int newMaxLevels = Ilog2(avalableSise / _base_block_size);
	// Resise Table
	if (newMaxLevels > Levels) {
		//todo increse array sise instead of replaceing it
		if (Levels == 0 || Freelists == NULL) {
			int NeededSpace = newMaxLevels * sizeof(MemoryLevel);
			
			if (NeededSpace > size) {
				Error("insufficent space for levels");
			}
			Freelists = (MemoryLevel*)(ptr);
			ptr += NeededSpace;
		    ptr = (((unsigned int)ptr + 15) & ~(unsigned int)0x0F);
			avalableSise -= NeededSpace;
		}
		else {
			//todo Copydata from old levels
			Error("notdone");
		}

		if (avalableSise < (32 + _base_block_size)) {
			Error("too small");
			return false;
		}

	//boundrys
	ptr = ptr -  4;
	((int*)ptr)[-1] = -1;
	*(int*)((unsigned int)ptr + avalableSise) = -1;
	




		if (Freelists == NULL) return false;
		for (size_t i = Levels; i < newMaxLevels; i++)
		{
			Freelists[i].Blocksize = _base_block_size << i;
			Freelists[i].Head = 0;
			Freelists[i].Tail = 0;
			Freelists[i].freeCount = 0;
		}
		Levels = newMaxLevels;
	}
	//Fill table
	int MemoryFree = avalableSise;
	char* currentptr = (char*)ptr;
	for (int lv = Levels - 1; lv >= 0; lv--)
	{
		if (MemoryFree <= 0) break;
		int blocksise = Freelists[lv].Blocksize;
		if (blocksise > MemoryFree) continue;
		int Blocks = MemoryFree / blocksise;
		MemoryFree -= Blocks * blocksise;
		if (avalableSise < 0) {
			Error("error allocating blocks");
		}
		for (size_t i = 0; i < Blocks; i++)
		{
			*((int*)currentptr) = blocksise;
			*((int*)(currentptr + blocksise - sizeof(int))) = blocksise ;
			AddBlock(lv, currentptr);
			currentptr += blocksise;
			
		}
	}
	Fullcheck();
	return true;
}


bool StartAllocator(void* Memory,size_t memorySise) {
	
	MemorySise = memorySise;
	memory = (char*)Memory;
	if (memory == NULL) return false;
	if (!ProcessMemory(memory, MemorySise)) {
		Error("Failed to allocate memory");
	}
	Note("Allocator Started \n");
	ready = true;
	return true;
}


static void* findBlock(size_t Size) {
	int Level = Ilog2((Size + _base_block_size - 1) / _base_block_size);

	int Uselevel = -1;
	for (size_t i = Level; i < Levels; i++)
	{
		if (Freelists[i].freeCount > 0 && Freelists[i].Blocksize >= Size) {
			Uselevel = i;
			break;
		}
	}
	if (Uselevel == -1) {
		return NULL;
	}
	else if (Uselevel == Level) {
		return popBlock(Uselevel);
	}
	else {
		void* result;
		int NewLevel = Uselevel;
		int lastSise = Freelists[Uselevel].Blocksize;
		result = popBlock(Uselevel);
		if (result == NULL) return NULL;
#ifdef NOTKERNAL
		if (!Check(result, Uselevel)) {
			return NULL;
		}
#endif // DEBUG

		NewLevel = Uselevel;
		while (NewLevel > Level) {
			if (NewLevel == 0) {
				Error("NewLevel == 0");
				return NULL;
			}
			NewLevel = NewLevel - 1;
			
			int newblocksise = Freelists[NewLevel].Blocksize;
			char* block = (char*)result;

			AddBorders(block, NewLevel);
			block += newblocksise;
		

		#ifndef _KERNAL
			int newblocks = (lastSise / newblocksise) - 1;
			lastSise = newblocksise;
			Assert(newblocks > 1, "too many blocks");
			Assert((lastSise / newblocksise) == 2, "not mutiple of 2");
		#endif // _DEBUG

				AddBorders(block, NewLevel);
			#ifndef _KERNAL
				if (!Check(block, NewLevel)) {
					return NULL;
				}
			#endif // DEBUG
				AddBlock(NewLevel, block);
				
			
		}

		return result;
	}
	return NULL;
}

void StopAllocator(){
	ready = false;
}

void* ClearAllocate(int requested_bytes) {
	void* result = Allocate(requested_bytes);
	memset(result, 0, requested_bytes);
	return result;
}


void* Allocate(int requested_bytes) {	
	
	size_t bytes_needed = requested_bytes + Overhead;
	

	//(n + align_to - 1) & ~ (align_to - 1) // round to nearest alignment
	if (bytes_needed < 0) {
		Error("too big");
		return (void* )-1;
	}
	void* block = findBlock(bytes_needed);
#ifdef NOTKERNAL
	//size_t s = (requested_bytes + Overhead + _base_block_size - 1) / _base_block_size;
	int Level = Ilog2((bytes_needed + _base_block_size - 1) / _base_block_size);
	if (!Check(block, Level)) {
		return NULL;
	}
#endif // DEBUG

	
	if (block == NULL || block == (void*)-1) {
		return NULL;
	}
	unsigned int header =  bytes_needed;
	
	block = (void*)((int*)block + 1);


	return block;


}





Obj* AllocateObject(Obj* baseClass) {
	if (baseClass == NULL) return NULL;
	
	int startElement = baseClass->CallMin;
	int endElement = baseClass->CallMax;
	bool foundVarable = false;
	for (size_t i = startElement; i < endElement; i++)
	{
		CallTableElement* item = GetCallElementPtr(baseClass, i);
		int type = item->type;
		if (type == CallTableNull || type == CallTableProgram || type == CallTableSystemReserved || type == CallTableBytecode)
		{

		}
		else {
				foundVarable = true;
				startElement = i;
				break;
		}
	}
	int maxfeilds = endElement - startElement;
	if (!foundVarable) {
		maxfeilds = 0;
		endElement = 0;
		startElement = 0;
	}
	
	size_t size = sizeof(Obj) + sizeof(CallTableElement) * maxfeilds;
	for (size_t i = startElement; i < endElement; i++)
	{
		CallTableElement* item = GetCallElementPtr(baseClass, i);
		int type = item->type;
		if (type == CallTableNull || type == CallTableProgram || type == CallTableSystemReserved || type == CallTableBytecode)
		{
			size += 0;
		}
		else if (type == CallTableArray || type == CallTableDouble ) {
			size += sizeof(int) * 2;
		}
		else if (type == CallTableString) {
			Error("string unfinised");
			return NULL;
		}
		else if (type > 255) {
			size += sizeof(int);
		}
		else {
			size += sizeof(int);
		}
	}
	Obj* result = (Obj*)ClearAllocate(size);
	if (result == NULL) return NULL;
	result->length = size;
	result->Baseclass = baseClass;
	result->Referances = 0;
	result->CallMax = endElement;
	result->CallMin = startElement;
	int ofset = 0;
	for (size_t i = startElement; i < endElement; i++)
	{
		CallTableElement* item = GetCallElementPtr(baseClass, i);
		CallTableElement* Resultcall = GetCallElementPtr(result, i);
		int type = item->type;
		Resultcall->type = type;
		Resultcall->ofset = ofset;
		if (type == CallTableNull || type == CallTableProgram || type == CallTableSystemReserved || type == CallTableBytecode)
		{
			Resultcall->type = CallTableNull;
			Resultcall->ofset = 0;
		}
		else if (type == CallTableArray || type == CallTableDouble) {
			ofset += sizeof(int) * 2;
		}
		else if (type == CallTableString) {
			Error("string unfinised");
			return NULL;	
		}
		else if (type > 255) {
			ofset += sizeof(int);
		}
		else {

		}
	}
	return result;
}


Obj* AllocatePointerObject(size_t count) {
	int sise = sizeof(Obj) + (sizeof(CallTableElement) + sizeof(void*)) * count;
	Obj* ptr = (Obj*)Allocate(sise)  ;
	if (ptr == NULL) return NULL;
	ptr->length = sise;
	ptr->Baseclass = 0;
	ptr->Referances = 0;
	ptr->CallMax = count;
	ptr->CallMin = 0;
	for (size_t i = 0; i < count; i++)
	{
		CallTableElement* element = GetCallElementPtr(ptr, i);
		element->type = CallTableProgram;
		element->ofset = i * sizeof(void*);
	}
	return ptr;
}

Obj* AllocateObject(size_t sise) {
	Obj* ptr = (Obj*)Allocate(sise);
	if (ptr == NULL) return NULL;
	ptr->length = sise;
	ptr->Baseclass = 0;
	ptr->Referances = 0;
	ptr->CallMax = 0;
	ptr->CallMin = 0;
	return ptr;
}

bool FreeObject( Obj* ptr) {
	if (ptr == NULL) return true;
	size_t sise = ptr->length;
	return Free( ptr ,sise);
}


bool Free(void* ptr) {
	if (ptr == NULL || ptr == (void*)-1) return false;
	char* Realstart = (char*)ptr - sizeof(int);
	int* ints = (int*)ptr;
	size_t BlockSise = ints[-1];
	Free(ptr, BlockSise);
}

bool Free(void* ptr, size_t sise) {


	
	if (ptr == NULL	|| ptr == (void*)-1) return false;
	char* Realstart = (char*)ptr - sizeof(int);
	


	int* ints = (int*)ptr;
	size_t BlockSise = ints[-1];
	if (BlockSise == ClearedHeader) {
		Error("cleared header");
		return false;
	}
	int* upperints = (int*)(Realstart + BlockSise);

	if (upperints[-1] != ints[-1]) {
		Error("corrupted memory");
		return false;
	}
	int Level = Ilog2((BlockSise + _base_block_size - 1) / _base_block_size);
#ifdef NOTKERNAL
	
	Check(Realstart, Level);
#endif // DEBUG
	



	int Lowerblock = ints[-2];
	int upperblock = upperints[0];
	if (Lowerblock != -1 && Lowerblock == BlockSise && false) {
		Error("unfinised lower coelessse");
		//todo
	}
	else if (upperblock != -1 && upperblock == BlockSise  && false) {
		//todo
		int newLevel = Level + 1;
		int newblocksise = Freelists[newLevel].Blocksize;
		upperints[-1] = ClearedHeader;
		upperints[0] = ClearedHeader;
		char* newstart =  Realstart ;
		*((int*)newstart) = newblocksise;
		*((int*)(newstart + newblocksise - sizeof(int))) = newblocksise ;
		AddBlock(newLevel, newstart);
	}

	if (!AddBlock(Level, Realstart)) {
		Error("unable to free memory");
		return false;
	}

	return true;
}

