// EXPECT_EXIT: 0
struct Pair { int first; int second; };
extern Pair constant_pair;
extern int constant_array[2];
int observe_constants() { return constant_pair.first + constant_array[1]; }
int early = observe_constants();
Pair constant_pair = {11, 13};
int constant_array[2] = {17, 19};
int counter;
int next_value() { return ++counter; }
int dynamic_array[] = {next_value(), next_value(), next_value()};
Pair dynamic_pair = {next_value(), next_value()};
const int readonly_array[] = {next_value(), next_value()};
char self_initialized[] = {self_initialized[0]};
int local_array(int n) { static const int values[] = {n, n + 1}; return values[1]; }
int main() {
    if (early != 30 || counter != 7) return 1;
    if (dynamic_array[0] != 1 || dynamic_array[2] != 3) return 2;
    if (dynamic_pair.first != 4 || dynamic_pair.second != 5) return 3;
    if (readonly_array[1] != 7 || local_array(11) != 12 || local_array(99) != 12) return 4;
    return self_initialized[0] != 0;
}
