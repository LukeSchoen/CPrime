// EXPECT_EXIT: 0

typedef long long i64;
typedef unsigned char ui8;

template<typename T>
class List
{
public:
  T *data;
  i64 size;

  List()
  {
    data = 0;
    size = 0;
  }
};

enum ResourceType : i64
{
  Resource_Texture = 1
};

struct Resource
{
  ResourceType type;
  List<ui8> buffer;
};

int main(void)
{
  Resource r;
  r.type = Resource_Texture;
  return r.type == Resource_Texture ? 0 : 1;
}
