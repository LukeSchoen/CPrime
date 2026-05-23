// EXPECT_EXIT: 0
struct cpcString
{
  int value;
};

cpcString cpcAssetsPath()
{
  cpcString path;
  path.value = 7;
  return path;
}

void cpcAssetsPath(cpcString *path)
{
  path->value = 11;
}

int main(void)
{
  cpcString a = cpcAssetsPath();
  cpcString b;
  b.value = 0;
  cpcAssetsPath(&b);
  if (a.value != 7)
    return 1;
  if (b.value != 11)
    return 2;
  return 0;
}
