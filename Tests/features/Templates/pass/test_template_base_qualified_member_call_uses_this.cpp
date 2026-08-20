typedef unsigned int ui32;

template<typename T>
struct ArrayBase
{
  T data[2];

  int Size() const { return 2; }
  T *Data() { return data; }
  const T *Data() const { return data; }
};

class ImageLike : public ArrayBase<ui32>
{
public:
  int Size() const { return ArrayBase<ui32>::Size(); }
  ui32 *Data() { return ArrayBase<ui32>::Data(); }
  const ui32 *Data() const { return ArrayBase<ui32>::Data(); }
};

static const ui32 *const_data(const ImageLike &image)
{
  return image.Data();
}

int main()
{
  ImageLike image;
  image.Data();
  const_data(image);
  return image.Size() == 2 ? 0 : 1;
}
