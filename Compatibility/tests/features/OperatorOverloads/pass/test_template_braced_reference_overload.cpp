#include <initializer_list>
struct Word { int value; };
struct Entry { Word first, second; };
template<class T> struct Collection {
    Collection() {}
    Collection(const std::initializer_list<T> &) {}
    int append(T &&entry) { return entry.first.value + entry.second.value; }
    int append(const T &) { return -1; }
    int append(const Collection &) { return -2; }
    int append(Collection &&) { return -3; }
    int append(const T *, int count = 1) { return -4; }
    int append(const std::initializer_list<T> &) { return -5; }
};
int evaluations;
Word touch(Word word) { ++evaluations; return word; }
int main() {
    Collection<Entry> collection;
    Word first = {3}, second = {4};
    if (collection.append({touch(first), touch(second)}) != 7) return 1;
    if (evaluations != 2) return 2;
    Entry entry = {first, second};
    if (collection.append(entry) != -1) return 3;
    if (collection.append({entry}) != -5) return 4;
    return 0;
}
