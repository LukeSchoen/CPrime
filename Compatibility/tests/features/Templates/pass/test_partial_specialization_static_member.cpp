template<class T, T val>
struct integral_constant
{
	static const T value = val;
};

template<class T, T val>
const T integral_constant<T, val>::value;

template<bool val>
struct integral_constant<bool, val>
{
	static const bool value = val;
};

template<bool val>
const bool integral_constant<bool, val>::value;

int main()
{
	return integral_constant<bool, true>::value ? 0 : 1;
}
