// EXPECT_COMPILE_ARGS: -std=c++17
// A standard attribute at the end of parameters-and-qualifiers
// (`int f(int) [[noreturn]];`).  It used to be read as the ancient
// `int c()[]` array suffix, which reported "']' expected (got '[')".
inline int attribute_after_parameters(int value) [[noreturn]];

inline int attribute_after_parameters(int value) [[noreturn]]
{
	throw value;
}

int main()
{
	try
	{
		attribute_after_parameters(1);
	}
	catch (int)
	{
		return 0;
	}
	return 1;
}
