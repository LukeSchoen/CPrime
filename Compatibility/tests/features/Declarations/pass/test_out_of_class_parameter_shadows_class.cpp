// An out-of-class member body in a namespace must keep a parameter's
// spelling when that parameter shadows the class name.
namespace Shadow
{
class Font
{
public:
	void SetNativeFont(int *Font);

private:
	int *nativeFont;
};

inline void
Font::SetNativeFont(int *Font)
{
	nativeFont = Font;
}
}

int main()
{
	Shadow::Font font;
	int value = 0;
	font.SetNativeFont(&value);
	return 0;
}
