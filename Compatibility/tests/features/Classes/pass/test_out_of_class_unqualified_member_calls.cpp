struct Point
{
  int value;
};

struct Reach
{
  int offset;
  int Index(int value) const;
  int PointIndex(Point point) const;
  bool Contains(int value) const;
  int Calculate() const;
};

int Reach::Index(int value) const
{
  return value + offset;
}

int Reach::PointIndex(Point point) const
{
  return Index(point.value);
}

bool Reach::Contains(int value) const
{
  return Index(value) == 12;
}

int Reach::Calculate() const
{
  return PointIndex((Point){7});
}

int main()
{
  Reach reach{5};
  return reach.Contains(7) && reach.Calculate() == 12 ? 0 : 1;
}
