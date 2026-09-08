/* Packaging gate for the native x64 stack-probe ABI. The bootstrap runtime
   intentionally uses a different frame-allocating helper and must not ship. */
#include <stdio.h>
extern int check_stack_probe_abi(void);
int main(void)
{
    if (check_stack_probe_abi()) {
        fputs("Runtime ABI check failed: __chkstk must preserve RSP, RAX and argument registers. Do not package the bootstrap runtime.\n", stderr);
        return 1;
    }
    return 0;
}
