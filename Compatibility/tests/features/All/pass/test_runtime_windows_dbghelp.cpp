#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "Dbghelp.lib")

int main()
{
  void* frames[4] = {};
  USHORT frameCount = CaptureStackBackTrace(0, 4, frames, 0);
  if (!frameCount || !frames[0])
    return 1;

  HANDLE process = GetCurrentProcess();
  if (!SymInitialize(process, 0, FALSE))
    return 2;

  char storage[sizeof(SYMBOL_INFO) + 128] = {};
  SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = 127;
  DWORD64 displacement = 0;
  SymFromAddr(process, reinterpret_cast<DWORD64>(frames[0]),
              &displacement, symbol);

  return SymCleanup(process) ? 0 : 3;
}
