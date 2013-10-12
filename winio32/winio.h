#ifndef WINIO_H
#define WINIO_H

#include "winio_nt.h"

typedef struct tagPhysStruct tagPhysStruct_t;

 BOOL  InitializeWinIo();
 void  ShutdownWinIo();
 PBYTE  MapPhysToLin(tagPhysStruct_t *PhysStruct);
 BOOL  UnmapPhysicalMemory(tagPhysStruct_t *PhysStruct);
 BOOL  GetPhysLong(PBYTE pbPhysAddr, PDWORD pdwPhysVal);
 BOOL  SetPhysLong(PBYTE pbPhysAddr, DWORD dwPhysVal);
 BOOL  GetPortVal(WORD wPortAddr, PDWORD pdwPortVal, BYTE bSize);
 BOOL  SetPortVal(WORD wPortAddr, DWORD dwPortVal, BYTE bSize);
 BOOL  InstallWinIoDriver(PWSTR pszWinIoDriverPath, BOOL IsDemandLoaded);
 BOOL  RemoveWinIoDriver();

extern HANDLE hDriver;
extern BOOL IsWinIoInitialized;
extern BOOL g_Is64BitOS;

#endif
