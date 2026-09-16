// EXPECT_EXIT: 0
// An attribute with arguments on a parameter declarator used to feed its
// arguments (aligned(8), section("data"), format(printf,1,2)) to the
// direct-initializer probe.  The declaration was then read as a direct
// initialization of a void object and rejected.  Attribute argument lists
// must not participate in that disambiguation.
struct Result {
    int value;
};

static void record(int value __attribute__((aligned(8))),
                   Result *out __attribute__((unused)),
                   int extra __attribute__((section("cpc_param_attr")))) {
    out->value = value + extra;
}

void report(const char *format __attribute__((format(printf, 1, 2))), ...);

void install(void (*handler)(const char *format, ...)
             __attribute__((noreturn, format(printf, 1, 2))));

int main() {
    Result r;
    r.value = 0;
    record(5, &r, 2);
    return r.value == 7 ? 0 : 1;
}
