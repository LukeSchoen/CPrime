// A conditional with a pointer operand and a class operand whose conversion
// operator adds qualification takes the conversion result as the composite
// pointer type (`char *` against `operator const char *`).

struct Text
{
  const char *value;

  operator const char *() const { return value; }
};

const char *select(bool use_text, char *text, const Text &fallback)
{
  return use_text ? text : fallback;
}

const char *select_const(bool first, const char *text, const Text &fallback)
{
  return first ? text : fallback;
}

struct MutableText {
  char *value;
  operator char *() const { return value; }
};
const char *select_mutable(bool first, const char *text, const MutableText &fallback)
{
  return first ? text : fallback;
}

int main()
{
  char buffer[3];
  buffer[0] = 'o';
  buffer[1] = 'k';
  buffer[2] = 0;

  Text fallback;
  fallback.value = "no";

  if (select(true, buffer, fallback) != buffer)
    return 1;
  if (select(false, buffer, fallback) != fallback.value)
    return 2;
  MutableText mutable_text = {buffer};
  if (select_const(true, buffer, fallback) != buffer
      || select_const(false, buffer, fallback) != fallback.value) return 3;
  if (select_mutable(true, fallback.value, mutable_text) != fallback.value
      || select_mutable(false, fallback.value, mutable_text) != buffer) return 4;
  return 0;
}
