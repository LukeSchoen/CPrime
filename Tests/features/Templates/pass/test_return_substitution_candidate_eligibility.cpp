// EXPECT_EXIT: 0
template<class T> T pick(T value) { return value; }
template<class... Args> int pick(Args... args) { return 7; }

// A return type cannot supply a missing template argument during deduction.
template<class T> T required();
template<class... Args> int required(Args... args) { return 11; }

// Instantiating this class for an int would be a hard error. The two-argument
// candidate is ineligible before its return type requires that instantiation.
template<class T> struct Result { typedef typename T::type type; };
template<class T> typename Result<T>::type selected(T, T);
template<class T> int selected(T) { return 13; }

template<class T> T defaulted(T value, int extra = 2) { return value + extra; }

int main() {
    if (pick() != 7) return 1;
    if (pick(3) != 3) return 2;
    if (pick(3, 4) != 7) return 3;
    if (required() != 11) return 4;
    if (selected(1) != 13) return 5;
    if (defaulted(17) != 19) return 6;
    return defaulted(17, 3) != 20;
}
