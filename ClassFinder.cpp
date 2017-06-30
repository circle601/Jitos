#include "stdafx.h"
#include "ClassFinder.h"
#include "Hashmap.h"

map_t classMap;

void StartClassStore() {
	classMap = hashmap_new();
}



