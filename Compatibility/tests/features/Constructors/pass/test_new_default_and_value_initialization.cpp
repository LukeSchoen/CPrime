// EXPECT_EXIT: 0
#include <new>
#include <string.h>
struct Plain { unsigned char value; };
struct Constructed { int value; Constructed() : value(31) {} };
alignas(Constructed) unsigned char storage[32];
int main() {
    memset(storage, 0x5a, sizeof(storage));
    new (storage) unsigned char;
    if (storage[0] != 0x5a) return 1;
    new (storage) unsigned char();
    if (storage[0] != 0) return 2;
    memset(storage, 0x5a, sizeof(storage));
    new (storage) Plain;
    if (storage[0] != 0x5a) return 3;
    Plain* plain = new (storage) Plain();
    if (plain->value != 0) return 4;
    Constructed* constructed = new (storage) Constructed;
    if (constructed->value != 31) return 5;
    int* scalar = new int(37);
    int result = *scalar != 37;
    delete scalar;
    return result;
}
