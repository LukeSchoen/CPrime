// Deducing a template argument from the parameter list of a pointer to member
// function keeps the reference that parameter declares, so a call can bind a
// temporary to the deduced `const int&` argument.
struct Subject {
  int value;
  int read(const int& delta) const { return value + delta; }
};

template <class _Ret, class _Tp, class _Arg>
class ConstMemFun1 {
public:
  explicit ConstMemFun1(_Ret (_Tp::*function)(_Arg) const) : function_(function) {}
  _Ret call(const _Tp* object, _Arg argument) const
  {
    return (object->*function_)(argument);
  }
private:
  _Ret (_Tp::*function_)(_Arg) const;
};

template <class _Ret, class _Tp, class _Arg>
ConstMemFun1<_Ret, _Tp, _Arg> mem_fun(_Ret (_Tp::*function)(_Arg) const)
{
  return ConstMemFun1<_Ret, _Tp, _Arg>(function);
}

int main()
{
  Subject subject;
  subject.value = 40;
  if (mem_fun(&Subject::read).call(&subject, 2) != 42) return 1;
  return 0;
}
