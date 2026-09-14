// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
// windows.h and shlobj.h both include <wincrypt.h>, so the packaged SDK has to
// provide it.  LIBZPAQ.cpp includes it for CryptGenRandom; this exercises the
// classic CryptoAPI surface the header now declares.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32")

int main()
{
  HCRYPTPROV provider = 0;
  BYTE random[16] = { 0 };
  if (!CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return 1;
  BOOL generated = CryptGenRandom(provider, sizeof(random), random);
  CryptReleaseContext(provider, 0);
  if (!generated) return 2;
  for (unsigned i = 0; i < sizeof(random); ++i)
    if (random[i]) return 0;
  return 3;
}
