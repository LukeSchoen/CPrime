typedef long long i64;
typedef unsigned int ui32;

struct MiniString
{
  MiniString(const char *text = "");
};

MiniString::MiniString(const char *text)
{
  (void)text;
}

template <typename T>
struct MiniVec2
{
  T x, y;

  MiniVec2(const T &value);
  MiniVec2(const T &_x, const T &_y);
  static MiniVec2<T> Zero();
};

template <typename T>
MiniVec2<T>::MiniVec2(const T &value)
  : x(value), y(value)
{
}

template <typename T>
MiniVec2<T>::MiniVec2(const T &_x, const T &_y)
  : x(_x), y(_y)
{
}

template <typename T>
MiniVec2<T> MiniVec2<T>::Zero()
{
  return MiniVec2<T>(0);
}

typedef MiniVec2<int> MiniVec2I;

enum MiniWindowFlags : i64
{
  MiniWindowDefault = 0
};

class MiniWindow
{
public:
  MiniWindow(const MiniVec2I &size,
             const MiniString &title = "",
             const MiniWindowFlags &flags = MiniWindowDefault);
  MiniWindow(const MiniString &title = "program",
             const MiniVec2I &size = MiniVec2I(800, 600),
             const MiniWindowFlags &flags = MiniWindowDefault);
  MiniWindow(const MiniString &title,
             const bool &openGL,
             const bool &fullscreen = false,
             const MiniVec2I &size = MiniVec2I(800, 600),
             const MiniWindowFlags &flags = MiniWindowDefault);

  void Clear(ui32 color = 0);

  MiniVec2I desktopDPI = 0;
};

MiniWindow::MiniWindow(const MiniVec2I &size,
                       const MiniString &title,
                       const MiniWindowFlags &flags)
{
  (void)size;
  (void)title;
  (void)flags;
}

MiniWindow::MiniWindow(const MiniString &title,
                       const MiniVec2I &size,
                       const MiniWindowFlags &flags)
{
  (void)title;
  (void)size;
  (void)flags;
}

MiniWindow::MiniWindow(const MiniString &title,
                       const bool &openGL,
                       const bool &fullscreen,
                       const MiniVec2I &size,
                       const MiniWindowFlags &flags)
{
  (void)title;
  (void)openGL;
  (void)fullscreen;
  (void)size;
  (void)flags;
}

void MiniWindow::Clear(ui32 color)
{
  (void)color;
}

int main()
{
  MiniVec2I res(704, 552);
  MiniWindow window("Window", true, false, res, MiniWindowDefault);
  window.Clear(0xff009500);
  return 0;
}
