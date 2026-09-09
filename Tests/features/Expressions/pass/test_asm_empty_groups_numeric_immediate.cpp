int main() {
    int value = 0;
    asm volatile("" ::: "memory");
    asm volatile("" ::: );
    asm volatile("" :: "r"(value));
    asm volatile("" :: );
    asm volatile("" : );
    asm volatile("");
    asm volatile("movl %1, %0" : "=r"(value) : "n"(6 * 7));
    if (value != 42) return 1;
    asm volatile("movl %1, %0" : "=r"(value) : "n"(-1 * (int)sizeof(&value)));
    return value != -(int)sizeof(&value);
}
