// EXPECT_EXIT: 0
typedef struct CpcString {
  int len;
} CpcString;

struct F
{
  static CpcString ReadText(CpcString path);
  static CpcString ReadText(const char *path, bool *ok);
  static CpcString ReadText(const char *path);
};

CpcString F::ReadText(CpcString path)
{
  CpcString result = { path.len + 1 };
  return result;
}

CpcString F::ReadText(const char *path, bool *ok)
{
  CpcString result = { path[0] + (*ok ? 2 : 3) };
  return result;
}

CpcString F::ReadText(const char *path)
{
  CpcString result = { path[0] + 4 };
  return result;
}

int main(void)
{
  CpcString input = { 10 };
  CpcString from_string = F::ReadText(input);
  if (from_string.len != 11)
    return 1;

  bool ok = true;
  CpcString with_ok = F::ReadText("A", &ok);
  if (with_ok.len != 67)
    return 2;

  const char *path = "A";
  CpcString from_chars = F::ReadText(path);
  if (from_chars.len != 69)
    return 3;

  return 0;
}
