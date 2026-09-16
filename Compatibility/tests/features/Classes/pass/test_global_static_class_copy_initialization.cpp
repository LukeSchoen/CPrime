int constructor_calls;

class Text
{
public:
  Text(const char *value) : first(value[0]) { constructor_calls++; }
  char first;
};

static Text text = "ok";

int main()
{
  if (constructor_calls != 1) return 1;
  if (text.first != 'o') return 2;
  return 0;
}
