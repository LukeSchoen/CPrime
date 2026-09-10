struct Base {
    int base;
};

struct Left : Base {
};

struct Right : Base {
};

struct Diamond : Left, Right {
};

struct Single : Base {
};

static int select(Base *)
{
    return 1;
}

static int select(void *)
{
    return 2;
}

int main()
{
    Diamond diamond;
    Diamond *ambiguous = &diamond;
    if (select(ambiguous) != 2)
        return 1;

    Single single;
    Single *unique = &single;
    Base *base = unique;
    return base == &single ? 0 : 2;
}
