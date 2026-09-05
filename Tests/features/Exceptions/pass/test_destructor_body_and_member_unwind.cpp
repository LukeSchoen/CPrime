static int order;
struct Member {
    int value;
    Member(int value = 1) : value(value) {}
    ~Member() noexcept(false) {
        order = order * 10 + value;
        if (value == 2) throw 107;
    }
};
struct FromBody {
    Member member;
    ~FromBody() noexcept(false) { throw 109; }
};
struct FromMember {
    Member first;
    Member last;
    FromMember() : first(1), last(2) {}
};
int main() {
    try { FromBody object; }
    catch (int code) { if (code != 109 || order != 1) return 1; }
    order = 0;
    try { FromMember object; }
    catch (int code) { if (code != 107 || order != 21) return 2; }
    return order != 21;
}
