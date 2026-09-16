struct Node { Node *children[3]{}; int values[2][2]{{2,3},{4,5}}; int tag{7}; Node() = default; explicit Node(int n): tag(n) {} };
int main(){Node a;Node b(9); return a.children[0] || a.children[1] || a.children[2] || a.values[1][1]!=5 || a.tag!=7 || b.children[2] || b.tag!=9; }
