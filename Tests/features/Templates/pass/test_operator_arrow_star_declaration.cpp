// EXPECT_COMPILE_ONLY: 1
struct Result
{
  void operator()();
};

struct Receiver
{
};

Result operator->* (Receiver, int);
