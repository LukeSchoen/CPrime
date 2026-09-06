#include <windows.h>
typedef void (__cdecl *register_fn)(int *);
int main(int argc, char **argv)
{
    int iteration;
    if (argc != 2) return 1;
    for (iteration = 0; iteration < 2; ++iteration) {
        int state = 0;
        HMODULE module = LoadLibraryA(argv[1]);
        register_fn registration;
        if (!module) return 2;
        registration = (register_fn)GetProcAddress(module, "register_module_callbacks");
        if (!registration) return 3;
        registration(&state);
        if (state) return 4;
        if (!FreeLibrary(module)) return 5;
        if (state != 321) return 6;
    }
    return 0;
}
