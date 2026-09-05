#ifndef _CPC_DBGHELP_H
#define _CPC_DBGHELP_H

#include <stdio.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SYMBOL_INFO
{
  ULONG SizeOfStruct;
  ULONG TypeIndex;
  ULONG64 Reserved[2];
  ULONG Index;
  ULONG Size;
  ULONG64 ModBase;
  ULONG Flags;
  ULONG64 Value;
  ULONG64 Address;
  ULONG Register;
  ULONG Scope;
  ULONG Tag;
  ULONG NameLen;
  ULONG MaxNameLen;
  CHAR Name[1];
} SYMBOL_INFO, *PSYMBOL_INFO;

DECLSPEC_IMPORT BOOL WINAPI SymInitialize(HANDLE process,
                                          PCSTR userSearchPath,
                                          BOOL invadeProcess);
DECLSPEC_IMPORT BOOL WINAPI SymCleanup(HANDLE process);
DECLSPEC_IMPORT BOOL WINAPI SymFromAddr(HANDLE process,
                                        DWORD64 address,
                                        PDWORD64 displacement,
                                        PSYMBOL_INFO symbol);

#ifdef __cplusplus
}
#endif

#endif
