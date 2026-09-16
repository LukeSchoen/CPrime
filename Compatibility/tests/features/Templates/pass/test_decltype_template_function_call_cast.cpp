// EXPECT_EXIT: 0

float source_value(void)
{
  return 2.0f;
}

int main(void)
{
  float value = (decltype(source_value()))(3.0f);
  return value > 2.9f && value < 3.1f ? 0 : 1;
}
