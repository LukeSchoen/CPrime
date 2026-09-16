// EXPECT_EXIT: 0
int value = 3;
extern const int external;
const int external = 17;
template<int &R> struct Ref { static void set(int n) { R = n; } };
template<const int &R> struct ConstRef { static int get() { return R; } };
template<int &R> int read(Ref<R>) { return R; }
struct Functions { static int get() { return value; } };
template<int (*F)()> int invoke() { return F(); }
int main() {
    Ref<value>::set(11);
    return read(Ref<value>()) != 11 || ConstRef<value>::get() != 11
        || invoke<Functions::get>() != 11;
}
