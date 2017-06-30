
#pragma once

#ifdef _DEBUG
#include <iostream>
#include <string>
#include <cassert>
void Runtests();
#else
#define assert(condition) 
#endif