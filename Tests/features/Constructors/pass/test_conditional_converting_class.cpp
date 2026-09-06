int calls;
struct Text {
  int value;
  Text(const char* s):value(*s){++calls;}
  Text(int n):value(n){++calls;}
};
Text choose(bool condition) { return condition ? Text(7) : "a"; }
Text reverse(bool condition) { return condition ? "b" : Text(9); }
int main(){Text a=choose(true);Text b=choose(false);Text c=reverse(true);Text d=reverse(false);
  return a.value!=7 || b.value!='a' || c.value!='b' || d.value!=9 || calls!=4;}
