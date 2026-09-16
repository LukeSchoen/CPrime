template<class T>struct Owner {
  struct Node {T value;};
  Node node;
  template<class V>Node* set(V v){node.value=v;return &node;}
  int run(){return set(7)->value;}
};
int main(){Owner<int> o;return o.run()!=7;}
