static unsigned char first;
static unsigned char second;
static unsigned char possible = 2;
static int calls;

static void observe(void) { ++calls; }
static void change_second(void) { second = 8; }

int main(void)
{
    int first_copy = first;
    unsigned int second_copy = second;

    if (first_copy < 4 && (first & 4)) observe();
    {
        int possible_copy = possible;
        if (possible_copy < 5 && (possible & 2)) observe();
    }
    if (second_copy < 8) {
        change_second();
        if (second & 8) observe();
    }
    return calls != 2;
}
