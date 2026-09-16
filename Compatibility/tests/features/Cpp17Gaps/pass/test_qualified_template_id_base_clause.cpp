// EXPECT_COMPILE_ARGS: -std=c++17
// A base clause that spells a qualified template-id whose qualifier starts at an
// enclosing namespace, with the base class template declared in the derived
// template's own namespace. Boost.ContainerHash's generated float support does
// this (`boost::hash_detail::call_ldexp<double>` inside namespace boost {
// namespace hash_detail { ... }). The replayed specialization kept the
// qualifier in front of the instantiated name, which already spells its
// namespace, so the base clause did not resolve: "base class type expected".
namespace outer {
namespace inner {

template<class T> struct base { T payload; };

template<int Tag> struct derived : outer::inner::base<int> { };

}
}

int main()
{
	outer::inner::derived<0> probe;
	probe.payload = 3;
	return probe.payload == 3 ? 0 : 1;
}
