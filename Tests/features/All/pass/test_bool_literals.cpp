// EXPECT_EXIT: 0

int main(void)
{
  if (false)
    return 1;
  if (!true)
    return 2;

  bool a = true;
  bool b = false;
  if (!a)
    return 3;
  if (b)
    return 4;

  return 0;
}

