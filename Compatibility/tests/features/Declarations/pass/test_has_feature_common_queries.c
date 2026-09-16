/* The shared __has_feature()/__has_extension() spellings answer in C too, and
   the C++-only feature set stays unknown there. */
#if !defined(__has_feature) || !defined(__has_extension)
#error capability macros undefined
#endif
#if !__has_feature(tls) || !__has_extension(tls) || !__has_feature(__tls__)
#error tls feature missing
#endif
#if !__has_feature(enumerator_attributes) || !__has_extension(__enumerator_attributes__)
#error enumerator attribute feature missing
#endif
#if !__has_feature(attribute_deprecated_with_message)
#error deprecated attribute message feature missing
#endif
#if !__has_extension(attribute_unavailable_with_message)
#error unavailable attribute message extension missing
#endif
#if !__has_extension(gnu_asm_goto_with_outputs) || __has_feature(gnu_asm_goto_with_outputs)
#error extension-only feature misreported
#endif
#if __has_feature(cxx_lambdas) || __has_extension(cxx_lambdas)
#error c++ feature reported for c
#endif
#if __has_feature(unknown_feature) || __has_extension(unknown_feature)
#error unknown feature reported
#endif

int main(void)
{
    return __has_feature(tls) ? 0 : 1;
}
