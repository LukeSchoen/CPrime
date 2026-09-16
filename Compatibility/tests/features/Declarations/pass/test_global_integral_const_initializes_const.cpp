static const char LineFeed = static_cast<char>(0x0a);
static const char LF = LineFeed;
static const int Width = 4;
static const int Area = Width * Width;

int main()
{
  return LF == '\n' && Area == 16 ? 0 : 1;
}
