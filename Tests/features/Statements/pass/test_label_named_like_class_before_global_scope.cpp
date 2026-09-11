// A label whose name is also a class: `Label: ::target();` must parse the
// leading `Label:` as a label and the following `::` as a global-scope
// qualifier. Reading it as a class-qualified name would demand a member name
// after the third colon and fail with "static data member name".

namespace
{
int calls;
}

void target() { ++calls; }

struct Label
{
  static int magnify(int value) { return value * 2; }
};

int main()
{
Label:
  ::target();
  if (calls != 1)
    return 1;
  // A genuine class-scope qualifier in an expression must still resolve.
  if (Label::magnify(3) != 6)
    return 2;
  return 0;
}
