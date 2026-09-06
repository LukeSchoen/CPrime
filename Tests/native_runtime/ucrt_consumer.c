/* Include wchar first to exercise the shared FILE declaration and streams. */
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>

#ifndef __CPRIME_UCRT__
#error This integration test requires the UCRT compiler runtime.
#endif

FILE *native_open(const char *);
FILE *native_stdout(void);
int native_write(FILE *);
int native_read(FILE *);
int native_close(FILE *);
void native_free(void *);

static const char *exit_path;
static void at_exit(void)
{
    FILE *stream = fopen(exit_path, "wb");
    if (stream) { fputs("finished", stream); fclose(stream); }
}

int main(int argc, char **argv)
{
    FILE *stream;
    char buffer[64], tiny[4], word[12];
    wchar_t wide[32], wide_word[12];
    int value;
    double number;
    _locale_t locale;
    long timezone_value;
    int fmode;
    if (argc != 4 || strcmp(argv[3], "argument with spaces")) return 1;
    if (!getenv("PATH") || sizeof(FILE) != sizeof(void *)) return 2;
    if (MB_CUR_MAX < 1 || !isalpha('A') || isalpha('3')) return 15;
    locale = _create_locale(LC_ALL, "C");
    if (!locale || !_isalpha_l('Z', locale) || _isalpha_l('7', locale)) return 16;
    _free_locale(locale);
    _tzset();
    if (_get_timezone(&timezone_value) || timezone_value != _timezone || !_tzname[0]) return 17;
    if (_get_fmode(&fmode) || fmode != _fmode || _sys_nerr < 1 || !_sys_errlist[0]) return 18;
    if (stdout != native_stdout()) return 3;
    stream = native_open(argv[1]);
    if (!stream || native_write(stream) != 15 || _ftelli64(stream) != 15) return 4;
    rewind(stream);
    if (fscanf(stream, "%11s %d %lf", word, &value, &number) != 3
        || strcmp(word, "native") || value != 42 || number != 3.25) return 5;
    if (fclose(stream)) return 6;
    stream = fopen(argv[1], "w+b");
    if (!stream || fprintf(stream, "cpc %d %.1f\n", 73, 6.5) != 11) return 7;
    rewind(stream);
    if (!native_read(stream) || native_close(stream)) return 8;
    if (snprintf(tiny, sizeof tiny, "%s", "abcdef") != 6
        || strcmp(tiny, "abc") || snprintf(NULL, 0, "%d", 12345) != 5) return 9;
    if (sprintf_s(buffer, sizeof buffer, "%s %lld %.1f", "secure", 12345678901LL, 2.5) != 22
        || strcmp(buffer, "secure 12345678901 2.5")) return 10;
    if (sscanf(buffer, "%11s %d", word, &value) != 2 || strcmp(word, "secure")) return 11;
    if (swprintf(wide, 32, L"%ls %d", L"wide", 42) != 7 || wcscmp(wide, L"wide 42")) return 12;
    if (swscanf(wide, L"%11ls %d", wide_word, &value) != 2
        || wcscmp(wide_word, L"wide") || value != 42) return 13;
    native_free(malloc(32));
    exit_path = argv[2];
    if (atexit(at_exit)) return 14;
    return 0;
}
