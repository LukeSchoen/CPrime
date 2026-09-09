int events[8], count, construction[8], built;
struct Item {
    int value;
    Item(int v) : value(v) { construction[built++] = v; }
    ~Item() { events[count++] = value; }
};
struct Left : Item { Left(int v) : Item(v) {} };
struct Right : Item { Right(int v) : Item(v) {} };
struct VirtualLeft : Item { VirtualLeft(int v) : Item(v) {} };
struct VirtualRight : Item { VirtualRight(int v) : Item(v) {} };
struct Complete : Left, Right, virtual VirtualLeft, virtual VirtualRight {
    Item first, second;
    Complete() : VirtualLeft(0), VirtualRight(1), Left(2), Right(3), first(4), second(5) {}
};
int main() {
    {
        Complete object;
        if (built != 6) return 3;
        for (int i = 0; i < 6; ++i) if (construction[i] != i) return 4;
    }
    if (count != 6) return 1;
    for (int i = 0; i < 6; ++i) if (events[i] != 5 - i) return 2;
    return 0;
}
