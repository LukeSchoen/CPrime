The complete `include/winapi/imm.h` and `include/_mingw_unicode.h` headers are
from the public-domain MinGW-w64 v12.0.0 release, retained without modification.

- https://github.com/mingw-w64/mingw-w64/blob/v12.0.0/mingw-w64-headers/include/imm.h
- https://github.com/mingw-w64/mingw-w64/blob/v12.0.0/mingw-w64-headers/crt/_mingw_unicode.h

Their original public-domain notices are retained in both files. These fill the
omitted Windows IME interface so `_mingw.h` no longer needs to force `NOIME`.
