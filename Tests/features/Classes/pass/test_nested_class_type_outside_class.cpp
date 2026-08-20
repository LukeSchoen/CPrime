template <typename T>
struct MiniList
{
  T item;
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

static int read_size(const FolderLike::FileInfo &info)
{
  return info.size;
}

int main()
{
  FolderLike::FileInfo info;
  info.size = 7;
  FolderLike folder;
  folder.files.item = info;
  return read_size(folder.files.item) == 7 ? 0 : 1;
}
