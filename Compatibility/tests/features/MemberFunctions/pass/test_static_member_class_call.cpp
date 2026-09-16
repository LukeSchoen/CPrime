// EXPECT_EXIT: 0
typedef struct CpcString {
  int len;
} CpcString;

struct H
{
  static CpcString ReadText(CpcString path);
};

CpcString H::ReadText(CpcString path)
{
  CpcString result = { path.len + 5 };
  return result;
}

int main(void)
{
  CpcString p = { 7 };
  CpcString s = H::ReadText(p);
  return s.len == 12 ? 0 : 1;
}
