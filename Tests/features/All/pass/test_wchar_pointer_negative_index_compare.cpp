typedef unsigned short wchar_t;

static int quoted(wchar_t *cursor, wchar_t *base)
{
  if (cursor > base && cursor[-1] == (wchar_t)'"')
    return 1;
  return 0;
}

int main()
{
  wchar_t text[2];
  text[0] = (wchar_t)'"';
  text[1] = 0;
  return quoted(text + 1, text) != 1;
}
