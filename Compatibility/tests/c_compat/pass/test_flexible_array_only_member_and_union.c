// EXPECT_EXIT: 0
struct Only { int values[]; } first = {{7, 11}}, second = {{3}};
union Choice { int scalar; int values[]; } array = {.values = {2, 4, 6}};
union Both { char bytes[]; int words[]; } words = {.words = {13, 17}};
union Both bytes = {.bytes = {1, 2, 3}};
union Choice scalar = {.scalar = 19};
int main(void) {
    return first.values[1] != 11 || second.values[0] != 3
        || array.values[2] != 6 || words.words[1] != 17
        || bytes.bytes[2] != 3 || scalar.scalar != 19 || sizeof(union Both) != 0
        || sizeof(struct Only) != 0 || sizeof(union Choice) != sizeof(int);
}
