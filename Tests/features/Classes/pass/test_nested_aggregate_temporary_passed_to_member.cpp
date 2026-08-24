struct AggregateVec3
{
  float x;
  float y;
  float z;
};

struct AggregateAtom
{
  AggregateVec3 position;
  AggregateVec3 velocity;
};

namespace AggregateSupport
{
float Pass(float value)
{
  return value;
}
}

int CheckAggregate(AggregateAtom &&value)
{
  return (value.position.x == 3.0f ? 0 : 1)
      | (value.position.y == 6.0f ? 0 : 2)
      | (value.position.z == 9.0f ? 0 : 4)
      | (value.velocity.x == -3.0f ? 0 : 8)
      | (value.velocity.y == -12.0f ? 0 : 16)
      | (value.velocity.z == -15.0f ? 0 : 32);
}

int main()
{
  float index = 3.0f;

  return CheckAggregate(AggregateAtom{
    AggregateVec3{AggregateSupport::Pass(index), index * 2.0f, index * 3.0f},
    AggregateVec3{-index, -index * 4.0f, -index * 5.0f}
  });
}

// EXPECT_EXIT: 0
