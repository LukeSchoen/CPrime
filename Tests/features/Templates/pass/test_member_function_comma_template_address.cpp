// EXPECT_EXIT: 0
struct node;

template <node* (node::*method)()>
struct member_tag {};

struct node {
  node* first(), second(member_tag<&node::first>);
};

node* node::first() { return this; }

int main()
{
  node value;
  return value.first() == &value ? 0 : 1;
}
