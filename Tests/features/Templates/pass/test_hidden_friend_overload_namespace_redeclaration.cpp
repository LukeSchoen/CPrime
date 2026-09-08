// EXPECT_EXIT: 0
namespace area {
struct Item { int value; friend int read(Item item) { return item.value + 2; } };
int read(int value) { return value + 20; }
}
template<class T> int via_adl(T value) { return read(value); }
namespace area { int read(Item); }
int main() {
    area::Item item = {7};
    return via_adl(item) != 9 || area::read(item) != 9 || area::read(7) != 27;
}
