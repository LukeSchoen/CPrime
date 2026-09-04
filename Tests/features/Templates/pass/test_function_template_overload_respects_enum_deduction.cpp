static int selected;

enum ResultCode
{
    RESULT_OK
};

bool select(const char*, ResultCode, ResultCode, bool = true, bool = false)
{
    selected = 1;
    return true;
}

bool select(const char*, bool, bool, bool = true, bool = false)
{
    selected = 2;
    return false;
}

template<class T>
bool select(const char*, T expected, T found, bool = true)
{
    selected = 3;
    return expected == found;
}

int main()
{
    ResultCode code = RESULT_OK;
    if (!select("enum", RESULT_OK, code) || selected != 1)
        return 1;

    int signedValue = 7;
    if (!select("signed", 7, signedValue) || selected != 3)
        return 2;

    unsigned value = 0x2020u;
    if (!select("unsigned", static_cast<unsigned>(0x2020), value)
        || selected != 3)
        return 3;
    if (!select("unsigned again", static_cast<unsigned>(0x2020), value)
        || selected != 3)
        return 4;

    select("mixed", static_cast<int>(RESULT_OK), code);
    return selected == 2 ? 0 : 5;
}
