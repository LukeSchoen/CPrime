struct First { int value; First(int n):value(n){} };
struct Second { int value; Second(First n):value(n.value){} };
int pick(Second) {return 1;}
int pick(First) {return 2;}
int main(){First first(7); Second second(first); return pick(7)!=2 || pick(second)!=1 || second.value!=7;}
