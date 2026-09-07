namespace N { struct Item {}; int choose(Item const&, int) { return 2; } }
int choose(N::Item const&, long) { return 1; }
namespace M { using ::choose; }
int main() {
 N::Item item;
 if (M::choose(item, 0) != 1) return 1;
 { using M::choose; if (choose(item, 0) != 2) return 2; }
 return 0;
}
