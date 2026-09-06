extern int value;
extern double fraction;
extern int *address;
static int observed;
struct Observer {
    Observer() {
        observed = (value == 21) + (fraction == 1.5) + (address == &value);
    }
} observer;
int value = 3 * 7;
double fraction = 3.0 / 2.0;
int *address = &value;
int main() { return observed != 3; }
