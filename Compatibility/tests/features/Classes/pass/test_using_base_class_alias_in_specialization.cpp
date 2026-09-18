/* boost/parameter's flat_like_arg_list<> names its base through a class-scope
   alias and then imports the base's members:

     template<class... T> class flat_like_arg_list {};
     template<> class flat_like_arg_list<>
       : public empty_arg_list
     {
       using _base_type = empty_arg_list;
      public:
       using _base_type::operator[];
       using _base_type::satisfies;
     };

   Parsing the explicit specialization's body did not keep the class-scope
   alias visible, so the using-declaration reported that it required a base
   class. */
struct ImportBase
{
  template<class Key> int operator[](Key const&) const { return 1; }
  template<class Key> int satisfies(Key const&) const { return 2; }
  int plain() const { return 3; }
};

template<class... T> class ImportList;

template<>
class ImportList<> : public ImportBase
{
  using base_type = ImportBase;

public:
  ImportList() {}
  using base_type::operator[];
  using base_type::plain;
  using base_type::satisfies;
};

int main()
{
  ImportList<> value;
  return value.plain() == 3 && value[0] == 1 && value.satisfies(0) == 2 ? 0 : 1;
}
