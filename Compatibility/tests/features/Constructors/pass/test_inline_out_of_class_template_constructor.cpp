// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Vec {
    T x, y;
    Vec(T value) : x(value), y(value) {}
};
template<class T> struct Bounds {
    using Vertex = Vec<T>;
    Bounds(const Vertex& point);
    inline T width() const;
    union {
        Vertex extents[2];
        struct { Vertex min; Vertex max; };
    };
};
template<class T>
inline Bounds<T>::Bounds(const Vertex& point) : min(point), max(point) {}
template<class T> inline T Bounds<T>::width() const { return max.x - min.x; }
int main() {
    Vec<float> point(3);
    Bounds<float> bounds(point);
    if (bounds.min.x != 3 || bounds.max.y != 3) return 1;
    return bounds.width() != 0;
}
