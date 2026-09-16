static const double source = 1.25;
static const double *pointer = &source;
static const double &reference = source;
extern const double later;
static const volatile double volatile_source = 3;
extern const double volatile_later;
static int failed;
struct Observer {
    Observer() { if (later != 2.5 || volatile_later != 0) failed = 1; }
};
static Observer observer;
const double later = source * 2;
const double volatile_later = volatile_source * 2;
static const float small = 1.5f;
static const double converted = small + source;
int main() {
    const double *address = &source;
    return failed || *address != 1.25 || converted != 2.75
        || pointer != address || &reference != address || volatile_later != 6;
}
