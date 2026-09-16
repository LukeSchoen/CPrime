template<typename T>
class Box
{
public:
    Box() {}
    ~Box() {}

    void Reset()
    {
        this->~Box<T>();
    }
};

int main()
{
    Box<int> box;
    box.Reset();
    return 0;
}
