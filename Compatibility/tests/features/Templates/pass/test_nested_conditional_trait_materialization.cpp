// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror

namespace traits {
template<class T,T V> struct Integral { static constexpr T value=V; typedef T value_type; typedef Integral type; };
template<bool V> using Constant=Integral<bool,V>;
typedef Integral<bool,false> False;
typedef Integral<bool,true> True;
template<bool C,class T,class U> struct Conditional { typedef T type; };
template<class T,class U> struct Conditional<false,T,U> { typedef U type; };
template<class T> struct RemoveReference { typedef T type; };
template<class T> struct RemoveReference<T&> { typedef T type; };
template<class T> struct RemoveReference<T&&> { typedef T type; };
template<class T> struct RemoveConst { typedef T type; };
template<class T> struct RemoveConst<const T> { typedef T type; };
template<class T> struct RemoveVolatile { typedef T type; };
template<class T> struct RemoveVolatile<volatile T> { typedef T type; };
template<class T> struct RemoveCV { typedef typename RemoveConst<typename RemoveVolatile<T>::type>::type type; };
template<class T> struct IsConst : False {};
template<class T> struct IsConst<const T> : True {};
template<class T> struct IsReference : False {};
template<class T> struct IsReference<T&> : True {};
template<class T> struct IsReference<T&&> : True {};
template<class T> struct IsFunction : Constant<!IsConst<const T>::value && !IsReference<T>::value> {};
template<class T> struct RemoveExtent { typedef T type; };
template<class T,unsigned long long N> struct RemoveExtent<T[N]> { typedef T type; };
template<class T> struct RemoveExtent<T[]> { typedef T type; };
template<class T> struct IsArray : False {};
template<class T,unsigned long long N> struct IsArray<T[N]> : True {};
template<class T> struct IsArray<T[]> : True {};
template<class A,class B> struct Same : False {};
template<class A> struct Same<A,A> : True {};
template<class T> struct Decay {
 typedef typename RemoveReference<T>::type U;
 typedef typename Conditional<IsArray<U>::value,
   typename RemoveExtent<U>::type*,
   typename Conditional<IsFunction<U>::value,U*,typename RemoveCV<U>::type>::type>::type type;
};
}
typedef int Array[3];
int main(){
 static_assert(traits::Same<traits::Decay<int>::type,int>::value,"scalar");
 static_assert(traits::Same<traits::Decay<const int>::type,int>::value,"const scalar");
 static_assert(traits::Same<traits::Decay<const int&>::type,int>::value,"reference");
 static_assert(traits::Same<traits::Decay<Array>::type,int*>::value,"array");
 return 0;
}
