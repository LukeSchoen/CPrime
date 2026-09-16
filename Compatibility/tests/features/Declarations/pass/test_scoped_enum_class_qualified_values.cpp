enum class AccessMode : unsigned
{
  Read = 3,
  Write = 7
};

int main()
{
  AccessMode mode = AccessMode::Write;
  if (mode != AccessMode::Write)
    return 1;
  return static_cast<unsigned>(mode) == 7u ? 0 : 2;
}
