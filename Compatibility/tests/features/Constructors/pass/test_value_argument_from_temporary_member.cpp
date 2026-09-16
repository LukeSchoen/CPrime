static int copies, moves;
struct Copy {
    Copy() {}
    Copy(const Copy&) { ++copies; }
};
struct Move {
    Move() {}
    Move(const Move&) { ++copies; }
    Move(Move&&) { ++moves; }
};
struct Container { Copy copy; Move move; };
void take_copy(Copy) {}
void take_move(Move) {}
int main() {
    take_copy(Container().copy);
    if (copies != 1) return 1;
    take_move(Container().move);
    if (copies != 1 || moves != 1) return 2;
    Container named;
    take_move(named.move);
    return copies != 2 || moves != 1;
}
