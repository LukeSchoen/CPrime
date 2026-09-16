namespace N {
template<long long A,long long B> struct Gcd { static const long long value=Gcd<B,A%B>::value; };
template<long long A> struct Gcd<A,0> { static_assert(A>0,"positive divisor"); static const long long value=A; };
}
int main(){return N::Gcd<84,30>::value!=6;}
