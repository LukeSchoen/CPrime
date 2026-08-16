template<typename T>
struct SelfTemplateIdReturnList {
    SelfTemplateIdReturnList<T>& assign(const SelfTemplateIdReturnList<T> &rhs);
};

typedef SelfTemplateIdReturnList<int> IntSelfTemplateIdReturnList;

int main()
{
    IntSelfTemplateIdReturnList list;
    (void)list;
    return 0;
}
