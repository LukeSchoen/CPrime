// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment(lib, "imm32")
int main() {
  HIMC (WINAPI *associate)(HWND, HIMC) = &ImmAssociateContext;
  COMPOSITIONFORM composition = {};
  CANDIDATEFORM candidate = {};
  if (!associate || sizeof(composition) != 28 || sizeof(candidate) != 32) return 1;
  HIMC context = ImmCreateContext();
  if (!context) return 2;
  return ImmDestroyContext(context) ? 0 : 3;
}
