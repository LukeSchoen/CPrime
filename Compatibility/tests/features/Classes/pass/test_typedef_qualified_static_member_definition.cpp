// EXPECT_EXIT: 0
// An out-of-class static data member definition may qualify the member with a
// typedef of its class.
struct Q
{
    enum { left = 1, right = 2 };
    static const int both;
    static const int value;
};

typedef Q Alias;
const int Alias::both = Alias::left | Alias::right;
const int Alias::value = 5;

int main()
{
    return Alias::both != 3 || Alias::value != 5;
}
