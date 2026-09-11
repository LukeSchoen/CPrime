#ifndef _CXXABI_H
#define _CXXABI_H 1

/* Itanium C++ ABI declarations. The runtime keeps its own type_info
   representation, but translation units that name the representation through
   the ABI spelling still need the declarations, as GCC's <cxxabi.h> provides
   them. Only the declared interface is offered here; the dynamic-cast entry
   point remains a call into the runtime. */

#include <typeinfo>
#include <stddef.h>

namespace __cxxabiv1
{
  class __shim_type_info : public std::type_info
  {
  public:
    virtual ~__shim_type_info ();

  protected:
    __shim_type_info () {}
  };

  class __class_type_info : public __shim_type_info
  {
  public:
    virtual ~__class_type_info ();
  };

  class __si_class_type_info : public __class_type_info
  {
  public:
    virtual ~__si_class_type_info ();
    const __class_type_info *__base_type;
  };

  class __vmi_class_type_info : public __class_type_info
  {
  public:
    virtual ~__vmi_class_type_info ();
    unsigned int __flags;
    unsigned int __base_count;

    struct __base_class_type_info
    {
      const __class_type_info *__base_type;
      long __offset_flags;
    };

    __base_class_type_info __base_info[1];
  };

  class __pbase_type_info : public __shim_type_info
  {
  public:
    virtual ~__pbase_type_info ();

    enum
    {
      __const_mask = 0x1,
      __volatile_mask = 0x2,
      __restrict_mask = 0x4,
      __incomplete_mask = 0x8,
      __incomplete_class_mask = 0x10,
      __transaction_safe_mask = 0x20
    };

    unsigned int __flags;
    const std::type_info *__pointee;
  };

  class __pointer_type_info : public __pbase_type_info
  {
  public:
    virtual ~__pointer_type_info ();
  };

  class __pointer_to_member_type_info : public __pbase_type_info
  {
  public:
    virtual ~__pointer_to_member_type_info ();
    const __class_type_info *__context;
  };

  extern "C" void *__dynamic_cast (const void *__src_ptr,
                                   const __class_type_info *__src_type,
                                   const __class_type_info *__dst_type,
                                   ptrdiff_t __src2dst_offset);
}

namespace abi = __cxxabiv1;

#endif
