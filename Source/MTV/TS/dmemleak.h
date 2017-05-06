#ifndef _DMEM_LEAK_H
#define _DMEM_LEAK_H

#include "voYYDef_TS.h"
#include <malloc.h>

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(WIN32)
#   include <tchar.h>
#elif defined(LINUX)
#   include "vocrstypes.h"
#endif

#ifdef DMEMLEAK

void __stdcall SetMemoryLogFile(const _TCHAR* filename);
void __stdcall DumpMemoryLeak(const char* title);

void __stdcall AddMemoryTrack(void* addr, unsigned int asize, const char *fname, unsigned int lnum);
void __stdcall RemoveMemoryTrack(void* addr);

inline void * __cdecl operator new(unsigned int size, 
								   const char *file, int line)
{
	void *ptr = (void *)malloc(size);
	if (ptr) //!!!
		AddMemoryTrack(ptr, size, file, line);
	return(ptr);
};

inline void __cdecl operator delete(void *p)
{
	if (p)
	{
		RemoveMemoryTrack(p);
		free(p);
	}
};

#else //DMEMLEAK

#define SetMemoryLogFile
#define DumpMemoryLeak

#endif //DMEMLEAK


#ifndef DEBUG_NEW
#ifdef DMEMLEAK
#define DEBUG_NEW new(__FILE__, __LINE__)
#else //DMEMLEAK
#define DEBUG_NEW new
#endif //DMEMLEAK
#define new DEBUG_NEW
#endif //DEBUG_NEW

#endif //._DMEM_LEAK_H