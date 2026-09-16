int main()
{
  volatile unsigned char data[12288];
  data[0] = 3;
  data[12287] = 7;
  return data[0] == 3 && data[12287] == 7 ? 0 : 1;
}
