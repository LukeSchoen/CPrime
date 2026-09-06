#include <stdio.h>
#include <stdlib.h>

FILE *native_open(const char *path) { return fopen(path, "w+b"); }
FILE *native_stdout(void) { return stdout; }
int native_write(FILE *stream) { return fprintf(stream, "native %d %.2f\n", 42, 3.25); }
int native_read(FILE *stream)
{
    int value = 0;
    double number = 0;
    return fscanf(stream, "cpc %d %lf", &value, &number) == 2
        && value == 73 && number == 6.5;
}
int native_close(FILE *stream) { return fclose(stream); }
void native_free(void *allocation) { free(allocation); }
