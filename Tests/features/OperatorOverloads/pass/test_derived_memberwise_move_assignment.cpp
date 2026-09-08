#include <utility>
int copies, moves;
template<class T> struct Buffer {
    T value;
    Buffer() : value(0) {}
    Buffer &operator=(const Buffer &other) { value = other.value; ++copies; return *this; }
    Buffer &operator=(Buffer &&other) { value = other.value; other.value = 0; ++moves; return *this; }
};
struct Prefix { long long padding; };
struct Image : Prefix, Buffer<int> {
    int extra;
    Buffer<int> samples[2];
};
struct WithDestructor : Buffer<int> { ~WithDestructor() {} };
struct WithCopyConstructor : Buffer<int> {
    WithCopyConstructor() {}
    WithCopyConstructor(const WithCopyConstructor &other) { value = other.value; }
};
struct WithExtraArgument : Buffer<int> {
    WithExtraArgument() {}
    WithExtraArgument(const WithExtraArgument &other, int extra) { value = other.value + extra; }
};
struct Object {
    Image image;
    Object &operator=(Object &&other) { image = std::move(other.image); return *this; }
};
int main() {
    Object a, b;
    b.image.value = 42;
    b.image.padding = 123;
    b.image.extra = 99;
    b.image.samples[0].value = 3;
    b.image.samples[1].value = 4;
    a = std::move(b);
    if (a.image.value != 42 || b.image.value != 0 || moves != 3 || copies) return 1;
    if (a.image.padding != 123 || a.image.extra != 99
        || a.image.samples[0].value != 3 || a.image.samples[1].value != 4
        || b.image.samples[0].value || b.image.samples[1].value) return 2;
    Image c;
    Image &result = (c = a.image);
    if (&result != &c) return 7;
    if (c.value != 42 || c.extra != 99 || copies != 3) return 3;
    copies = moves = 0;
    WithDestructor d, e;
    e.value = 8;
    d = std::move(e);
    if (d.value != 8 || e.value != 8 || copies != 1 || moves) return 4;
    WithCopyConstructor f, g;
    g.value = 9;
    f = std::move(g);
    if (f.value != 9 || g.value != 9 || copies != 2 || moves) return 5;
    const Image &constant = c;
    a.image = std::move(constant);
    if (c.value != 42 || copies != 5 || moves) return 6;
    WithExtraArgument h, i;
    i.value = 11;
    h = std::move(i);
    return h.value != 11 || i.value || copies != 5 || moves != 1;
}
