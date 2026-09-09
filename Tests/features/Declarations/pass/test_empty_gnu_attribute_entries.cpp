void function() __attribute__(( , , ));
void function() {}
struct __attribute__(( , packed, , )) Packed { char byte; int value; };
struct __attribute__(( , aligned(16), , )) Aligned { char byte; };
int main() {
    function();
    return sizeof(Packed) != 5 || alignof(Aligned) != 16;
}
