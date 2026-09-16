#include <libcprime.h>

/* Called by test_RunExceptions.ps1. Each image creates an FLS exception
   context on the host thread. Deleting an image must unregister its cleanup
   callback before freeing executable memory, including at host process exit. */
int main(int argc, char **argv)
{
    int iteration;
    const char *source =
        "int calls; struct Guard { ~Guard(){++calls;} };"
        "int main(){try {Guard guard; throw 7;}"
        "catch(int value){return value!=7 || calls!=1;}}";
    if (argc != 2) return 1;
    for (iteration = 0; iteration < 24; ++iteration) {
        CPRIMEState *state = cprime_new();
        int result;
        if (!state) return 2;
        cprime_set_lib_path(state, argv[1]);
        if (cprime_set_output_type(state, CPRIME_OUTPUT_MEMORY) < 0) return 3;
        if (cprime_compile_string_file(state, source, "exception_image.cpp") < 0) return 4;
        result = cprime_run(state, 0, 0);
        cprime_delete(state);
        if (result != 0) return 5;
    }
    return 0;
}
