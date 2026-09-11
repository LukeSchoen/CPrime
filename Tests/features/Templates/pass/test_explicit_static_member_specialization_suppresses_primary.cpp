// EXPECT_EXIT: 0
// An explicit specialization of a static data member suppresses the primary
// template's out-of-class definition for that specialization.  Other
// instances still receive the primary definition.
int calls;
int initialize() { return ++calls + 4; }

template <int i> struct Slot { static int value; };
template <int i> int Slot<i>::value = initialize();

template <> int Slot<0>::value;
template <> int Slot<0>::value = 0;

int main()
{
    Slot<0>::value = 9;
    Slot<1>::value = 20;
    return calls != 1 || Slot<0>::value != 9 || Slot<1>::value != 20;
}
