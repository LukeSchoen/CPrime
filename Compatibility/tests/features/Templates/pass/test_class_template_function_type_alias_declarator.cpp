/* A class template's `using` alias for an abstract function, array or
   pointer-to-function type has its declared name slot inside the declarator.
   The template replay spells the alias as a typedef, so it has to place the
   name there: `using F = bool(int);` becomes `typedef bool F(int);`, not the
   ill-formed `typedef bool(int) F;`. */
// EXPECT_EXIT: 0

template <class T> struct function_alias {
  using F = bool(T, int);
  F *pointer;
};

template <class T> struct array_alias {
  using A = T[4];
  A *pointer;
};

template <class T> struct function_pointer_alias {
  using P = bool (*)(int, int);
  P pointer;
  T value;
};

int main() {
  function_alias<int> function;
  array_alias<int> array;
  function_pointer_alias<int> function_pointer;
  function.pointer = 0;
  array.pointer = 0;
  function_pointer.pointer = 0;
  if (sizeof(function) == 0) return 1;
  if (sizeof(array) == 0) return 2;
  if (sizeof(function_pointer) == 0) return 3;
  return 0;
}
