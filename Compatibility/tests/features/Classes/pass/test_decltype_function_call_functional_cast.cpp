float DecltypeCastSource();

int main()
{
  float value = (decltype(DecltypeCastSource()))(3.0f);
  (void)value;
  return 0;
}
