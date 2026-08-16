template<typename T>
struct RefChoiceBox
{
  int selected;
  RefChoiceBox();
  void Add(T &&value);
  void Add(const T &value);
  void CopyFrom(const RefChoiceBox &other);
};

template<typename T>
RefChoiceBox<T>::RefChoiceBox() : selected(0)
{
}

template<typename T>
void RefChoiceBox<T>::Add(T &&value)
{
  selected = 1;
}

template<typename T>
void RefChoiceBox<T>::Add(const T &value)
{
  selected = 2;
}

template<typename T>
void RefChoiceBox<T>::CopyFrom(const RefChoiceBox &other)
{
  Add(other.selected);
}

typedef RefChoiceBox<int> IntRefChoiceBox;

int main()
{
  IntRefChoiceBox first;
  IntRefChoiceBox second;
  second.CopyFrom(first);
  return second.selected != 2;
}
