#pragma once




void Note(const char* errorcode);
void Error(const char* errorcode);
void Warn(const char* errorcode);
void Ping();

#ifdef _KERNAL

extern void PrintError(const char* errorcode);
#define Assert(x,y) ;
#define Assert(x) ;
#else
void PrintError(const char* errorcode);
#define Assert(x,y) if(x) { Error(y); };
#define Assert(x) if(!x){ Error("x"); };
#endif // _DEBUG
