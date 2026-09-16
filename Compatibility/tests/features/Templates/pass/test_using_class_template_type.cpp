namespace imported
{
	template<bool B> struct Value
	{
		Value() : value(B) {}
		bool value;
	};
}

namespace target
{
	using ::imported::Value;
}

int main()
{
	target::Value<true> value;
	return value.value ? 0 : 1;
}
