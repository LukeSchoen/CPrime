template<char... Values> int count_values() { return sizeof...(Values); }
int main() { return count_values<'a', 'b', 'c'>() != 3; }