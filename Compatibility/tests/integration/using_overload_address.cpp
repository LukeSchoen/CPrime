namespace api {
    int choose(int value) { return value + 10; }
    int choose(double value) { return (int)value + 20; }
}
namespace imported { using api::choose; }
namespace chained { using imported::choose; }
int apply(int (*function)(double)) { return function(2.0); }
template<class T> int forward(int (*function)(T), T value) { return function(value); }
int main() {
    int (*a)(int) = imported::choose;
    int (*b)(double) = imported::choose;
    int (*c)(double) = &chained::choose;
    int (*d)(double) = api::choose;
    a = &imported::choose;
    return a(1) != 11 || b(2.0) != 22 || c(2.0) != 22
        || d(2.0) != 22 || apply(chained::choose) != 22
        || forward(chained::choose, 2.0) != 22;
}
