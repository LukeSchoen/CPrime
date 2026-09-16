typedef long long i64;
typedef unsigned char ui8;

template<typename T>
struct TypedefSelfTemplateIdParamList {
    void erase(const TypedefSelfTemplateIdParamList<i64> &indexes);
};

typedef TypedefSelfTemplateIdParamList<ui8> ByteSelfTemplateIdParamList;

int main()
{
    ByteSelfTemplateIdParamList list;
    (void)list;
    return 0;
}
