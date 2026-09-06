#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
static void completed(void)
{
    FILE *stream = fopen("gui-run.txt", "wb");
    if (stream) { fputs("gui exit", stream); fclose(stream); }
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show)
{
    (void)instance; (void)previous; (void)command; (void)show;
    return atexit(completed);
}
