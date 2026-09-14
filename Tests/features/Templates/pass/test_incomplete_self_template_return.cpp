// A return declaration must not instantiate storage for its incomplete owner.
// Reduced from the vector/quad scratch reproducers; no external headers needed.
#include <inttypes.h>

template<class T> struct Pair {
    T x, y;
    typedef T ElementType;
    typedef Pair<T> VectorType;
    enum { ElementCount = 2 };
    Pair() = default;
    Pair(const T &a, const T &b) : x(a), y(b) {}
    template<class U> explicit Pair(const Pair<U> other)
        : x(T(other.x)), y(T(other.y)) {}
    template<class U, class V> explicit Pair(const U &a, const V &b)
        : x(T(a)), y(T(b)) {}
    static Pair Zero();
    auto Normalized() const;
    Pair &Normalize();
    Pair &operator+=(const T &value);
    template<class U> auto operator+(const Pair<U> &other) const;
};

template<class T> Pair<T> Pair<T>::Zero() { return Pair<T>(); }
template<class T> auto Pair<T>::Normalized() const { return Pair<T>(x, y); }
template<class T> Pair<T> &Pair<T>::Normalize() { return *this = Normalized(); }
template<class T> Pair<T> &Pair<T>::operator+=(const T &value) {
    x += value; y += value; return *this;
}
template<class T> template<class U>
auto Pair<T>::operator+(const Pair<U> &other) const {
    return Pair<T>(x + other.x, y + other.y);
}

template<class T> struct Triple { T x, y, z; Triple() = default; };
template<class T> struct Four { T x, y, z, w; Four() = default; };
template<class T> struct List { T *data; };

struct Shape {
    Shape() = default;
    Shape(const Shape &) = default;
    Shape(const Triple<float> &, const Pair<float> &, const Triple<float> &);
    Shape(const Four<Triple<float>> &);
    Pair<Shape> Split() const;
    List<Shape> Children() const;
    Triple<float> center;
    Pair<float> dimensions;
    Four<Triple<float>> corners;
    int64_t tag;
};

Pair<Shape> Shape::Split() const { return Pair<Shape>(*this, *this); }

struct PlainSelf {
    Pair<PlainSelf> Split() const;
    int value;
};

int main() {
    Shape shape{};
    shape.tag = 17;
    shape.dimensions = Pair<float>(3.0f, 4.0f);
    Pair<Shape> halves = shape.Split();
    if (halves.x.tag != 17 || halves.y.dimensions.y != 4.0f) return 1;
    Pair<int> pair = Pair<int>::Zero();
    pair += 3;
    pair.Normalize();
    Pair<long> converted(pair);
    Pair<int> sum = pair + Pair<int>(1, 2);
    return converted.x != 3 || sum.x != 4 || sum.y != 5;
}
