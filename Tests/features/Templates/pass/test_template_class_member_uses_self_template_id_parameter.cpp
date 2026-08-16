typedef long long i64;

template<typename T>
struct SelfTemplateIdParamList {
    void erase(const SelfTemplateIdParamList<i64> &indexes);
};

int main()
{
    SelfTemplateIdParamList<int> list;
    (void)list;
    return 0;
}
