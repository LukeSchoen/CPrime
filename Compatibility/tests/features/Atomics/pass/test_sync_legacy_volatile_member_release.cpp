// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
// A volatile member function releases a lock flag through its object.
struct T {
    bool t;
    void foo() volatile {
        __sync_lock_release(&t);
        __sync_synchronize();
    }
    long long bump() volatile {
        return __sync_add_and_fetch(&count, 2);
    }
    long long count;
};

int main() {
    T t = { true, 0 };
    t.foo();
    if (t.t) return 1;
    if (__sync_fetch_and_add(&t.count, 5) != 0 || t.count != 5) return 2;
    if (t.bump() != 7 || t.count != 7) return 3;
    if (!__sync_bool_compare_and_swap(&t.count, 7, 1) || t.count != 1) return 4;
    if (__sync_lock_test_and_set(&t.count, 0) != 1) return 5;
    return 0;
}
