static int destroyed;
struct Member { ~Member() { ++destroyed; } };
class Value {
public:
    Value() {}
private:
    Member member;
};
int main() {
    Value{};
    if (destroyed != 1) return 1;
    { Value local; }
    return destroyed != 2;
}
