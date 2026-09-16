// Consolidated from similar standalone regressions; each cpc_case_N preserves one.
#include <stdbool.h>
#include <string.h>

namespace cpc_case_0
{
// EXPECT_EXIT: 0

typedef long long i64;

class cpcString
{
private:
  char m_text[64];
  i64 m_len;
  void assignLen(const char *text, i64 len)
  {
    m_len = len;
    memcpy(m_text, text, (size_t)m_len);
    m_text[m_len] = 0;
  }

public:
  cpcString() { this->assignLen("", 0); }
  cpcString(const char *text) { this->assignLen(text, (i64)strlen(text)); }
  bool Empty() const { return m_text[0] == 0; }
  bool Equals(const char *rhs) const { return strcmp(m_text, rhs) == 0; }
  cpcString SubstringFrom(i64 start) const
  {
    cpcString out;
    out.assignLen(m_text + start, m_len - start);
    return out;
  }
};

static void cpcString_DefaultAndLiteral()
{
  cpcString empty;
  if (!empty.Empty())
    return;
}

class Tests
{
public:
  static bool Register(const char *suiteName, const char *testName, void (*fn)());
  static bool RunAll();
};

bool Tests::Register(const char *suiteName, const char *testName, void (*fn)())
{
  return suiteName[0] == 'c' && testName[0] == 'D' && fn != 0;
}

static void RegisterStringTests()
{
  Tests::Register("cpcString", "DefaultAndLiteral", cpcString_DefaultAndLiteral);
}

bool Tests::RunAll()
{
  RegisterStringTests();
  return true;
}

int run()
{
  return Tests::RunAll() ? 0 : 1;
}
}

namespace cpc_case_1
{
// EXPECT_EXIT: 0
class Tests
{
public:
  static bool RunAll();
};

bool Tests::RunAll()
{
  return true;
}

int run()
{
  return Tests::RunAll() ? 0 : 1;
}
}

namespace cpc_case_2
{
// EXPECT_EXIT: 0
class Tests
{
public:
  static bool RunAll();
};


bool Tests::RunAll()
{
  return true;
}

int run()
{
  return Tests::RunAll() ? 0 : 1;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}
