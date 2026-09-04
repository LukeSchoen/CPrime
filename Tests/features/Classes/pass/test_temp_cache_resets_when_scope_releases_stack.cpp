class ScopeTempVirtual
{
public:
    ScopeTempVirtual(int initial) : value(initial) {}

    virtual int Read() const
    {
        return value;
    }

private:
    int value;
};

int Add(int left, int right)
{
    return left + right;
}

int ExerciseScopeReuse(ScopeTempVirtual* external)
{
    {
        int early = Add(external->Read(), external->Read());
        if (early != 14)
            return 2;
    }

    ScopeTempVirtual local(21);
    int later = Add(local.Read(), local.Read());
    return later == 42 ? 0 : 1;
}

int main()
{
    ScopeTempVirtual external(7);
    return ExerciseScopeReuse(&external);
}
