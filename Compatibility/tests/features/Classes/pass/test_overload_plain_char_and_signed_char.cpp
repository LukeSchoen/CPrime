int CharacterKind(const char *)
{
  return 1;
}

typedef signed char SignedByte;

int CharacterKind(const SignedByte *)
{
  return 2;
}

int main()
{
  char plain = 0;
  SignedByte explicit_signed = 0;
  return CharacterKind(&plain) != 1 || CharacterKind(&explicit_signed) != 2;
}
