extern int (*entry)(void);
__asm__(
    ".text\n"
    ".globl \"?value@@\"\n"
    ".type \"?value@@\",@function\n"
    "\"?value@@\":\n"
    "mov $42, %eax\n"
    "ret\n"
    ".set \"?alias@@\", \"?value@@\"\n"
    ".data\n"
    ".globl entry\n"
    "entry:\n"
    ".quad \"?alias@@\"\n"
);
int main(void) { return entry() != 42; }
