#include "stdafx.h"

#include "Hashmap.h"
#include "bytecode.h"
#include "ClassFinder.h"

map_t classMap;

void StartClassStore() {
	classMap = hashmap_new();
}

void CleanUpClassStore() {
	hashmap_free(classMap);
}


void AddJitClass(const char* name, Obj* object) {
	if (name == NULL || object == NULL) return;
	hashmap_put(classMap, name, object);
}

Obj* ResolveClass(const char* name) {
	if (name == NULL) return NULL;
	Obj* result;
	if (hashmap_get(classMap, name, (void**)&result) == MAP_OK) {
		return result;
	}
	return NULL;
}