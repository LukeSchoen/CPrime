namespace api {
    int choose(int);
    int choose(double);
}
namespace imported { using api::choose; }
namespace chained { using imported::choose; }
namespace api {
    int choose(int value) { return value + 10; }
    int choose(double value) { return (int)value + 20; }
}
int main() {
    return imported::choose(1) != 11 || imported::choose(2.0) != 22
        || chained::choose(1) != 11 || chained::choose(2.0) != 22
        || api::choose(1) != 11 || api::choose(2.0) != 22;
}
