// A braced initializer fixes the extent of a trailing flexible array member,
// including for deferred dynamic initialization of namespace-scope and
// function-local static objects.

int trace[4];

long long guard_before = 0x1122334455667788LL;

struct Record
{
  int tag;
  int values[];
};

struct Item
{
  explicit Item(int value) : value(value) {}
  int value;
};

struct Items
{
  int tag;
  Item elements[];
};

Record global_constant = {1, {2, 3, 4}};
Record global_dynamic = {5, {++trace[0], ++trace[1]}};
Record global_empty = {6, {}};
Items global_items = {7, {Item(8), Item(9)}};

long long guard_after = 0x8877665544332211LL;

static int check_namespace_scope_objects()
{
  if (global_constant.tag != 1 || global_constant.values[0] != 2
      || global_constant.values[2] != 4) { return 1; }
  if (global_dynamic.tag != 5 || global_dynamic.values[0] != 1
      || global_dynamic.values[1] != 1) { return 2; }
  if (trace[0] != 1 || trace[1] != 1) { return 3; }
  if (guard_before != 0x1122334455667788LL
      || guard_after != 0x8877665544332211LL) { return 4; }
  if (global_empty.tag != 6) { return 5; }
  if (global_items.tag != 7 || global_items.elements[0].value != 8
      || global_items.elements[1].value != 9) { return 6; }
  return 0;
}

static int check_function_local_static()
{
  static Record local_dynamic = {10, {++trace[2], ++trace[3], 12}};
  if (local_dynamic.tag != 10 || local_dynamic.values[0] != 1
      || local_dynamic.values[1] != 1 || local_dynamic.values[2] != 12) { return 1; }
  return trace[2] == 1 && trace[3] == 1 ? 0 : 2;
}

int main()
{
  if (int code = check_namespace_scope_objects()) { return code; }
  if (int code = check_function_local_static()) { return code + 10; }
  return check_function_local_static() == 0 ? 0 : 20;
}
