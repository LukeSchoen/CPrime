// EXPECT_COMPILE_ARGS: -Werror
#include <shlwapi.h>
#include <shobjidl.h>
int main() {
  FILETYPEATTRIBUTEFLAGS flags = FTA_Show | FTA_NoEdit;
  flags &= ~FTA_NoEdit;
  flags ^= FTA_NoRemove;
  return sizeof(ITEMIDLIST)!=3 || sizeof(STRRET)!=272
    || flags!=(FTA_Show|FTA_NoRemove);
}
