// EXPECT_EXIT: 0
struct Value { int n; const Value *self; Value(int input):n(input),self(this){} };
int inspect(const Value *a, const Value *b) { return a->n==1 && b->n==2 && a->self==a && b->self==b && a!=b; }
int main(){ return !inspect(Value(1).self,Value(2).self); }
