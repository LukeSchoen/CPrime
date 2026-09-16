// EXPECT_EXIT: 0
int object = 11;
typedef unsigned int Count;
template<int *Address> int choose(int);
template<unsigned int Number> int choose(int);
template<int *Pointer> int choose(int) { return *Pointer; }
template<Count Value> int choose(int) { return 7; }
int main() {
    return choose<3>(0) != 7 || choose<0>(0) != 7 || choose<&object>(0) != 11;
}
