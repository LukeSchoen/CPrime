struct GlobalItem { int value; };
template<class T> int global_qualified_function() { return sizeof(T); }
template<class T> int global_qualified_argument(T value) { return sizeof(value); }
template<class T> static int internal_template_function(T value) { return sizeof(value); }
namespace { struct LocalIdentity { int value; }; }
template<class T> struct IdentityHolder {
    typedef T Value;
    typedef Value* Pointer;
    static int value() { return sizeof(Value); }
};
