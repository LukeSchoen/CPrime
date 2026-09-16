// A linkage-specification can introduce a declaration with its own storage
// class, as in the Windows SDK's STRSAFE_INLINE_API expansion.
extern "C" static __inline__ int extern_c_static_value(void)
{
	return 7;
}

extern "C" static int extern_c_static_object = 11;

int main()
{
	return extern_c_static_value() + extern_c_static_object == 18 ? 0 : 1;
}
