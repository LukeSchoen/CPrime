namespace sample { struct Node { int n; }; Node* get(Node* p) {return p;} }
int main() {
  sample::Node n = {4};
  if (sample::Node* p = sample::get(&n)) { if(p->n != 4) return 1; }
  else return 2;
  if (sample::get(&n) == 0) return 3;
  return 0;
}
