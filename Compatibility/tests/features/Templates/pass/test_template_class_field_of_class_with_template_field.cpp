typedef signed long long int i64;

template<typename T>
class MiniList
{
public:
    MiniList() { m_data = 0; m_size = 0; }
    void Clear() { m_size = 0; }

private:
    T *m_data;
    i64 m_size;
};

struct Resource
{
    MiniList<int> bytes;
};

class RenderObject
{
public:
    RenderObject();

private:
    MiniList<Resource> resources;
};

class PolyModel
{
private:
    MiniList<RenderObject> models;
};

int main()
{
    return 0;
}
