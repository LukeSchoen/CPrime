struct ArrayRefCtorProbe {
    int value;
    ArrayRefCtorProbe(const int (&values)[1])
      : value(values[0])
    {
    }
};

int main()
{
    int values[1];
    values[0] = 5;
    ArrayRefCtorProbe probe(values);
    return probe.value != 5;
}
