namespace imported
{
	template<bool B> struct Value
	{
		static const bool value = B;
	};
}

namespace target
{
	using ::imported::Value;
	using ::imported::Value;
}

int main()
{
	return target::Value<true>::value ? 0 : 1;
}
