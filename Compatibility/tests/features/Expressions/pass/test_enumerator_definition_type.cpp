// EXPECT_EXIT: 0
const char letter = 'a';
const unsigned short number = 7;
enum Values { first = letter, first_size = sizeof(first),
              second = number, second_size = sizeof(second) };
int main() {
    return first_size != sizeof(char) || second_size != sizeof(unsigned short)
        || sizeof(first) != sizeof(Values) || sizeof(second) != sizeof(Values);
}
