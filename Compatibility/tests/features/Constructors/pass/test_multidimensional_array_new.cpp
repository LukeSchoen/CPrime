int live, next_id, destroyed;
struct Item {
  int id;
  Item() : id(++next_id) { ++live; }
  ~Item() { if (id != next_id - destroyed) __builtin_abort(); ++destroyed; --live; }
};
static_assert(sizeof(new double[5]) == sizeof(void*), "unevaluated allocation");
template<int N> struct Size {};
template<int N> Size<sizeof(new double[N])> probe() { return Size<sizeof(void*)>(); }
template Size<sizeof(void*)> probe<2>();
int main() {
  int rows = 3;
  int (*values)[4] = new int[rows++][4]();
  if (rows != 4 || values[2][3]) return 1;
  values[2][3] = 7;
  if (values[2][3] != 7) return 2;
  delete[] values;
  Item (*items)[4] = new Item[2][4];
  if (live != 8 || items[1][3].id != 8) return 3;
  delete[] items;
  return live || destroyed != 8;
}
