template<typename T>
struct TraitValueInTemplateBody
{
  int Probe()
  {
    if (std::is_trivially_copyable<T>::value)
      return 1;
    return 0;
  }
};

typedef TraitValueInTemplateBody<int> IntTraitValueInTemplateBody;

int main()
{
  IntTraitValueInTemplateBody probe;
  return probe.Probe();
}
