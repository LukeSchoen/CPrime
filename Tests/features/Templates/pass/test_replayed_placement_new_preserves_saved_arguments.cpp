struct PlacementValue
{
  int value;

  PlacementValue(int input) : value(input) {}
};

template<typename T>
T *rebuild_at(void *storage, T value)
{
  return new (storage) T(value);
}

int main()
{
  long long scalar_storage = 0;
  long long object_storage[2] = {0, 0};
  int *scalar = rebuild_at<int>(&scalar_storage, 41);
  PlacementValue *object = new (object_storage) PlacementValue(72);

  return *scalar == 41 && object->value == 72 ? 0 : 1;
}

// EXPECT_EXIT: 0
