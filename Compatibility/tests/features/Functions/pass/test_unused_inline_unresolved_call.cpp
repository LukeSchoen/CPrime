void external_function();
int external_value();
template<class T> void unused_template_target(T) { external_function(); }
template<class T> void unused_template_relay(T value) { unused_template_target(value); }
inline void unused_template_call() { unused_template_relay(1); }
inline auto unused_template_address() { return &unused_template_relay<int>; }
inline auto unused_template_decay() { return unused_template_relay<long>; }
inline decltype(auto) unused_template_reference() { return (unused_template_relay<char>); }
struct TemplateOwner {
    template<class T> static void call(T) { external_function(); }
};
inline void unused_member_template_call() { TemplateOwner::call(1); }
inline void unused_function() { external_function(); }
inline int unused_static() {
    static int value = external_value();
    return value;
}
struct ExternalLifetime {
    ExternalLifetime();
    ~ExternalLifetime();
};
inline void unused_static_object() { static ExternalLifetime value; }
inline void unused_local_class() {
    struct Local { void call() { external_function(); } };
    Local value;
    value.call();
}
struct Owner {
    void unused_member() { external_function(); }
};
int main() {}
