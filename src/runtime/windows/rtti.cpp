#include <typeinfo>

extern "C" void *__cpc_dynamic_cast_reference(void *result)
{
    if (!result) throw std::bad_cast();
    return result;
}
