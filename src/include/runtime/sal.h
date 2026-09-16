#ifndef _CPRIME_SAL_H
#define _CPRIME_SAL_H

/* SAL annotations are declarative only.  CPC removes them so the Windows
   headers can be parsed as ordinary C++ declarations. */
#define _In_
#define _In_opt_
#define _Inout_
#define _Inout_opt_
#define _Out_
#define _Out_opt_
#define _In_z_
#define _Inout_z_
#define _Out_z_
#define _Ret_maybenull_
#define _Ret_notnull_
#define _Ret_writes_(x)
#define _Ret_writes_bytes_(x)
#define _Ret_writes_maybenull_(x)
#define _Ret_maybenull_z_
#define _Notnull_
#define _Maybe_raises_SEH_exception_
#define _In_reads_(x)
#define _In_reads_opt_(x)
#define _In_reads_bytes_(x)
#define _In_reads_bytes_opt_(x)
#define _Out_writes_(x)
#define _Out_writes_opt_(x)
#define _Out_writes_bytes_(x)
#define _Out_writes_bytes_opt_(x)
#define _Out_writes_bytes_to_(x, y)
#define _Out_writes_to_(x, y)
#define _Out_writes_to_opt_(x, y)
#define _Outptr_
#define _Outptr_opt_
#define _Outptr_result_maybenull_
#define _Outptr_result_buffer_(x)
#define _Outptr_result_bytebuffer_(x)
#define _Outptr_result_z_
#define _Outptr_opt_result_maybenull_
#define _Outptr_opt_result_z_
#define _Field_size_(x)
#define _Field_size_opt_(x)
#define _Field_size_bytes_(x)
#define _Field_size_bytes_opt_(x)
#define _Post_equal_to_(x)
#define _Post_satisfies_(x)
#define _Pre_satisfies_(x)
#define _Out_range_(x, y)
#define _In_range_(x, y)
#define _Deref_pre_z_
#define _Deref_post_z_
#define _Inexpressible_(x)
#define _Always_(x)
#define _Return_type_success_(x)
#define _Success_(x)

#define _In_bytecount_(x)
#define _Inout_z_cap_(x)
#define _Out_z_cap_(x)
#define _Out_cap_(x)
#define _Out_bytecap_(x)
#define _Out_z_bytecap_(x)
#define _Printf_format_string_
#define _Scanf_format_string_impl_

#endif
