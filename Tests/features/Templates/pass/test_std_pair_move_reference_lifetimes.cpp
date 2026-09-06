#include <utility>
#include <functional>

struct Owned {
    static int alive;
    int n;
    explicit Owned(int v) : n(v) { ++alive; }
    Owned(const Owned&) = delete;
    Owned(Owned&& other) : n(other.n) { other.n = 0; ++alive; }
    ~Owned() { --alive; }
};
int Owned::alive = 0;

struct Explicit {
    int n;
    explicit Explicit(int v) : n(v) {}
};

int main() {
    int n = 7;
    {
        auto p = std::make_pair(Owned(9), std::ref(n));
        if (p.first.n != 9 || Owned::alive != 1) return 1;
        p.second = 11;
        if (n != 11) return 2;
        auto q = std::move(p);
        if (q.first.n != 9 || p.first.n != 0 || Owned::alive != 2) return 3;
        std::pair<Explicit, int> e(5, 6);
        if (e.first.n != 5) return 4;
    }
    return Owned::alive;
}
