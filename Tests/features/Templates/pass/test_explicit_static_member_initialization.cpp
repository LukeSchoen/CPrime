// EXPECT_EXIT: 0
int calls;
int initialize() { return ++calls + 16; }
template<class T> struct Data { static T value; static T array[2]; };
template<> int Data<int>::value = initialize();
template<> int Data<int>::array[2] = {3, 5};
template<> double Data<double>::value = 2.5;
int main() {
    return calls != 1 || Data<int>::value != 17 || Data<int>::array[1] != 5
        || Data<double>::value != 2.5;
}
