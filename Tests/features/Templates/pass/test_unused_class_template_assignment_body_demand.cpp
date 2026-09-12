// EXPECT_EXIT: 0
//
// A capability query for a non-template holder's implicit assignment must not
// materialize the body of an unused class-template member.  `Holder` has a
// `List<DeletedAssign>` member, so asking whether Holder can accept a const
// source previously resolved `List<DeletedAssign>::operator=`, emitted its
// body, and left an undefined reference to the deleted element assignment.
// The List<int> assignment below makes sure a real call still demands and
// emits the same out-of-class member body.

struct DeletedAssign
{
  DeletedAssign() {}
  DeletedAssign(const DeletedAssign &) = delete;
  DeletedAssign(DeletedAssign &&) {}
  DeletedAssign &operator=(const DeletedAssign &) = delete;
};

template <typename T>
struct List
{
  T *data;
  List &operator=(const List &other);
};

template <typename T>
List<T> &List<T>::operator=(const List<T> &other)
{
  data[0] = other.data[0];
  return *this;
}

struct Holder
{
  List<DeletedAssign> items;
};

struct Assignable
{
  int value;
  Assignable() : value(0) {}
  Assignable &operator=(const Assignable &other)
  {
    value = other.value + 1;
    return *this;
  }
};

struct AssignableHolder
{
  List<Assignable> items;
};

int main()
{
  Holder holder;
  (void)holder;
  int left_value = 0;
  int right_value = 7;
  List<int> left = {&left_value};
  List<int> right = {&right_value};

  left = right;
  if (left_value != 7) return 1;

  Assignable assignable_left_value;
  Assignable assignable_right_value;
  assignable_right_value.value = 4;
  AssignableHolder assignable_left = {{&assignable_left_value}};
  AssignableHolder assignable_right = {{&assignable_right_value}};

  assignable_left = assignable_right;
  return assignable_left_value.value == 5 ? 0 : 2;
}
