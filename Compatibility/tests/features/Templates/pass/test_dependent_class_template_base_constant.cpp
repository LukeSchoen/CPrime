// EXPECT_EXIT: 0

template <bool Value>
struct CpcConstant
{
  static const bool value = Value;
};

template <unsigned Size>
struct CpcOkTag
{
  char bytes[Size];
};

namespace CpcTraitDetail
{

template <class T>
CpcOkTag<sizeof(T)> check_complete(int);

template <class T>
char check_complete(...);

template <class T, bool Complete>
struct nothrow_imp
{
  static const bool value = false;
};

template <class T>
struct nothrow_imp<T, true>
{
  static const bool value = noexcept(T());
};

} // namespace CpcTraitDetail

template <class T>
struct CpcIsComplete
  : CpcConstant<
        false
        || (sizeof(CpcTraitDetail::check_complete<T>(0)) != sizeof(char))>
{
};

template <class T>
struct CpcHasNothrowDefault
  : CpcConstant<CpcTraitDetail::nothrow_imp<T, true>::value>
{
};

static_assert(CpcIsComplete<unsigned int>::value,
              "complete type is complete");
static_assert(CpcHasNothrowDefault<unsigned int>::value,
              "builtin default construction is noexcept");

int main()
{
  return CpcIsComplete<unsigned int>::value
      && CpcHasNothrowDefault<unsigned int>::value ? 0 : 1;
}
