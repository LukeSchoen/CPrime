// EXPECT_EXIT: 0
int calls;
int step() { return ++calls; }
int main() {
    int first = ({ int value = step(); value + 10; });
    int second = false ? ({ step(); 20; }) : ({ int value = step(); value + 30; });
    return first != 11 || second != 32 || calls != 2;
}
