// EXPECT_EXIT: 0
int logv;

struct Member
{
  int value;
  Member(int x);
};

Member::Member(int x)
{
  value = x;
  logv = logv * 10 + x;
}

struct Owner
{
  Member first;
  Member second;
  int tail;

  Owner();
};

Owner::Owner() : first(4), second(5), tail(6)
{
  logv = logv * 10 + tail;
}

int main(void)
{
  Owner owner;
  return owner.first.value == 4 && owner.second.value == 5 &&
         owner.tail == 6 && logv == 456 ? 0 : 1;
}

