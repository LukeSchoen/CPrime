// EXPECT_EXIT: 0
enum Empty {};
enum class Scoped : unsigned char {};
class Ast {
  enum Kind {};
  Kind kind : 8;
public:
  Ast() : kind(static_cast<Kind>(0)) {}
  int value() { return kind; }
};
int main() {
  Ast ast;
  return ast.value() || sizeof(Scoped) != 1 || sizeof(Empty) != sizeof(int);
}
