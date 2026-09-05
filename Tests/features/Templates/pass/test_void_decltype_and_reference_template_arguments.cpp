// EXPECT_EXIT: 0
template<class T> struct Box { typedef T type; };
void empty() {}
int number = 7;
int &number_reference() { return number; }
template<class T> struct Forward { typedef typename Box<T &>::type reference; };
int main() {
    Box<decltype(empty())>::type *nothing = 0;
    Box<int &>::type reference = number;
    Box<decltype(number_reference())>::type returned = number;
    Forward<int &>::reference collapsed = number;
    reference = 11;
    if (number != 11) return 2;
    returned += 13;
    if (number != 24) return 3;
    collapsed += 17;
    if (number != 41) return 4;
    Box<int &&>::type moved = static_cast<int &&>(number);
    if (moved != 41) return 5;
    return nothing == 0 ? 0 : 6;
}
