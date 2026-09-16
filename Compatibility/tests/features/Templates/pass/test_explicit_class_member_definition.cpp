// EXPECT_EXIT: 0
template<class T> struct Box;
template<> struct Box<int> { int get(); };
int Box<int>::get() { return 17; }
int main() { Box<int> box; return box.get() != 17; }
