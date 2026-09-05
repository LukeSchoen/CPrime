// EXPECT_COMPILE_ARGS: -Werror
// EXPECT_SOURCES: ["global_qualified_identity_helper.cpp"]
#include "global_qualified_identity.h"
void* global_qualified_address();
void* global_qualified_argument_address();
void* internal_template_address();
void* internal_argument_address();
void* nested_internal_argument_address();
void* internal_member_address();
int main() {
    if (global_qualified_address() != (void*)&global_qualified_function<::GlobalItem>) return 1;
    if (global_qualified_argument_address() != (void*)&global_qualified_argument<::GlobalItem>) return 2;
    if (internal_template_address() == (void*)&internal_template_function<int>) return 3;
    if (internal_argument_address() == (void*)&global_qualified_function<LocalIdentity*>) return 4;
    if (nested_internal_argument_address() == (void*)&global_qualified_function<IdentityHolder<LocalIdentity>>) return 5;
    if (internal_member_address() == (void*)&IdentityHolder<LocalIdentity>::value) return 6;
    return 0;
}
