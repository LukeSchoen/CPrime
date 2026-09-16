struct DifferentTokenOrder { int helper_only_name; };
#include "global_qualified_identity.h"
void* global_qualified_address() { return (void*)&global_qualified_function<::GlobalItem>; }
void* global_qualified_argument_address() { return (void*)&global_qualified_argument<::GlobalItem>; }
void* internal_template_address() { return (void*)&internal_template_function<int>; }
void* internal_argument_address() { return (void*)&global_qualified_function<LocalIdentity*>; }
void* nested_internal_argument_address() { return (void*)&global_qualified_function<IdentityHolder<LocalIdentity>>; }
void* internal_member_address() { return (void*)&IdentityHolder<LocalIdentity>::value; }
