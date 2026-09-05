static int destroyed;
struct Guard {
  int value;
  explicit Guard(int n) : value(n) { if (n == 2) throw n; }
  ~Guard() { destroyed = destroyed * 10 + value; }
};
static void fail() { Guard first(1); Guard incomplete(2); }
int main() {
  try { fail(); }
  catch (int n) { return n == 2 && destroyed == 1 ? 0 : 1; }
  return 2;
}
