struct S { char x[9]; };
int main() { S a = {{1,2,3,4,5,6,7,8,9}}, b = {{9,8,7,6,5,4,3,2,1}}, old; __atomic_exchange(&a, &b, &old, __ATOMIC_SEQ_CST); return old.x[0] == 1 && a.x[0] == 9 ? 0 : 1; }
