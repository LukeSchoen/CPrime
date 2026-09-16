template <typename T>
struct MiniList
{
  T data[2];

  int Size() const { return 2; }
  T &operator[](int index) { return data[index]; }
};

class FolderLike
{
public:
  struct FileInfo
  {
    int size;
  };

  MiniList<FileInfo> files;
};

int main()
{
  FolderLike folder;
  folder.files.data[0].size = 3;
  folder.files.data[1].size = 4;
  int sum = 0;
  for (FolderLike::FileInfo &file : folder.files)
    sum += file.size;
  return sum == 7 ? 0 : 1;
}
