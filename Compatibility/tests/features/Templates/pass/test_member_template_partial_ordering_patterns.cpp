// EXPECT_EXIT: 0
template<class T> struct Box { T value; };
template<class T> struct Choices {
    template<class U> int select(const Box<U>&, int) { return 1; }
    template<class U> int select(const U&, int) { return 2; }
    template<class A, class B> int later(const A&, const B&) { return 3; }
    template<class A, class B> int later(const A&, const Box<B>&) { return 4; }
    template<class A, class B> int repeated(const A&, const B&) { return 5; }
    template<class A> int repeated(const A&, const A&) { return 6; }
};
template<class T> struct Reversed {
    template<class U> int select(const U&, int) { return 2; }
    template<class U> int select(const Box<U>&, int) { return 1; }
};
int main() {
    Choices<int> choices;
    Reversed<int> reversed;
    Box<double> box;
    if (choices.select(box, 0) != 1 || reversed.select(box, 0) != 1) return 1;
    if (choices.later(7, box) != 4 || choices.later(7, 8) != 3) return 2;
    if (choices.repeated(7, 8) != 6 || choices.repeated(7, 8.0) != 5) return 3;
    return 0;
}
