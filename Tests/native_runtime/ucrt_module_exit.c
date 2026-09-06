#include <stdio.h>
#include <stdlib.h>
extern void native_register_exit(void (*callback)(void));
extern _Bool __scrt_initialize_onexit_tables(int);
static const char *marker;
static void record(char value)
{
    FILE *stream = fopen(marker, "ab");
    if (!stream) abort();
    fputc(value, stream);
    fclose(stream);
}
static void first(void) { record('C'); }
static int second(void) { record('O'); return 0; }
static void third(void) { record('N'); }
static void quick(void) { record('Q'); }
int main(int argc, char **argv)
{
    if (argc != 3) return 1;
    marker = argv[1];
    /* Native thread-static initialization repeats this DLL fallback even
       inside an EXE. It must preserve the ownership selected by startup. */
    if (!__scrt_initialize_onexit_tables(0)) return 2;
    if (atexit(first) || !_onexit(second)) return 3;
    native_register_exit(third);
    if (at_quick_exit(quick)) return 4;
    if (argv[2][0] == 'q') quick_exit(0);
    return 0;
}
