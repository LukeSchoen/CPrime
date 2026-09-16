// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
template<typename T>
void destroy(T *value)
{
}

int run()
{
  char value = 0;
  destroy(&value);
  return 0;
}
}

namespace cpc_case_1
{
template<typename T>
void destroy(T *value)
{
}

class PointerThing
{
public:
  int value;
};

int run()
{
  PointerThing value;
  destroy(&value);
  return 0;
}
}

namespace cpc_case_2
{
template<typename T>
void destroy(T *value)
{
}

int run()
{
  long long value = 0;
  destroy(&value);
  return 0;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}
