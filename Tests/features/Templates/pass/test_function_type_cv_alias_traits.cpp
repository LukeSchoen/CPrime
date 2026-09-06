template<class T> struct IsConst { static const bool value=false; };
template<class T> struct IsConst<const T> { static const bool value=true; };
template<class T> struct IsReference { static const bool value=false; };
template<class T> struct IsReference<T&> { static const bool value=true; };
template<class T> struct IsReference<T&&> { static const bool value=true; };
template<class T> struct IsFunction { static const bool value=!IsConst<const T>::value && !IsReference<T>::value; };
template<class T,class U> struct Same { static const bool value=false; };
template<class T> struct Same<T,T> { static const bool value=true; };
typedef int Function(double);
typedef int Variadic(int,...);
typedef int Qualified() const;
namespace names {
  template<class T> struct is_same_label {
    static const int value=17;
    typedef T type;
  };
  template<class T> struct is_integral_method {
    static int value(){return 23;}
  };
}
int main() {
 static_assert(!IsConst<Function>::value,"function not const");
 static_assert(!IsConst<const Function>::value,"const typedef ignored");
 static_assert(Same<Function,const Function>::value,"same type");
 static_assert(IsFunction<Function>::value,"plain function");
 static_assert(IsFunction<Variadic>::value,"variadic function");
 static_assert(IsFunction<Qualified>::value,"qualified function");
 static_assert(!IsFunction<Function*>::value,"function pointer");
 static_assert(!IsFunction<Function&>::value,"function reference");
 static_assert(!IsFunction<int>::value,"object");
 static_assert(!IsFunction<const int>::value,"const object");
 static_assert(names::is_same_label<int>::value==17,"names do not define semantics");
 names::is_same_label<int>::type n=4;
 return names::is_integral_method<int>::value()!=23 || n!=4;
}
