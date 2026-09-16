// EXPECT_EXIT: 0
typedef void (*TestFn)();

class Tests
{
public:
  static bool Register(const char *name, TestFn fn);
};

static int count = 0;

bool Tests::Register(const char *name, TestFn fn)
{
  count++;
  fn();
  return name[0] == 'S';
}

static int ran = 0;

static void SampleTest()
{
  ran = 1;
}

static bool SampleTest_registered = Tests::Register("SampleTest", SampleTest);

int main()
{
  return SampleTest_registered && count == 1 && ran == 1 ? 0 : 1;
}
