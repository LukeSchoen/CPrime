// These types share more than 128 initial characters. The four-argument
// specialization and its member symbols exceed 512 characters.
struct TypeWithALongSharedPrefix_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_A { char value; };
struct TypeWithALongSharedPrefix_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_B { double value; };
#define A TypeWithALongSharedPrefix_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_A
#define B TypeWithALongSharedPrefix_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_abcdefghijklmnopqrstuvwxyz_B
template<class T> struct Size { static int get() { return sizeof(T); } };
template<class T, class U, class V, class W> struct Four {
    int count;
    Four() : count(4) {}
    int read() const { return count; }
    int add(int value) { count += value; return count; }
    int add(double value) { count += (int)value + 1; return count; }
};
int main() {
    if (Size<A>::get() != 1 || Size<B>::get() != sizeof(double)) return 1;
    Four<A, B, A, B> object;
    if (object.read() != 4 || object.add(2) != 6) return 2;
    if (object.add(2.0) != 9) return 3;
    return 0;
}
