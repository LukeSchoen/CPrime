// EXPECT_EXIT: 0
#include <type_traits>

double ordinary_value;
auto* ordinary_pointer() { return &ordinary_value; }
auto& ordinary_reference() { return ordinary_value; }
decltype(auto) ordinary_expression() { return (ordinary_value); }

template<class T> auto* pointer(T* value) { return value; }
template<class T> const auto* const_pointer(T* value) { return value; }
template<class T> auto& reference(T& value) { return value; }
template<class T> const auto& const_reference(T& value) { return value; }
template<class T> auto&& forward_reference(T& value) { return value; }
template<class T> auto&& rvalue_reference(T& value) { return static_cast<T&&>(value); }
template<class T> decltype(auto) declared_reference(T& value) { return value; }
template<class T> decltype(auto) expression_reference(T& value) { return (value); }
template<class T> decltype(auto) copy_value(T value) { return value; }
template<class T> auto copy_plain(T& value) { return value; }

template<class T> struct Access {
    T value;
    auto* pointer() { return &value; }
    auto& reference() { return value; }
    const auto& read() const { return value; }
    decltype(auto) expression() { return (value); }
    decltype(auto) copy() { return value; }
    decltype(auto) copy() const { return value; }
    decltype(auto) expression() const { return (value); }
    static auto* pointer_to(T* value) { return value; }
    static auto& reference_to(T& value) { return value; }
    static auto&& forward_to(T& value) { return value; }
    static decltype(auto) expression_to(T& value) { return (value); }
};
struct StaticAccess {
    static auto* pointer(double* value) { return value; }
    static auto& reference(double& value) { return value; }
    static decltype(auto) expression(double& value) { return (value); }
};
template<class T, int N> auto& array_reference(T (&value)[N]) { return value; }
template<class T, int N> auto* array_pointer(T (&value)[N]) { return value; }
template<class T> struct Measure {
    T value;
    auto length() const { return value; }
};
template<class T> struct DoubleMeasure {
    Measure<T> measure;
    auto length() const;
};
template<class T> auto DoubleMeasure<T>::length() const { return measure.length() * 2; }

int main() {
    double value = 2.5;
    const double constant = 3.5;
    static_assert(std::is_same<decltype(pointer(&value)), double*>::value, "auto pointer");
    static_assert(std::is_same<decltype(const_pointer(&value)), const double*>::value, "const pointee");
    static_assert(std::is_same<decltype(reference(value)), double&>::value, "lvalue reference");
    static_assert(std::is_same<decltype(reference(constant)), const double&>::value, "preserve referred cv");
    static_assert(std::is_same<decltype(const_reference(value)), const double&>::value, "add referred cv");
    static_assert(std::is_same<decltype(forward_reference(value)), double&>::value, "forwarding collapse");
    static_assert(std::is_same<decltype(rvalue_reference(value)), double&&>::value, "xvalue reference");
    static_assert(std::is_same<decltype(declared_reference(value)), double&>::value, "declared reference");
    static_assert(std::is_same<decltype(expression_reference(value)), double&>::value, "expression reference");
    static_assert(std::is_same<decltype(copy_value(value)), double>::value, "declared value");
    static_assert(std::is_same<decltype(copy_plain(constant)), double>::value, "plain auto strips cv");
    if (pointer(&value) != &value || const_pointer(&value) != &value) return 1;
    reference(value) = 4.5;
    forward_reference(value) = 5.5;
    expression_reference(value) = 6.5;
    if (value != 6.5 || copy_value(value) != 6.5) return 2;
    Access<double> access = {7.5};
    access.reference() = 8.5;
    access.expression() = 9.5;
    const Access<double>& view = access;
    static_assert(std::is_same<decltype(view.copy()), double>::value, "declared member type");
    static_assert(std::is_same<decltype(view.expression()), const double&>::value, "const member expression");
    if (access.pointer() != &access.value || view.read() != 9.5 || access.copy() != 9.5) return 3;
    Access<double>::reference_to(value) = 10.5;
    Access<double>::forward_to(value) = 11.5;
    Access<double>::expression_to(value) = 12.5;
    if (Access<double>::pointer_to(&value) != &value || value != 12.5) return 4;
    StaticAccess::reference(value) = 13.5;
    StaticAccess::expression(value) = 14.5;
    if (StaticAccess::pointer(&value) != &value || value != 14.5) return 5;
    int values[3] = {1, 2, 3};
    using ArrayReference = int (&)[3];
    static_assert(std::is_same<decltype(array_reference(values)), ArrayReference>::value, "array reference extent");
    static_assert(std::is_same<decltype(array_pointer(values)), int*>::value, "array decay");
    array_reference(values)[1] = 9;
    if (array_pointer(values) != values || values[1] != 9) return 6;
    ordinary_reference() = 15.5;
    ordinary_expression() = 16.5;
    if (*ordinary_pointer() != 16.5) return 7;
    DoubleMeasure<double> measured = {{2.5}};
    static_assert(std::is_same<decltype(measured.length()), double>::value, "nested member deduction");
    return measured.length() != 5.0;
}
