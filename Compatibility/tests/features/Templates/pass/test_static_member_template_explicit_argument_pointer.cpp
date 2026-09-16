// EXPECT_EXIT: 0
// A static member template's explicit argument list selects the overload whose
// own template parameter list fits it. A shorter sibling declaration must not
// capture the call, even when the member is addressed inside a file-scope
// initializer that is replayed as a dynamic-initialization function.
namespace util {
struct option_value {
  bool value;
};
template <class T>
struct options_map_impl {
  typedef T options_struct_type;
  typedef bool (*opt_func)(const option_value&, options_struct_type&);

  template <class V, V K>
  static bool set_member_constant(const option_value&, options_struct_type& target,
                                  V options_struct_type::*member) {
    return target.*member == K;
  }

  template <class V, V options_struct_type::*member, V K>
  static bool set_member_constant(const option_value& opt,
                                  options_struct_type& target) {
    return set_member_constant<V, K>(opt, target, member);
  }
};
}

struct cflat_options {
  bool show_precharges;
};

typedef util::options_map_impl<cflat_options> options_map_impl_type;

class register_options_modifier {
  typedef options_map_impl_type::opt_func modifier_type;

public:
  register_options_modifier(const char*, const modifier_type com, const char*)
      : modifier(com) {}
  modifier_type modifier;
};

static const register_options_modifier cflat_opt_mod_show_precharges(
    "precharges",
    &options_map_impl_type::set_member_constant<bool,
                                                &cflat_options::show_precharges,
                                                true>,
    "show precharge expressions"),
    cflat_opt_mod_no_show_precharges(
        "no-precharges",
        &options_map_impl_type::set_member_constant<bool,
                                                    &cflat_options::show_precharges,
                                                    false>,
        "hide precharge expressions");

int main() {
  util::option_value visible = {true};
  cflat_options options = {true};
  if (!cflat_opt_mod_show_precharges.modifier(visible, options)) return 1;
  if (cflat_opt_mod_no_show_precharges.modifier(visible, options)) return 2;
  return 0;
}
