static int values[8];
enum { distance = &values[6] - &values[2], backwards = &values[1] - &values[5] };
_Static_assert(distance == 4, "positive pointer difference");
_Static_assert(backwards == -4, "negative pointer difference");
int main(void) {
    return distance != 4 || backwards != -4;
}
