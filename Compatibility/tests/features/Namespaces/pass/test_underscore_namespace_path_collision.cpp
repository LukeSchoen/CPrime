/* A namespace path cannot be identified by joining names with underscores:
   the global namespace `a_b` and the nested namespace `a::b` both mangle to
   `__cpc_ns_a_b`.  Entering the nested namespace for its non-type template
   partial specialization must retain depth 2, not rewind to depth 1. */

namespace a_b
{
}

namespace a
{
namespace b
{

template <bool B>
struct selector;

template <>
struct selector<true>
{
};

}
}

int main()
{
  return 0;
}
