#include <cwchar>

int main()
{
	return std::wcslen(L"abc") == 3 ? 0 : 1;
}
