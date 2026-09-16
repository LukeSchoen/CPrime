// EXPECT_COMPILE_ONLY: 1
// EXPECT_COMPILE_ARGS: -Werror -DUNICODE
#include <shlobj.h>
#include <objbase.h>
HRESULT resolve(IShellLink* link,IPersistFile* file,wchar_t* path){
 HRESULT result=file->Load(path,STGM_READ);
 if(SUCCEEDED(result)) result=link->GetPath(path,260,0,0);
 file->Release();link->Release();return result;
}
