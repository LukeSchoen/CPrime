namespace first {
  enum value { ready = 3, next = ready + 1 };
  namespace nested { enum kind { ready = 7 }; }
  int read() { return next + nested::ready; }
}
namespace second { enum value { ready = 11 }; }
int main() {
  enum local { ready = 13 };
  return first::ready != 3 || first::read() != 11
      || second::ready != 11 || ready != 13;
}
