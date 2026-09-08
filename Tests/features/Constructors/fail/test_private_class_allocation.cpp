// EXPECT_COMPILE_FAIL: 1
#include <new>
class Object {
    void* operator new(std::size_t);
};
int main() { new Object; }
