int Ready = 99;
namespace data {
  struct Holder {
    enum Axis { Zero = 0, Ready = 3 };
    struct Inner { enum Code : unsigned long long { High = 0xf123456789abcdefULL }; };
  };
  enum State { Start = 7, End = 9 };
}
using AxisAlias = data::Holder::Axis;
template<class T> struct Query {
  bool accepts(unsigned char axis) const;
};
template<class T> bool Query<T>::accepts(unsigned char axis) const
{
  return axis == data::Holder::Axis::Ready;
}
int main()
{
  Query<int> query;
  if (!query.accepts(3) || query.accepts(0)) return 1;
  if (data::Holder::Ready != 3 || AxisAlias::Ready != 3) return 2;
  if (data::State::Start != 7 || data::State::End != 9) return 3;
  if (data::Holder::Inner::Code::High != 0xf123456789abcdefULL) return 4;
  if ((true ? data::Holder::Axis::Ready : Ready) != 3) return 5;
  return 0;
}
