template <typename> class holder;

template <typename target, typename source>
holder<target> convert(const holder<source> &);

template <typename type>
class holder {
 public:
  template <template <class, class> class caster, typename source>
  static holder<type> cast(const holder<source> &) {
    return holder<type>();
  }

  template <typename target, typename source>
  friend holder<target> convert(const holder<source> &);
};

template <class, class> class caster;

template <typename target, typename source>
holder<target> convert(const holder<source> &value) {
  return holder<target>::template cast<caster, source>(value);
}

int main() {
  holder<int> value;
  holder<const int> result = convert<const int>(value);
  return sizeof(result) != sizeof(value);
}
