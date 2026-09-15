// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: std::any reports its dynamic type, rejects a cast to a type it
// does not hold, supports pointer casts, emplace, swap and make_any.
#include <any>
#include <string>

int main() {
    std::any empty;
    if (empty.has_value()) return 1;
    if (empty.type() != typeid(void)) return 2;
    if (std::any_cast<int>(&empty) != 0) return 3;

    std::any number = 5;
    if (number.type() != typeid(int)) return 4;
    if (std::any_cast<int>(number) != 5) return 5;
    if (std::any_cast<char>(&number) != 0) return 6;

    std::any copy = number;
    if (std::any_cast<int>(copy) != 5) return 7;

    std::any text = std::make_any<std::string>("hi");
    if (text.type() != typeid(std::string)) return 8;
    std::string *text_pointer = std::any_cast<std::string>(&text);
    if (!text_pointer) return 9;
    *text_pointer = "yo";
    const std::any *constant = &text;
    if (*std::any_cast<std::string>(constant) != "yo") return 10;

    std::any other;
    other.swap(text);
    if (text.has_value() || !other.has_value()) return 11;
    int &slot = other.emplace<int>(42);
    if (slot != 42 || std::any_cast<int>(other) != 42) return 12;
    other = std::string("z");
    if (*std::any_cast<std::string>(&other) != "z") return 13;
    return 0;
}
