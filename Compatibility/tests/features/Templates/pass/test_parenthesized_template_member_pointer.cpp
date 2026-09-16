// EXPECT_EXIT: 0
template<class T> struct Box { int get() { return 19; } };
int invoke(Box<int>& box, int (Box<int>::*method)()) { return (box.*method)(); }
int main() { Box<int> box; return invoke(box, &Box<int>::get) != 19; }
