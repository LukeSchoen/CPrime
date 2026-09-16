static int live;
struct Tracked {
    Tracked() { ++live; }
    Tracked(const Tracked&) { ++live; }
    ~Tracked() { --live; }
};
struct Bomb {
    Bomb() {}
    Bomb(const Bomb&) { throw 9; }
};
struct Owner {
    Tracked first;
    Bomb second;
    Owner() = default;
    Owner(const Owner&) noexcept(false) = default;
};
struct ArrayOwner {
    Tracked first[2];
    Bomb second;
    ArrayOwner() = default;
    ArrayOwner(const ArrayOwner&) noexcept(false) = default;
};
int main() {
    {
        Owner source;
        if (live != 1) return 1;
        try { Owner copy(source); return 2; }
        catch (int value) { if (value != 9 || live != 1) return 3; }
    }
    {
        ArrayOwner source;
        if (live != 2) return 4;
        try { ArrayOwner copy(source); return 5; }
        catch (int value) { if (value != 9 || live != 2) return 6; }
    }
    return live != 0;
}
