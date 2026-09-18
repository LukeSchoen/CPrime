// A qualified name whose path passes through an inline namespace names the
// entity the spelling that writes the inline namespace out names, at every
// level of the path and not only for the name at its end.  `boost::log`
// declares its `aux` namespace inside `inline namespace v2s_st`, so
// `boost::log::aux::light_function<...>` has to reach
// `boost::log::v2s_st::aux::light_function`; resolving only the last component
// left the qualifier spelled as written, and no declaration carried that name.
namespace api {
  inline namespace v1 {
    namespace detail {
      template< class T > struct handler;
      template< class R, class... Args > struct handler< R (Args...) > {
        typedef R result_type;
        static int id() { return 7; }
      };
    }
  }
}

typedef api::detail::handler< bool (int const&) > filter_type;

int main()
{
  filter_type direct;
  api::v1::detail::handler< bool (int const&) > spelled_out;
  return direct.id() == 7 && spelled_out.id() == 7 ? 0 : 1;
}
