// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
// EXPECT_EXIT: 0

template<typename T>
class X
{
public:
  template<typename K>
  T* find(const K& key);
  int value;
};

int run(void)
{
  X<int> x;
  x.value = 9;
  return x.value == 9 ? 0 : 1;
}
}

namespace cpc_case_1
{
// EXPECT_EXIT: 0

template<typename T>
class cpcList
{
public:
  T *items;
  int count;

  cpcList()
  {
    this->items = (T *)0;
    this->count = 0;
  }

  ~cpcList()
  {
    this->count = 0;
  }
};

template<typename T>
class Holder
{
  cpcList<T> items;
  int count;

  int size()
  {
    return this->count;
  }
};

int run(void)
{
  Holder<int> h;
  h.count = 7;
  return h.size() == 7 ? 0 : 1;
}
}

namespace cpc_case_2
{
template<typename T>
struct SelfTemplateIdReturnList {
    SelfTemplateIdReturnList<T>& assign(const SelfTemplateIdReturnList<T> &rhs);
};

typedef SelfTemplateIdReturnList<int> IntSelfTemplateIdReturnList;

int run()
{
    IntSelfTemplateIdReturnList list;
    (void)list;
    return 0;
}
}

namespace cpc_case_3
{
typedef long long i64;

template<typename T>
struct SelfTemplateIdParamList {
    void erase(const SelfTemplateIdParamList<i64> &indexes);
};

int run()
{
    SelfTemplateIdParamList<int> list;
    (void)list;
    return 0;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  if (int code = cpc_case_3::run()) { return code; }
  return 0;
}
