// EXPECT_COMPILE_ARGS: -lgdiplus -lgdi32
#include <gdiplus.h>

int main()
{
	Gdiplus::GdiplusStartupInput input;
	return input.GdiplusVersion == 1 ? 0 : 1;
}
