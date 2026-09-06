// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror

template<class A, class B> struct Same { static const bool value=false; };
template<class A> struct Same<A,A> { static const bool value=true; };
template<class T> struct Extent { static const int value=-1; typedef T type; };
template<class T,unsigned long long N> struct Extent<T[N]> { static const int value=N; typedef T type; };
template<class T> struct Extent<T[]> { static const int value=0; typedef T type; };
template<class T> struct IsPointer { static const bool value=false; };
template<class T> struct IsPointer<T*> { static const bool value=true; };
template<class T> struct Dimensions { static const int value=-1; };
template<class T,unsigned long long N,unsigned long long M> struct Dimensions<T[N][M]> { static const int value=N*100+M; };
template<class T> struct Square { static const bool value=false; };
template<class T,unsigned long long N> struct Square<T[N][N]> { static const bool value=true; };
template<class T> struct PointerElement { static const bool value=false; };
template<class T,unsigned long long N> struct PointerElement<T*[N]> { static const bool value=true; };
template<class T> struct Fixed { static const int value=0; };
template<class T> struct Fixed<T[3]> { static const int value=3; };
typedef const int Array[4];
typedef int Unknown[];
typedef int Matrix[2][3];
int main(){
 static_assert(Extent<Array>::value==4,"known bound");
 static_assert(Extent<Unknown>::value==0,"unknown bound");
 static_assert(Dimensions<Matrix>::value==203,"both bounds");
 static_assert(!IsPointer<Array>::value,"array is not pointer");
 static_assert(Square<int[3][3]>::value,"repeat bound");
 static_assert(!Square<Matrix>::value,"conflicting bound");
 static_assert(PointerElement<int*[2]>::value,"array of pointers");
 static_assert(!PointerElement<Matrix>::value,"array element is not pointer");
 static_assert(Fixed<int[3]>::value==3 && Fixed<int[4]>::value==0,"fixed bound");
 static_assert(!Same<int[], int[3]>::value, "unknown bound is a distinct type");
 static_assert(!Same<int[3], int[4]>::value, "array bound is part of identity");
 static_assert(!Same<int(*)[], int(*)[3]>::value, "nested array identity");
 static_assert(Same<int[2][3], Matrix>::value, "direct and alias multidimensional type");
 static_assert(Same<int(*)(double), int(*)(double)>::value, "function pointer type-id");
 static_assert(!Same<int(*)(double), int(*)(int)>::value, "function parameter identity");
 Extent<Array>::type value=7;
 return value!=7;
}
