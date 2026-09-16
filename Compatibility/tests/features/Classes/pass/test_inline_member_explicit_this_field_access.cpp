struct CameraExplicitThis
{
  int value;

  void Set(int v)
  {
    this->value = v;
  }
};

int main()
{
  CameraExplicitThis camera;
  camera.Set(42);
  return camera.value != 42;
}
