#include <initializer_list>

static int live;
static int constructed;
static int destroyed;
static int copies;

struct Entry {
    char value;
    Entry(const char *text) : value(text[0]) { ++live; ++constructed; }
    Entry(const Entry& other) : value(other.value) { ++live; ++copies; }
    ~Entry() { --live; ++destroyed; }
};

template<class T> int first_value(const T *values, int index = 0) {
    return values[index].value;
}

struct Table {
    int count;
    int checksum;
    int error;
    Table(std::initializer_list<Entry> entries)
        : count((int)entries.size()), checksum(0), error(live != count) {
        for (int i = 0; i < count; ++i) {
            if (entries.begin()[i].value != 'A' + i % 26) error = 1;
            checksum += entries.begin()[i].value;
        }
        if (count && first_value(entries.begin()) != 'A') error = 1;
    }
};

int main() {
    // More than the compiler's former function-argument limit; line markers
    // also exercise saved-clause ownership during deferred template emission.
    Table table = {
    "A entry",
    "B entry",
    "C entry",
    "D entry",
    "E entry",
    "F entry",
    "G entry",
    "H entry",
    "I entry",
    "J entry",
    "K entry",
    "L entry",
    "M entry",
    "N entry",
    "O entry",
    "P entry",
    "Q entry",
    "R entry",
    "S entry",
    "T entry",
    "U entry",
    "V entry",
    "W entry",
    "X entry",
    "Y entry",
    "Z entry",
    "A entry",
    "B entry",
    "C entry",
    "D entry",
    "E entry",
    "F entry",
    "G entry",
    "H entry",
    "I entry",
    "J entry",
    "K entry",
    "L entry",
    "M entry",
    "N entry",
    "O entry",
    "P entry",
    "Q entry",
    "R entry",
    "S entry",
    "T entry",
    "U entry",
    "V entry",
    "W entry",
    "X entry",
    "Y entry",
    "Z entry",
    "A entry",
    "B entry",
    "C entry",
    "D entry",
    "E entry",
    "F entry",
    "G entry",
    "H entry",
    "I entry",
    "J entry",
    "K entry",
    "L entry",
    "M entry",
    "N entry",
    "O entry",
    "P entry",
    "Q entry",
    "R entry",
    "S entry",
    "T entry",
    "U entry",
    "V entry",
    "W entry",
    "X entry",
    "Y entry",
    "Z entry",
    "A entry",
    "B entry",
    "C entry",
    "D entry",
    "E entry",
    "F entry",
    "G entry",
    "H entry",
    "I entry",
    "J entry",
    "K entry",
    "L entry",
    "M entry",
    "N entry",
    "O entry",
    "P entry",
    "Q entry",
    "R entry",
    "S entry",
    "T entry",
    "U entry",
    "V entry",
    "W entry",
    "X entry",
    "Y entry",
    "Z entry",
    "A entry",
    "B entry",
    "C entry",
    "D entry",
    "E entry",
    "F entry",
    "G entry",
    "H entry",
    "I entry",
    "J entry",
    "K entry",
    "L entry",
    "M entry",
    "N entry",
    "O entry",
    "P entry",
    "Q entry",
    "R entry",
    "S entry",
    "T entry",
    "U entry",
    "V entry",
    "W entry",
    "X entry",
    "Y entry",
    "Z entry",
    "A entry",
    "B entry",
    "C entry",
    "D entry",
    "E entry",
    "F entry",
    "G entry",
    "H entry",
    "I entry",
    "J entry",
    };
    if (table.error || table.count != 140 || copies) return 1;
    if (live || constructed != 140 || destroyed != 140) return 2;
    int expected = 0;
    for (int i = 0; i < 140; ++i) expected += 'A' + i % 26;
    if (table.checksum != expected) return 3;
    Table empty = {};
    return empty.error || empty.count || live || destroyed != 140;
}
