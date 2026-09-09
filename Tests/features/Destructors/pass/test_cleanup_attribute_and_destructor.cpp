// EXPECT_EXIT: 0
int sequence;
struct Object {
    ~Object() { sequence = sequence * 10 + 2; }
};
void cleanup(Object*) { sequence = sequence * 10 + 1; }
void normal() { Object object __attribute__((cleanup(cleanup))); }
void unwind() { Object object __attribute__((cleanup(cleanup))); throw 7; }
int main() {
    normal();
    if (sequence != 12) return 1;
    sequence = 0;
    try { unwind(); } catch (int) {}
    return sequence != 12;
}
