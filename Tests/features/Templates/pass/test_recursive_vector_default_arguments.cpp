// Reduced from the model-tools scratch family: macro-generated overloads,
// a static template factory in a default argument, and recursive vector math.
template<class T> struct Vec {
    T x, y, z;
    Vec(T a = 0, T b = 0, T c = 0) : x(a), y(b), z(c) {}
    static Vec Zero() { return Vec(); }
    Vec operator*(int scale) const { return Vec(x * scale, y * scale, z * scale); }
    Vec operator+(const Vec &o) const { return Vec(x + o.x, y + o.y, z + o.z); }
};
typedef Vec<int> Position;
struct Node { unsigned address, mask, color; };
#define STREAMABLE(T) \
    inline int write(const T *p) { return p->color; } \
    inline int read(T *p) { return p->mask; }
STREAMABLE(Node);

static int visited, total;
void walk(const Node *nodes, Position pos = Position::Zero(), int node = 0, int depth = 0) {
    if (!nodes[node].address) {
        ++visited;
        total += pos.x + 10 * pos.y + 100 * pos.z + write(nodes + node);
        return;
    }
    int index = 0;
    for (int z = 0; z != 2; ++z)
        for (int y = 0; y != 2; ++y)
            for (int x = 0; x != 2; ++x, ++index)
                if (nodes[node].mask & (1u << index))
                    walk(nodes, pos * 2 + Position(x, y, z),
                         nodes[node].address + index, depth + 1);
}
struct Scalar {
    int value;
    static Scalar Zero() { return Scalar{0}; }
};
int scalar_default(Scalar value = Scalar::Zero(), int tail = 3) { return value.value + tail; }
int main() {
    Node nodes[9] = {{1, 129, 0}, {0, 0, 2}, {}, {}, {}, {}, {}, {}, {0, 0, 3}};
    walk(nodes);
    return visited != 2 || total != 116 || read(nodes) != 129 || scalar_default() != 3;
}
