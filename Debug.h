#pragma once

#define Naked   __declspec( naked )  


void Note(const char* errorcode);
void Error(const char* errorcode);
void Warn(const char* errorcode);


#ifndef _KERNAL

#define Assert(x,y) if(x) { Error(y); };
#else
#define Assert(x,y) ;
#endif // _DEBUG
