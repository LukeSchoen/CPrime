namespace other {
template<class...Args>struct Tuple;
template<>struct Tuple<>{template<class F>void run(F&f){f();}};
template<class First,class...Rest>struct Tuple<First,Rest...>{First first;Tuple<Rest...>rest;template<class V,class...Vs>Tuple(V&&v,Vs&&...vs):first(static_cast<V&&>(v)),rest(static_cast<Vs&&>(vs)...){}};
}
using namespace other;
int main(){Tuple<int,double> t(3,2.5);return sizeof(t)<12||t.first!=3||t.rest.first!=2.5;}
