// Must use constant static initialization and return zero in C mode.
static _Complex double real_only = 3.0;
static _Complex double parts = 1.0 + 2.0i;
int main(void) {
    static _Complex double local = 4.0 + 5.0i;
    return __real__ real_only != 3.0 || __imag__ real_only != 0.0
        || __real__ parts != 1.0 || __imag__ parts != 2.0
        || __real__ local != 4.0 || __imag__ local != 5.0;
}
