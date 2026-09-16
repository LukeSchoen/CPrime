// EXPECT_EXIT: 0
// EXPECT_SOURCES: ["parameterless_instantiation_provider.cpp"]
// A parameterless function template cannot deduce its argument list from a
// parameter list, so the provider's `template int* pointer_value();` names it
// through the declared result type.  The call below links against that
// instantiation: only the pointer declaration deduces T = int from int*().
template<class T> T* pointer_value();

int main() {
  return pointer_value<int>() != 0;
}
