template<class T> struct Outer { template<class U> struct Inner {}; };
template<class T> struct Match {
    template<class U> struct Predicate { enum { value = 0 }; };
    template<class U> struct Predicate<typename Outer<T>::template Inner<U> > {
        enum { value = sizeof(U) };
    };
};
int main() {
    return Match<int>::Predicate<Outer<int>::Inner<double> >::value != sizeof(double)
        || Match<int>::Predicate<Outer<long>::Inner<double> >::value != 0;
}
