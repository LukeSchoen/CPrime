int live, order, next_id;
struct Member {
    int id;
    Member() : id(++next_id) { ++live; }
    Member(const Member& other) : id(other.id) { ++live; }
    ~Member() { --live; order = order * 10 + id; }
};
struct Error {
    Error() {}
    Error(const Error&) { throw 17; }
};
struct Aggregate { Member first; Error error; Member last; };
int main() {
    {
        Member first, last;
        Error error;
        try { Aggregate value = {first, error, last}; return 1; }
        catch (int value) {
            if (value != 17 || live != 2 || order != 1) return 2;
        }
    }
    return live || order != 121;
}
