// EXPECT_EXIT: 0
struct target {
  void visit(int) {}
};

struct holder {
  typedef void (target::*target_member)(int);
  typedef void (holder::*holder_member)(int);

  template <target_member>
  void dispatch(int) {}
};

int main()
{
  holder value;
  holder::holder_member member = &holder::dispatch<&target::visit>;
  (value.*member)(1);
  return 0;
}
