// EXPECT_EXIT: 0
struct Plain { int value; };
typedef Plain Aligned __attribute__((aligned(32)));
Plain ordinary;
Aligned aligned;
typedef struct SameName { int value; } SameName __attribute__((aligned(16)));
int main() {
    return __alignof__(SameName) != 16 || __alignof__(struct SameName) != __alignof__(int)
        || __alignof__(Aligned) != 32 || __alignof__(aligned) != 32
        || __alignof__(Plain) != __alignof__(int)
        || __alignof__(ordinary) != __alignof__(int)
        || __alignof__(sizeof(Aligned)) != __alignof__(sizeof(int));
}
