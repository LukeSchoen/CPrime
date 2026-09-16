// EXPECT_COMPILE_ARGS: -std=c++17
// A standard attribute in front of a function template definition, which is
// what BOOST_NORETURN expands to in BOOST_NORETURN void f(...).  The saved
// definition's `[[` used to be read as a lambda introducer, so the replayed
// instantiation reported "'noreturn' undeclared".  An attribute between
// declaration specifiers (`inline [[noreturn]] void f(int const &);`, the
// Explorer++ shape) is the same position in the decl-specifier-seq.
inline [[noreturn]] void attribute_declared(int const &);

inline [[noreturn]] void attribute_declared(int const &e)
{
	throw e;
}

template<class E>
[[noreturn]] inline void attribute_throw(E const &e)
{
	throw e;
}

int main()
{
	try
	{
		attribute_throw(1);
		attribute_declared(2);
	}
	catch (int)
	{
		return 0;
	}
	return 1;
}
