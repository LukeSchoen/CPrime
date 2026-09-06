/* Formatted I/O for the UCRT target.  The DLL exports the common engines,
   while the public C entry points belong to the compiler runtime.  FILE
   objects, locales and va_list values are passed through the UCRT ABI. */
#ifdef __CPRIME_UCRT__
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

/* UCRT options from corecrt_stdio_config.h. */
#define CPC_PRINTF_LEGACY_TERMINATION 1ULL
#define CPC_PRINTF_STANDARD_SNPRINTF 2ULL
#define CPC_SCANF_SECURE 1ULL

#define CPC_VARARG(name, signature, last, call) \
    int __cdecl name signature { \
        va_list args; int result; va_start(args, last); \
        result = call; va_end(args); return result; \
    }

static int cpc_printf_result(int value) { return value < 0 ? -1 : value; }
int __cdecl __stdio_common_vfprintf(unsigned long long, FILE *, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vsprintf(unsigned long long, char *, size_t, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vfprintf_s(unsigned long long, FILE *, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vsprintf_s(unsigned long long, char *, size_t, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vfprintf_p(unsigned long long, FILE *, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vsprintf_p(unsigned long long, char *, size_t, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vsnprintf_s(unsigned long long, char *, size_t, size_t, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vfscanf(unsigned long long, FILE *, const char *, _locale_t, va_list);
int __cdecl __stdio_common_vsscanf(unsigned long long, const char *, size_t, const char *, _locale_t, va_list);
int __cdecl _vfprintf_l(FILE *stream, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vfprintf(0, stream, format, locale, args); }
int __cdecl vfprintf(FILE *stream, const char *format, va_list args)
{ return _vfprintf_l(stream, format, NULL, args); }
CPC_VARARG(_fprintf_l, (FILE *stream, const char *format, _locale_t locale, ...), locale, _vfprintf_l(stream, format, locale, args))
CPC_VARARG(fprintf, (FILE *stream, const char *format, ...), format, vfprintf(stream, format, args))
int __cdecl _vprintf_l(const char *format, _locale_t locale, va_list args)
{ return _vfprintf_l(stdout, format, locale, args); }
int __cdecl vprintf(const char *format, va_list args)
{ return _vfprintf_l(stdout, format, NULL, args); }
CPC_VARARG(_printf_l, (const char *format, _locale_t locale, ...), locale, _vfprintf_l(stdout, format, locale, args))
CPC_VARARG(printf, (const char *format, ...), format, vprintf(format, args))
int __cdecl _vfprintf_s_l(FILE *stream, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vfprintf_s(0, stream, format, locale, args); }
int __cdecl vfprintf_s(FILE *stream, const char *format, va_list args)
{ return _vfprintf_s_l(stream, format, NULL, args); }
CPC_VARARG(_fprintf_s_l, (FILE *stream, const char *format, _locale_t locale, ...), locale, _vfprintf_s_l(stream, format, locale, args))
CPC_VARARG(fprintf_s, (FILE *stream, const char *format, ...), format, vfprintf_s(stream, format, args))
int __cdecl _vprintf_s_l(const char *format, _locale_t locale, va_list args)
{ return _vfprintf_s_l(stdout, format, locale, args); }
int __cdecl vprintf_s(const char *format, va_list args)
{ return _vfprintf_s_l(stdout, format, NULL, args); }
CPC_VARARG(_printf_s_l, (const char *format, _locale_t locale, ...), locale, _vfprintf_s_l(stdout, format, locale, args))
CPC_VARARG(printf_s, (const char *format, ...), format, vprintf_s(format, args))
int __cdecl _vfprintf_p_l(FILE *stream, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vfprintf_p(0, stream, format, locale, args); }
int __cdecl _vfprintf_p(FILE *stream, const char *format, va_list args)
{ return _vfprintf_p_l(stream, format, NULL, args); }
CPC_VARARG(_fprintf_p_l, (FILE *stream, const char *format, _locale_t locale, ...), locale, _vfprintf_p_l(stream, format, locale, args))
CPC_VARARG(_fprintf_p, (FILE *stream, const char *format, ...), format, _vfprintf_p(stream, format, args))
int __cdecl _vprintf_p_l(const char *format, _locale_t locale, va_list args)
{ return _vfprintf_p_l(stdout, format, locale, args); }
int __cdecl _vprintf_p(const char *format, va_list args)
{ return _vfprintf_p_l(stdout, format, NULL, args); }
CPC_VARARG(_printf_p_l, (const char *format, _locale_t locale, ...), locale, _vfprintf_p_l(stdout, format, locale, args))
CPC_VARARG(_printf_p, (const char *format, ...), format, _vprintf_p(format, args))
int __cdecl _vsnprintf(char *buffer, size_t count, const char *format, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf(CPC_PRINTF_LEGACY_TERMINATION, buffer, count, format, NULL, args)); }
CPC_VARARG(_snprintf, (char *buffer, size_t count, const char *format, ...), format, _vsnprintf(buffer, count, format, args))
int __cdecl _vsnprintf_l(char *buffer, size_t count, const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf(CPC_PRINTF_LEGACY_TERMINATION, buffer, count, format, locale, args)); }
CPC_VARARG(_snprintf_l, (char *buffer, size_t count, const char *format, _locale_t locale, ...), locale, _vsnprintf_l(buffer, count, format, locale, args))
int __cdecl vsnprintf(char *buffer, size_t count, const char *format, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf(CPC_PRINTF_STANDARD_SNPRINTF, buffer, count, format, NULL, args)); }
CPC_VARARG(snprintf, (char *buffer, size_t count, const char *format, ...), format, vsnprintf(buffer, count, format, args))
int __cdecl _vsnprintf_c(char *buffer, size_t count, const char *format, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf(0, buffer, count, format, NULL, args)); }
CPC_VARARG(_snprintf_c, (char *buffer, size_t count, const char *format, ...), format, _vsnprintf_c(buffer, count, format, args))
int __cdecl _vsnprintf_c_l(char *buffer, size_t count, const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf(0, buffer, count, format, locale, args)); }
CPC_VARARG(_snprintf_c_l, (char *buffer, size_t count, const char *format, _locale_t locale, ...), locale, _vsnprintf_c_l(buffer, count, format, locale, args))
int __cdecl vsprintf_s(char *buffer, size_t count, const char *format, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf_s(0, buffer, count, format, NULL, args)); }
CPC_VARARG(sprintf_s, (char *buffer, size_t count, const char *format, ...), format, vsprintf_s(buffer, count, format, args))
int __cdecl _vsprintf_s_l(char *buffer, size_t count, const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf_s(0, buffer, count, format, locale, args)); }
CPC_VARARG(_sprintf_s_l, (char *buffer, size_t count, const char *format, _locale_t locale, ...), locale, _vsprintf_s_l(buffer, count, format, locale, args))
int __cdecl _vsprintf_p(char *buffer, size_t count, const char *format, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf_p(0, buffer, count, format, NULL, args)); }
CPC_VARARG(_sprintf_p, (char *buffer, size_t count, const char *format, ...), format, _vsprintf_p(buffer, count, format, args))
int __cdecl _vsprintf_p_l(char *buffer, size_t count, const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf_p(0, buffer, count, format, locale, args)); }
CPC_VARARG(_sprintf_p_l, (char *buffer, size_t count, const char *format, _locale_t locale, ...), locale, _vsprintf_p_l(buffer, count, format, locale, args))
int __cdecl _vsprintf_l(char *buffer, const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf(CPC_PRINTF_LEGACY_TERMINATION, buffer, (size_t)-1, format, locale, args)); }
int __cdecl vsprintf(char *buffer, const char *format, va_list args)
{ return _vsprintf_l(buffer, format, NULL, args); }
CPC_VARARG(sprintf, (char *buffer, const char *format, ...), format, vsprintf(buffer, format, args))
CPC_VARARG(_sprintf_l, (char *buffer, const char *format, _locale_t locale, ...), locale, _vsprintf_l(buffer, format, locale, args))
int __cdecl _vscprintf_l(const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf(CPC_PRINTF_STANDARD_SNPRINTF, NULL, 0, format, locale, args)); }
int __cdecl _vscprintf(const char *format, va_list args)
{ return _vscprintf_l(format, NULL, args); }
CPC_VARARG(_scprintf, (const char *format, ...), format, _vscprintf(format, args))
CPC_VARARG(_scprintf_l, (const char *format, _locale_t locale, ...), locale, _vscprintf_l(format, locale, args))
int __cdecl _vscprintf_p_l(const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsprintf_p(CPC_PRINTF_STANDARD_SNPRINTF, NULL, 0, format, locale, args)); }
int __cdecl _vscprintf_p(const char *format, va_list args)
{ return _vscprintf_p_l(format, NULL, args); }
CPC_VARARG(_scprintf_p, (const char *format, ...), format, _vscprintf_p(format, args))
CPC_VARARG(_scprintf_p_l, (const char *format, _locale_t locale, ...), locale, _vscprintf_p_l(format, locale, args))
int __cdecl _vsnprintf_s_l(char *buffer, size_t count, size_t maximum, const char *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsnprintf_s(0, buffer, count, maximum, format, locale, args)); }
int __cdecl _vsnprintf_s(char *buffer, size_t count, size_t maximum, const char *format, va_list args)
{ return _vsnprintf_s_l(buffer, count, maximum, format, NULL, args); }
CPC_VARARG(_snprintf_s, (char *buffer, size_t count, size_t maximum, const char *format, ...), format, _vsnprintf_s(buffer, count, maximum, format, args))
CPC_VARARG(_snprintf_s_l, (char *buffer, size_t count, size_t maximum, const char *format, _locale_t locale, ...), locale, _vsnprintf_s_l(buffer, count, maximum, format, locale, args))
int __cdecl vsnprintf_s(char *buffer, size_t count, size_t maximum, const char *format, va_list args)
{ return _vsnprintf_s(buffer, count, maximum, format, args); }
int __cdecl _vfscanf_l(FILE *stream, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vfscanf(0, stream, format, locale, args); }
int __cdecl vfscanf(FILE *stream, const char *format, va_list args)
{ return _vfscanf_l(stream, format, NULL, args); }
CPC_VARARG(fscanf, (FILE *stream, const char *format, ...), format, vfscanf(stream, format, args))
CPC_VARARG(_fscanf_l, (FILE *stream, const char *format, _locale_t locale, ...), locale, _vfscanf_l(stream, format, locale, args))
int __cdecl _vscanf_l(const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vfscanf(0, stdin, format, locale, args); }
int __cdecl vscanf(const char *format, va_list args)
{ return _vscanf_l(format, NULL, args); }
CPC_VARARG(scanf, (const char *format, ...), format, vscanf(format, args))
CPC_VARARG(_scanf_l, (const char *format, _locale_t locale, ...), locale, _vscanf_l(format, locale, args))
int __cdecl _vsscanf_l(const char *input, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vsscanf(0, input, (size_t)-1, format, locale, args); }
int __cdecl vsscanf(const char *input, const char *format, va_list args)
{ return _vsscanf_l(input, format, NULL, args); }
CPC_VARARG(sscanf, (const char *input, const char *format, ...), format, vsscanf(input, format, args))
CPC_VARARG(_sscanf_l, (const char *input, const char *format, _locale_t locale, ...), locale, _vsscanf_l(input, format, locale, args))
int __cdecl _vsnscanf_l(const char *input, size_t count, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vsscanf(0, input, count, format, locale, args); }
int __cdecl _vsnscanf(const char *input, size_t count, const char *format, va_list args)
{ return _vsnscanf_l(input, count, format, NULL, args); }
CPC_VARARG(_snscanf, (const char *input, size_t count, const char *format, ...), format, _vsnscanf(input, count, format, args))
CPC_VARARG(_snscanf_l, (const char *input, size_t count, const char *format, _locale_t locale, ...), locale, _vsnscanf_l(input, count, format, locale, args))
int __cdecl _vfscanf_s_l(FILE *stream, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vfscanf(CPC_SCANF_SECURE, stream, format, locale, args); }
int __cdecl vfscanf_s(FILE *stream, const char *format, va_list args)
{ return _vfscanf_s_l(stream, format, NULL, args); }
CPC_VARARG(fscanf_s, (FILE *stream, const char *format, ...), format, vfscanf_s(stream, format, args))
CPC_VARARG(_fscanf_s_l, (FILE *stream, const char *format, _locale_t locale, ...), locale, _vfscanf_s_l(stream, format, locale, args))
int __cdecl _vscanf_s_l(const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vfscanf(CPC_SCANF_SECURE, stdin, format, locale, args); }
int __cdecl vscanf_s(const char *format, va_list args)
{ return _vscanf_s_l(format, NULL, args); }
CPC_VARARG(scanf_s, (const char *format, ...), format, vscanf_s(format, args))
CPC_VARARG(_scanf_s_l, (const char *format, _locale_t locale, ...), locale, _vscanf_s_l(format, locale, args))
int __cdecl _vsscanf_s_l(const char *input, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vsscanf(CPC_SCANF_SECURE, input, (size_t)-1, format, locale, args); }
int __cdecl vsscanf_s(const char *input, const char *format, va_list args)
{ return _vsscanf_s_l(input, format, NULL, args); }
CPC_VARARG(sscanf_s, (const char *input, const char *format, ...), format, vsscanf_s(input, format, args))
CPC_VARARG(_sscanf_s_l, (const char *input, const char *format, _locale_t locale, ...), locale, _vsscanf_s_l(input, format, locale, args))
int __cdecl _vsnscanf_s_l(const char *input, size_t count, const char *format, _locale_t locale, va_list args)
{ return __stdio_common_vsscanf(CPC_SCANF_SECURE, input, count, format, locale, args); }
int __cdecl _vsnscanf_s(const char *input, size_t count, const char *format, va_list args)
{ return _vsnscanf_s_l(input, count, format, NULL, args); }
CPC_VARARG(_snscanf_s, (const char *input, size_t count, const char *format, ...), format, _vsnscanf_s(input, count, format, args))
CPC_VARARG(_snscanf_s_l, (const char *input, size_t count, const char *format, _locale_t locale, ...), locale, _vsnscanf_s_l(input, count, format, locale, args))
int __cdecl __stdio_common_vfwprintf(unsigned long long, FILE *, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vswprintf(unsigned long long, wchar_t *, size_t, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vfwprintf_s(unsigned long long, FILE *, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vswprintf_s(unsigned long long, wchar_t *, size_t, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vfwprintf_p(unsigned long long, FILE *, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vswprintf_p(unsigned long long, wchar_t *, size_t, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vsnwprintf_s(unsigned long long, wchar_t *, size_t, size_t, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vfwscanf(unsigned long long, FILE *, const wchar_t *, _locale_t, va_list);
int __cdecl __stdio_common_vswscanf(unsigned long long, const wchar_t *, size_t, const wchar_t *, _locale_t, va_list);
int __cdecl _vfwprintf_l(FILE *stream, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vfwprintf(0, stream, format, locale, args); }
int __cdecl vfwprintf(FILE *stream, const wchar_t *format, va_list args)
{ return _vfwprintf_l(stream, format, NULL, args); }
CPC_VARARG(_fwprintf_l, (FILE *stream, const wchar_t *format, _locale_t locale, ...), locale, _vfwprintf_l(stream, format, locale, args))
CPC_VARARG(fwprintf, (FILE *stream, const wchar_t *format, ...), format, vfwprintf(stream, format, args))
int __cdecl _vwprintf_l(const wchar_t *format, _locale_t locale, va_list args)
{ return _vfwprintf_l(stdout, format, locale, args); }
int __cdecl vwprintf(const wchar_t *format, va_list args)
{ return _vfwprintf_l(stdout, format, NULL, args); }
CPC_VARARG(_wprintf_l, (const wchar_t *format, _locale_t locale, ...), locale, _vfwprintf_l(stdout, format, locale, args))
CPC_VARARG(wprintf, (const wchar_t *format, ...), format, vwprintf(format, args))
int __cdecl _vfwprintf_s_l(FILE *stream, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vfwprintf_s(0, stream, format, locale, args); }
int __cdecl vfwprintf_s(FILE *stream, const wchar_t *format, va_list args)
{ return _vfwprintf_s_l(stream, format, NULL, args); }
CPC_VARARG(_fwprintf_s_l, (FILE *stream, const wchar_t *format, _locale_t locale, ...), locale, _vfwprintf_s_l(stream, format, locale, args))
CPC_VARARG(fwprintf_s, (FILE *stream, const wchar_t *format, ...), format, vfwprintf_s(stream, format, args))
int __cdecl _vwprintf_s_l(const wchar_t *format, _locale_t locale, va_list args)
{ return _vfwprintf_s_l(stdout, format, locale, args); }
int __cdecl vwprintf_s(const wchar_t *format, va_list args)
{ return _vfwprintf_s_l(stdout, format, NULL, args); }
CPC_VARARG(_wprintf_s_l, (const wchar_t *format, _locale_t locale, ...), locale, _vfwprintf_s_l(stdout, format, locale, args))
CPC_VARARG(wprintf_s, (const wchar_t *format, ...), format, vwprintf_s(format, args))
int __cdecl _vfwprintf_p_l(FILE *stream, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vfwprintf_p(0, stream, format, locale, args); }
int __cdecl _vfwprintf_p(FILE *stream, const wchar_t *format, va_list args)
{ return _vfwprintf_p_l(stream, format, NULL, args); }
CPC_VARARG(_fwprintf_p_l, (FILE *stream, const wchar_t *format, _locale_t locale, ...), locale, _vfwprintf_p_l(stream, format, locale, args))
CPC_VARARG(_fwprintf_p, (FILE *stream, const wchar_t *format, ...), format, _vfwprintf_p(stream, format, args))
int __cdecl _vwprintf_p_l(const wchar_t *format, _locale_t locale, va_list args)
{ return _vfwprintf_p_l(stdout, format, locale, args); }
int __cdecl _vwprintf_p(const wchar_t *format, va_list args)
{ return _vfwprintf_p_l(stdout, format, NULL, args); }
CPC_VARARG(_wprintf_p_l, (const wchar_t *format, _locale_t locale, ...), locale, _vfwprintf_p_l(stdout, format, locale, args))
CPC_VARARG(_wprintf_p, (const wchar_t *format, ...), format, _vwprintf_p(format, args))
int __cdecl _vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf(CPC_PRINTF_LEGACY_TERMINATION, buffer, count, format, NULL, args)); }
CPC_VARARG(_snwprintf, (wchar_t *buffer, size_t count, const wchar_t *format, ...), format, _vsnwprintf(buffer, count, format, args))
int __cdecl _vsnwprintf_l(wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf(CPC_PRINTF_LEGACY_TERMINATION, buffer, count, format, locale, args)); }
CPC_VARARG(_snwprintf_l, (wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, ...), locale, _vsnwprintf_l(buffer, count, format, locale, args))
int __cdecl vswprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf(0, buffer, count, format, NULL, args)); }
CPC_VARARG(swprintf, (wchar_t *buffer, size_t count, const wchar_t *format, ...), format, vswprintf(buffer, count, format, args))
int __cdecl _vswprintf_c(wchar_t *buffer, size_t count, const wchar_t *format, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf(0, buffer, count, format, NULL, args)); }
CPC_VARARG(_swprintf_c, (wchar_t *buffer, size_t count, const wchar_t *format, ...), format, _vswprintf_c(buffer, count, format, args))
int __cdecl _vswprintf_c_l(wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf(0, buffer, count, format, locale, args)); }
CPC_VARARG(_swprintf_c_l, (wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, ...), locale, _vswprintf_c_l(buffer, count, format, locale, args))
int __cdecl vswprintf_s(wchar_t *buffer, size_t count, const wchar_t *format, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf_s(0, buffer, count, format, NULL, args)); }
CPC_VARARG(swprintf_s, (wchar_t *buffer, size_t count, const wchar_t *format, ...), format, vswprintf_s(buffer, count, format, args))
int __cdecl _vswprintf_s_l(wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf_s(0, buffer, count, format, locale, args)); }
CPC_VARARG(_swprintf_s_l, (wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, ...), locale, _vswprintf_s_l(buffer, count, format, locale, args))
int __cdecl _vswprintf_p(wchar_t *buffer, size_t count, const wchar_t *format, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf_p(0, buffer, count, format, NULL, args)); }
CPC_VARARG(_swprintf_p, (wchar_t *buffer, size_t count, const wchar_t *format, ...), format, _vswprintf_p(buffer, count, format, args))
int __cdecl _vswprintf_p_l(wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf_p(0, buffer, count, format, locale, args)); }
CPC_VARARG(_swprintf_p_l, (wchar_t *buffer, size_t count, const wchar_t *format, _locale_t locale, ...), locale, _vswprintf_p_l(buffer, count, format, locale, args))
int __cdecl __vswprintf_l(wchar_t *buffer, const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf(CPC_PRINTF_LEGACY_TERMINATION, buffer, (size_t)-1, format, locale, args)); }
int __cdecl _vswprintf(wchar_t *buffer, const wchar_t *format, va_list args)
{ return __vswprintf_l(buffer, format, NULL, args); }
CPC_VARARG(_swprintf, (wchar_t *buffer, const wchar_t *format, ...), format, _vswprintf(buffer, format, args))
CPC_VARARG(__swprintf_l, (wchar_t *buffer, const wchar_t *format, _locale_t locale, ...), locale, __vswprintf_l(buffer, format, locale, args))
int __cdecl _vscwprintf_l(const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf(CPC_PRINTF_STANDARD_SNPRINTF, NULL, 0, format, locale, args)); }
int __cdecl _vscwprintf(const wchar_t *format, va_list args)
{ return _vscwprintf_l(format, NULL, args); }
CPC_VARARG(_scwprintf, (const wchar_t *format, ...), format, _vscwprintf(format, args))
CPC_VARARG(_scwprintf_l, (const wchar_t *format, _locale_t locale, ...), locale, _vscwprintf_l(format, locale, args))
int __cdecl _vscwprintf_p_l(const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vswprintf_p(CPC_PRINTF_STANDARD_SNPRINTF, NULL, 0, format, locale, args)); }
int __cdecl _vscwprintf_p(const wchar_t *format, va_list args)
{ return _vscwprintf_p_l(format, NULL, args); }
CPC_VARARG(_scwprintf_p, (const wchar_t *format, ...), format, _vscwprintf_p(format, args))
CPC_VARARG(_scwprintf_p_l, (const wchar_t *format, _locale_t locale, ...), locale, _vscwprintf_p_l(format, locale, args))
int __cdecl _vsnwprintf_s_l(wchar_t *buffer, size_t count, size_t maximum, const wchar_t *format, _locale_t locale, va_list args)
{ return cpc_printf_result(__stdio_common_vsnwprintf_s(0, buffer, count, maximum, format, locale, args)); }
int __cdecl _vsnwprintf_s(wchar_t *buffer, size_t count, size_t maximum, const wchar_t *format, va_list args)
{ return _vsnwprintf_s_l(buffer, count, maximum, format, NULL, args); }
CPC_VARARG(_snwprintf_s, (wchar_t *buffer, size_t count, size_t maximum, const wchar_t *format, ...), format, _vsnwprintf_s(buffer, count, maximum, format, args))
CPC_VARARG(_snwprintf_s_l, (wchar_t *buffer, size_t count, size_t maximum, const wchar_t *format, _locale_t locale, ...), locale, _vsnwprintf_s_l(buffer, count, maximum, format, locale, args))
int __cdecl _vfwscanf_l(FILE *stream, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vfwscanf(0, stream, format, locale, args); }
int __cdecl vfwscanf(FILE *stream, const wchar_t *format, va_list args)
{ return _vfwscanf_l(stream, format, NULL, args); }
CPC_VARARG(fwscanf, (FILE *stream, const wchar_t *format, ...), format, vfwscanf(stream, format, args))
CPC_VARARG(_fwscanf_l, (FILE *stream, const wchar_t *format, _locale_t locale, ...), locale, _vfwscanf_l(stream, format, locale, args))
int __cdecl _vwscanf_l(const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vfwscanf(0, stdin, format, locale, args); }
int __cdecl vwscanf(const wchar_t *format, va_list args)
{ return _vwscanf_l(format, NULL, args); }
CPC_VARARG(wscanf, (const wchar_t *format, ...), format, vwscanf(format, args))
CPC_VARARG(_wscanf_l, (const wchar_t *format, _locale_t locale, ...), locale, _vwscanf_l(format, locale, args))
int __cdecl _vswscanf_l(const wchar_t *input, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vswscanf(0, input, (size_t)-1, format, locale, args); }
int __cdecl vswscanf(const wchar_t *input, const wchar_t *format, va_list args)
{ return _vswscanf_l(input, format, NULL, args); }
CPC_VARARG(swscanf, (const wchar_t *input, const wchar_t *format, ...), format, vswscanf(input, format, args))
CPC_VARARG(_swscanf_l, (const wchar_t *input, const wchar_t *format, _locale_t locale, ...), locale, _vswscanf_l(input, format, locale, args))
int __cdecl _vsnwscanf_l(const wchar_t *input, size_t count, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vswscanf(0, input, count, format, locale, args); }
int __cdecl _vsnwscanf(const wchar_t *input, size_t count, const wchar_t *format, va_list args)
{ return _vsnwscanf_l(input, count, format, NULL, args); }
CPC_VARARG(_snwscanf, (const wchar_t *input, size_t count, const wchar_t *format, ...), format, _vsnwscanf(input, count, format, args))
CPC_VARARG(_snwscanf_l, (const wchar_t *input, size_t count, const wchar_t *format, _locale_t locale, ...), locale, _vsnwscanf_l(input, count, format, locale, args))
int __cdecl _vfwscanf_s_l(FILE *stream, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vfwscanf(CPC_SCANF_SECURE, stream, format, locale, args); }
int __cdecl vfwscanf_s(FILE *stream, const wchar_t *format, va_list args)
{ return _vfwscanf_s_l(stream, format, NULL, args); }
CPC_VARARG(fwscanf_s, (FILE *stream, const wchar_t *format, ...), format, vfwscanf_s(stream, format, args))
CPC_VARARG(_fwscanf_s_l, (FILE *stream, const wchar_t *format, _locale_t locale, ...), locale, _vfwscanf_s_l(stream, format, locale, args))
int __cdecl _vwscanf_s_l(const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vfwscanf(CPC_SCANF_SECURE, stdin, format, locale, args); }
int __cdecl vwscanf_s(const wchar_t *format, va_list args)
{ return _vwscanf_s_l(format, NULL, args); }
CPC_VARARG(wscanf_s, (const wchar_t *format, ...), format, vwscanf_s(format, args))
CPC_VARARG(_wscanf_s_l, (const wchar_t *format, _locale_t locale, ...), locale, _vwscanf_s_l(format, locale, args))
int __cdecl _vswscanf_s_l(const wchar_t *input, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vswscanf(CPC_SCANF_SECURE, input, (size_t)-1, format, locale, args); }
int __cdecl vswscanf_s(const wchar_t *input, const wchar_t *format, va_list args)
{ return _vswscanf_s_l(input, format, NULL, args); }
CPC_VARARG(swscanf_s, (const wchar_t *input, const wchar_t *format, ...), format, vswscanf_s(input, format, args))
CPC_VARARG(_swscanf_s_l, (const wchar_t *input, const wchar_t *format, _locale_t locale, ...), locale, _vswscanf_s_l(input, format, locale, args))
int __cdecl _vsnwscanf_s_l(const wchar_t *input, size_t count, const wchar_t *format, _locale_t locale, va_list args)
{ return __stdio_common_vswscanf(CPC_SCANF_SECURE, input, count, format, locale, args); }
int __cdecl _vsnwscanf_s(const wchar_t *input, size_t count, const wchar_t *format, va_list args)
{ return _vsnwscanf_s_l(input, count, format, NULL, args); }
CPC_VARARG(_snwscanf_s, (const wchar_t *input, size_t count, const wchar_t *format, ...), format, _vsnwscanf_s(input, count, format, args))
CPC_VARARG(_snwscanf_s_l, (const wchar_t *input, size_t count, const wchar_t *format, _locale_t locale, ...), locale, _vsnwscanf_s_l(input, count, format, locale, args))

#undef CPC_VARARG
#endif

