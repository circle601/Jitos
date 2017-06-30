#include "Copy.h"


#ifdef _DEBUG

#else

extern void* memcpy(void* destination, const void* source, size_t num);

#endif
