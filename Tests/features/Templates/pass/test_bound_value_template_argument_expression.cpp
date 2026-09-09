// EXPECT_EXIT: 0
template<int Number> struct Value {};
template<int Number> int previous(Value<Number - 1>) { return Number; }
template<int First, int Second> int sum(Value<First + Second>) { return First + Second; }
template<int Number> int twice(Value<Number>, Value<Number * 2>) { return Number; }
int main() {
    return previous<5>(Value<4>()) != 5 || sum<1, 2>(Value<3>()) != 3
        || twice(Value<4>(), Value<8>()) != 4;
}
