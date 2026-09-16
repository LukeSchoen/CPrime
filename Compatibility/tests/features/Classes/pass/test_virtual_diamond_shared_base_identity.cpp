// EXPECT_EXIT: 0
struct Root { int value; virtual ~Root() {} };
struct Left : virtual Root {};
struct Right : virtual Root {};
struct Object : Left, Right {};
int main() {
    Object object; Left* left = &object; Right* right = &object;
    Root* a = left; Root* b = right;
    a->value = 29;
    return a != b || b->value != 29 || dynamic_cast<Object*>(b) != &object;
}
