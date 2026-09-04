int main()
{
  const int buffer_length = 10;
  char buffer[buffer_length] = { 0 };
  buffer[buffer_length - 1] = 'x';
  return buffer[0] == 0 && buffer[9] == 'x' ? 0 : 1;
}
