/* Calling through a static data member of function-pointer type: the member
   access yields the pointer value and the ordinary call path applies.  It is
   not a member-function call, so the pointer target must not be required to be
   a function declaration of the class. */

static int calls;

static void hook_one()
{
  ++calls;
}

static int add_one(int value)
{
  return value + 1;
}

struct Handler
{
  static void (*hook)();
  static int (*convert)(int);
};

void (*Handler::hook)() = hook_one;
int (*Handler::convert)(int) = add_one;

int main()
{
  Handler handler;

  handler.hook();
  if (calls != 1)
    return 1;

  Handler *pointer = &handler;
  pointer->hook();
  if (calls != 2)
    return 2;

  (handler.hook)();
  if (calls != 3)
    return 3;

  if (handler.convert(4) != 5)
    return 4;
  if (pointer->convert(9) != 10)
    return 5;
  return 0;
}
