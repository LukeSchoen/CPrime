struct String
{
  String(const char *s = "") {}
};

struct Vec2
{
  int x;
  int y;

  Vec2(int _x = 0, int _y = 0)
  {
    x = _x;
    y = _y;
  }
};

enum Flags : long long
{
  Default = 0
};

class Window
{
public:
  Window(const String &title = "program",
         const Vec2 &size = Vec2(800, 600),
         const Flags &flags = Default);
};

Window::Window(const String &title, const Vec2 &size, const Flags &flags)
{
}

int main()
{
  Window window;
  return 0;
}
