// EXPECT_EXIT: 0
namespace Types { typedef int Integer; }
struct Aliases { typedef int Integer; };
typedef const int ConstInteger;
typedef int *Pointer;
int calls;
int value() { ++calls; return 7; }
int main() {
    int number = 3;
    Pointer pointer = &number;
    number.ConstInteger::~ConstInteger();
    (&number)->Aliases::Integer::~Integer();
    (&number)->Types::Integer::~Integer();
    pointer.Pointer::~Pointer();
    value().ConstInteger::~ConstInteger();
    return calls != 1;
}
