typedef signed long long int i64;
typedef unsigned char ui8;

template<typename T>
class MiniList
{
public:
    MiniList() { m_data = 0; m_size = 0; m_capacity = 0; }
    ~MiniList() { this->ClearStorage(); }
    void Clear() { m_size = 0; }
    void ClearStorage() { this->Clear(); m_data = 0; m_size = 0; m_capacity = 0; }

private:
    T *m_data;
    i64 m_size;
    i64 m_capacity;
};

struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };
struct Mat4 { float m[16]; };
struct StringLike { char *text; i64 len; };

struct MeshTriangle
{
    i64 a;
    i64 b;
    i64 c;
};

struct MeshMaterial
{
    StringLike name;
    Vec4 color;
    MiniList<ui8> bytes;
};

class Mesh
{
private:
    StringLike source;
    MiniList<Vec3> positions;
    MiniList<Vec4> colors;
    MiniList<Vec3> normals;
    MiniList<Vec2> texCoords;
    MiniList<MeshTriangle> triangles;
    MiniList<MeshMaterial> materials;
};

struct Resource
{
    StringLike name;
    MiniList<ui8> buffer;
    Vec4 value;
    Mat4 matrix;
};

typedef MiniList<Vec2> Vec2List;
typedef MiniList<Vec3> Vec3List;
typedef MiniList<Vec4> Vec4List;

class RenderObject
{
public:
    RenderObject();
    ~RenderObject();

private:
    MiniList<Resource> resources;
    StringLike vertPath;
    StringLike fragPath;
    Vec3 position;
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
