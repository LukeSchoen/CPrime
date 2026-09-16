// Five namespace forms share one translation unit; each has a distinct check.
namespace Alpha {
int value = 42;
int get() { return 7; }
}
namespace Outer { namespace Inner {
int value = 9;
int plus(int n) { return value + n; }
} }
namespace Reopen { int a = 5; }
namespace Reopen {
int b = 8;
int sum() { return a + b; }
}
namespace compatibility {
int directive_value = 27;
int Read() { return directive_value; }
}
using namespace compatibility;
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
    if (Alpha::value != 42 || Alpha::get() != 7) return 1;
    if (Outer::Inner::plus(15) != 24) return 2;
    if (Reopen::sum() != 13) return 3;
    if (directive_value != 27 || Read() != 27) return 4;
    value = 11;
    Item item;
    item.n = Outer::Nested::other;
    if (Source::value != 11 || Alias::value != 11 || item.n != 7) return 5;
    return 0;
}
