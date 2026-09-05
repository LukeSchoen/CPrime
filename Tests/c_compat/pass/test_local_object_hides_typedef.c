typedef struct Scope { int n; } scope;
int main(void) {
  struct Scope object;
  struct Scope *scope = &object;
  scope->n = 3;
  return scope->n != 3;
}
