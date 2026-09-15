struct Member { Member() {} };
class Value { Member member; };
int main() {
    Value value;
    static_assert(__is_constructible(Value), "implicit constructor is public");
}
