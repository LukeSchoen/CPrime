template<typename T>
struct CopyCtorMemberCallBox
{
  int count;
  CopyCtorMemberCallBox();
  CopyCtorMemberCallBox(const CopyCtorMemberCallBox &other);
  int Size() const;
  void Reserve(int new_count);
};

template<typename T>
CopyCtorMemberCallBox<T>::CopyCtorMemberCallBox() : count(0)
{
}

template<typename T>
CopyCtorMemberCallBox<T>::CopyCtorMemberCallBox(const CopyCtorMemberCallBox &other)
  : CopyCtorMemberCallBox<T>()
{
  Reserve(other.Size());
}

template<typename T>
int CopyCtorMemberCallBox<T>::Size() const
{
  return count;
}

template<typename T>
void CopyCtorMemberCallBox<T>::Reserve(int new_count)
{
  count = new_count;
}

typedef CopyCtorMemberCallBox<int> IntCopyCtorMemberCallBox;

void ForceCopyCtorMemberCall(const IntCopyCtorMemberCallBox &first)
{
  IntCopyCtorMemberCallBox second(first);
}

int main()
{
  return 0;
}
