namespace compatibility
{
class Utility
{
public:
  static const char* SkipSpaces(const char* text)
  {
    while (IsSpace(*text))
      ++text;
    return text;
  }

  static bool IsSpace(char value)
  {
    return !IsContinuation(value) && value == ' ';
  }

  static bool IsContinuation(char value)
  {
    return (value & 0x80) != 0;
  }
};
}

int main()
{
  const char* result = compatibility::Utility::SkipSpaces("  value");
  return result[0] == 'v' ? 0 : 1;
}
