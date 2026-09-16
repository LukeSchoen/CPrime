struct Text { Text(const char*) {} };
struct Base {
  int calls = 0;
  template<class... Args> void log(const Text&, Args&&... args) {
    int used[] = { (Text(args), 0)... };
    calls += sizeof...(args);
  }
};
struct Derived : Base {};
int main() {
  Derived object;
  object.log("event", "a", Text("b"));
  object.log("event", "reason", Text("b"));
  return object.calls != 4;
}
