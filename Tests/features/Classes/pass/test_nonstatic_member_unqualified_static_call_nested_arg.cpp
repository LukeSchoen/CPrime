// EXPECT_EXIT: 0

class CameraLike
{
public:
  void SetDirection(const int &value);
  void LookAt(const int &currentPos, const int &targetPos);
  static int Normalize(const int &value);
  int stored;
};

void CameraLike::SetDirection(const int &value)
{
  stored = value;
}

int CameraLike::Normalize(const int &value)
{
  return value;
}

void CameraLike::LookAt(const int &currentPos, const int &targetPos)
{
  SetDirection(Normalize(targetPos - currentPos));
}

int main()
{
  CameraLike camera;
  camera.LookAt(2, 5);
  return camera.stored == 3 ? 0 : 1;
}
