// EXPECT_EXIT: 0
int caught;

struct Value {
    Value() try { throw 17; }
    catch (int value) { caught = value; }
};

int main()
{
    try { Value value; return 1; }
    catch (int value) { return caught == 17 && value == 17 ? 0 : 2; }
}
