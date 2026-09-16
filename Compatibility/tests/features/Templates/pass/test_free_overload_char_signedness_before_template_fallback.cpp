// EXPECT_EXIT: 0

int select(char *) { return 1; }
int select(signed char *) { return 2; }
int select(unsigned char *) { return 3; }

template<typename T>
int select(T *) { return 4; }

int main()
{
  char plain = 0;
  signed char signedValue = 0;
  unsigned char unsignedValue = 0;
  if (select(&plain) != 1)
    return 1;
  if (select(&signedValue) != 2)
    return 2;
  if (select(&unsignedValue) != 3)
    return 3;
  return 0;
}
