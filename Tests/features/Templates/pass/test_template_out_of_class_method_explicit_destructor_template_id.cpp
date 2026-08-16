template<typename T>
class ResetList
{
public:
    ResetList() {}
    ~ResetList() {}
    void SetData(T *data);

private:
    T *m_data;
};

template<typename T>
void ResetList<T>::SetData(T *data)
{
    this->~ResetList<T>();
    m_data = data;
}

int main()
{
    ResetList<int> list;
    int value = 1;
    list.SetData(&value);
    return 0;
}
