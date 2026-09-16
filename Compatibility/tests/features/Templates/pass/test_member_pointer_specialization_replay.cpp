// EXPECT_COMPILE_ONLY: 1

template<typename T> struct takes_member_ptr;
template<typename T, typename Klasse> struct takes_member_ptr<T Klasse::*> {};

template<typename T, typename Klasse>
void fun_takes_member_ptr(T Klasse::*) {}

struct X {
  void bar(float) {}
};

void use() {
  sizeof(takes_member_ptr<void (X::*)(float)>);
  fun_takes_member_ptr(&X::bar);
}
