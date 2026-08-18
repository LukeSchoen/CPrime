struct Value
{
  Value &operator +=(const Value &o);
  Value &operator -=(const Value &o);
  Value &operator *=(const Value &o);
  Value &operator /=(const Value &o);
  Value &operator >>=(const Value &o);
  Value &operator <<=(const Value &o);
};

int main()
{
  return 0;
}
