typedef unsigned short wchar_t;

class WideString
{
public:
  WideString(const wchar_t *text) : value(text ? 1 : 0) {}

  int value;
};

class PathLike
{
public:
  PathLike(const WideString &text) : value(text.value) {}

  int value;
};

static PathLike temp_path()
{
  wchar_t path[8] = { 0 };
  return path;
}

int main()
{
  PathLike path = temp_path();
  return path.value == 1 ? 0 : 1;
}
