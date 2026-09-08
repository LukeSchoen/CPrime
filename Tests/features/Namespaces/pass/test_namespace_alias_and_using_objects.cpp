// EXPECT_EXIT: 0
namespace Source {
    int value = 3;
    struct Item { int n; };
    namespace Inner { int other = 7; }
}
namespace Alias = Source;
namespace Outer { namespace Nested = ::Source::Inner; }
using Alias::value;
using Source::Item;
int main() {
    value = 11;
    Item item;
    item.n = Outer::Nested::other;
    return Source::value != 11 || Alias::value != 11 || item.n != 7;
}
