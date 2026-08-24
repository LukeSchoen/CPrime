struct Handle
{
  Handle(void *pointer, const long long &identifier)
    : m_pointer(pointer), m_identifier(identifier)
  {
  }

  bool m_active = true;
  long long m_identifier = -1;
  int m_values[8] = { 0 };
  void *m_pointer = nullptr;
};

int main()
{
  long long identifier = 7;
  Handle value((void *)0, identifier);
  return value.m_active && value.m_identifier == 7
         && value.m_values[0] == 0 && value.m_pointer == 0 ? 0 : 1;
}
