#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *marker;
static void complete(void)
{
    FILE *stream = fopen(marker, "wb");
    if (stream) { fputs("run completed", stream); fclose(stream); }
}
int main(int argc, char **argv)
{
    if (argc != 3 || strcmp(argv[2], "argument with spaces")) return 1;
    if (!getenv("PATH")) return 2;
    marker = argv[1];
    return atexit(complete);
}
