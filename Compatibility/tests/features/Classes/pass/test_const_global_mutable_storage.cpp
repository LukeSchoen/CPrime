// EXPECT_EXIT: 0
struct Cell { mutable int value; };
struct Row { Cell cells[2]; };
const Row rows[] = {{{{1}, {2}}}, {{{3}, {4}}}};
int main() {
    rows[1].cells[0].value = 17;
    return rows[1].cells[0].value != 17 || rows[0].cells[1].value != 2;
}
