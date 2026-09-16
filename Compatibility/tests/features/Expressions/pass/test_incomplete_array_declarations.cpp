// EXPECT_EXIT: 0
struct Incomplete;
extern Incomplete array[];
struct Holder { static Incomplete array[]; };
void accept(Incomplete const *);
void use() { accept(array); accept(Holder::array); }
struct Incomplete { int value; };
Incomplete array[2] = {{3}, {4}};
Incomplete Holder::array[1] = {{5}};
int total;
void accept(Incomplete const *p) { total += p->value; }
int main() { Incomplete (*pointer)[] = &array; use(); return total != 8 || (*pointer)[1].value != 4; }
