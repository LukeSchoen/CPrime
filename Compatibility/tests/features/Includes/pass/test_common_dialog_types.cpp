// EXPECT_COMPILE_ARGS: -lcomdlg32
#define UNICODE
#include <commdlg.h>
#include <stddef.h>
int main(){
  OPENFILENAME a={0};a.lStructSize=sizeof(a);a.lpstrFilter=L"All\0*.*\0";
  a.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR;
  if(sizeof(OPENFILENAMEA)!=sizeof(OPENFILENAMEW))return 1;
  if(sizeof(void*)==8 && (sizeof(a)!=152||offsetof(OPENFILENAMEW,lpstrFile)!=48))return 2;
  WINBOOL(WINAPI*open)(LPOPENFILENAMEW)=GetOpenFileName;
  return open==0;
}
