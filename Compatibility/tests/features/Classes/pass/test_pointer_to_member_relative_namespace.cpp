/* The owner of a pointer-to-member may be spelled through an enclosing
   namespace, including while that class is still incomplete:

     namespace detail {
     struct owner;
     typedef void (detail::owner::*member_fn)();
     }

   Resolving the relative namespace qualifier to its mangled namespace token
   made the replayed owner type unparsable because parse_btype only recognizes
   the written namespace spelling in a qualified type name. */
namespace outer {
namespace detail {

struct owner;
typedef void (detail::owner::*member_fn)();

struct owner
{
  void member() {}
};

} // namespace detail
} // namespace outer

int main()
{
  outer::detail::member_fn member = &outer::detail::owner::member;
  outer::detail::owner value;
  (value.*member)();
  return 0;
}
