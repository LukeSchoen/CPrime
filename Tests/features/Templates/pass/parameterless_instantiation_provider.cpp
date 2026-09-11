// The explicit instantiation below names this parameterless template through
// its declared result type.  The pointer declaration is more specialized than
// the plain T declaration, so it supplies the emitted body and its argument
// list (T = int) fixes the symbol the consumer links against.
template<class T> T pointer_value();
template<class T> T* pointer_value() { return 0; }

template int* pointer_value();
