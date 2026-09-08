// EXPECT_EXIT: 0
int constant_string[__builtin_constant_p(&"hello"[0]) ? 1 : -1];
int constant_offset[__builtin_constant_p(&"hello"[3]) ? 1 : -1];
int variable;
int nonconstant_object[!__builtin_constant_p(&variable) ? 1 : -1];
int main() { return sizeof(constant_string) != sizeof(int)
    || sizeof(constant_offset) != sizeof(int); }
