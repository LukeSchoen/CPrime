#define USING_GLOBALS
#include "cprime.h"

//******************************************************
// Global Variables

/* loc : local variable index
   ind : output code index
   rsym: return symbol
   anon_sym: anonymous symbol index
*/
ST_DATA int rsym, anon_sym, ind, loc;

ST_DATA Sym *global_stack;
ST_DATA Sym *local_stack;
ST_DATA Sym *define_stack;
ST_DATA Sym *global_label_stack;
ST_DATA Sym *local_label_stack;

static Sym *sym_free_first;
static void **sym_pools;
static int nb_sym_pools;

static Sym *all_cleanups, *pending_gotos;
static int local_scope;
ST_DATA char debug_modes;

ST_DATA SValue *vtop;
static SValue _vstack[1 + VSTACK_SIZE];
#define vstack (_vstack + 1)

ST_DATA int nocode_wanted; // No Code Generation Wanted
#define NODATA_WANTED (nocode_wanted > 0) // No Static Data Output Wanted Either 
#define DATA_ONLY_WANTED 0x80000000 // ON outside of functions and for static initializers 

// no code output after unconditional jumps such as with if (0) ...
#define CODE_OFF_BIT 0x20000000
#define CODE_OFF() if(!nocode_wanted)(nocode_wanted |= CODE_OFF_BIT)
#define CODE_ON() (nocode_wanted &= ~CODE_OFF_BIT)

// No Code Output When Parsing Sizeof()/Typeof() Etc. (Using Nocode_Wanted++/--)
#define NOEVAL_MASK 0x0000FFFF
#define NOEVAL_WANTED (nocode_wanted & NOEVAL_MASK)

// No Code Output When Parsing Constant Expressions
#define CONST_WANTED_BIT  0x00010000
#define CONST_WANTED_MASK 0x0FFF0000
#define CONST_WANTED  (nocode_wanted & CONST_WANTED_MASK)

ST_DATA int global_expr;  // true if compound literals must be allocated globally (used during initializers parsing
ST_DATA CType func_vt; // Current Function Return Type (Used By Return Instruction)
ST_DATA int func_var; // true if current function is variadic (used by return instruction)
ST_DATA int func_vc;
ST_DATA int func_ind;
ST_DATA const char *funcname;
ST_DATA CType int_type, func_old_type, char_type, char_pointer_type;
static CString initstr;

#if PTR_SIZE == 4
#define VT_SIZE_T (VT_INT | VT_UNSIGNED)
#define VT_PTRDIFF_T VT_INT
#elif LONG_SIZE == 4
#define VT_SIZE_T (VT_LLONG | VT_UNSIGNED)
#define VT_PTRDIFF_T VT_LLONG
#else
#define VT_SIZE_T (VT_LONG | VT_LLONG | VT_UNSIGNED)
#define VT_PTRDIFF_T (VT_LONG | VT_LLONG)
#endif

static struct switch_t
{
  struct case_t
  {
    int64_t v1, v2;
    int ind, line;
  } **p; int n; // List Of Case Ranges
  int def_sym; // Default Symbol
  int nocode_wanted;
  int *bsym;
  struct scope *scope;
  struct switch_t *prev;
  SValue sv;
} *cur_switch; // Current Switch

#define MAX_TEMP_LOCAL_VARIABLE_NUMBER 8
// List Of Temporary Local Variables On The Stack In Current Function.
static struct temp_local_variable
{
  int location; // offset on stack. Svalue.c.i
  short size;
  short align;
} arr_temp_local_vars[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
static int nb_temp_local_vars;

static struct scope
{
  struct scope *prev;
  struct { int loc, locorig, num; } vla;
  struct { Sym *s; int n; } cl;
  int *bsym, *csym;
  Sym *lstk, *llstk;
  int stack_loc, stack_min;
} *cur_scope, *loop_scope, *root_scope;

typedef struct
{
  Section *sec;
  int local_offset;
  Sym *flex_array_ref;
  int capture_integral_constexpr;
  int integral_constexpr_valid;
  long long integral_constexpr_value;
} init_params;

#if 1
#define precedence_parser
static void init_prec(void);
#endif

static void block(int flags);
#define STMT_EXPR 1
#define STMT_COMPOUND 2

static void gen_cast(CType *type);
static void gen_cast_s(int t);
ST_FUNC void indir(void);
static inline CType *pointed_type(CType *type);
static int is_compatible_types(CType *type1, CType *type2);
static int parse_btype(CType *type, AttributeDef *ad, int ignore_label);
static CType *type_decl(CType *type, AttributeDef *ad, int *v, int td);
static void parse_expr_type(CType *type);
static void parse_decltype_type(CType *type);
static int make_type_from_type_arg_tok(CType *type, int type_tok);
static void struct_decl(CType *type, int u, int is_class_tag);
static void init_putv(init_params *p, CType *type, unsigned long c);
static void decl_initializer(init_params *p, CType *type, unsigned long c, int flags);
static void decl_initializer_alloc(CType *type, AttributeDef *ad, int r, int has_init,
                                   int has_ctor_init, TokenString *copy_ctor_init,
                                   int v, int decl_scope);
static int decl(int l);
static void expr_eq(void);
static void vpush_type_size(CType *type, int *a);
static int is_compatible_unqualified_types(CType *type1, CType *type2);
static int same_concrete_template_instantiation(CType *type1, CType *type2);
static inline int64_t expr_const64(void);
static void vpush64(int ty, unsigned long long v);
static void vpush(CType *type);
static void save_lvalues(void);
static void gfunc_param_typed(Sym *func, Sym *arg);
static int gvtst(int inv, int t);
static void type_to_str(char *buf, int buf_size, CType *type, const char *varstr);
static int get_struct_type_name_tok(CType *type);
static inline void convert_parameter_type(CType *pt);
static void gen_inline_functions(CPRIMEState *s);
static void free_inline_functions(CPRIMEState *s);
static void skip_or_save_block(TokenString **str);
static void skip_or_save_param_default(TokenString **str);
static void expand_saved_single_object_macro(TokenString **str);
static void qualify_saved_default_arg_current_class(TokenString **str);
static void gv_dup(void);
static void compile_pending_member_funcs(int start);
static int get_temp_local_var(int size, int align, int *r2);
static void cast_error(CType *st, CType *dt);
static void end_switch(void);
static void do_Static_assert(void);
static void parse_template_decl(void);
static void compile_pending_template_specs(void);
static void queue_demanded_template_member_bodies(void);
static int struct_has_member_init_list(int struct_tok);
static int token_string_contains_tok(TokenString *str, int needle);
static int saved_params_contains_name(TokenString *params, int name_tok);
static int body_identifier_is_decl_target(TokenString *body, int i,
                                          int prev_i, int next_i);
static Sym *resolve_member_func(CType *type, int method_tok);
static int member_func_explicit_arg_count(CType *lowered_type);
static Sym *resolve_member_func_by_arg_count(CType *type, int method_tok,
                                              int explicit_arg_count);
static Sym *resolve_initializer_list_constructor(CType *type);
static int class_has_initializer_list_constructor_candidate(CType *type);
static Sym *resolve_member_func_by_arg_types(CType *type, int method_tok,
                                             CType *arg_types,
                                             int explicit_arg_count);
static Sym *resolve_member_field_func_by_arg_types(CType *type, int method_tok,
                                                   CType *arg_types,
                                                   int explicit_arg_count);
static int member_overload_exists_for_call(CType *type, int method_tok,
                                           int explicit_arg_count);
static int member_overload_count_for_call(CType *type, int method_tok,
                                          int explicit_arg_count);
static int class_has_single_arg_constructor_for(CType *class_type,
                                                CType *arg_type);
static int member_func_arg_match_rank(Sym *s, CType *arg_types,
                                      int explicit_arg_count);
static int type_is_std_initializer_list(CType *type);
static int same_lowered_member_func_signature(CType *type1, CType *type2);
static int type_has_member_func_name(CType *type, int member_tok);
static int class_or_inst_has_member_template_name(int class_tok,
                                                  int member_tok);
static int is_lifecycle_member_tok(int t);
static int template_return_ctype_from_struct_tok(CType *ret_type, int struct_tok);
static int append_type_mangle(char *name, int name_size, CType *type);
static Sym *resolve_free_func_by_arg_types(int name_tok, CType *arg_types,
                                           int explicit_arg_count,
                                           int template_deduction_failed);
static Sym *resolve_free_func_by_arg_count(int name_tok,
                                           int explicit_arg_count,
                                           int template_deduction_failed);
static Sym *declare_static_member_func(CType *struct_type, int method_tok,
                                       CType *func_type);
static Sym *find_field_try(CType *type, int v, int *cumofs);
static Sym *find_static_member_try(CType *type, int member_tok, int *owner_tok);
typedef struct VirtualMethodInfo VirtualMethodInfo;
static Sym *find_static_member_by_class_try(int class_tok, int member_tok,
                                             int *owner_tok);
static int is_reference_type(CType *type);
static int class_has_static_member_func(int class_tok, int member_tok);
static int class_has_base(int class_tok, int base_tok);
static int class_subobject_offset(int class_tok, int base_tok, int *offset);
static void note_virtual_method(CType *class_type, int method_tok,
                                CType *func_type, int is_pure);
static int member_overrides_virtual_method(CType *class_type, int method_tok,
                                           CType *func_type);
static void emit_virtual_tables_for_class(int class_tok);
static void emit_virtual_table_relocations(void);
static void initialize_virtual_tables_for_pointer(CType *type,
                                                  SValue *base_ptr,
                                                  int object_offset);
static VirtualMethodInfo *find_virtual_method_for_class(
  int class_tok, int method_tok, CType *func_type);
static VirtualMethodInfo *find_virtual_method_for_symbol(int class_tok,
                                                         int mangled_tok);
static int virtual_vptr_field_tok(int root_tok);
static int virtual_vptr_offset(int class_tok, int root_tok);
static void push_virtual_call_target(int receiver_class_tok,
                                     VirtualMethodInfo *vm, Sym *func_sym);
static void instantiate_static_template_member_for_call(int class_mangled_tok,
                                                        int member_tok);
static int try_call_cpp_binary_operator(int op);
static int try_call_cpp_free_binary_operator(int op);
static int try_call_cpp_unary_minus_operator(void);
static int try_call_cpp_unary_operator(int op);
static int try_call_cpp_index_operator(void);
static int try_call_cpp_call_operator(void);
static int try_call_cpp_assignment_operator(void);
static int try_call_cpp_compound_assign_operator(int op);
static int try_call_cpp_bool_conversion_operator(void);
static int try_parse_cpp_functional_type_cast(int type_tok);
static CType make_lowered_member_func_type(CType *struct_type, CType *func_type);
static int struct_needs_memberwise_assignment(CType *type);
static void assign_struct_memberwise_from_base_ptr(CType *type, SValue *dst_ptr,
                                                   SValue *src_ptr,
                                                   int base_offset);
static int try_materialize_constructor_conversion(CType *type);
static Sym *resolve_copy_constructor_func(CType *type, CType *source_type);
static int struct_needs_memberwise_copy(CType *type);
static void copy_construct_struct_memberwise_from_base_ptr(CType *type,
                                                           SValue *dst_ptr,
                                                           SValue *src_ptr,
                                                           int base_offset,
                                                           int move_source);
static void note_defaulted_member_func(int struct_tok, int method_tok,
                                       CType *func_type);
static int is_defaulted_lifecycle_constructor(CType *struct_type,
                                              Sym *func_sym);
static void restore_cpp_lifecycle_probe(TokenString *replay);
static int make_type_from_saved_type_tokens(CType *type, TokenString *tokens);
static int tok_str_add_integer_const_sym(TokenString *str, Sym *s);
static void instantiate_template_member_body_for_func_tok(CType *type,
                                                          int method_tok,
                                                          int func_tok);
static int is_namespace_tok(int ns_tok);
static void note_namespace_tok(int ns_tok);
static void note_struct_member_init_list(int struct_tok);
static void note_auto_return_member_tok(int member_tok);
static int member_is_auto_return_tok(int member_tok);
static TokenString *parse_constructor_member_initializers(CType *struct_type);
static int constructor_field_was_explicitly_initialized(TokenString *fields,
                                                        int field_tok);
static void push_saved_param_scope(TokenString *params, Sym **saved_ls,
                                   int *saved_scope);
static void pop_saved_param_scope(Sym *saved_ls, int saved_scope);
static int is_template_keyword_tok(int t);
static CType make_func_type_from_saved_params(CType *ret_type,
                                              TokenString *params);

typedef struct TemplateArgList TemplateArgList;

typedef struct TemplateDef
{
  int name_tok;
  int lookup_tok;
  int *type_param_toks;
  int nb_type_params;
  int nb_required_type_params;
  int variadic_param_index;
  int func_min_args;
  int func_max_args;
  int func_is_variadic;
  unsigned func_sig_hash;
  int has_body;
  int is_partial_specialization;
  int is_class;
  TokenString *def_str;
  int *inst_type_toks;
  TemplateArgList *inst_arg_lists;
  int *inst_name_toks;
  unsigned char *inst_states;
  int nb_inst, al_inst;
  struct TemplateDef *lookup_bucket_next;
} TemplateDef;

typedef struct TemplateInstOwner
{
  int inst_tok;
  TemplateDef *owner;
  struct TemplateInstOwner *next;
} TemplateInstOwner;

struct TemplateArgList
{
  int toks[16];
  int nb;
};

static int infer_template_args_from_call(TemplateDef *td, CType *arg_types,
                                         int arg_count,
                                         TemplateArgList *args);
static TemplateDef *find_function_template_for_call(int name_tok,
                                                    CType *arg_types,
                                                    int arg_count);
static TemplateDef *find_class_template_def(int name_tok);
static TemplateDef *find_function_template_def(int name_tok);
static int template_def_first_param_matches_call(TemplateDef *td,
                                                 CType *arg_types,
                                                 int arg_count);
static int instantiate_template_if_needed(TemplateDef *td,
                                          TemplateArgList *args);
static void compile_pending_template_specs_without_member_flush(void);

typedef struct TemplateMemberDef
{
  int class_tok;
  int lookup_class_tok;
  int nested_class_tok;
  int method_tok;
  unsigned char method_tok_known;
  int type_param_tok;
  int stripped_member_type_param_toks[16];
  int nb_stripped_member_type_params;
  int stripped_member_value_param_toks[16];
  int nb_stripped_member_value_params;
  TokenString *def_str;
  int *inst_type_toks;
  int *inst_pack_toks;
  int *inst_func_toks;
  unsigned char *inst_states;
  CType *inst_call_arg_types;
  unsigned char *inst_call_arg_counts;
  int nb_inst, al_inst;
  int *inst_class_toks;
  CType *inst_ret_types;
  int nb_inst_ret_types, al_inst_ret_types;
  int *param_class_toks;
  int *param_type_toks;
  int *param_member_toks;
  CType *param_types;
  int nb_param_types, al_param_types;
  int instantiating;
  struct TemplateMemberDef *bucket_next;
} TemplateMemberDef;

typedef struct PendingTemplateMemberBodyRequest
{
  TemplateMemberDef *md;
  int type_tok;
  int class_mangled_tok;
  int member_type_arg_tok;
} PendingTemplateMemberBodyRequest;

typedef struct TemplateAliasInst
{
  int class_tok;
  int alias_tok;
  int scoped_tok;
} TemplateAliasInst;

typedef struct ClassBaseInfo
{
  int class_tok;
  int base_tok;
  Sym *field;
  struct ClassBaseInfo *next;
} ClassBaseInfo;

struct VirtualMethodInfo
{
  int class_tok;
  int root_tok;
  int method_tok;
  int mangled_tok;
  int explicit_arg_count;
  int slot;
  CType func_type;
  unsigned char is_pure;
  struct VirtualMethodInfo *next;
};

typedef struct VirtualTableInfo
{
  int class_tok;
  int root_tok;
  int symbol_tok;
  Sym *sym;
  unsigned long offset;
  int referenced;
  int relocations_emitted;
  struct VirtualTableInfo *next;
} VirtualTableInfo;

static int instantiate_template_if_needed(TemplateDef *td, TemplateArgList *args);
static CType make_template_func_type(int type_tok, int typed_first_param);
static int make_template_inst_name_tok(TemplateDef *td, TemplateArgList *args);
static int infer_template_return_struct_tok(TemplateDef *td,
                                             TemplateArgList *args);
static int infer_template_return_scalar_ctype(TemplateDef *td,
                                              TemplateArgList *args,
                                              CType *ret_type);
static int standard_type_trait_value(int trait_tok, TemplateArgList *args);
static void template_arg_list_one(TemplateArgList *args, int type_tok);
static int parse_template_type_arg(void);
static void parse_template_type_args(TemplateArgList *args);
static void parse_template_type_args_subst(TemplateArgList *out_args,
                                           TemplateDef *ctx_td,
                                           TemplateArgList *ctx_args);
static void instantiate_template_member_for_call(CType *type, int method_tok,
                                                CType *arg_types,
                                                int explicit_arg_count);
static void adjust_template_member_auto_return_for_call(CType *type,
                                                        int method_tok,
                                                        CType *arg_types,
                                                        int explicit_arg_count,
                                                        Sym *func_sym);
static void instantiate_template_member_if_needed(TemplateMemberDef *md,
                                                  int type_tok,
                                                  int class_mangled_tok,
                                                  int member_type_arg_tok);
static void instantiate_template_member_body_if_needed(TemplateMemberDef *md,
                                                       int type_tok,
                                                       int class_mangled_tok,
                                                       int member_type_arg_tok);
static void drain_template_member_body_requests(void);
static int template_member_def_method_tok(TemplateMemberDef *md);
static int template_member_def_is_scoped(TemplateMemberDef *md);
static int template_member_def_param_count(TemplateMemberDef *md,
                                           int method_tok);
static int token_string_has_initializer_list(TokenString *str);
static TemplateDef *find_class_template_def_for_class_tok(int class_tok);
static int is_class_template_instantiation_tok(int class_tok);
static int ctype_is_initializer_list(CType *type);
static void compile_pending_template_specs_without_member_flush(void);
static void move_ref_to_global(Sym *s);
static Sym *find_field_try(CType *type, int v, int *cumofs);
static Sym *find_field_try_with_owner(CType *type, int v, int *cumofs,
                                      int *owner_tok);
static int make_class_type_from_tok(CType *type, int class_tok);
static int class_or_inst_has_member_template_name(int class_tok,
                                                  int member_tok);
static int is_same_template_family_conversion_ctor(CType *class_type,
                                                    CType *arg_type);
static int same_template_family_compatible_elements(CType *type1, CType *type2);
static int same_template_family_instantiations(CType *type1, CType *type2);
static int try_materialize_same_template_family_conversion(CType *type);
static int template_lookup_inst(TemplateDef *td, TemplateArgList *args);
static void note_template_inst(TemplateDef *td, TemplateArgList *args,
                               int mangled_tok);
static void infer_saved_arg_types(TokenString **args, CType *types, int nb_args);
static void tok_str_append_without_eof(TokenString *dst, TokenString *src);
static int template_type_tok_from_ctype(CType *type);
static int template_type_arg_toks_match(int t1, int t2);

static void infer_expr_type_from_tokens(TokenString *expr, CType *type)
{
  int saved_nocode_wanted = nocode_wanted;
  int saved_tok = tok;
  CValue saved_tokc = tokc;
  TokenString macro;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;

  macro = *expr;
  if (getenv("CPC_TRACE_SHALLOW")
      && token_string_contains_tok(expr, tok_alloc_const("clVector2")))
    fprintf(stderr, "CPC_SHALLOW infer-expr func=%s len=%d\n",
            funcname ? funcname : "<none>", expr->len);
  nocode_wanted++;
  begin_macro(&macro, 0);
  next();
  expr_eq();
  *type = vtop->type;
  vpop();
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  nocode_wanted = saved_nocode_wanted;
}

static TemplateDef **template_defs;
static int nb_template_defs;
#define TEMPLATE_LOOKUP_BUCKETS 4096
static TemplateDef *template_def_buckets[TEMPLATE_LOOKUP_BUCKETS];
static TemplateDef *template_def_bucket_tails[TEMPLATE_LOOKUP_BUCKETS];
static TemplateInstOwner *template_inst_owner_buckets[TEMPLATE_LOOKUP_BUCKETS];
static TemplateInstOwner *template_inst_owner_bucket_tails[TEMPLATE_LOOKUP_BUCKETS];
static TemplateMemberDef **template_member_defs;
static int nb_template_member_defs;
static TemplateAliasInst *template_alias_insts;
static int nb_template_alias_insts;
static int al_template_alias_insts;
static TokenString **pending_template_specs;
static int *pending_template_spec_dependency_toks;
static int nb_pending_template_specs;
static int al_pending_template_specs;
static int suppress_template_member_flush;
static int pending_cpp_extern_linkage;
static int range_for_temp_counter;
static int compiled_template_specs;
static int compiling_pending_template_specs;
static int finalizing_template_bodies;
static int *member_init_list_struct_toks;
static int nb_member_init_list_struct_toks;
static int al_member_init_list_struct_toks;
static int *auto_return_member_toks;
static int nb_auto_return_member_toks;
static int al_auto_return_member_toks;
static int namespace_stack[16];
static int nb_namespace_stack;
static int *namespace_toks;
static int nb_namespace_toks;
static int *using_namespace_toks;
static int nb_using_namespace_toks;
static int defining_class_stack[32];
static int nb_defining_class_stack;
static int compiling_non_lifecycle_template_member_body;
static int materializing_template_interface;
static TemplateMemberDef *requested_template_member_body;
static PendingTemplateMemberBodyRequest *pending_template_member_body_requests;
static int nb_pending_template_member_body_requests;
static int al_pending_template_member_body_requests;
static int draining_template_member_body_requests;
static int last_decl_was_auto;
static CType template_member_call_arg_types[16];
static int nb_template_member_call_arg_types;
static int explicit_member_template_arg_tok;
static int explicitly_qualified_nonvirtual_class_tok;
static int explicitly_qualified_nonvirtual_method_tok;
static int template_member_extra_param_toks[15];
static int template_member_extra_arg_toks[15];
static int nb_template_member_extra_params;
static int last_btype_was_typedef;
static int last_btype_was_decltype;
static int suppress_integral_constexpr_fold;
static int integral_constant_expression_wanted;
static int last_instantiated_member_func_tok;
static ClassBaseInfo *class_base_infos;
static VirtualMethodInfo *virtual_method_infos;
static VirtualTableInfo *virtual_table_infos;

static void queue_pending_template_spec(TokenString *str, int dependency_tok)
{
  if (nb_pending_template_specs >= al_pending_template_specs)
  {
    al_pending_template_specs = al_pending_template_specs
                                ? al_pending_template_specs * 2 : 16;
    pending_template_specs = cprime_realloc(
      pending_template_specs,
      al_pending_template_specs * sizeof(*pending_template_specs));
    pending_template_spec_dependency_toks = cprime_realloc(
      pending_template_spec_dependency_toks,
      al_pending_template_specs
        * sizeof(*pending_template_spec_dependency_toks));
  }
  pending_template_specs[nb_pending_template_specs] = str;
  pending_template_spec_dependency_toks[nb_pending_template_specs] =
    dependency_tok;
  ++nb_pending_template_specs;
}

static int al_namespace_toks;
static int al_using_namespace_toks;

typedef struct PendingMemberFunc
{
  TokenString *str;
  int func_tok;
  int struct_tok;
  int is_template_member;
  int is_lifecycle_member;
  int is_static_member;
  int is_local_class_member;
} PendingMemberFunc;

static PendingMemberFunc **pending_member_funcs;
static int nb_pending_member_funcs;
static int defer_pending_member_funcs;
static int compiling_pending_member_funcs;
static int compile_local_member_funcs_now;
static int *compiled_pending_member_func_toks;
static int nb_compiled_pending_member_func_toks;
static int al_compiled_pending_member_func_toks;
static int compile_lifecycle_member_funcs_only;
static int nb_pending_global_inits;

typedef struct MemberFuncOverload
{
  int struct_tok;
  int method_tok;
  int mangled_tok;
  int explicit_arg_count;
  int min_arg_count;
  int is_const;
  CType func_type;
  struct MemberFuncOverload *next;
  struct MemberFuncOverload *bucket_next;
} MemberFuncOverload;

typedef struct FreeFuncOverload
{
  int name_tok;
  int mangled_tok;
  int explicit_arg_count;
  int min_arg_count;
  int is_provisional_auto_member;
  CType func_type;
  struct FreeFuncOverload *next;
} FreeFuncOverload;

static MemberFuncOverload *member_func_overloads;
#define MEMBER_CANDIDATE_BUCKETS 4096
static MemberFuncOverload *member_func_overload_buckets[MEMBER_CANDIDATE_BUCKETS];
static TemplateMemberDef *template_member_buckets[MEMBER_CANDIDATE_BUCKETS];
static TemplateMemberDef *template_member_bucket_tails[MEMBER_CANDIDATE_BUCKETS];
static unsigned long long profile_template_calls;
static unsigned long long profile_template_def_scans;
static unsigned long long profile_template_member_scans;
static unsigned long long profile_overload_scans;
static unsigned long long profile_other_overload_scans[8];
static int profile_scans_enabled;
#define MATERIALIZED_MEMBER_BODY_BUCKETS 4096
static int materialized_member_body_toks[MATERIALIZED_MEMBER_BODY_BUCKETS];
#define OVERLOAD_TOKEN_BUCKETS 8192
static int member_func_mangled_toks[OVERLOAD_TOKEN_BUCKETS];
static int free_func_name_toks[OVERLOAD_TOKEN_BUCKETS];
#define TEMPLATE_TYPE_CACHE_BUCKETS 2048
typedef struct TemplateTypeCacheEntry
{
  int type_flags;
  Sym *type_ref;
  CType stable_type;
  int type_tok;
} TemplateTypeCacheEntry;
static TemplateTypeCacheEntry
  template_type_cache[TEMPLATE_TYPE_CACHE_BUCKETS];
static FreeFuncOverload *free_func_overloads;

static unsigned member_candidate_bucket(int struct_tok, int method_tok)
{
  unsigned a = (unsigned)struct_tok;
  unsigned b = (unsigned)method_tok;
  return ((a * 2654435761u) ^ (b * 2246822519u))
         & (MEMBER_CANDIDATE_BUCKETS - 1);
}

static unsigned template_tok_bucket(int tok)
{
  return ((unsigned)tok * 2654435761u) & (TEMPLATE_LOOKUP_BUCKETS - 1);
}

/* Return -1 only when a completely full set cannot prove absence.  Callers
   retain their original list walk as an overflow fallback, keeping these
   fixed, allocation-free indexes semantically transparent. */
static int overload_tok_set_has(int *set, int tok)
{
  unsigned slot, start;

  if (!tok)
    return 0;
  start = slot = ((unsigned)tok * 2654435761u)
                 & (OVERLOAD_TOKEN_BUCKETS - 1);
  do
  {
    int candidate = set[slot];
    if (!candidate)
      return 0;
    if (candidate == tok)
      return 1;
    slot = (slot + 1) & (OVERLOAD_TOKEN_BUCKETS - 1);
  }
  while (slot != start);
  return -1;
}

static void overload_tok_set_note(int *set, int tok)
{
  unsigned slot, start;

  if (!tok)
    return;
  start = slot = ((unsigned)tok * 2654435761u)
                 & (OVERLOAD_TOKEN_BUCKETS - 1);
  do
  {
    if (!set[slot] || set[slot] == tok)
    {
      set[slot] = tok;
      return;
    }
    slot = (slot + 1) & (OVERLOAD_TOKEN_BUCKETS - 1);
  }
  while (slot != start);
}

static unsigned template_type_cache_bucket(CType *type)
{
  size_t ref = (size_t)type->ref;
  return ((unsigned)type->t * 2654435761u
          ^ (unsigned)(ref >> 4) ^ (unsigned)(ref >> 17))
         & (TEMPLATE_TYPE_CACHE_BUCKETS - 1);
}

static int template_type_cache_lookup(CType *type)
{
  unsigned slot, start;

  start = slot = template_type_cache_bucket(type);
  do
  {
    TemplateTypeCacheEntry *entry = &template_type_cache[slot];
    if (!entry->type_tok)
      return 0;
    if (entry->type_flags == type->t && entry->type_ref == type->ref)
    {
      /* Pointer nodes owned by a local symbol stack can later be reused.
         Validate the live key against the globally stable type snapshot
         before accepting an identity hit. */
      if (entry->stable_type.t == type->t
          && is_compatible_types(&entry->stable_type, type))
        return entry->type_tok;
      return 0;
    }
    slot = (slot + 1) & (TEMPLATE_TYPE_CACHE_BUCKETS - 1);
  }
  while (slot != start);
  return 0;
}

static void template_type_cache_note(CType *type, CType *stable_type,
                                     int type_tok)
{
  unsigned slot, start;

  if (!type_tok)
    return;
  start = slot = template_type_cache_bucket(type);
  do
  {
    TemplateTypeCacheEntry *entry = &template_type_cache[slot];
    if (!entry->type_tok
        || (entry->type_flags == type->t && entry->type_ref == type->ref))
    {
      entry->type_flags = type->t;
      entry->type_ref = type->ref;
      entry->stable_type = *stable_type;
      entry->type_tok = type_tok;
      return;
    }
    slot = (slot + 1) & (TEMPLATE_TYPE_CACHE_BUCKETS - 1);
  }
  while (slot != start);
}

static int materialized_member_body_has(int func_tok)
{
  unsigned slot, start;
  int i, j;

  if (!func_tok)
    return 0;
  start = slot = ((unsigned)func_tok * 2654435761u)
                 & (MATERIALIZED_MEMBER_BODY_BUCKETS - 1);
  do
  {
    int candidate = materialized_member_body_toks[slot];
    if (!candidate)
      return 0;
    if (candidate == func_tok)
      return 1;
    slot = (slot + 1) & (MATERIALIZED_MEMBER_BODY_BUCKETS - 1);
  }
  while (slot != start);

  /* A pathological translation unit can fill the fixed, allocation-free
     fast set.  Preserve the old exhaustive semantics in that rare case
     instead of risking a duplicate concrete member definition. */
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (!md || !md->inst_func_toks || !md->inst_states)
      continue;
    for (j = 0; j < md->nb_inst; ++j)
      if (md->inst_func_toks[j] == func_tok && md->inst_states[j] >= 2)
        return 1;
  }
  return 0;
}

static void materialized_member_body_note(int func_tok)
{
  unsigned slot, start;

  if (!func_tok)
    return;
  start = slot = ((unsigned)func_tok * 2654435761u)
                 & (MATERIALIZED_MEMBER_BODY_BUCKETS - 1);
  do
  {
    int candidate = materialized_member_body_toks[slot];
    if (!candidate || candidate == func_tok)
    {
      materialized_member_body_toks[slot] = func_tok;
      return;
    }
    slot = (slot + 1) & (MATERIALIZED_MEMBER_BODY_BUCKETS - 1);
  }
  while (slot != start);
}

static int template_def_lookup_tok(TemplateDef *td)
{
  return td->lookup_tok ? td->lookup_tok : td->name_tok;
}

static void index_template_def(TemplateDef *td)
{
  unsigned bucket = template_tok_bucket(template_def_lookup_tok(td));
  if (template_def_bucket_tails[bucket])
    template_def_bucket_tails[bucket]->lookup_bucket_next = td;
  else
    template_def_buckets[bucket] = td;
  template_def_bucket_tails[bucket] = td;
}

static TemplateDef *template_def_candidates(int name_tok)
{
  return template_def_buckets[template_tok_bucket(name_tok)];
}

static void index_template_inst_owner(TemplateDef *td, int inst_tok)
{
  unsigned bucket = template_tok_bucket(inst_tok);
  TemplateInstOwner *entry = cprime_mallocz(sizeof(*entry));
  entry->inst_tok = inst_tok;
  entry->owner = td;
  if (template_inst_owner_bucket_tails[bucket])
    template_inst_owner_bucket_tails[bucket]->next = entry;
  else
    template_inst_owner_buckets[bucket] = entry;
  template_inst_owner_bucket_tails[bucket] = entry;
}

static TemplateInstOwner *find_template_inst_owner_entry(int inst_tok)
{
  TemplateInstOwner *entry;
  for (entry = template_inst_owner_buckets[template_tok_bucket(inst_tok)];
       entry; entry = entry->next)
    if (entry->inst_tok == inst_tok)
      return entry;
  return NULL;
}

static MemberFuncOverload *member_func_candidates(int struct_tok,
                                                  int method_tok)
{
  return member_func_overload_buckets[
    member_candidate_bucket(struct_tok, method_tok)];
}

static void index_member_func_overload(MemberFuncOverload *o)
{
  unsigned bucket = member_candidate_bucket(o->struct_tok, o->method_tok);
  o->bucket_next = member_func_overload_buckets[bucket];
  member_func_overload_buckets[bucket] = o;
}

static TemplateMemberDef *template_member_candidates(int class_tok,
                                                      int method_tok)
{
  return template_member_buckets[
    member_candidate_bucket(class_tok, method_tok)];
}

static void index_template_member(TemplateMemberDef *md)
{
  unsigned bucket = member_candidate_bucket(md->class_tok, md->method_tok);
  if (template_member_bucket_tails[bucket])
    template_member_bucket_tails[bucket]->bucket_next = md;
  else
    template_member_buckets[bucket] = md;
  template_member_bucket_tails[bucket] = md;
}
static int *defaulted_member_struct_toks;
static int *defaulted_member_method_toks;
static CType *defaulted_member_func_types;
static int nb_defaulted_member_funcs;
static int al_defaulted_member_funcs;
static int tok_public;
static int tok_protected;
static int tok_private;
static int tok_explicit;
static int tok_mutable;
static int tok_constexpr;
static TokenString *last_cpp_conversion_operator_type_tokens;

static int tok_str_value_extra_words(const int *str, int len, int i)
{
  int t;

  if (!str || i < 0 || i >= len)
    return 0;
  t = str[i];
  switch (t)
  {
  case TOK_CINT:
  case TOK_CUINT:
  case TOK_CCHAR:
  case TOK_LCHAR:
  case TOK_CFLOAT:
  case TOK_LINENUM:
#if LONG_SIZE == 4
  case TOK_CLONG:
  case TOK_CULONG:
#endif
    return i + 1 < len ? 1 : 0;
  case TOK_CDOUBLE:
  case TOK_CLLONG:
  case TOK_CULLONG:
#if LONG_SIZE == 8
  case TOK_CLONG:
  case TOK_CULONG:
#endif
    return i + 2 < len ? 2 : 0;
  case TOK_CLDOUBLE:
#if LDOUBLE_SIZE == 8 || defined CPRIME_USING_DOUBLE_FOR_LDOUBLE
    return i + 2 < len ? 2 : 0;
#elif LDOUBLE_SIZE == 12
    return i + 3 < len ? 3 : 0;
#elif LDOUBLE_SIZE == 16
    return i + 4 < len ? 4 : 0;
#else
#error add long double size support
#endif
  case TOK_PPNUM:
  case TOK_PPSTR:
  case TOK_STR:
  case TOK_LSTR:
    if (i + 1 < len)
      return 1 + (str[i + 1] + (int)sizeof(int) - 1) / (int)sizeof(int);
    return 0;
  default:
    return 0;
  }
}

static void tok_str_add_record(TokenString *dst, const int *src, int len,
                               int *index)
{
  int i = *index;
  int extra = tok_str_value_extra_words(src, len, i);
  int end = i + extra;

  if (end >= len)
    end = len - 1;
  for (; i <= end; ++i)
    tok_str_add(dst, src[i]);
  *index = end;
}

static int tok_str_next_non_linenum_index(const int *str, int len, int i)
{
  while (i < len && str[i] == TOK_LINENUM)
    i += 1 + tok_str_value_extra_words(str, len, i);
  return i;
}

static int tok_str_prev_token_index(const int *str, int len, int i)
{
  int j, prev = -1;

  for (j = 0; j < i && j < len && str[j] != TOK_EOF;)
  {
    if (TOK_HAS_VALUE(str[j]))
      j += 1 + tok_str_value_extra_words(str, len, j);
    else
    {
      prev = j;
      ++j;
    }
  }
  return prev;
}

static int pending_member_func_has_tok(int func_tok)
{
  int i, j;

  for (i = 0; i < nb_pending_member_funcs; ++i)
  {
    PendingMemberFunc *pm = pending_member_funcs[i];
    if (!pm || !pm->str)
      continue;
    for (j = 0; j < pm->str->len; ++j)
      if (pm->str->str[j] == func_tok)
        return 1;
  }
  return 0;
}

static int compiled_pending_member_func_has_tok(int func_tok)
{
  int i;

  for (i = 0; i < nb_compiled_pending_member_func_toks; ++i)
    if (compiled_pending_member_func_toks[i] == func_tok)
      return 1;
  return 0;
}

static int pending_member_func_has_body_tok(int func_tok)
{
  int i, j;

  if (compiled_pending_member_func_has_tok(func_tok))
    return 1;

  for (i = 0; i < nb_pending_member_funcs; ++i)
  {
    PendingMemberFunc *pm = pending_member_funcs[i];
    int has_declarator_tok = 0;
    int paren = 0;
    if (!pm || !pm->str)
      continue;
    for (j = 0; j < pm->str->len; ++j)
    {
      int t = pm->str->str[j];
      if (TOK_HAS_VALUE(t))
      {
        j += tok_str_value_extra_words(pm->str->str, pm->str->len, j);
        continue;
      }
      if (t == func_tok && paren == 0)
        has_declarator_tok = 1;
      else if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if (paren == 0 && t == '{')
      {
        if (has_declarator_tok)
          return 1;
        /* This is another function's body.  A call to func_tok inside it is
           not a queued definition of func_tok. */
        break;
      }
      else if (paren == 0 && t == ';')
        break;
    }
  }
  return 0;
}

static void free_template_state(void)
{
  int i;
  MemberFuncOverload *o;
  FreeFuncOverload *fo;
  ClassBaseInfo *bi;
  VirtualMethodInfo *vm;
  VirtualTableInfo *vt;

  if (profile_scans_enabled)
    fprintf(stderr,
            "CPC_PROFILE template_calls=%llu template_defs=%llu template_members=%llu overloads=%llu defs=%d members=%d\n",
            profile_template_calls, profile_template_def_scans,
            profile_template_member_scans, profile_overload_scans,
            nb_template_defs, nb_template_member_defs);
  if (profile_scans_enabled)
    fprintf(stderr,
            "CPC_PROFILE other_overloads=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
            profile_other_overload_scans[0], profile_other_overload_scans[1],
            profile_other_overload_scans[2], profile_other_overload_scans[3],
            profile_other_overload_scans[4], profile_other_overload_scans[5],
            profile_other_overload_scans[6], profile_other_overload_scans[7]);

  /* Compiled pending specs are freed by end_macro() and their slots are
     NULLed during compile_pending_template_specs().  Entries that were
     queued but never compiled still own tokstr allocations and must be
     released here before the backing arena is torn down. */
  for (i = 0; i < nb_pending_template_specs; ++i)
    if (pending_template_specs[i])
      tok_str_free(pending_template_specs[i]);
  cprime_free(pending_template_specs);
  cprime_free(pending_template_spec_dependency_toks);
  pending_template_specs = NULL;
  pending_template_spec_dependency_toks = NULL;
  nb_pending_template_specs = 0;
  al_pending_template_specs = 0;

  for (i = 0; i < nb_pending_member_funcs; ++i)
  {
    PendingMemberFunc *pm = pending_member_funcs[i];
    if (!pm)
      continue;
    tok_str_free(pm->str);
    cprime_free(pm);
  }
  cprime_free(pending_member_funcs);
  pending_member_funcs = NULL;
  nb_pending_member_funcs = 0;
  cprime_free(compiled_pending_member_func_toks);
  compiled_pending_member_func_toks = NULL;
  nb_compiled_pending_member_func_toks = 0;
  al_compiled_pending_member_func_toks = 0;

  cprime_free(pending_template_member_body_requests);
  pending_template_member_body_requests = NULL;
  nb_pending_template_member_body_requests = 0;
  al_pending_template_member_body_requests = 0;

  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (!md)
      continue;
    tok_str_free(md->def_str);
    cprime_free(md->inst_type_toks);
    cprime_free(md->inst_pack_toks);
    cprime_free(md->inst_func_toks);
    cprime_free(md->inst_states);
    cprime_free(md->inst_call_arg_types);
    cprime_free(md->inst_call_arg_counts);
    cprime_free(md->inst_class_toks);
    cprime_free(md->inst_ret_types);
    cprime_free(md->param_class_toks);
    cprime_free(md->param_type_toks);
    cprime_free(md->param_member_toks);
    cprime_free(md->param_types);
    cprime_free(md);
  }
  cprime_free(template_member_defs);
  memset(template_member_buckets, 0, sizeof(template_member_buckets));
  memset(template_member_bucket_tails, 0,
         sizeof(template_member_bucket_tails));
  template_member_defs = NULL;
  nb_template_member_defs = 0;
  memset(materialized_member_body_toks, 0,
         sizeof(materialized_member_body_toks));

  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td)
      continue;
    tok_str_free(td->def_str);
    cprime_free(td->type_param_toks);
    cprime_free(td->inst_type_toks);
    cprime_free(td->inst_arg_lists);
    cprime_free(td->inst_name_toks);
    cprime_free(td->inst_states);
    cprime_free(td);
  }
  cprime_free(template_defs);
  memset(template_def_buckets, 0, sizeof(template_def_buckets));
  memset(template_def_bucket_tails, 0, sizeof(template_def_bucket_tails));
  for (i = 0; i < TEMPLATE_LOOKUP_BUCKETS; ++i)
  {
    TemplateInstOwner *entry = template_inst_owner_buckets[i];
    while (entry)
    {
      TemplateInstOwner *next = entry->next;
      cprime_free(entry);
      entry = next;
    }
  }
  memset(template_inst_owner_buckets, 0,
         sizeof(template_inst_owner_buckets));
  memset(template_inst_owner_bucket_tails, 0,
         sizeof(template_inst_owner_bucket_tails));
  template_defs = NULL;
  nb_template_defs = 0;

  cprime_free(member_init_list_struct_toks);
  member_init_list_struct_toks = NULL;
  nb_member_init_list_struct_toks = 0;
  al_member_init_list_struct_toks = 0;

  cprime_free(namespace_toks);
  namespace_toks = NULL;
  nb_namespace_toks = 0;
  al_namespace_toks = 0;
  cprime_free(using_namespace_toks);
  using_namespace_toks = NULL;
  nb_using_namespace_toks = 0;
  al_using_namespace_toks = 0;

  while (member_func_overloads)
  {
    o = member_func_overloads;
    member_func_overloads = o->next;
    cprime_free(o);
  }
  memset(member_func_overload_buckets, 0,
         sizeof(member_func_overload_buckets));
  memset(member_func_mangled_toks, 0, sizeof(member_func_mangled_toks));
  while (free_func_overloads)
  {
    fo = free_func_overloads;
    free_func_overloads = fo->next;
    cprime_free(fo);
  }
  memset(free_func_name_toks, 0, sizeof(free_func_name_toks));
  memset(template_type_cache, 0, sizeof(template_type_cache));
  while (class_base_infos)
  {
    bi = class_base_infos;
    class_base_infos = bi->next;
    cprime_free(bi);
  }
  while (virtual_method_infos)
  {
    vm = virtual_method_infos;
    virtual_method_infos = vm->next;
    cprime_free(vm);
  }
  while (virtual_table_infos)
  {
    vt = virtual_table_infos;
    virtual_table_infos = vt->next;
    cprime_free(vt);
  }

  compiled_template_specs = 0;
  defer_pending_member_funcs = 0;
  nb_namespace_stack = 0;
}

// -------------------------------------------------------------------------
// Automagical code suppression

// Clear 'nocode_wanted' at forward label if it was used
ST_FUNC void gsym(int t)
{
  if (t)
  {
    gsym_addr(t, ind);
    x86_64_asm_label(t);
    CODE_ON();
  }
}

// Clear 'nocode_wanted' if current pc is a label
static int gind()
{
  int t = ind;
  CODE_ON();
  if (debug_modes)
    cprime_tcov_block_begin(cprime_state);
  return t;
}

// Set 'nocode_wanted' after unconditional (backwards) jump
static void gjmp_addr_acs(int t)
{
  gjmp_addr(t);
  CODE_OFF();
}

// Set 'nocode_wanted' after unconditional (forwards) jump
static int gjmp_acs(int t)
{
  t = gjmp(t);
  CODE_OFF();
  return t;
}

// These are #undef'd at the end of this file
#define gjmp_addr gjmp_addr_acs
#define gjmp gjmp_acs
// -------------------------------------------------------------------------

ST_INLN int is_float(int t)
{
  int bt = t &VT_BTYPE;
  return bt == VT_LDOUBLE
         || bt == VT_DOUBLE
         || bt == VT_FLOAT
         || bt == VT_QFLOAT;
}

static inline int is_integer_btype(int bt)
{
  return bt == VT_BYTE
         || bt == VT_BOOL
         || bt == VT_SHORT
         || bt == VT_INT
         || bt == VT_LLONG;
}

static int btype_size(int bt)
{
  return bt == VT_BYTE || bt == VT_BOOL ? 1 :
         bt == VT_SHORT ? 2 :
         bt == VT_INT ? 4 :
         bt == VT_LLONG ? 8 :
         bt == VT_PTR ? PTR_SIZE : 0;
}

// Returns Function Return Register From Type
static int R_RET(int t)
{
  if (!is_float(t))
    return REG_IRET;
#ifdef CPRIME_TARGET_X86_64
  if ((t & VT_BTYPE) == VT_LDOUBLE)
    return TREG_ST0;
#elif defined CPRIME_TARGET_RISCV64
  if ((t & VT_BTYPE) == VT_LDOUBLE)
    return REG_IRET;
#endif
  return REG_FRET;
}

// returns 2nd function return register, if any
static int R2_RET(int t)
{
  t &= VT_BTYPE;
#if PTR_SIZE == 4
  if (t == VT_LLONG)
    return REG_IRE2;
#elif defined CPRIME_TARGET_X86_64
  if (t == VT_QLONG)
    return REG_IRE2;
  if (t == VT_QFLOAT)
    return REG_FRE2;
#elif defined CPRIME_TARGET_RISCV64
  if (t == VT_LDOUBLE)
    return REG_IRE2;
#endif
  return VT_CONST;
}

// Returns True For Two-Word Types
#define USING_TWO_WORDS(t) (R2_RET(t) != VT_CONST)

// Put Function Return Registers To Stack Value
static void PUT_R_RET(SValue *sv, int t)
{
  sv->r = R_RET(t), sv->r2 = R2_RET(t);
}

// Returns Function Return Register Class For Type T
static int RC_RET(int t)
{
  return reg_classes[R_RET(t)] & ~(RC_FLOAT | RC_INT);
}

// Returns Generic Register Class For Type T
static int RC_TYPE(int t)
{
  if (!is_float(t))
    return RC_INT;
#ifdef CPRIME_TARGET_X86_64
  if ((t & VT_BTYPE) == VT_LDOUBLE)
    return RC_ST0;
  if ((t & VT_BTYPE) == VT_QFLOAT)
    return RC_FRET;
#elif defined CPRIME_TARGET_RISCV64
  if ((t & VT_BTYPE) == VT_LDOUBLE)
    return RC_INT;
#endif
  return RC_FLOAT;
}

// Returns 2Nd Register Class Corresponding To T And Rc
static int RC2_TYPE(int t, int rc)
{
  if (!USING_TWO_WORDS(t))
    return 0;
#ifdef RC_IRE2
  if (rc == RC_IRET)
    return RC_IRE2;
#endif
#ifdef RC_FRE2
  if (rc == RC_FRET)
    return RC_FRE2;
#endif
  if (rc & RC_FLOAT)
    return RC_FLOAT;
  return RC_INT;
}

/* we use our own 'finite' function to avoid potential problems with
   non standard math libs */
// XXX: endianness dependent
ST_FUNC int ieee_finite(double d)
{
  int p[4];
  memcpy(p, &d, sizeof(double));
  return ((unsigned)((p[1] | 0x800fffff) + 1)) >> 31;
}

// compiling intel long double natively
#if (defined __i386__ || defined __x86_64__) \
    && (defined CPRIME_TARGET_I386 || defined CPRIME_TARGET_X86_64)
# define CPRIME_IS_NATIVE_387
#endif

ST_FUNC void test_lvalue(void)
{
  if (!(vtop->r & VT_LVAL))
    expect("lvalue");
}

ST_FUNC void check_vstack(void)
{
  if (vtop != vstack - 1)
    cprime_error("internal compiler error: vstack leak (%d)",
              (int)(vtop - vstack + 1));
}

#if 0
// Dump 'B' Vstack Entries Starting With 'A'
void pv (const char *lbl, int a, int b)
{
  int i;
  for (i = a; i < a + b; ++i)
  {
    SValue *p = &vtop[-i];
    printf("%s vtop[-%d] : type.t:%04x  r:%04x  r2:%04x  c.i:%d\n",
           lbl, i, p->type.t, p->r, p->r2, (int)p->c.i);
  }
}

// Dump Symbols On Stack From S ... Last
static inline void psyms(const char *msg, Sym *s, Sym *last)
{
  printf("%-8s scope         v        c        r   type.t\n", msg);
  while (s && s != last)
  {
    printf("      %8x  %08x %08x %08x %08x %s\n",
           s->sym_scope, s->v, s->c, s->r, s->type.t, get_tok_str(s->v, 0));
    s = s->prev;
  }
}

static void type_to_str(char *buf, int buf_size, CType *type, const char *varstr);

// Print Type
static void ptype(const char *msg, CType *type, int v)
{
  char buf[500];
  type_to_str(buf, sizeof(buf), type,
              (v & ~SYM_FIELD) ? get_tok_str(v, NULL) : NULL);
  printf("%s : %s;\n", msg, buf);
}
#endif

// -------------------------------------------------------------------------
// initialize vstack and types.  This must be done also for cpc -E
ST_FUNC void cprimegen_init(CPRIMEState *s1)
{
  profile_scans_enabled = getenv("CPC_PROFILE_SCANS") != NULL;
  vtop = vstack - 1;
  memset(vtop, 0, sizeof *vtop);

  // Define Some Often Used Types
  int_type.t = VT_INT;

  char_type.t = VT_BYTE;
  if (s1->char_is_unsigned)
    char_type.t |= VT_UNSIGNED;
  char_pointer_type = char_type;
  mk_pointer(&char_pointer_type);

  func_old_type.t = VT_FUNC;
  func_old_type.ref = sym_push(SYM_FIELD, &int_type, 0, 0);
  func_old_type.ref->f.func_call = FUNC_CDECL;
  func_old_type.ref->f.func_type = FUNC_OLD;
  tok_public = tok_alloc_const("public");
  tok_protected = tok_alloc_const("protected");
  tok_private = tok_alloc_const("private");
  tok_explicit = tok_alloc_const("explicit");
  tok_mutable = tok_alloc_const("mutable");
  tok_constexpr = tok_alloc_const("constexpr");
#ifdef precedence_parser
  init_prec();
#endif
  cstr_new(&initstr);
}

ST_FUNC int cprimegen_compile(CPRIMEState *s1)
{
  funcname = "";
  func_ind = -1;
  anon_sym = SYM_FIRST_ANOM;
  nocode_wanted = DATA_ONLY_WANTED; // No Code Outside Of Functions
  debug_modes = (s1->do_debug ? 1 : 0) | s1->test_coverage << 1;
  global_expr = 0;

  cprime_debug_start(s1);
  cprime_tcov_start (s1);
#ifdef CPRIME_TARGET_ARM
  arm_init(s1);
#endif
#ifdef INC_DEBUG
  printf("%s: **** new file\n", file->filename);
#endif
  parse_flags = PARSE_FLAG_PREPROCESS | PARSE_FLAG_TOK_NUM | PARSE_FLAG_TOK_STR;
  next();
  decl(VT_CONST);
  finalizing_template_bodies = 1;
  for (;;)
  {
    int old_compiled_specs = compiled_template_specs;
    int old_pending_members = nb_pending_member_funcs;
    compile_pending_template_specs();
    queue_demanded_template_member_bodies();
    if (nb_pending_member_funcs)
      compile_pending_member_funcs(0);
    drain_template_member_body_requests();
    drain_template_member_body_requests();
    if (!nb_pending_template_member_body_requests
        && old_compiled_specs == compiled_template_specs
        && old_pending_members == nb_pending_member_funcs)
      break;
  }
  /* Materialize vtable function references only after deferred member and
     template bodies have been replayed.  Emitting these relocations while a
     class is still being parsed can prematurely publish pending member
     symbols and corrupt later overload resolution.  Do this before inline
     emission so the references also force virtual inline bodies to be kept. */
  emit_virtual_table_relocations();
  gen_inline_functions(s1);
  /* An emitted inline body can be the first place a polymorphic object is
     constructed.  Pick up any tables it made live, then let those new
     relocations force their inline virtual targets in turn. */
  emit_virtual_table_relocations();
  gen_inline_functions(s1);
  finalizing_template_bodies = 0;
  check_vstack();
  // End Of Translation Unit Info
#if CPRIME_EH_FRAME
  cprime_eh_frame_end(s1);
#endif
  cprime_debug_end(s1);
  cprime_tcov_end(s1);
  return 0;
}

ST_FUNC void cprimegen_finish(CPRIMEState *s1)
{
  cprime_debug_end(s1); // Just In Case Of Errors: Free Memory
  free_inline_functions(s1);
  free_template_state();
  cprime_asm_reset();
  sym_pop(&global_stack, NULL, 0);
  sym_pop(&local_stack, NULL, 0);
  // Free Preprocessor Macros
  free_defines(NULL);
  // Free Sym_Pools
  dynarray_reset(&sym_pools, &nb_sym_pools);
  cstr_free(&initstr);
  dynarray_reset(&stk_data, &nb_stk_data);
  while (cur_switch)
    end_switch();
  local_scope = 0;
  loop_scope = NULL;
  all_cleanups = NULL;
  pending_gotos = NULL;
  nb_temp_local_vars = 0;
  global_label_stack = NULL;
  local_label_stack = NULL;
  cur_text_section = NULL;
  sym_free_first = NULL;
}

// -------------------------------------------------------------------------
ST_FUNC ObjSym *elfsym(Sym *s)
{
  if (!s || !s->c)
    return NULL;
  return &((ObjSym *)symtab_section->data)[s->c];
}

// apply storage attributes to Elf symbol
ST_FUNC void update_storage(Sym *sym)
{
  ObjSym *esym;
  int sym_bind, old_sym_bind;

  esym = elfsym(sym);
  if (!esym)
    return;

  if (sym->a.visibility)
    esym->st_other = (esym->st_other & ~Obj64_ST_VISIBILITY(-1))
                     | sym->a.visibility;

  if (sym->type.t & VT_STATIC)
    sym_bind = STB_LOCAL;
  else if (sym->type.t & VT_INLINE)
    sym_bind = STB_WEAK;
  else if (sym->a.weak)
    sym_bind = STB_WEAK;
  else
    sym_bind = STB_GLOBAL;
  old_sym_bind = Obj64_ST_BIND(esym->st_info);
  if (sym_bind != old_sym_bind)
    esym->st_info = Obj64_ST_INFO(sym_bind, Obj64_ST_TYPE(esym->st_info));

#ifdef CPRIME_TARGET_PE
  if (sym->a.dllimport)
    esym->st_other |= ST_PE_IMPORT;
  if (sym->a.dllexport)
    esym->st_other |= ST_PE_EXPORT;
#endif

#if 0
  printf("storage %s: bind=%c vis=%d exp=%d imp=%d\n",
         get_tok_str(sym->v, NULL),
         sym_bind == STB_WEAK ? 'w' : sym_bind == STB_LOCAL ? 'l' : 'g',
         sym->a.visibility,
         sym->a.dllexport,
         sym->a.dllimport
        );
#endif
}

// -------------------------------------------------------------------------
/* update sym->c so that it points to an external symbol in section
   'section' with value 'value' */

ST_FUNC void put_extern_sym2(Sym *sym, int sh_num,
                             addr_t value, unsigned long size,
                             int can_add_underscore)
{
  int sym_type, sym_bind, info, other, t;
  ObjSym *esym;
  const char *name;
  char buf1[256];

  if (!sym->c)
  {
    name = get_tok_str(sym->v, NULL);
    t = sym->type.t;
    if ((t & VT_BTYPE) == VT_FUNC)
      sym_type = STT_FUNC;
    else if ((t & VT_BTYPE) == VT_VOID)
    {
      sym_type = STT_NOTYPE;
      if (IS_ASM_FUNC(t))
        sym_type = STT_FUNC;
    }
    else
      sym_type = STT_OBJECT;
    if (t & VT_STATIC)
      sym_bind = STB_LOCAL;
    else if (t & VT_INLINE)
      sym_bind = STB_WEAK;
    else
      sym_bind = STB_GLOBAL;
    other = 0;

#ifdef CPRIME_TARGET_PE
    if (sym_type == STT_FUNC && sym->type.ref)
    {
      Sym *ref = sym->type.ref;
      if (ref->a.nodecorate)
        can_add_underscore = 0;
      if (ref->f.func_call == FUNC_STDCALL && can_add_underscore)
      {
        sprintf(buf1, "_%s@%d", name, ref->f.func_args *PTR_SIZE);
        name = buf1;
        other |= ST_PE_STDCALL;
        can_add_underscore = 0;
      }
    }
#endif

    if (sym->asm_label)
    {
      name = get_tok_str(sym->asm_label, NULL);
      can_add_underscore = 0;
    }

    if (cprime_state->leading_underscore && can_add_underscore)
    {
      buf1[0] = '_';
      pstrcpy(buf1 + 1, sizeof(buf1) - 1, name);
      name = buf1;
    }

    info = Obj64_ST_INFO(sym_bind, sym_type);
    sym->c = put_elf_sym(symtab_section, value, size, info, other, sh_num, name);

    if (debug_modes)
      cprime_debug_extern_sym(cprime_state, sym, sh_num, sym_bind, sym_type);

  }
  else
  {
    esym = elfsym(sym);
    esym->st_value = value;
    esym->st_size = size;
    esym->st_shndx = sh_num;
  }
  update_storage(sym);
}

ST_FUNC void put_extern_sym(Sym *sym, Section *s, addr_t value, unsigned long size)
{
  if (nocode_wanted && (NODATA_WANTED || (s && s == cur_text_section)))
    return;
  put_extern_sym2(sym, s ? s->sh_num : SHN_UNDEF, value, size, 1);
}

// Add A New Relocation Entry To Symbol 'Sym' In Section 'S'
ST_FUNC void greloca(Section *s, Sym *sym, unsigned long offset, int type,
                     addr_t addend)
{
  int c = 0;

  if (nocode_wanted && s == cur_text_section)
    return;

  if (sym)
  {
    if (0 == sym->c)
    {
      put_extern_sym(sym, NULL, 0, 0);
      if (sym->sym_scope
          && (sym->type.t & (VT_STATIC | VT_EXTERN)) == (VT_STATIC | VT_EXTERN))
      {
        /* when a local function declaraion redeclares a global static one
           then cprimeelf would not resolve them to the same symbol. */
        Sym *s = sym;
        while (s->prev_tok)
          s = s->prev_tok;
        s->c = sym->c;
      }
    }
    c = sym->c;
  }

  // now we can add ELF relocation info
  put_elf_reloca(symtab_section, s, offset, type, c, addend);
}

#if PTR_SIZE == 4
ST_FUNC void greloc(Section *s, Sym *sym, unsigned long offset, int type)
{
  greloca(s, sym, offset, type, 0);
}
#endif

// -------------------------------------------------------------------------
// Symbol Allocator
static Sym *__sym_malloc(void)
{
  Sym *sym_pool, *sym, *last_sym;
  int i;

  sym_pool = cprime_malloc(SYM_POOL_NB *sizeof(Sym));
  dynarray_add(&sym_pools, &nb_sym_pools, sym_pool);

  last_sym = sym_free_first;
  sym = sym_pool;
  for (i = 0; i < SYM_POOL_NB; i++)
  {
    sym->next = last_sym;
    last_sym = sym;
    sym++;
  }
  sym_free_first = last_sym;
  return last_sym;
}

static inline Sym *sym_malloc(void)
{
  Sym *sym;
#ifndef SYM_DEBUG
  sym = sym_free_first;
  if (!sym)
    sym = __sym_malloc();
  sym_free_first = sym->next;
  return sym;
#else
  sym = cprime_malloc(sizeof(Sym));
  return sym;
#endif
}

ST_INLN void sym_free(Sym *sym)
{
#ifndef SYM_DEBUG
  sym->next = sym_free_first;
  sym_free_first = sym;
#else
  cprime_free(sym);
#endif
}

// Push, Without Hashing
ST_FUNC Sym *sym_push2(Sym **ps, int v, int t, int c)
{
  Sym *s;

  s = sym_malloc();
  memset(s, 0, sizeof *s);
  s->v = v;
  s->type.t = t;
  s->c = c;
  // Add In Stack
  s->prev = *ps;
  *ps = s;
  return s;
}

/* find a symbol and return its associated structure. 's' is the top
   of the symbol stack */
ST_FUNC Sym *sym_find2(Sym *s, int v)
{
  while (s)
  {
    if (s->v == v)
      return s;
    s = s->prev;
  }
  return NULL;
}

// Structure Lookup
ST_INLN Sym *struct_find(int v)
{
  v -= TOK_IDENT;
  if ((unsigned)v >= (unsigned)(tok_ident - TOK_IDENT))
    return NULL;
  return table_ident[v]->sym_struct;
}

static int find_any_nested_struct_type_tok(int name_tok)
{
  const char *name = get_tok_str(name_tok, NULL);
  int name_len = (int)strlen(name);
  int i;

  for (i = 0; i < tok_ident - TOK_IDENT; ++i)
  {
    int candidate_tok = i + TOK_IDENT;
    const char *candidate;
    int candidate_len;

    if (!table_ident[i]->sym_struct)
      continue;
    candidate = get_tok_str(candidate_tok, NULL);
    candidate_len = (int)strlen(candidate);
    if (candidate_len > name_len
        && candidate[candidate_len - name_len - 1] == '_'
        && !strcmp(candidate + candidate_len - name_len, name))
      return candidate_tok;
  }
  return 0;
}

// Find An Identifier
ST_INLN Sym *sym_find(int v)
{
  v -= TOK_IDENT;
  if ((unsigned)v >= (unsigned)(tok_ident - TOK_IDENT))
    return NULL;
  return table_ident[v]->sym_identifier;
}

// Make Sym In-/Visible To The Parser
static inline void sym_link(Sym *s, int yes)
{
  TokenSym *ts = table_ident[(s->v & ~SYM_STRUCT) - TOK_IDENT];
  Sym **ps;
  if (s->v & SYM_STRUCT)
    ps = &ts->sym_struct;
  else
    ps = &ts->sym_identifier;
  if (yes)
  {
    s->prev_tok = *ps, *ps = s;
    s->sym_scope = local_scope;
  }
  else
    *ps = s->prev_tok;
}

static inline int sym_scope_ex(Sym *s)
{
  // Enums Have 'Sym_Scope' Overwritten By 'Enum_Val'
  return IS_ENUM_VAL (s->type.t)
         ? s->type.ref->sym_scope
         : s->sym_scope;
}

// Push A Given Symbol On The Symbol Stack
ST_FUNC Sym *sym_push(int v, CType *type, int r, int c)
{
  Sym *s, **ps;
  if (local_stack)
    ps = &local_stack;
  else
    ps = &global_stack;
  s = sym_push2(ps, v, type->t, c);
  s->type.ref = type->ref;
  s->r = r;
  // don't record fields or anonymous symbols
  if ((v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
  {
    // Record Symbol In Token Array
    sym_link(s, 1);
    if (s->prev_tok && sym_scope_ex(s->prev_tok) == local_scope)
      cprime_error("redeclaration of '%s'", get_tok_str(s->v, NULL));
  }
  return s;
}

// Push A Global Identifier
ST_FUNC Sym *global_identifier_push(int v, int t, int c)
{
  Sym *s, **ps;
  s = sym_push2(&global_stack, v, t, c);
  s->r = VT_CONST | VT_SYM;
  // don't record anonymous symbol
  if (v < SYM_FIRST_ANOM)
  {
    ps = &table_ident[v - TOK_IDENT]->sym_identifier;
    /* modify the top most local identifier, so that sym_identifier will
       point to 's' when popped; happens when called from inline asm */
    while (*ps != NULL && (*ps)->sym_scope)
      ps = &(*ps)->prev_tok;
    s->prev_tok = *ps;
    *ps = s;
  }
  return s;
}

/* pop symbols until top reaches 'b'.  If KEEP is non-zero don't really
   pop them yet from the list, but do remove them from the token array.  */
ST_FUNC void sym_pop(Sym **ptop, Sym *b, int keep)
{
  Sym *s, *ss;
  int v;

  s = *ptop;
  while (s != b)
  {
    ss = s->prev;
    v = s->v;
    // Remove Symbol In Token Array
    if ((v & ~SYM_STRUCT) >= TOK_IDENT
        && (v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
      sym_link(s, 0);
    if (!keep)
      sym_free(s);
    s = ss;
  }
  if (!keep)
    *ptop = b;
}

// Label Lookup
ST_FUNC Sym *label_find(int v)
{
  v -= TOK_IDENT;
  if ((unsigned)v >= (unsigned)(tok_ident - TOK_IDENT))
    return NULL;
  return table_ident[v]->sym_label;
}

ST_FUNC Sym *label_push(Sym **ptop, int v, int flags)
{
  Sym *s, **ps;
  s = sym_push2(ptop, v, VT_STATIC, 0);
  s->r = flags;
  ps = &table_ident[v - TOK_IDENT]->sym_label;
  if (ptop == &global_label_stack)
  {
    /* modify the top most local identifier, so that
       sym_identifier will point to 's' when popped */
    while (*ps != NULL)
      ps = &(*ps)->prev_tok;
  }
  s->prev_tok = *ps;
  *ps = s;
  return s;
}

/* pop labels until element last is reached. Look if any labels are
   undefined. Define symbols if '&&label' was used. */
ST_FUNC void label_pop(Sym **ptop, Sym *slast, int keep)
{
  Sym *s, *s1;
  for (s = *ptop; s != slast; s = s1)
  {
    s1 = s->prev;
    if (s->r == LABEL_DECLARED)
      cprime_warning_c(warn_all)("label '%s' declared but not used", get_tok_str(s->v, NULL));
    else if (s->r == LABEL_FORWARD)
    {
      cprime_error("label '%s' used but not defined",
                get_tok_str(s->v, NULL));
    }
    else
    {
      if (s->c)
      {
        /* define corresponding symbol. A size of
           1 is put. */
        put_extern_sym(s, cur_text_section, s->jnext, 1);
      }
    }
    // Remove Label
    if (s->r != LABEL_GONE)
      table_ident[s->v - TOK_IDENT]->sym_label = s->prev_tok;
    if (!keep)
      sym_free(s);
    else
      s->r = LABEL_GONE;
  }
  if (!keep)
    *ptop = slast;
}

// -------------------------------------------------------------------------
static void vcheck_cmp(void)
{
  /* cannot let cpu flags if other instruction are generated. Also
     avoid leaving VT_JMP anywhere except on the top of the stack
     because it would complicate the code generator.

     Don't do this when nocode_wanted.  vtop might come from
     !nocode_wanted regions (see 88_codeopt.c) and transforming
     it to a register without actually generating code is wrong
     as their value might still be used for real.  All values
     we push under nocode_wanted will eventually be popped
     again, so that the VT_CMP/VT_JMP value will be in vtop
     when code is unsuppressed again. */

  /* However if it's just automatic suppression via CODE_OFF/ON()
     then it seems that we better let things work undisturbed.
     How can it work at all under nocode_wanted?  Well, gv() will
     actually clear it at the gsym() in load()/VT_JMP in the
     generator backends */

  if (vtop->r == VT_CMP && 0 == (nocode_wanted & ~CODE_OFF_BIT))
    gv(RC_INT);
}

static void vsetc(CType *type, int r, CValue *vc)
{
  if (vtop >= vstack + (VSTACK_SIZE - 1))
    cprime_error("memory full (vstack)");
  vcheck_cmp();
  vtop++;
  vtop->type = *type;
  vtop->r = r;
  vtop->r2 = VT_CONST;
  vtop->c = *vc;
  vtop->sym = NULL;
}

ST_FUNC void vswap(void)
{
  SValue tmp;

  vcheck_cmp();
  tmp = vtop[0];
  vtop[0] = vtop[-1];
  vtop[-1] = tmp;
}

// Pop Stack Value
ST_FUNC void vpop(void)
{
  int v;
  v = vtop->r &VT_VALMASK;
#if defined(CPRIME_TARGET_I386) || defined(CPRIME_TARGET_X86_64)
  // for x86, we need to pop the FP stack
  if (v == TREG_ST0)
  {
    o(0xd8dd); // Fstp %St(0)
  }
  else
#endif
    if (v == VT_CMP)
    {
      // need to put correct jump if && or || without test
      gsym(vtop->jtrue);
      gsym(vtop->jfalse);
    }
  vtop--;
}

// Push Constant Of Type "Type" With Useless Value
static void vpush(CType *type)
{
  vset(type, VT_CONST, 0);
}

// Push Arbitrary 64Bit Constant
static void vpush64(int ty, unsigned long long v)
{
  CValue cval;
  CType ctype;
  ctype.t = ty;
  ctype.ref = NULL;
  cval.i = v;
  vsetc(&ctype, VT_CONST, &cval);
}

// Push Integer Constant
ST_FUNC void vpushi(int v)
{
  vpush64(VT_INT, v);
}

// Push A Pointer Sized Constant
static void vpushs(addr_t v)
{
  vpush64(VT_SIZE_T, v);
}

// Push Long Long Constant
static inline void vpushll(long long v)
{
  vpush64(VT_LLONG, v);
}

ST_FUNC void vset(CType *type, int r, int v)
{
  CValue cval;
  cval.i = v;
  vsetc(type, r, &cval);
}

static void vseti(int r, int v)
{
  CType type;
  type.t = VT_INT;
  type.ref = NULL;
  vset(&type, r, v);
}

ST_FUNC void vpushv(SValue *v)
{
  if (vtop >= vstack + (VSTACK_SIZE - 1))
    cprime_error("memory full (vstack)");
  vtop++;
  *vtop = *v;
}

static void vdup(void)
{
  vpushv(vtop);
}

// Rotate The Stack Element At Position N-1 To The Top
ST_FUNC void vrotb(int n)
{
  SValue tmp;
  if (--n < 1)
    return;
  vcheck_cmp();
  tmp = vtop[-n];
  memmove(vtop - n, vtop - n + 1, sizeof *vtop *n);
  vtop[0] = tmp;
}

// Rotate The Top Stack Element Into Position N-1
ST_FUNC void vrott(int n)
{
  SValue tmp;
  if (--n < 1)
    return;
  vcheck_cmp();
  tmp = vtop[0];
  memmove(vtop - n + 1, vtop - n, sizeof *vtop *n);
  vtop[-n] = tmp;
}

// Reverse Order Of The The First N Stack Elements
ST_FUNC void vrev(int n)
{
  int i;
  SValue tmp;
  vcheck_cmp();
  for (i = 0, n = -n; i > ++n; --i)
    tmp = vtop[i], vtop[i] = vtop[n], vtop[n] = tmp;
}

// -------------------------------------------------------------------------
// vtop->r = VT_CMP means CPU-flags have been set from comparison or test.

// Called From Generators To Set The Result From Relational Ops
ST_FUNC void vset_VT_CMP(int op)
{
  vtop->r = VT_CMP;
  vtop->cmp_op = op;
  vtop->jfalse = 0;
  vtop->jtrue = 0;
}

// called once before asking generators to load VT_CMP to a register
static void vset_VT_JMP(void)
{
  int op = vtop->cmp_op;

  if (vtop->jtrue || vtop->jfalse)
  {
    int origt = vtop->type.t;
    // we need to jump to 'mov $0,%R' or 'mov $1,%R'
    int inv = op & (op < 2); // Small Optimization
    vseti(VT_JMP + inv, gvtst(inv, 0));
    vtop->type.t |= origt & (VT_UNSIGNED | VT_DEFSIGN);
  }
  else
  {
    // Otherwise Convert Flags (Rsp. 0/1) To Register
    vtop->c.i = op;
    if (op < 2) // doesn't seem to happen
      vtop->r = VT_CONST;
  }
}

// Set CPU Flags, doesn't yet jump
static void gvtst_set(int inv, int t)
{
  int *p;

  if (vtop->r != VT_CMP)
  {
    vpushi(0);
    gen_op(TOK_NE);
    if (vtop->r != VT_CMP) // must be VT_CONST then
      vset_VT_CMP(vtop->c.i != 0);
  }

  p = inv ? &vtop->jfalse : &vtop->jtrue;
  *p = gjmp_append(*p, t);
}

/* Generate value test
 *
 * Generate a test for any value (jump, comparison and integers) */
static int gvtst(int inv, int t)
{
  int op, x, u;

  gvtst_set(inv, t);
  t = vtop->jtrue, u = vtop->jfalse;
  if (inv)
    x = u, u = t, t = x;
  op = vtop->cmp_op;

  // Jump To The Wanted Target
  if (op > 1)
    t = gjmp_cond(op ^inv, t);
  else if (op != inv)
    t = gjmp(t);
  // Resolve Complementary Jumps To Here
  gsym(u);

  vtop--;
  return t;
}

// Generate A Zero Or Nozero Test
static void gen_test_zero(int op)
{
  if (vtop->r == VT_CMP)
  {
    int j;
    if (op == TOK_EQ)
    {
      j = vtop->jfalse;
      vtop->jfalse = vtop->jtrue;
      vtop->jtrue = j;
      vtop->cmp_op ^= 1;
    }
  }
  else
  {
    vpushi(0);
    gen_op(op);
  }
}

// -------------------------------------------------------------------------
// push a symbol value of TYPE
ST_FUNC void vpushsym(CType *type, Sym *sym)
{
  CValue cval;
  cval.i = 0;
  vsetc(type, VT_CONST | VT_SYM, &cval);
  vtop->sym = sym;
}

// Return a static symbol pointing to a section
ST_FUNC Sym *get_sym_ref(CType *type, Section *sec, unsigned long offset, unsigned long size)
{
  int v;
  Sym *sym;

  v = anon_sym++;
  sym = sym_push(v, type, VT_CONST | VT_SYM, 0);
  sym->type.t |= VT_STATIC;
  put_extern_sym(sym, sec, offset, size);
  return sym;
}

// Push A Reference To A Section Offset By Adding A Dummy Symbol
static void vpush_ref(CType *type, Section *sec, unsigned long offset, unsigned long size)
{
  vpushsym(type, get_sym_ref(type, sec, offset, size));
}

// Define A New External Reference To A Symbol 'V' Of Type 'U'
ST_FUNC Sym *external_global_sym(int v, CType *type)
{
  Sym *s;

  if (getenv("CPC_TRACE_EXTERNAL_SYM")
      && strstr(get_tok_str(v, NULL), "clMaxComponent"))
    fprintf(stderr, "CPC_EXTERNAL_SYM_IN name=%s type_t=%04x ret_t=%04x ret_s=%s\n",
            get_tok_str(v, NULL), type ? type->t : 0,
            type && (type->t & VT_BTYPE) == VT_FUNC && type->ref
              ? type->ref->type.t : 0,
            type && (type->t & VT_BTYPE) == VT_FUNC && type->ref
              ? get_tok_str(get_struct_type_name_tok(&type->ref->type), NULL)
              : "<none>");
  s = sym_find(v);
  if (!s)
  {
    // Push Forward Reference
    s = global_identifier_push(v, type->t | VT_EXTERN, 0);
    s->type.ref = type->ref;
  }
  else if (IS_ASM_SYM(s))
  {
    s->type.t = type->t | (s->type.t &VT_EXTERN);
    s->type.ref = type->ref;
    update_storage(s);
  }
  else if ((s->type.t & VT_BTYPE) == VT_FUNC && (type->t & VT_BTYPE) == VT_FUNC
           && s->type.ref && type->ref)
  {
    /*
     * Keep scoped/member lowering stable: if an earlier forward symbol
     * captured a void-return signature, upgrade it when a concrete
     * non-void declaration/definition arrives.
     */
    int old_ret_bt = s->type.ref->type.t & VT_BTYPE;
    int new_ret_bt = type->ref->type.t & VT_BTYPE;
    if (old_ret_bt == VT_VOID && new_ret_bt != VT_VOID)
    {
      s->type.t = type->t | (s->type.t & VT_EXTERN);
      s->type.ref = type->ref;
    }
  }
  if (local_stack)
    move_ref_to_global(s);
  return s;
}

/* create an external reference with no specific type similar to asm labels.
   This avoids type conflicts if the symbol is used from C too */
ST_FUNC Sym *external_helper_sym(int v)
{
  CType ct = { VT_ASM_FUNC, NULL };
  return external_global_sym(v, &ct);
}

// Push A Reference To An Helper Function (Such As Memmove)
ST_FUNC void vpush_helper_func(int v)
{
  vpushsym(&func_old_type, external_helper_sym(v));
}

// Merge symbol attributes.
static void merge_symattr(struct SymAttr *sa, struct SymAttr *sa1)
{
  if (sa1->aligned && !sa->aligned)
    sa->aligned = sa1->aligned;
  sa->packed |= sa1->packed;
  sa->weak |= sa1->weak;
  sa->nodebug |= sa1->nodebug;
  if (sa1->visibility != STV_DEFAULT)
  {
    int vis = sa->visibility;
    if (vis == STV_DEFAULT
        || vis > sa1->visibility)
      vis = sa1->visibility;
    sa->visibility = vis;
  }
  sa->dllexport |= sa1->dllexport;
  sa->nodecorate |= sa1->nodecorate;
  sa->dllimport |= sa1->dllimport;
}

// Merge function attributes.
static void merge_funcattr(struct FuncAttr *fa, struct FuncAttr *fa1)
{
  if (fa1->func_call && !fa->func_call)
    fa->func_call = fa1->func_call;
  if (fa1->func_type && !fa->func_type)
    fa->func_type = fa1->func_type;
  if (fa1->func_args && !fa->func_args)
    fa->func_args = fa1->func_args;
  if (fa1->func_noreturn)
    fa->func_noreturn = 1;
  if (fa1->func_ctor)
    fa->func_ctor = 1;
  if (fa1->func_dtor)
    fa->func_dtor = 1;
}

static void preserve_func_default_args(CType *dst, CType *src)
{
  Sym *dst_arg, *src_arg;

  if (!dst || !src || (dst->t & VT_BTYPE) != VT_FUNC
      || (src->t & VT_BTYPE) != VT_FUNC || !dst->ref || !src->ref)
    return;
  dst_arg = dst->ref->next;
  src_arg = src->ref->next;
  while (dst_arg && src_arg)
  {
    if (!dst_arg->default_arg && src_arg->default_arg)
      dst_arg->default_arg = src_arg->default_arg;
    dst_arg = dst_arg->next;
    src_arg = src_arg->next;
  }
}

static void use_overload_func_type(Sym *s, CType *overload_type)
{
  struct FuncAttr f;

  if (!s || !overload_type || (s->type.t & VT_BTYPE) != VT_FUNC
      || (overload_type->t & VT_BTYPE) != VT_FUNC || !s->type.ref
      || !overload_type->ref)
    return;
  f = s->type.ref->f;
  s->type.ref = overload_type->ref;
  merge_funcattr(&s->type.ref->f, &f);
}

static CType *find_overload_func_type_by_mangled(int mangled_tok)
{
  FreeFuncOverload *fo;
  MemberFuncOverload *mo;

  for (fo = free_func_overloads; fo; fo = fo->next)
    if (fo->mangled_tok == mangled_tok && fo->func_type.ref)
      return &fo->func_type;
  for (mo = member_func_overloads; mo; mo = mo->next)
    if (mo->mangled_tok == mangled_tok && mo->func_type.ref)
      return &mo->func_type;
  return NULL;
}

/* sym_pop(..., keep=1) hides a saved local stack without freeing it.  Restore
   its token-table links from oldest to newest after an isolated replay has
   finished, preserving ordinary shadowing order. */
static void sym_relink_stack(Sym *s)
{
  int v;

  if (!s)
    return;
  sym_relink_stack(s->prev);
  v = s->v;
  if ((v & ~SYM_STRUCT) >= TOK_IDENT
      && (v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
    sym_link(s, 1);
}

static int sym_is_local_type_binding(Sym *s)
{
  int v = s->v;
  return (v & SYM_STRUCT) || (s->type.t & VT_TYPEDEF);
}

static void sym_relink_local_types(Sym *s)
{
  int v;

  if (!s)
    return;
  sym_relink_local_types(s->prev);
  v = s->v;
  if (sym_is_local_type_binding(s)
      && (v & ~SYM_STRUCT) >= TOK_IDENT
      && (v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
    sym_link(s, 1);
}

static void sym_unlink_local_types(Sym *s)
{
  int v;

  for (; s; s = s->prev)
  {
    v = s->v;
    if (sym_is_local_type_binding(s)
        && (v & ~SYM_STRUCT) >= TOK_IDENT
        && (v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
      sym_link(s, 0);
  }
}

// Merge attributes.
static void merge_attr(AttributeDef *ad, AttributeDef *ad1)
{
  merge_symattr(&ad->a, &ad1->a);
  merge_funcattr(&ad->f, &ad1->f);

  if (ad1->section)
    ad->section = ad1->section;
  if (ad1->alias_target)
    ad->alias_target = ad1->alias_target;
  if (ad1->asm_label)
    ad->asm_label = ad1->asm_label;
  if (ad1->attr_mode)
    ad->attr_mode = ad1->attr_mode;
}

// Merge some type attributes.
static void patch_type(Sym *sym, CType *type)
{
  int sym_was_extern = sym->type.t & VT_EXTERN;

  if (!(type->t & VT_EXTERN) || IS_ENUM_VAL(sym->type.t))
  {
    if (!(sym->type.t & VT_EXTERN)
        && !((sym->type.t & VT_BTYPE) == VT_FUNC
             && (type->t & VT_BTYPE) == VT_FUNC
             && ((sym->type.t | type->t) & VT_INLINE)))
      cprime_error("redefinition of '%s'", get_tok_str(sym->v, NULL));
    sym->type.t &= ~VT_EXTERN;
  }

  if (IS_ASM_SYM(sym))
  {
    // stay static if both are static
    sym->type.t = type->t & (sym->type.t | ~VT_STATIC);
    sym->type.ref = type->ref;
    if ((type->t & VT_BTYPE) != VT_FUNC && !(type->t & VT_ARRAY))
      sym->r |= VT_LVAL;
  }

  if (!is_compatible_types(&sym->type, type))
  {
    if ((sym->type.t & VT_BTYPE) == VT_FUNC
        && (type->t & VT_BTYPE) == VT_FUNC
        && sym->type.ref && type->ref)
    {
      CType old_func_type = sym->type;
      CType new_func_type = *type;
      old_func_type.t &= ~(VT_STATIC | VT_INLINE | VT_EXTERN);
      new_func_type.t &= ~(VT_STATIC | VT_INLINE | VT_EXTERN);
      if (is_compatible_types(&old_func_type, &new_func_type)
          || sym->type.ref->f.func_args == type->ref->f.func_args)
      {
        preserve_func_default_args(type, &sym->type);
        sym->type.t = type->t | (sym->type.t & VT_EXTERN);
        sym->type.ref = type->ref;
        return;
      }
      {
        char old_type_buf[256], new_type_buf[256];
        int read_i, write_i;
        const char *new_cmp;
        type_to_str(old_type_buf, sizeof(old_type_buf), &sym->type, NULL);
        type_to_str(new_type_buf, sizeof(new_type_buf), type, NULL);
        /* C++ reference collapsing makes T& & and T& denote the same type.
           Class-pattern parsing can retain the intermediate self-reference
           layer, so compare canonical spellings rather than rejecting the
           later concrete definition. */
        for (read_i = write_i = 0; old_type_buf[read_i]; ++read_i)
          if (!(old_type_buf[read_i] == '&' && write_i > 0
                && old_type_buf[write_i - 1] == '&'))
            old_type_buf[write_i++] = old_type_buf[read_i];
        old_type_buf[write_i] = '\0';
        for (read_i = write_i = 0; new_type_buf[read_i]; ++read_i)
          if (!(new_type_buf[read_i] == '&' && write_i > 0
                && new_type_buf[write_i - 1] == '&'))
            new_type_buf[write_i++] = new_type_buf[read_i];
        new_type_buf[write_i] = '\0';
        new_cmp = new_type_buf;
        if (!strncmp(new_cmp, "static inline ", 14))
          new_cmp += 14;
        if (!strcmp(old_type_buf, new_cmp))
        {
          preserve_func_default_args(type, &sym->type);
          sym->type.t = type->t | (sym->type.t & VT_EXTERN);
          sym->type.ref = type->ref;
          return;
        }
      }
    }
    if ((sym->type.t & VT_BTYPE) == VT_FUNC
        && (type->t & VT_BTYPE) == VT_FUNC
        && sym->type.ref && type->ref)
    {
      CType old_ret = sym->type.ref->type;
      CType new_ret = type->ref->type;
      if ((old_ret.t & VT_BTYPE) == VT_VOID
          && (new_ret.t & VT_BTYPE) != VT_VOID)
      {
        preserve_func_default_args(type, &sym->type);
        sym->type.t = type->t | (sym->type.t & VT_EXTERN);
        sym->type.ref = type->ref;
        return;
      }
    }
    if (sym_was_extern
        && (sym->type.t & VT_BTYPE) == VT_FUNC
        && (type->t & VT_BTYPE) == VT_FUNC
        && sym->type.ref && type->ref)
    {
      CType old_ret = sym->type.ref->type;
      CType new_ret = type->ref->type;
      if ((old_ret.t & VT_BTYPE) == VT_VOID
          && (new_ret.t & VT_BTYPE) != VT_VOID)
      {
        preserve_func_default_args(type, &sym->type);
        sym->type.t = type->t | (sym->type.t & VT_EXTERN);
        sym->type.ref = type->ref;
        return;
      }
      if ((old_ret.t & VT_BTYPE) == VT_PTR
          && (new_ret.t & VT_BTYPE) == VT_PTR
          && (pointed_type(&old_ret)->t & VT_BTYPE) == VT_STRUCT
          && (pointed_type(&new_ret)->t & VT_BTYPE) == VT_STRUCT)
      {
        preserve_func_default_args(type, &sym->type);
        sym->type.t = type->t | (sym->type.t & VT_EXTERN);
        sym->type.ref = type->ref;
        return;
      }
    }
    cprime_error("incompatible types for redefinition of '%s'",
              get_tok_str(sym->v, NULL));

  }
  else if ((sym->type.t & VT_BTYPE) == VT_FUNC)
  {
    int static_proto = sym->type.t &VT_STATIC;
    int ft1 = sym->type.ref->f.func_type;
    int ft2 = type->ref->f.func_type;

    // warn if static follows non-static function declaration
    if ((type->t & VT_STATIC) && !static_proto
        /* XXX this test for inline shouldn't be here.  Until we
           implement classic-inline mode again it silences a warning for
           mingw caused by our workarounds.  */
        && !((type->t | sym->type.t) & VT_INLINE))
      cprime_warning("static storage ignored for redefinition of '%s'",
                  get_tok_str(sym->v, NULL));

    // set 'inline' if both agree or if one has static
    if ((type->t | sym->type.t) & VT_INLINE)
    {
      if (!((type->t ^sym->type.t) & VT_INLINE)
          || ((type->t | sym->type.t) & VT_STATIC))
        static_proto |= VT_INLINE;
    }

    if (0 == (type->t & VT_EXTERN))
    {
      struct FuncAttr f = sym->type.ref->f;
      // Put Complete Type, Use Static From Prototype
      preserve_func_default_args(type, &sym->type);
      sym->type.t = (type->t & ~(VT_STATIC | VT_INLINE)) | static_proto;
      if (ft1 != FUNC_OLD)
        type->ref->f.func_type = ft1;
      sym->type.ref = type->ref;
      merge_funcattr(&sym->type.ref->f, &f);
    }
    else
    {
      sym->type.t &= ~VT_INLINE | static_proto;
      if (ft1 == FUNC_OLD && ft2 != FUNC_OLD)
        sym->type.ref = type->ref;
    }

  }
  else
  {
    if ((sym->type.t & VT_ARRAY) && type->ref->c >= 0)
    {
      // set array size if it was omitted in extern declaration
      sym->type.ref->c = type->ref->c;
    }
    if ((type->t ^sym->type.t) & VT_STATIC)
      cprime_warning("storage mismatch for redefinition of '%s'",
                  get_tok_str(sym->v, NULL));
  }
}

// Merge some storage attributes.
static void patch_storage(Sym *sym, AttributeDef *ad, CType *type)
{
  if (type)
    patch_type(sym, type);

#ifdef CPRIME_TARGET_PE
  if (sym->a.dllimport != ad->a.dllimport)
    cprime_error("incompatible dll linkage for redefinition of '%s'",
              get_tok_str(sym->v, NULL));
#endif
  merge_symattr(&sym->a, &ad->a);
  if (ad->asm_label)
    sym->asm_label = ad->asm_label;
  update_storage(sym);
}

// Copy Sym To Other Stack
static Sym *sym_copy(Sym *s0, Sym **ps)
{
  Sym *s;
  s = sym_malloc(), *s = *s0;
  s->prev = *ps, *ps = s;
  if ((s->v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
    sym_link(s, 1);
  return s;
}

/* Symbol 's' was locally declared 'extern' (or as function), and is
   on global_stack.  Now must copy its 'ref' to global_stack too */
static void move_ref_to_global(Sym *s)
{
  Sym *l, **lp;
  int n, bt;

  bt = s->type.t &VT_BTYPE;
  if (!(bt == VT_PTR
        || bt == VT_FUNC
        || bt == VT_STRUCT
        || IS_ENUM(s->type.t)))
    return;

  for (s = s->type.ref, n = 0; s; s = s->next)
  {
    for (lp = &local_stack; !!(l = *lp); lp = &l->prev)
    {
      if (l == s)
      {
        *lp = s->prev;
        s->prev = global_stack, global_stack = s;
        if (n || bt == VT_PTR || bt == VT_FUNC)
          move_ref_to_global(s);
        else
        {
          if ((s->v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
          {
            // Copy Struct/Enum Tag To Local Scope
            s->v |= SYM_FIELD;
            l = sym_copy(s, lp);
            l->v &= ~SYM_FIELD;
          }
        }
        if (bt != VT_PTR)
          n = 1;
        break;
      }
    }
    if (n == 0) // no next (VT_PTR) or ref not on local_stack
      break;
  }
}

// Define A New External Reference To A Symbol 'V'
static Sym *external_sym(int v, CType *type, int r, AttributeDef *ad)
{
  Sym *s;

  if (getenv("CPC_TRACE_EXTERNAL_SYM")
      && strstr(get_tok_str(v, NULL), getenv("CPC_TRACE_EXTERNAL_SYM")))
    fprintf(stderr, "CPC_EXTERNAL_SYM_DEF name=%s type_t=%04x ret_t=%04x ret_s=%s\n",
            get_tok_str(v, NULL), type ? type->t : 0,
            type && (type->t & VT_BTYPE) == VT_FUNC && type->ref
              ? type->ref->type.t : 0,
            type && (type->t & VT_BTYPE) == VT_FUNC && type->ref
              ? get_tok_str(get_struct_type_name_tok(&type->ref->type), NULL)
              : "<none>");
  // Look For Global Symbol
  s = sym_find(v);
  while (s && s->sym_scope)
    s = s->prev_tok;
  if (!s)
  {
    // Push Forward Reference
    s = global_identifier_push(v, type->t, 0);
    s->r |= r;
    s->a = ad->a;
    s->asm_label = ad->asm_label;
    s->type.ref = type->ref;
  }
  else
    patch_storage(s, ad, type);
  if (local_stack)
  {
    // Make Sure That Type->Ref Is On Global Stack
    move_ref_to_global(s);
    // Put Into Local Scope
    s = sym_copy(s, &local_stack);
  }
  return s;
}

// Save Registers Up To (Vtop - N) Stack Entry
ST_FUNC void save_regs(int n)
{
  SValue *p, *p1;
  for (p = vstack, p1 = vtop - n; p <= p1; p++)
    save_reg(p->r);
}

// Save R To The Memory Stack, And Mark It As Being Free
ST_FUNC void save_reg(int r)
{
  save_reg_upstack(r, 0);
}

/* save r to the memory stack, and mark it as being free,
   if seen up to (vtop - n) stack entry */
ST_FUNC void save_reg_upstack(int r, int n)
{
  int l, size, align, bt, r2;
  SValue *p, *p1, sv;

  if ((r &= VT_VALMASK) >= VT_CONST)
    return;
  if (nocode_wanted)
    return;
  l = r2 = 0;
  for (p = vstack, p1 = vtop - n; p <= p1; p++)
  {
    if ((p->r & VT_VALMASK) == r || p->r2 == r)
    {
      // must save value on stack if not already done
      if (!l)
      {
        bt = p->type.t &VT_BTYPE;
        if (bt == VT_VOID)
          continue;
        if ((p->r & VT_LVAL) || bt == VT_FUNC)
          bt = VT_PTR;
        sv.type.t = bt;
        size = type_size(&sv.type, &align);
        l = get_temp_local_var(size, align, &r2);
        sv.r = VT_LOCAL | VT_LVAL;
        sv.c.i = l;
        sv.sym = NULL;
        store(p->r &VT_VALMASK, &sv);
#if defined(CPRIME_TARGET_I386) || defined(CPRIME_TARGET_X86_64)
        // x86 specific: need to pop fp register ST0 if saved
        if (r == TREG_ST0)
        {
          o(0xd8dd); // Fstp %St(0)
        }
#endif
        // Special Long Long Case
        if (p->r2 < VT_CONST && USING_TWO_WORDS(bt))
        {
          sv.c.i += PTR_SIZE;
          store(p->r2, &sv);
        }
      }
      // Mark That Stack Entry As Being Saved On The Stack
      if (p->r & VT_LVAL)
      {
        /* also clear the bounded flag because the
           relocation address of the function was stored in
           p->c.i */
        p->r = (p->r & ~(VT_VALMASK | VT_BOUNDED)) | VT_LLOCAL;
      }
      else
      {
        p->r = VT_LVAL | VT_LOCAL;
        p->type.t &= ~VT_ARRAY; // cannot combine VT_LVAL with VT_ARRAY
      }
      p->sym = NULL;
      p->r2 = r2;
      p->c.i = l;
    }
  }
}

#ifdef CPRIME_TARGET_ARM
/* find a register of class 'rc2' with at most one reference on stack.
 * If none, call get_reg(rc) */
ST_FUNC int get_reg_ex(int rc, int rc2)
{
  int r;
  SValue *p;

  for (r = 0; r < NB_REGS; r++)
  {
    if (reg_classes[r] & rc2)
    {
      int n;
      n = 0;
      for (p = vstack; p <= vtop; p++)
      {
        if ((p->r & VT_VALMASK) == r ||
            p->r2 == r)
          n++;
      }
      if (n <= 1)
        return r;
    }
  }
  return get_reg(rc);
}
#endif

// find a free register of class 'rc'. If none, save one register
ST_FUNC int get_reg(int rc)
{
  int r;
  SValue *p;

  // Find A Free Register
  for (r = 0; r < NB_REGS; r++)
  {
    if (reg_classes[r] & rc)
    {
      if (nocode_wanted)
        return r;
      for (p = vstack; p <= vtop; p++)
      {
        if ((p->r & VT_VALMASK) == r ||
            p->r2 == r)
          goto notfound;
      }
      return r;
    }
notfound: ;
  }

  /* no register left : free the first one on the stack (VERY
     IMPORTANT to start from the bottom to ensure that we don't
     spill registers used in gen_opi()) */
  for (p = vstack; p <= vtop; p++)
  {
    // Look At Second Register (If Long Long)
    r = p->r2;
    if (r < VT_CONST && (reg_classes[r] & rc))
      goto save_found;
    r = p->r &VT_VALMASK;
    if (r < VT_CONST && (reg_classes[r] & rc))
    {
save_found:
      save_reg(r);
      return r;
    }
  }
  // Should never comes here
  return -1;
}

/* find a free temporary local variable (return the offset on stack) match
   size and align. If none, add new temporary stack variable */
static int get_temp_local_var(int size, int align, int *r2)
{
  int i;
  struct temp_local_variable *temp_var;
  SValue *p;
  int r;
  unsigned used = 0;

  // Mark Locations That Are Still In Use
  for (p = vstack; p <= vtop; p++)
  {
    r = p->r &VT_VALMASK;
    if (r == VT_LOCAL || r == VT_LLOCAL)
    {
      r = p->r2 - (VT_CONST + 1);
      if (r >= 0 && r < MAX_TEMP_LOCAL_VARIABLE_NUMBER)
        used |= 1 << r;
    }
  }
  for (i = 0; i < nb_temp_local_vars; i++)
  {
    temp_var = &arr_temp_local_vars[i];
    if (!(used & 1 << i)
        && temp_var->size >= size
        && temp_var->align >= align)
    {
ret_tmp:
      *r2 = (VT_CONST + 1) + i;
      return temp_var->location;
    }
  }
  loc = (loc - size) & -align;
  if (nb_temp_local_vars < MAX_TEMP_LOCAL_VARIABLE_NUMBER)
  {
    temp_var = &arr_temp_local_vars[nb_temp_local_vars];
    temp_var->location = loc;
    temp_var->size = size;
    temp_var->align = align;
    nb_temp_local_vars++;
    goto ret_tmp;
  }
  *r2 = VT_CONST;
  return loc;
}

/* move register 's' (of type 't') to 'r', and flush previous value of r to memory
   if needed */
static void move_reg(int r, int s, int t)
{
  SValue sv;

  if (r != s)
  {
    save_reg(r);
    sv.type.t = t;
    sv.type.ref = NULL;
    sv.r = s;
    sv.c.i = 0;
    load(r, &sv);
  }
}

// get address of vtop (vtop MUST BE an lvalue)
ST_FUNC void gaddrof(void)
{
  vtop->r &= ~VT_LVAL;
  // tricky: if saved lvalue, then we can go back to lvalue
  if ((vtop->r & VT_VALMASK) == VT_LLOCAL)
    vtop->r = (vtop->r & ~VT_VALMASK) | VT_LOCAL | VT_LVAL;
}

#ifdef CONFIG_CPRIME_BCHECK
// Generate A Bounded Pointer Addition
static void gen_bounded_ptr_add(void)
{
  int save = (vtop[-1].r &VT_VALMASK) == VT_LOCAL;
  if (save)
  {
    vpushv(&vtop[-1]);
    vrott(3);
  }
  vpush_helper_func(TOK___bound_ptr_add);
  vrott(3);
  gfunc_call(2);
  vtop -= save;
  vpushi(0);
  // returned pointer is in REG_IRET
  vtop->r = REG_IRET | VT_BOUNDED;
  if (nocode_wanted)
    return;
  // Relocation Offset Of The Bounding Function Call Point
  vtop->c.i = (cur_text_section->reloc->data_offset - sizeof(ObjW_Rel));
}

/* patch pointer addition in vtop so that pointer dereferencing is
   also tested */
static void gen_bounded_ptr_deref(void)
{
  addr_t func;
  int size, align;
  ObjW_Rel *rel;
  Sym *sym;

  if (nocode_wanted)
    return;

  size = type_size(&vtop->type, &align);
  switch (size)
  {
  case  1: func = TOK___bound_ptr_indir1; break;
  case  2: func = TOK___bound_ptr_indir2; break;
  case  4: func = TOK___bound_ptr_indir4; break;
  case  8: func = TOK___bound_ptr_indir8; break;
  case 12: func = TOK___bound_ptr_indir12; break;
  case 16: func = TOK___bound_ptr_indir16; break;
  default:
    // May Happen With Struct Member Access
    return;
  }
  sym = external_helper_sym(func);
  if (!sym->c)
    put_extern_sym(sym, NULL, 0, 0);
  // Patch Relocation
  // XXX: find a better solution ?
  rel = (ObjW_Rel *)(cur_text_section->reloc->data + vtop->c.i);
  rel->r_info = Obj64_R_INFO(sym->c, Obj64_R_TYPE(rel->r_info));
}

// Generate Lvalue Bound Code
static void gbound(void)
{
  CType type1;

  vtop->r &= ~VT_MUSTBOUND;
  // if lvalue, then use checking code before dereferencing
  if (vtop->r & VT_LVAL)
  {
    // if not VT_BOUNDED value, then make one
    if (!(vtop->r & VT_BOUNDED))
    {
      // Must Save Type Because We Must Set It To Int To Get Pointer
      type1 = vtop->type;
      vtop->type.t = VT_PTR;
      gaddrof();
      vpushi(0);
      gen_bounded_ptr_add();
      vtop->r |= VT_LVAL;
      vtop->type = type1;
    }
    // Then Check For Dereferencing
    gen_bounded_ptr_deref();
  }
}

/* we need to call __bound_ptr_add before we start to load function
   args into registers */
ST_FUNC void gbound_args(int nb_args)
{
  int i, v;
  SValue *sv;

  for (i = 1; i <= nb_args; ++i)
    if (vtop[1 - i].r & VT_MUSTBOUND)
    {
      vrotb(i);
      gbound();
      vrott(i);
    }

  sv = vtop - nb_args;
  if (sv->r & VT_SYM)
  {
    v = sv->sym->v;
    if (v == TOK_setjmp
        || v == TOK__setjmp
#ifndef CPRIME_TARGET_PE
        || v == TOK_sigsetjmp
        || v == TOK___sigsetjmp
#endif
       )
    {
      vpush_helper_func(TOK___bound_setjmp);
      vpushv(sv + 1);
      gfunc_call(1);
      func_bound_add_epilog = 1;
    }
    if (v == TOK_alloca)
      func_bound_add_epilog = 1;
#if TARGETOS_NetBSD
    if (v == TOK_longjmp) // Undo Rename To __Longjmp14
      sv->sym->asm_label = TOK___bound_longjmp;
#endif
  }
}

// Add bounds for local symbols from S to E (via ->prev)
static void add_local_bounds(Sym *s, Sym *e)
{
  for (; s != e; s = s->prev)
  {
    if (!s->v || (s->r & VT_VALMASK) != VT_LOCAL)
      continue;
    // Add arrays/structs/unions because we always take address
    if ((s->type.t & VT_ARRAY)
        || (s->type.t & VT_BTYPE) == VT_STRUCT
        || s->a.addrtaken)
    {
      // Add Local Bound Info
      int align, size = type_size(&s->type, &align);
      addr_t *bounds_ptr = section_ptr_add(lbounds_section,
                                           2 * sizeof(addr_t));
      bounds_ptr[0] = s->c;
      bounds_ptr[1] = size;
    }
  }
}
#endif

/* add debug info for locals or function parameters, optionally
   register bounds */
static void cprime_debug_end_scope(Sym *b, int bounds)
{
#ifdef CONFIG_CPRIME_BCHECK
  if (cprime_state->do_bounds_check && bounds)
    add_local_bounds(local_stack, b);
#endif
  cprime_add_debug_info (cprime_state, local_stack, b);
}

// Increment An Lvalue Pointer
static void incr_offset(int offset)
{
  int t = vtop->type.t;
  gaddrof(); // remove VT_LVAL
  vtop->type.t = VT_PTRDIFF_T; // Set Scalar Type
  vpushs(offset);
  gen_op('+');
  vtop->r |= VT_LVAL;
  vtop->type.t = t;
}

static void incr_bf_adr(int o)
{
  vtop->type.t = VT_BYTE | VT_UNSIGNED;
  incr_offset(o);
}

// Single-Byte Load Mode For Packed Or Otherwise Unaligned Bitfields
static void load_packed_bf(CType *type, int bit_pos, int bit_size)
{
  int n, o, bits;
  save_reg_upstack(vtop->r, 1);
  vpush64(type->t &VT_BTYPE, 0);  // B X
  bits = 0, o = bit_pos >> 3, bit_pos &= 7;
  do
  {
    vswap(); // X B
    incr_bf_adr(o);
    vdup(); // X B B
    n = 8 - bit_pos;
    if (n > bit_size)
      n = bit_size;
    if (bit_pos)
      vpushi(bit_pos), gen_op(TOK_SHR), bit_pos = 0; // X B Y
    if (n < 8)
      vpushi((1 << n) - 1), gen_op('&');
    gen_cast(type);
    if (bits)
      vpushi(bits), gen_op(TOK_SHL);
    vrotb(3); // B Y X
    gen_op('|'); // B X
    bits += n, bit_size -= n, o = 1;
  }
  while (bit_size);
  vswap(), vpop();
  if (!(type->t & VT_UNSIGNED))
  {
    n = ((type->t &VT_BTYPE) == VT_LLONG ? 64 : 32) - bits;
    vpushi(n), gen_op(TOK_SHL);
    vpushi(n), gen_op(TOK_SAR);
  }
}

// Single-Byte Store Mode For Packed Or Otherwise Unaligned Bitfields
static void store_packed_bf(int bit_pos, int bit_size)
{
  int bits, n, o, m, c;
  c = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
  vswap(); // X B
  save_reg_upstack(vtop->r, 1);
  bits = 0, o = bit_pos >> 3, bit_pos &= 7;
  do
  {
    incr_bf_adr(o); // X B
    vswap(); // B X
    c ? vdup() : gv_dup(); // B V X
    vrott(3); // X B V
    if (bits)
      vpushi(bits), gen_op(TOK_SHR);
    if (bit_pos)
      vpushi(bit_pos), gen_op(TOK_SHL);
    n = 8 - bit_pos;
    if (n > bit_size)
      n = bit_size;
    if (n < 8)
    {
      m = ((1 << n) - 1) << bit_pos;
      vpushi(m), gen_op('&'); // X B V1
      vpushv(vtop - 1); // X B V1 B
      vpushi(m & 0x80 ? ~m & 0x7f : ~m);
      gen_op('&'); // X B V1 B1
      gen_op('|'); // X B V2
    }
    vdup(), vtop[-1] = vtop[-2]; // X B B V2
    vstore(), vpop(); // X B
    bits += n, bit_size -= n, bit_pos = 0, o = 1;
  }
  while (bit_size);
  vpop(), vpop();
}

static int adjust_bf(SValue *sv, int bit_pos, int bit_size)
{
  int t;
  if (0 == sv->type.ref)
    return 0;
  t = sv->type.ref->auxtype;
  if (t != -1 && t != VT_STRUCT)
  {
    sv->type.t = (sv->type.t & ~(VT_BTYPE | VT_LONG)) | t;
    sv->r |= VT_LVAL;
  }
  return t;
}

/* store vtop a register belonging to class 'rc'. lvalues are
   converted to values. Cannot be used if cannot be converted to
   register value (such as structures). */
ST_FUNC int gv(int rc)
{
  int r, r2, r_ok, r2_ok, rc2, bt;
  int bit_pos, bit_size, size, align;

  // NOTE: get_reg can modify vstack[]
  if (vtop->type.t & VT_BITFIELD)
  {
    CType type;

    bit_pos = BIT_POS(vtop->type.t);
    bit_size = BIT_SIZE(vtop->type.t);
    // Remove Bit Field Info To Avoid Loops
    vtop->type.t &= ~VT_STRUCT_MASK;

    type.ref = NULL;
    type.t = vtop->type.t &VT_UNSIGNED;
    if ((vtop->type.t & VT_BTYPE) == VT_BOOL)
      type.t |= VT_UNSIGNED;

    r = adjust_bf(vtop, bit_pos, bit_size);

    if ((vtop->type.t & VT_BTYPE) == VT_LLONG)
      type.t |= VT_LLONG;
    else
      type.t |= VT_INT;

    if (r == VT_STRUCT)
      load_packed_bf(&type, bit_pos, bit_size);
    else
    {
      int bits = (type.t &VT_BTYPE) == VT_LLONG ? 64 : 32;
      // Cast To Int To Propagate Signedness In Following Ops
      gen_cast(&type);
      // Generate Shifts
      vpushi(bits - (bit_pos + bit_size));
      gen_op(TOK_SHL);
      vpushi(bits - bit_size);
      // NOTE: transformed to SHR if unsigned
      gen_op(TOK_SAR);
    }
    r = gv(rc);
  }
  else
  {
    if (is_float(vtop->type.t) &&
        (vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST)
    {
      /* CPUs usually cannot use float constants, so we store them
         generically in data segment */
      init_params p = { rodata_section };
      unsigned long offset;
      size = type_size(&vtop->type, &align);
      if (NODATA_WANTED)
        size = 0, align = 1;
      offset = section_add(p.sec, size, align);
      vpush_ref(&vtop->type, p.sec, offset, size);
      vswap();
      init_putv(&p, &vtop->type, offset);
      vtop->r |= VT_LVAL;
    }
#ifdef CONFIG_CPRIME_BCHECK
    if (vtop->r & VT_MUSTBOUND)
      gbound();
#endif

    bt = vtop->type.t &VT_BTYPE;

#ifdef CPRIME_TARGET_RISCV64
    // XXX mega hack
    if (bt == VT_LDOUBLE && rc == RC_FLOAT)
      rc = RC_INT;
#endif
    rc2 = RC2_TYPE(bt, rc);

    /* need to reload if:
       - constant
       - lvalue (need to dereference pointer)
       - already a register, but not in the right class */
    r = vtop->r &VT_VALMASK;
    r_ok = !(vtop->r &VT_LVAL) && (r < VT_CONST) && (reg_classes[r] & rc);
    r2_ok = !rc2 || ((vtop->r2 < VT_CONST) && (reg_classes[vtop->r2] & rc2));

    if (!r_ok || !r2_ok)
    {

      if (!r_ok)
      {
        if (1 // We Can 'Mov (R),R' In Cases
            && r < VT_CONST
            && (reg_classes[r] & rc)
            && !rc2
           )
          save_reg_upstack(r, 1);
        else
          r = get_reg(rc);
      }

      if (rc2)
      {
        int load_type = (bt == VT_QFLOAT) ? VT_DOUBLE : VT_PTRDIFF_T;
        int original_type = vtop->type.t;

        /* two register type load :
           expand to two words temporarily */
        if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST)
        {
          // Load Constant
          unsigned long long ll = vtop->c.i;
          vtop->c.i = ll; // First Word
          load(r, vtop);
          vtop->r = r; // Save Register Value
          vpushi(ll >> 32); // Second Word
        }
        else if (vtop->r & VT_LVAL)
        {
          /* We do not want to modifier the long long pointer here.
             So we save any other instances down the stack */
          save_reg_upstack(vtop->r, 1);
          // Load From Memory
          vtop->type.t = load_type;
          load(r, vtop);
          vdup();
          vtop[-1].r = r; // Save Register Value
          // Increment Pointer To Get Second Word
          incr_offset(PTR_SIZE);
        }
        else
        {
          // Move Registers
          if (!r_ok)
            load(r, vtop);
          if (r2_ok && vtop->r2 < VT_CONST)
            goto done;
          vdup();
          vtop[-1].r = r; // Save Register Value
          vtop->r = vtop[-1].r2;
        }
        /* Allocate second register. Here we rely on the fact that
           get_reg() tries first to free r2 of an SValue. */
        r2 = get_reg(rc2);
        load(r2, vtop);
        vpop();
        // Write Second Register
        vtop->r2 = r2;
done:
        vtop->type.t = original_type;
      }
      else
      {
        if (vtop->r == VT_CMP)
          vset_VT_JMP();
        // One Register Type Load
        load(r, vtop);
      }
    }
    vtop->r = r;
#ifdef CPRIME_TARGET_C67
    // uses register pairs for doubles
    if (bt == VT_DOUBLE)
      vtop->r2 = r + 1;
#endif
  }
  return r;
}

// Generate Vtop[-1] And Vtop[0] In Resp. Classes Rc1 And Rc2
ST_FUNC void gv2(int rc1, int rc2)
{
  /* generate more generic register first. But VT_JMP or VT_CMP
     values must be generated first in all cases to avoid possible
     reload errors */
  if (vtop->r != VT_CMP && rc1 <= rc2)
  {
    vswap();
    gv(rc1);
    vswap();
    gv(rc2);
    // test if reload is needed for first register
    if ((vtop[-1].r & VT_VALMASK) >= VT_CONST)
    {
      vswap();
      gv(rc1);
      vswap();
    }
  }
  else
  {
    gv(rc2);
    vswap();
    gv(rc1);
    vswap();
    // test if reload is needed for first register
    if ((vtop[0].r & VT_VALMASK) >= VT_CONST)
      gv(rc2);
  }
}

#if PTR_SIZE == 4
// Expand 64Bit On Stack In Two Ints
ST_FUNC void lexpand(void)
{
  int u, v;
  u = vtop->type.t & (VT_DEFSIGN | VT_UNSIGNED);
  v = vtop->r & (VT_VALMASK | VT_LVAL);
  if (v == VT_CONST)
  {
    vdup();
    vtop[0].c.i >>= 32;
  }
  else if (v == (VT_LVAL | VT_CONST) || v == (VT_LVAL | VT_LOCAL))
  {
    vdup();
    vtop[0].c.i += 4;
  }
  else
  {
    gv(RC_INT);
    vdup();
    vtop[0].r = vtop[-1].r2;
    vtop[0].r2 = vtop[-1].r2 = VT_CONST;
  }
  vtop[0].type.t = vtop[-1].type.t = VT_INT | u;
}
#endif

#if PTR_SIZE == 4
// Build A Long Long From Two Ints
static void lbuild(int t)
{
  gv2(RC_INT, RC_INT);
  vtop[-1].r2 = vtop[0].r;
  vtop[-1].type.t = t;
  vpop();
}
#endif

/* convert stack entry to register and duplicate its value in another
   register */
static void gv_dup(void)
{
  int t, rc, r;

  t = vtop->type.t;
#if PTR_SIZE == 4
  if ((t & VT_BTYPE) == VT_LLONG)
  {
    if (t & VT_BITFIELD)
    {
      gv(RC_INT);
      t = vtop->type.t;
    }
    lexpand();
    gv_dup();
    vswap();
    vrotb(3);
    gv_dup();
    vrotb(4);
    // stack: H L L1 H1
    lbuild(t);
    vrotb(3);
    vrotb(3);
    vswap();
    lbuild(t);
    vswap();
    return;
  }
#endif
  // Duplicate Value
  rc = RC_TYPE(t);
  gv(rc);
  r = get_reg(rc);
  vdup();
  load(r, vtop);
  vtop->r = r;
}

#if PTR_SIZE == 4
// generate CPU independent (unsigned) long long operations
static void gen_opl(int op)
{
  int t, a, b, op1, c, i;
  int func;
  unsigned short reg_iret = REG_IRET;
  unsigned short reg_lret = REG_IRE2;
  SValue tmp;

  switch (op)
  {
  case '/':
  case TOK_PDIV:
    func = TOK___divdi3;
    goto gen_func;
  case TOK_UDIV:
    func = TOK___udivdi3;
    goto gen_func;
  case '%':
    func = TOK___moddi3;
    goto gen_mod_func;
  case TOK_UMOD:
    func = TOK___umoddi3;
gen_mod_func:
#ifdef CPRIME_ARM_EABI
    reg_iret = TREG_R2;
    reg_lret = TREG_R3;
#endif
gen_func:
    // Call Generic Long Long Function
    vpush_helper_func(func);
    vrott(3);
    gfunc_call(2);
    vpushi(0);
    vtop->r = reg_iret;
    vtop->r2 = reg_lret;
    break;
  case '^':
  case '&':
  case '|':
  case '*':
  case '+':
  case '-':
    //pv("gen_opl A",0,2);
    t = vtop->type.t;
    vswap();
    lexpand();
    vrotb(3);
    lexpand();
    // stack: L1 H1 L2 H2
    tmp = vtop[0];
    vtop[0] = vtop[-3];
    vtop[-3] = tmp;
    tmp = vtop[-2];
    vtop[-2] = vtop[-3];
    vtop[-3] = tmp;
    vswap();
    // stack: H1 H2 L1 L2
    //pv("gen_opl B",0,4);
    if (op == '*')
    {
      vpushv(vtop - 1);
      vpushv(vtop - 1);
      gen_op(TOK_UMULL);
      lexpand();
      // stack: H1 H2 L1 L2 ML MH
      for (i = 0; i < 4; i++)
        vrotb(6);
      // stack: ML MH H1 H2 L1 L2
      tmp = vtop[0];
      vtop[0] = vtop[-2];
      vtop[-2] = tmp;
      // stack: ML MH H1 L2 H2 L1
      gen_op('*');
      vrotb(3);
      vrotb(3);
      gen_op('*');
      // stack: ML MH M1 M2
      gen_op('+');
      gen_op('+');
    }
    else if (op == '+' || op == '-')
    {
      // XXX: add non carry method too (for MIPS or alpha)
      if (op == '+')
        op1 = TOK_ADDC1;
      else
        op1 = TOK_SUBC1;
      gen_op(op1);
      // stack: H1 H2 (L1 op L2)
      vrotb(3);
      vrotb(3);
      gen_op(op1 + 1); // TOK_xxxC2
    }
    else
    {
      gen_op(op);
      // stack: H1 H2 (L1 op L2)
      vrotb(3);
      vrotb(3);
      // stack: (L1 op L2) H1 H2
      gen_op(op);
      // stack: (L1 op L2) (H1 op H2)
    }
    // stack: L H
    lbuild(t);
    break;
  case TOK_SAR:
  case TOK_SHR:
  case TOK_SHL:
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST)
    {
      t = vtop[-1].type.t;
      vswap();
      lexpand();
      vrotb(3);
      // stack: L H shift
      c = (int)vtop->c.i;
      // Constant: Simpler
      /* NOTE: all comments are for SHL. the other cases are
         done by swapping words */
      vpop();
      if (op != TOK_SHL)
        vswap();
      if (c >= 32)
      {
        // stack: L H
        vpop();
        if (c > 32)
        {
          vpushi(c - 32);
          gen_op(op);
        }
        if (op != TOK_SAR)
          vpushi(0);
        else
        {
          gv_dup();
          vpushi(31);
          gen_op(TOK_SAR);
        }
        vswap();
      }
      else
      {
        vswap();
        gv_dup();
        // stack: H L L
        vpushi(c);
        gen_op(op);
        vswap();
        vpushi(32 - c);
        if (op == TOK_SHL)
          gen_op(TOK_SHR);
        else
          gen_op(TOK_SHL);
        vrotb(3);
        // stack: L L H
        vpushi(c);
        if (op == TOK_SHL)
          gen_op(TOK_SHL);
        else
          gen_op(TOK_SHR);
        gen_op('|');
      }
      if (op != TOK_SHL)
        vswap();
      lbuild(t);
    }
    else
    {
      // XXX: should provide a faster fallback on x86 ?
      switch (op)
      {
      case TOK_SAR:
        func = TOK___ashrdi3;
        goto gen_func;
      case TOK_SHR:
        func = TOK___lshrdi3;
        goto gen_func;
      case TOK_SHL:
        func = TOK___ashldi3;
        goto gen_func;
      }
    }
    break;
  default:
    // Compare Operations
    t = vtop->type.t;
    vswap();
    lexpand();
    vrotb(3);
    lexpand();
    // stack: L1 H1 L2 H2
    tmp = vtop[-1];
    vtop[-1] = vtop[-2];
    vtop[-2] = tmp;
    // stack: L1 L2 H1 H2
    if (!cur_switch || cur_switch->bsym)
    {
      /* avoid differnt registers being saved in branches.
         This is not needed when comparing switch cases */
      save_regs(4);
    }
    // Compare High
    op1 = op;
    /* when values are equal, we need to compare low words. since
       the jump is inverted, we invert the test too. */
    if (op1 == TOK_LT)
      op1 = TOK_LE;
    else if (op1 == TOK_GT)
      op1 = TOK_GE;
    else if (op1 == TOK_ULT)
      op1 = TOK_ULE;
    else if (op1 == TOK_UGT)
      op1 = TOK_UGE;
    a = 0;
    b = 0;
    gen_op(op1);
    if (op == TOK_NE)
      b = gvtst(0, 0);
    else
    {
      a = gvtst(1, 0);
      if (op != TOK_EQ)
      {
        // Generate Non Equal Test
        vpushi(0);
        vset_VT_CMP(TOK_NE);
        b = gvtst(0, 0);
      }
    }
    // compare low. Always unsigned
    op1 = op;
    if (op1 == TOK_LT)
      op1 = TOK_ULT;
    else if (op1 == TOK_LE)
      op1 = TOK_ULE;
    else if (op1 == TOK_GT)
      op1 = TOK_UGT;
    else if (op1 == TOK_GE)
      op1 = TOK_UGE;
    gen_op(op1);
#if 0// def CPRIME_TARGET_I386
    if (op == TOK_NE) { gsym(b); break; }
    if (op == TOK_EQ) { gsym(a); break; }
#endif
    gvtst_set(1, a);
    gvtst_set(0, b);
    break;
  }
}
#endif

// Normalize Values
static uint64_t value64(uint64_t l1, int t)
{
  if ((t & VT_BTYPE) == VT_LLONG
      || (PTR_SIZE == 8 && (t & VT_BTYPE) == VT_PTR))
    return l1;
  else if (t & VT_UNSIGNED)
    return (uint32_t)l1;
  else
    return (uint32_t)l1 | -(l1 & 0x80000000);
}

static uint64_t gen_opic_sdiv(uint64_t a, uint64_t b)
{
  uint64_t x = (a >> 63 ? -a : a) / (b >> 63 ? -b : b);
  return (a ^b) >> 63 ? -x : x;
}

static int gen_opic_lt(uint64_t a, uint64_t b)
{
  return (a ^ (uint64_t)1 << 63) < (b ^ (uint64_t)1 << 63);
}

/* handle integer constant optimizations and various machine
   independent opt */
static void gen_opic(int op)
{
  SValue *v1 = vtop - 1;
  SValue *v2 = vtop;
  int t1 = v1->type.t &VT_BTYPE;
  int t2 = v2->type.t &VT_BTYPE;
  int c1 = (v1->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
  int c2 = (v2->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
  uint64_t l1 = c1 ? value64(v1->c.i, v1->type.t) : 0;
  uint64_t l2 = c2 ? value64(v2->c.i, v2->type.t) : 0;
  int shm = (t1 == VT_LLONG) ? 63 : 31;
  int r;

  if (c1 && c2)
  {
    switch (op)
    {
    case '+': l1 += l2; break;
    case '-': l1 -= l2; break;
    case '&': l1 &= l2; break;
    case '^': l1 ^= l2; break;
    case '|': l1 |= l2; break;
    case '*': l1 *= l2; break;

    case TOK_PDIV:
    case '/':
    case '%':
    case TOK_UDIV:
    case TOK_UMOD:
      // if division by zero, generate explicit division
      if (l2 == 0)
      {
        if (CONST_WANTED && !NOEVAL_WANTED)
          cprime_error("division by zero in constant");
        goto general_case;
      }
      switch (op)
      {
      default: l1 = gen_opic_sdiv(l1, l2); break;
      case '%': l1 = l1 - l2 * gen_opic_sdiv(l1, l2); break;
      case TOK_UDIV: l1 = l1 / l2; break;
      case TOK_UMOD: l1 = l1 % l2; break;
      }
      break;
    case TOK_SHL: l1 <<= (l2 &shm); break;
    case TOK_SHR: l1 >>= (l2 &shm); break;
    case TOK_SAR:
      l1 = (l1 >> 63) ? ~(~l1 >> (l2 &shm)) : l1 >> (l2 &shm);
      break;
    // Tests
    case TOK_ULT: l1 = l1 < l2; break;
    case TOK_UGE: l1 = l1 >= l2; break;
    case TOK_EQ: l1 = l1 == l2; break;
    case TOK_NE: l1 = l1 != l2; break;
    case TOK_ULE: l1 = l1 <= l2; break;
    case TOK_UGT: l1 = l1 > l2; break;
    case TOK_LT: l1 = gen_opic_lt(l1, l2); break;
    case TOK_GE: l1 = !gen_opic_lt(l1, l2); break;
    case TOK_LE: l1 = !gen_opic_lt(l2, l1); break;
    case TOK_GT: l1 = gen_opic_lt(l2, l1); break;
    // Logical
    case TOK_LAND: l1 = l1 && l2; break;
    case TOK_LOR: l1 = l1 || l2; break;
    default:
      goto general_case;
    }
    v1->c.i = value64(l1, v1->type.t);
    v1->r |= v2->r &VT_NONCONST;
    vtop--;
  }
  else
  {
    // if commutative ops, put c2 as constant
    if (c1 && (op == '+' || op == '&' || op == '^' ||
               op == '|' || op == '*' || op == TOK_EQ || op == TOK_NE))
    {
      vswap();
      c2 = c1; //c = c1, c1 = c2, c2 = c;
      l2 = l1; //l = l1, l1 = l2, l2 = l;
    }
    if (c1 && ((l1 == 0 &&
                (op == TOK_SHL || op == TOK_SHR || op == TOK_SAR)) ||
               (l1 == -1 && op == TOK_SAR)))
    {
      // Treat (0 << X), (0 >> X) And (-1 >> X) As Constant
      vpop();
    }
    else if (c2 && ((l2 == 0 && (op == '&' || op == '*')) ||
                    (op == '|' &&
                     (l2 == -1 || (l2 == 0xFFFFFFFF && t2 != VT_LLONG))) ||
                    (l2 == 1 && (op == '%' || op == TOK_UMOD))))
    {
      // Treat (X & 0), (X * 0), (X | -1) And (X % 1) As Constant
      if (l2 == 1)
        vtop->c.i = 0;
      vswap();
      vtop--;
    }
    else if (c2 && (((op == '*' || op == '/' || op == TOK_UDIV ||
                      op == TOK_PDIV) &&
                     l2 == 1) ||
                    ((op == '+' || op == '-' || op == '|' || op == '^' ||
                      op == TOK_SHL || op == TOK_SHR || op == TOK_SAR) &&
                     l2 == 0) ||
                    (op == '&' &&
                     (l2 == -1 || (l2 == 0xFFFFFFFF && t2 != VT_LLONG)))))
    {
      // filter out NOP operations like x*1, x-0, x&-1...
      vtop--;
    }
    else if (c2 && (op == '*' || op == TOK_PDIV || op == TOK_UDIV || op == TOK_UMOD))
    {
      // Try To Use Shifts Instead Of Muls Or Divs
      if (l2 > 0 && (l2 & (l2 - 1)) == 0)
      {
        int n = -1;
        if (op == TOK_UMOD)
        {
          vtop->c.i = l2 - 1;
          op = '&';
          goto general_case;
        }
        while (l2)
        {
          l2 >>= 1;
          n++;
        }
        vtop->c.i = n;
        if (op == '*')
          op = TOK_SHL;
        else if (op == TOK_PDIV)
          op = TOK_SAR;
        else
          op = TOK_SHR;
      }
      goto general_case;
    }
    else if (c2 && (op == '+' || op == '-') &&
             (r = vtop[-1].r & (VT_VALMASK | VT_LVAL | VT_SYM),
              r == (VT_CONST | VT_SYM) || r == VT_LOCAL))
    {
      // Symbol + Constant Case
      if (op == '-')
        l2 = -l2;
      l2 += vtop[-1].c.i;
      /* The backends can't always deal with addends to symbols
         larger than +-1<<31.  Don't construct such.  */
      if ((int)l2 != l2)
        goto general_case;
      vtop--;
      vtop->c.i = l2;
    }
    else
    {
general_case:
      // Call Low Level Op Generator
      if (t1 == VT_LLONG || t2 == VT_LLONG ||
          (PTR_SIZE == 8 && (t1 == VT_PTR || t2 == VT_PTR)))
        gen_opl(op);
      else
        gen_opi(op);
    }
    if (vtop->r == VT_CONST)
      vtop->r |= VT_NONCONST; // Is Const, But Only By Optimization
  }
}

#if defined CPRIME_TARGET_X86_64 || defined CPRIME_TARGET_I386 || defined CPRIME_TARGET_ARM64
# define gen_negf gen_opf
#elif defined CPRIME_TARGET_ARM
void gen_negf(int op)
{
  // Arm Will Detect 0-X And Replace By Vneg
  vpushi(0), vswap(), gen_op('-');
}
#else
// XXX: implement in gen_opf() for other backends too
void gen_negf(int op)
{
  /* In IEEE negate(x) isn't subtract(0,x).  Without NaNs it's
     subtract(-0, x), but with them it's really a sign flip
     operation.  We implement this with bit manipulation and have
     to do some type reinterpretation for this, which CPRIME can do
     only via memory.  */

  int align, size, bt;

  size = type_size(&vtop->type, &align);
  bt = vtop->type.t &VT_BTYPE;
  save_reg(gv(RC_TYPE(bt)));
  vdup();
  incr_bf_adr(size - 1);
  vdup();
  vpushi(0x80); // Flip Sign
  gen_op('^');
  vstore();
  vpop();
}
#endif

// Generate A Floating Point Operation With Constant Propagation
static void gen_opif(int op)
{
  int c1, c2, i, bt;
  SValue *v1, *v2;
#if defined _MSC_VER && defined __x86_64__
  // Avoid Bad Optimization With F1 -= F2 For F1:-0.0, F2:0.0
  volatile
#endif
  long double f1, f2;

  v1 = vtop - 1;
  v2 = vtop;
  if (op == TOK_NEG)
    v1 = v2;
  bt = v1->type.t &VT_BTYPE;

  // currently, we cannot do computations with forward symbols
  c1 = (v1->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
  c2 = (v2->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
  if (c1 && c2)
  {
    if (bt == VT_FLOAT)
    {
      f1 = v1->c.f;
      f2 = v2->c.f;
    }
    else if (bt == VT_DOUBLE)
    {
      f1 = v1->c.d;
      f2 = v2->c.d;
    }
    else
    {
      f1 = v1->c.ld;
      f2 = v2->c.ld;
    }
    /* NOTE: we only do constant propagation if finite number (not
       NaN or infinity) (ANSI spec) */
    if (!(ieee_finite(f1) || !ieee_finite(f2)) && !CONST_WANTED)
      goto general_case;
    switch (op)
    {
    case '+': f1 += f2; break;
    case '-': f1 -= f2; break;
    case '*': f1 *= f2; break;
    case '/':
      if (f2 == 0.0)
      {
        union { float f; unsigned u; } x1, x2, y;
        /* If not in initializer we need to potentially generate
           FP exceptions at runtime, otherwise we want to fold.  */
        if (!CONST_WANTED)
          goto general_case;
        /* the run-time result of 0.0/0.0 on x87, also of other compilers
           when used to compile the f1 /= f2 below, would be -nan */
        x1.f = f1, x2.f = f2;
        if (f1 == 0.0)
          y.u = 0x7fc00000; // Nan
        else
          y.u = 0x7f800000; // Infinity
        y.u |= (x1.u ^x2.u) & 0x80000000;  // Set Sign
        f1 = y.f;
        break;
      }
      f1 /= f2;
      break;
    case TOK_NEG:
      f1 = -f1;
      goto unary_result;
    case TOK_EQ:
      i = f1 == f2;
make_int:
      vtop -= 2;
      vpushi(i);
      return;
    case TOK_NE:
      i = f1 != f2;
      goto make_int;
    case TOK_LT:
      i = f1 < f2;
      goto make_int;
    case TOK_GE:
      i = f1 >= f2;
      goto make_int;
    case TOK_LE:
      i = f1 <= f2;
      goto make_int;
    case TOK_GT:
      i = f1 > f2;
      goto make_int;
    default:
      goto general_case;
    }
    vtop--;
unary_result:
    // XXX: overflow test ?
    if (bt == VT_FLOAT)
      v1->c.f = f1;
    else if (bt == VT_DOUBLE)
      v1->c.d = f1;
    else
      v1->c.ld = f1;
  }
  else
  {
general_case:
    if (op == TOK_NEG)
      gen_negf(op);
    else
      gen_opf(op);
  }
}

/* print a type. If 'varstr' is not NULL, then the variable is also
   printed in the type */
// XXX: union
// XXX: add array and function pointers
static void type_to_str(char *buf, int buf_size,
                        CType *type, const char *varstr)
{
  int bt, v, t;
  Sym *s, *sa;
  char buf1[256];
  const char *tstr;

  t = type->t;
  bt = t &VT_BTYPE;
  buf[0] = '\0';

  if (t & VT_EXTERN)
    pstrcat(buf, buf_size, "extern ");
  if (t & VT_STATIC)
    pstrcat(buf, buf_size, "static ");
  if (t & VT_TYPEDEF)
    pstrcat(buf, buf_size, "typedef ");
  if (t & VT_INLINE)
    pstrcat(buf, buf_size, "inline ");
  if (bt != VT_PTR)
  {
    if (t & VT_VOLATILE)
      pstrcat(buf, buf_size, "volatile ");
    if (t & VT_CONSTANT)
      pstrcat(buf, buf_size, "const ");
  }
  if (((t & VT_DEFSIGN) && bt == VT_BYTE)
      || ((t & VT_UNSIGNED)
          && (bt == VT_SHORT || bt == VT_INT || bt == VT_LLONG)
          && !(t & VT_WCHAR_T)
          && !IS_ENUM(t)
         ))
    pstrcat(buf, buf_size, (t & VT_UNSIGNED) ? "unsigned " : "signed ");

  buf_size -= strlen(buf);
  buf += strlen(buf);

  switch (bt)
  {
  case VT_VOID:
    tstr = "void";
    goto add_tstr;
  case VT_BOOL:
    tstr = "_Bool";
    goto add_tstr;
  case VT_BYTE:
    tstr = "char";
    goto add_tstr;
  case VT_SHORT:
    tstr = (t & VT_WCHAR_T) ? "wchar_t" : "short";
    goto add_tstr;
  case VT_INT:
    tstr = "int";
    goto maybe_long;
  case VT_LLONG:
    tstr = "long long";
maybe_long:
    if (t & VT_LONG)
      tstr = "long";
    if (!IS_ENUM(t))
      goto add_tstr;
    tstr = "enum ";
    goto tstruct;
  case VT_FLOAT:
    tstr = "float";
    goto add_tstr;
  case VT_DOUBLE:
    tstr = "double";
    if (!(t & VT_LONG))
      goto add_tstr;
  case VT_LDOUBLE:
    tstr = "long double";
add_tstr:
    pstrcat(buf, buf_size, tstr);
    break;
  case VT_STRUCT:
    tstr = "struct ";
    if (IS_UNION(t))
      tstr = "union ";
tstruct:
    pstrcat(buf, buf_size, tstr);
    v = type->ref->v & ~SYM_STRUCT;
    if (v >= SYM_FIRST_ANOM)
      pstrcat(buf, buf_size, "<anonymous>");
    else
      pstrcat(buf, buf_size, get_tok_str(v, NULL));
    break;
  case VT_FUNC:
    s = type->ref;
    buf1[0] = 0;
    if (varstr && '*' == *varstr)
    {
      pstrcat(buf1, sizeof(buf1), "(");
      pstrcat(buf1, sizeof(buf1), varstr);
      pstrcat(buf1, sizeof(buf1), ")");
    }
    pstrcat(buf1, buf_size, "(");
    sa = s->next;
    while (sa != NULL)
    {
      char buf2[256];
      type_to_str(buf2, sizeof(buf2), &sa->type, NULL);
      pstrcat(buf1, sizeof(buf1), buf2);
      sa = sa->next;
      if (sa)
        pstrcat(buf1, sizeof(buf1), ", ");
    }
    if (s->f.func_type == FUNC_ELLIPSIS)
      pstrcat(buf1, sizeof(buf1), ", ...");
    pstrcat(buf1, sizeof(buf1), ")");
    type_to_str(buf, buf_size, &s->type, buf1);
    goto no_var;
  case VT_PTR:
    s = type->ref;
    if (t & (VT_ARRAY | VT_VLA))
    {
      if (varstr && '*' == *varstr)
        snprintf(buf1, sizeof(buf1), "(%s)[%d]", varstr, s->c);
      else
        snprintf(buf1, sizeof(buf1), "%s[%d]", varstr ? varstr : "", s->c);
      type_to_str(buf, buf_size, &s->type, buf1);
      goto no_var;
    }
    pstrcpy(buf1, sizeof(buf1), (t & VT_REFERENCE) ? "&" : "*");
    if (t & VT_CONSTANT)
      pstrcat(buf1, buf_size, "const ");
    if (t & VT_VOLATILE)
      pstrcat(buf1, buf_size, "volatile ");
    if (varstr)
      pstrcat(buf1, sizeof(buf1), varstr);
    type_to_str(buf, buf_size, &s->type, buf1);
    goto no_var;
  }
  if (varstr)
  {
    pstrcat(buf, buf_size, " ");
    pstrcat(buf, buf_size, varstr);
  }
no_var: ;
}

static void type_incompatibility_error(CType *st, CType *dt, const char *fmt)
{
  char buf1[256], buf2[256];
  type_to_str(buf1, sizeof(buf1), st, NULL);
  type_to_str(buf2, sizeof(buf2), dt, NULL);
  cprime_error(fmt, buf1, buf2);
}

static void type_incompatibility_warning(CType *st, CType *dt, const char *fmt)
{
  char buf1[256], buf2[256];
  type_to_str(buf1, sizeof(buf1), st, NULL);
  type_to_str(buf2, sizeof(buf2), dt, NULL);
  cprime_warning(fmt, buf1, buf2);
}

static int pointed_size(CType *type)
{
  int align;
  return type_size(pointed_type(type), &align);
}

static inline int is_null_pointer(SValue *p)
{
  if ((p->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
    return 0;
  return ((p->type.t &VT_BTYPE) == VT_INT && (uint32_t)p->c.i == 0) ||
         ((p->type.t &VT_BTYPE) == VT_LLONG && p->c.i == 0) ||
         ((p->type.t &VT_BTYPE) == VT_PTR &&
          (PTR_SIZE == 4 ? (uint32_t)p->c.i == 0 : p->c.i == 0) &&
          ((pointed_type(&p->type)->t &VT_BTYPE) == VT_VOID) &&
          0 == (pointed_type(&p->type)->t & (VT_CONSTANT | VT_VOLATILE))
         );
}

// compare function types. OLD functions match any new functions
static int is_compatible_func(CType *type1, CType *type2)
{
  Sym *s1, *s2;

  s1 = type1->ref;
  s2 = type2->ref;
  if (s1->f.func_call != s2->f.func_call)
    return 0;
  if (s1->f.func_type != s2->f.func_type
      && s1->f.func_type != FUNC_OLD
      && s2->f.func_type != FUNC_OLD)
    return 0;
  for (;;)
  {
    if (!is_compatible_unqualified_types(&s1->type, &s2->type))
      return 0;
    if (s1->f.func_type == FUNC_OLD || s2->f.func_type == FUNC_OLD )
      return 1;
    s1 = s1->next;
    s2 = s2->next;
    if (!s1)
      return !s2;
    if (!s2)
      return 0;
  }
}

/* return true if type1 and type2 are the same.  If unqualified is
   true, qualifiers on the types are ignored.
 */
static int compare_types(CType *type1, CType *type2, int unqualified)
{
  int bt1, t1, t2;

  /* CType objects and their referenced type graphs are immutable for the
     duration of overload probing.  Most template-heavy comparisons ask the
     same question repeatedly; avoid recursively walking pointer/function
     types when both the flags and canonical type node already agree. */
  if (type1 == type2
      || (type1->t == type2->t && type1->ref == type2->ref))
    return 1;

  if (IS_ENUM(type1->t))
  {
    if (IS_ENUM(type2->t))
      return type1->ref == type2->ref;
    type1 = &type1->ref->type;
  }
  else if (IS_ENUM(type2->t))
    type2 = &type2->ref->type;

  t1 = type1->t &VT_TYPE;
  t2 = type2->t &VT_TYPE;
  if (unqualified)
  {
    // Strip Qualifiers Before Comparing
    t1 &= ~(VT_CONSTANT | VT_VOLATILE);
    t2 &= ~(VT_CONSTANT | VT_VOLATILE);
  }

  // Default Vs explicit signedness only matters for char
  if ((t1 & VT_BTYPE) != VT_BYTE)
  {
    t1 &= ~VT_DEFSIGN;
    t2 &= ~VT_DEFSIGN;
  }
  // XXX: bitfields ?
  if (t1 != t2)
    return 0;

  bt1 = t1 & VT_BTYPE;
  if (bt1 != VT_PTR && bt1 != VT_STRUCT && bt1 != VT_FUNC)
    return 1;
  if (type1->ref == type2->ref)
    return 1;

  if ((t1 & VT_ARRAY)
      && !(type1->ref->c < 0
           || type2->ref->c < 0
           || type1->ref->c == type2->ref->c))
    return 0;

  // Test More Complicated Cases
  if (bt1 == VT_PTR)
  {
    type1 = pointed_type(type1);
    type2 = pointed_type(type2);
    return is_compatible_types(type1, type2);
  }
  else if (bt1 == VT_STRUCT)
    return (type1->ref == type2->ref);
  else if (bt1 == VT_FUNC)
    return is_compatible_func(type1, type2);
  else
    return 1;
}

#define CMP_OP 'C'
#define SHIFT_OP 'S'

/* Check if OP1 and OP2 can be "combined" with operation OP, the combined
   type is stored in DEST if non-null (except for pointer plus/minus) . */
static int combine_types(CType *dest, SValue *op1, SValue *op2, int op)
{
  CType *type1, *type2, type;
  int t1, t2, bt1, bt2;
  int ret = 1;

  // For Shifts, 'Combine' Only Left Operand
  if (op == SHIFT_OP)
    op2 = op1;

  type1 = &op1->type, type2 = &op2->type;
  t1 = type1->t, t2 = type2->t;
  bt1 = t1 &VT_BTYPE, bt2 = t2 &VT_BTYPE;

  type.t = VT_VOID;
  type.ref = NULL;

  if (bt1 == VT_VOID || bt2 == VT_VOID)
  {
    ret = op == '?' ? 1 : 0;
    // NOTE: as an extension, we accept void on only one side
    type.t = VT_VOID;
  }
  else if (bt1 == VT_PTR || bt2 == VT_PTR)
  {
    if (op == '+')
    {
      if (!is_integer_btype(bt1 == VT_PTR ? bt2 : bt1))
        ret = 0;
    }
    // Http://Port70.Net/~Nsz/C/C99/N1256.Html#6.5.15P6
    // If one is a null ptr constant the result type is the other.
    else if (is_null_pointer (op2)) type = *type1;
    else if (is_null_pointer (op1)) type = *type2;
    else if (bt1 != bt2)
    {
      /* accept comparison or cond-expr between pointer and integer
         with a warning */
      if ((op == '?' || op == CMP_OP)
          && (is_integer_btype(bt1) || is_integer_btype(bt2)))
        cprime_warning("pointer/integer mismatch in %s",
                    op == '?' ? "conditional expression" : "comparison");
      else if (op != '-' || !is_integer_btype(bt2))
        ret = 0;
      type = *(bt1 == VT_PTR ? type1 : type2);
    }
    else
    {
      CType *pt1 = pointed_type(type1);
      CType *pt2 = pointed_type(type2);
      int pbt1 = pt1->t &VT_BTYPE;
      int pbt2 = pt2->t &VT_BTYPE;
      int newquals, copied = 0;
      if (pbt1 != VT_VOID && pbt2 != VT_VOID
          && !compare_types(pt1, pt2, 1/*unqualif*/))
      {
        if (op != '?' && op != CMP_OP)
          ret = 0;
        else
          type_incompatibility_warning(type1, type2,
                                       op == '?'
                                       ? "pointer type mismatch in conditional expression ('%s' and '%s')"
                                       : "pointer type mismatch in comparison('%s' and '%s')");
      }
      if (op == '?')
      {
        /* pointers to void get preferred, otherwise the
           pointed to types minus qualifs should be compatible */
        type = *((pbt1 == VT_VOID) ? type1 : type2);
        // Combine Qualifs
        newquals = ((pt1->t | pt2->t) & (VT_CONSTANT | VT_VOLATILE));
        if ((~pointed_type(&type)->t & (VT_CONSTANT | VT_VOLATILE))
            & newquals)
        {
          // Copy The Pointer Target Symbol
          type.ref = sym_push(SYM_FIELD, &type.ref->type,
                              0, type.ref->c);
          copied = 1;
          pointed_type(&type)->t |= newquals;
        }
        /* pointers to incomplete arrays get converted to
           pointers to completed ones if possible */
        if (pt1->t & VT_ARRAY
            && pt2->t & VT_ARRAY
            && pointed_type(&type)->ref->c < 0
            && (pt1->ref->c > 0 || pt2->ref->c > 0))
        {
          if (!copied)
            type.ref = sym_push(SYM_FIELD, &type.ref->type,
                                0, type.ref->c);
          pointed_type(&type)->ref =
            sym_push(SYM_FIELD, &pointed_type(&type)->ref->type,
                     0, pointed_type(&type)->ref->c);
          pointed_type(&type)->ref->c =
            0 < pt1->ref->c ? pt1->ref->c : pt2->ref->c;
        }
      }
    }
    if (op == CMP_OP)
      type.t = VT_SIZE_T;
  }
  else if (bt1 == VT_STRUCT || bt2 == VT_STRUCT)
  {
    if (op != '?' || !compare_types(type1, type2, 1))
      ret = 0;
    type = *type1;
  }
  else if (is_float(bt1) || is_float(bt2))
  {
    if (bt1 == VT_LDOUBLE || bt2 == VT_LDOUBLE)
      type.t = VT_LDOUBLE;
    else if (bt1 == VT_DOUBLE || bt2 == VT_DOUBLE)
      type.t = VT_DOUBLE;
    else
      type.t = VT_FLOAT;
  }
  else if (bt1 == VT_LLONG || bt2 == VT_LLONG)
  {
    // Cast To Biggest Op
    type.t = VT_LLONG | VT_LONG;
    if (bt1 == VT_LLONG)
      type.t &= t1;
    if (bt2 == VT_LLONG)
      type.t &= t2;
    // convert to unsigned if it does not fit in a long long
    if ((t1 & (VT_BTYPE | VT_UNSIGNED)) == (VT_LLONG | VT_UNSIGNED) ||
        (t2 & (VT_BTYPE | VT_UNSIGNED)) == (VT_LLONG | VT_UNSIGNED))
      type.t |= VT_UNSIGNED;
  }
  else
  {
    // Integer Operations
    type.t = VT_INT | (VT_LONG & (t1 | t2));
    // convert to unsigned if it does not fit in an integer
    if (((t1 & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED)
         && (!(t1 & VT_BITFIELD) || BIT_SIZE(t1) == 32))
        || ((t2 & (VT_BTYPE | VT_UNSIGNED)) == (VT_INT | VT_UNSIGNED)
            && (!(t2 & VT_BITFIELD) || BIT_SIZE(t2) == 32)))
      type.t |= VT_UNSIGNED;
  }
  if (dest)
    *dest = type;
  return ret;
}

// Generic Gen_Op: Handles Types Problems
ST_FUNC void gen_op(int op)
{
  int t1, t2, bt1, bt2, t;
  CType type1, combtype;
  int op_class = op;

  if (op == TOK_SHR || op == TOK_SAR || op == TOK_SHL)
    op_class = SHIFT_OP;
  else if (TOK_ISCOND(op)) // == != > ...
    op_class = CMP_OP;

redo:
  t1 = vtop[-1].type.t;
  t2 = vtop[0].type.t;
  bt1 = t1 &VT_BTYPE;
  bt2 = t2 &VT_BTYPE;

  if (bt1 == VT_FUNC || bt2 == VT_FUNC)
  {
    if (bt2 == VT_FUNC)
    {
      mk_pointer(&vtop->type);
      gaddrof();
    }
    if (bt1 == VT_FUNC)
    {
      vswap();
      mk_pointer(&vtop->type);
      gaddrof();
      vswap();
    }
    goto redo;
  }
  else if ((is_reference_type(&vtop[-1].type)
            && ((pointed_type(&vtop[-1].type)->t & VT_BTYPE) == VT_STRUCT))
           || (is_reference_type(&vtop->type)
               && ((pointed_type(&vtop->type)->t & VT_BTYPE) == VT_STRUCT)))
  {
    /* Prefer an operator on the left class before considering free
       candidates.  Template replay can have an already-instantiated
       reversed free operator in the registry (scalar * Matrix); it is not a
       viable replacement for Matrix::operator*(scalar). */
    if (((vtop[-1].type.t & VT_BTYPE) == VT_STRUCT
         || (is_reference_type(&vtop[-1].type)
             && ((pointed_type(&vtop[-1].type)->t & VT_BTYPE) == VT_STRUCT)))
        && try_call_cpp_binary_operator(op))
      return;
    if (try_call_cpp_free_binary_operator(op))
      return;
    if (try_call_cpp_binary_operator(op))
      return;
  }
  else if (bt1 == VT_STRUCT)
  {
    if (try_call_cpp_binary_operator(op))
      return;
    if (try_call_cpp_free_binary_operator(op))
      return;
    /* A proxy class can participate in a builtin operation through its
       conversion operator (for example clBitRef::operator bool()).  The
       left operand is one slot below vtop, so temporarily expose it while
       attempting the conversion, then retry normal type combination. */
    vswap();
    if (try_call_cpp_bool_conversion_operator())
    {
      vswap();
      goto redo;
    }
    vswap();
    goto op_err;
  }
  else if (!combine_types(&combtype, vtop - 1, vtop, op_class))
  {
    if (try_call_cpp_free_binary_operator(op))
      return;
    if (bt2 == VT_STRUCT && try_call_cpp_bool_conversion_operator())
      goto redo;
    if (try_call_cpp_binary_operator(op))
      return;
op_err:
    cprime_error("invalid operand types for binary operation");
  }
  else if (bt1 == VT_PTR || bt2 == VT_PTR)
  {
    // At Least One Operand Is A Pointer
    // Relational Op: Must Be Both Pointers
    int align;
    if (op_class == CMP_OP)
      goto std_op;
    // if both pointers, then it must be the '-' op
    if (bt1 == VT_PTR && bt2 == VT_PTR)
    {
      if (op != '-')
        goto op_err;
      vpush_type_size(pointed_type(&vtop[-1].type), &align);
      vtop->type.t &= ~VT_UNSIGNED;
      vrott(3);
      gen_opic(op);
      vtop->type.t = VT_PTRDIFF_T;
      vswap();
      gen_op(TOK_PDIV);
    }
    else
    {
      // Exactly One Pointer : Must Be '+' Or '-'.
      if (op != '-' && op != '+')
        goto op_err;
      // Put pointer as first operand
      if (bt2 == VT_PTR)
      {
        vswap();
        t = t1, t1 = t2, t2 = t;
        bt2 = bt1;
      }
#if PTR_SIZE == 4
      if (bt2 == VT_LLONG)
        // XXX: truncate here because gen_opl can't handle ptr + long long
        gen_cast_s(VT_INT);
#endif
      type1 = vtop[-1].type;
      vpush_type_size(pointed_type(&vtop[-1].type), &align);
      if (!(vtop[-1].type.t & VT_UNSIGNED))
        gen_cast_s(VT_PTRDIFF_T);
      gen_op('*');
#ifdef CONFIG_CPRIME_BCHECK
      if (cprime_state->do_bounds_check && !CONST_WANTED)
      {
        /* if bounded pointers, we generate a special code to
           test bounds */
        if (op == '-')
        {
          vpushi(0);
          vswap();
          gen_op('-');
        }
        gen_bounded_ptr_add();
      }
      else
#endif
      {
        gen_opic(op);
      }
      type1.t &= ~(VT_ARRAY | VT_VLA);
      // put again type if gen_opic() swaped operands
      vtop->type = type1;
    }
  }
  else
  {
    // Floats Can Only Be Used For A Few Operations
    if (is_float(combtype.t)
        && op != '+' && op != '-' && op != '*' && op != '/'
        && op_class != CMP_OP)
      goto op_err;
std_op:
    t = t2 = combtype.t;
    /* special case for shifts and long long: we keep the shift as
       an integer */
    if (op_class == SHIFT_OP)
      t2 = VT_INT;
    /* XXX: currently, some unsigned operations are explicit, so
       we modify them here */
    if (t & VT_UNSIGNED)
    {
      if (op == TOK_SAR)
        op = TOK_SHR;
      else if (op == '/')
        op = TOK_UDIV;
      else if (op == '%')
        op = TOK_UMOD;
      else if (op == TOK_LT)
        op = TOK_ULT;
      else if (op == TOK_GT)
        op = TOK_UGT;
      else if (op == TOK_LE)
        op = TOK_ULE;
      else if (op == TOK_GE)
        op = TOK_UGE;
    }
    vswap();
    gen_cast_s(t);
    vswap();
    gen_cast_s(t2);
    if (is_float(t))
      gen_opif(op);
    else
      gen_opic(op);
    if (op_class == CMP_OP)
    {
      // Relational Op: The Result Is An Int
      vtop->type.t = VT_INT;
    }
    else
      vtop->type.t = t;
  }
  // Make sure that we have converted to an rvalue:
  if (vtop->r & VT_LVAL)
    gv(is_float(vtop->type.t & VT_BTYPE) ? RC_FLOAT : RC_INT);
}

#if defined CPRIME_TARGET_ARM64 || defined CPRIME_TARGET_RISCV64 || defined CPRIME_TARGET_ARM
#define gen_cvt_itof1 gen_cvt_itof
#else
// Generic Itof For Unsigned Long Long Case
static void gen_cvt_itof1(int t)
{
  if ((vtop->type.t & (VT_BTYPE | VT_UNSIGNED)) ==
      (VT_LLONG | VT_UNSIGNED))
  {

    if (t == VT_FLOAT)
      vpush_helper_func(TOK___floatundisf);
#if LDOUBLE_SIZE != 8
    else if (t == VT_LDOUBLE)
      vpush_helper_func(TOK___floatundixf);
#endif
    else
      vpush_helper_func(TOK___floatundidf);
    vrott(2);
    gfunc_call(1);
    vpushi(0);
    PUT_R_RET(vtop, t);
  }
  else
    gen_cvt_itof(t);
}
#endif

#if defined CPRIME_TARGET_ARM64 || defined CPRIME_TARGET_RISCV64
#define gen_cvt_ftoi1 gen_cvt_ftoi
#else
// Generic Ftoi For Unsigned Long Long Case
static void gen_cvt_ftoi1(int t)
{
  int st;
  if (t == (VT_LLONG | VT_UNSIGNED))
  {
    // Not Handled Natively
    st = vtop->type.t &VT_BTYPE;
    if (st == VT_FLOAT)
      vpush_helper_func(TOK___fixunssfdi);
#if LDOUBLE_SIZE != 8
    else if (st == VT_LDOUBLE)
      vpush_helper_func(TOK___fixunsxfdi);
#endif
    else
      vpush_helper_func(TOK___fixunsdfdi);
    vrott(2);
    gfunc_call(1);
    vpushi(0);
    PUT_R_RET(vtop, t);
  }
  else
    gen_cvt_ftoi(t);
}
#endif

// Special Delayed Cast For Char/Short
static void force_charshort_cast(void)
{
  int sbt = BFGET(vtop->r, VT_MUSTCAST) == 2 ? VT_LLONG : VT_INT;
  int dbt = vtop->type.t;
  vtop->r &= ~VT_MUSTCAST;
  vtop->type.t = sbt;
  gen_cast_s(dbt == VT_BOOL ? VT_BYTE | VT_UNSIGNED : dbt);
  vtop->type.t = dbt;
}

static void gen_cast_s(int t)
{
  CType type;
  type.t = t;
  type.ref = NULL;
  gen_cast(&type);
}

// cast 'vtop' to 'type'. Casting to bitfields is forbidden.
static void gen_cast(CType *type)
{
  int sbt, dbt, sf, df, c;
  int dbt_bt, sbt_bt, ds, ss, bits, trunc;

  // Special Delayed Cast For Char/Short
  if (vtop->r & VT_MUSTCAST)
    force_charshort_cast();

  // Bitfields First Get Cast To Ints
  if (vtop->type.t & VT_BITFIELD)
    gv(RC_INT);

  if (IS_ENUM(type->t) && type->ref->c < 0)
    cprime_error("cast to incomplete type");

  dbt = type->t & (VT_BTYPE | VT_UNSIGNED);
  sbt = vtop->type.t & (VT_BTYPE | VT_UNSIGNED);
  if (sbt == VT_FUNC)
    sbt = VT_PTR;

again:
  if (sbt != dbt)
  {
    sf = is_float(sbt);
    df = is_float(dbt);
    dbt_bt = dbt &VT_BTYPE;
    sbt_bt = sbt &VT_BTYPE;
    if (dbt_bt == VT_VOID)
      goto done;
    if (sbt_bt == VT_VOID)
    {
error:
      cast_error(&vtop->type, type);
    }

    c = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
#if !defined CPRIME_IS_NATIVE && !defined CPRIME_IS_NATIVE_387
    /* don't try to convert to ldouble when cross-compiling
       (except when it's '0' which is needed for arm:gen_negf()) */
    if (dbt_bt == VT_LDOUBLE && !nocode_wanted && (sf || vtop->c.i != 0))
      c = 0;
#endif
    if (c)
    {
      // constant case: we can do it now
      // XXX: in ISOC, cannot do it if error in convert
      if (sbt == VT_FLOAT)
        vtop->c.ld = vtop->c.f;
      else if (sbt == VT_DOUBLE)
        vtop->c.ld = vtop->c.d;

      if (df)
      {
        if (sbt_bt == VT_LLONG)
        {
          if ((sbt & VT_UNSIGNED) || !(vtop->c.i >> 63))
            vtop->c.ld = vtop->c.i;
          else
            vtop->c.ld = -(long double) - vtop->c.i;
        }
        else if (!sf)
        {
          if ((sbt & VT_UNSIGNED) || !(vtop->c.i >> 31))
            vtop->c.ld = (uint32_t)vtop->c.i;
          else
            vtop->c.ld = -(long double) - (uint32_t)vtop->c.i;
        }

        if (dbt == VT_FLOAT)
          vtop->c.f = (float)vtop->c.ld;
        else if (dbt == VT_DOUBLE)
          vtop->c.d = (double)vtop->c.ld;
      }
      else if (sf && dbt == VT_BOOL)
        vtop->c.i = (vtop->c.ld != 0);
      else
      {
        if (sf)
        {
          if (dbt & VT_UNSIGNED)
            vtop->c.i = (uint64_t)vtop->c.ld;
          else
            vtop->c.i = (int64_t)vtop->c.ld;
        }
        else if (sbt_bt == VT_LLONG || (PTR_SIZE == 8 && sbt == VT_PTR))
          ;
        else if (sbt & VT_UNSIGNED)
          vtop->c.i = (uint32_t)vtop->c.i;
        else
          vtop->c.i = ((uint32_t)vtop->c.i | -(vtop->c.i & 0x80000000));

        if (dbt_bt == VT_LLONG || (PTR_SIZE == 8 && dbt == VT_PTR))
          ;
        else if (dbt == VT_BOOL)
          vtop->c.i = (vtop->c.i != 0);
        else
        {
          uint32_t m = dbt_bt == VT_BYTE ? 0xff :
                       dbt_bt == VT_SHORT ? 0xffff :
                       0xffffffff;
          vtop->c.i &= m;
          if (!(dbt & VT_UNSIGNED))
            vtop->c.i |= -(vtop->c.i & ((m >> 1) + 1));
        }
      }
      goto done;

    }
    else if (dbt == VT_BOOL
             && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM))
             == (VT_CONST | VT_SYM))
    {
      // Addresses Are Considered Non-Zero (See Tcctest.C:Sinit23)
      vtop->r = VT_CONST;
      vtop->c.i = 1;
      goto done;
    }

    // Cannot Generate Code For Global Or Static Initializers
    if (nocode_wanted & DATA_ONLY_WANTED)
      goto done;

    // Non Constant Case: Generate Code
    if (dbt == VT_BOOL)
    {
      gen_test_zero(TOK_NE);
      goto done;
    }

    if (sf || df)
    {
      if (sf && df)
      {
        // Convert From Fp To Fp
        gen_cvt_ftof(dbt);
      }
      else if (df)
      {
        // Convert Int To Fp
        gen_cvt_itof1(dbt);
      }
      else
      {
        // Convert Fp To Int
        sbt = dbt;
        if (dbt_bt != VT_LLONG && dbt_bt != VT_INT)
          sbt = VT_INT;
        gen_cvt_ftoi1(sbt);
        goto again; // May Need Char/Short Cast
      }
      goto done;
    }

    ds = btype_size(dbt_bt);
    ss = btype_size(sbt_bt);
    if (ds == 0 || ss == 0)
      goto error;

    // Same Size And No Sign Conversion Needed
    if (ds == ss && ds >= 4)
      goto done;
    if (dbt_bt == VT_PTR || sbt_bt == VT_PTR)
    {
      cprime_warning("cast between pointer and integer of different size");
      if (sbt_bt == VT_PTR)
      {
        // Put Integer Type To Allow Logical Operations Below
        vtop->type.t = (PTR_SIZE == 8 ? VT_LLONG : VT_INT);
      }
    }

    /* processor allows { int a = 0, b = *(char*)&a; }
       That means that if we cast to less width, we can just
       change the type and read it still later. */
#define ALLOW_SUBTYPE_ACCESS 1

    if (ALLOW_SUBTYPE_ACCESS && (vtop->r & VT_LVAL))
    {
      // Value Still In Memory
      if (ds <= ss)
        goto done;
      // Ss <= 4 Here
      if (ds <= 4 && !(dbt == (VT_SHORT | VT_UNSIGNED) && sbt == VT_BYTE))
      {
        gv(RC_INT);
        goto done; // No 64Bit Envolved
      }
    }
    gv(RC_INT);

    trunc = 0;
#if PTR_SIZE == 4
    if (ds == 8)
    {
      // Generate High Word
      if (sbt & VT_UNSIGNED)
      {
        vpushi(0);
        gv(RC_INT);
      }
      else
      {
        gv_dup();
        vpushi(31);
        gen_op(TOK_SAR);
      }
      lbuild(dbt);
    }
    else if (ss == 8)
    {
      // From Long Long: Just Take Low Order Word
      lexpand();
      vpop();
    }
    ss = 4;

#elif PTR_SIZE == 8
    if (ds == 8)
    {
      // Need To Convert From 32Bit To 64Bit
      if (sbt & VT_UNSIGNED)
      {
#if defined(CPRIME_TARGET_RISCV64)
        /* RISC-V keeps 32bit vals in registers sign-extended.
           So here we need a zero-extension.  */
        trunc = 32;
#else
        goto done;
#endif
      }
      else
      {
        gen_cvt_sxtw();
        goto done;
      }
      ss = ds, ds = 4, dbt = sbt;
    }
    else if (ss == 8)
    {
      /* RISC-V keeps 32bit vals in registers sign-extended.
         So here we need a sign-extension for signed types and
         zero-extension. for unsigned types. */
#if !defined(CPRIME_TARGET_RISCV64)
      trunc = 32; // zero upper 32 bits for non RISC-V targets
#endif
    }
    else
      ss = 4;
#endif

    if (ds >= ss)
      goto done;
#if defined CPRIME_TARGET_I386 || defined CPRIME_TARGET_X86_64 || defined CPRIME_TARGET_ARM64
    if (ss == 4)
    {
      gen_cvt_csti(dbt);
      goto done;
    }
#endif
    bits = (ss - ds) * 8;
    // for unsigned, gen_op will convert SAR to SHR
    vtop->type.t = (ss == 8 ? VT_LLONG : VT_INT) | (dbt &VT_UNSIGNED);
    vpushi(bits);
    gen_op(TOK_SHL);
    vpushi(bits - trunc);
    gen_op(TOK_SAR);
    vpushi(trunc);
    gen_op(TOK_SHR);
  }
done:
  vtop->type = *type;
  vtop->type.t &= ~ ( VT_CONSTANT | VT_VOLATILE | VT_ARRAY );
}

// return type size as known at compile time. Put alignment at 'a'
ST_FUNC int type_size(CType *type, int *a)
{
  Sym *s;
  int bt;

  bt = type->t &VT_BTYPE;
  if (bt == VT_STRUCT)
  {
    // Struct/Union
    s = type->ref;
    *a = s->r;
    return s->c;
  }
  else if (bt == VT_PTR)
  {
    if (type->t & VT_ARRAY)
    {
      int ts;
      s = type->ref;
      ts = type_size(&s->type, a);
      if (s->c < 0)
        return s->c;
      return ts * s->c;
    }
    else
    {
      *a = PTR_SIZE;
      return PTR_SIZE;
    }
  }
  else if (IS_ENUM(type->t) && type->ref->c < 0)
  {
    *a = 0;
    return -1; // Incomplete Enum
  }
  else if (bt == VT_LDOUBLE)
  {
    *a = LDOUBLE_ALIGN;
    return LDOUBLE_SIZE;
  }
  else if (bt == VT_DOUBLE || bt == VT_LLONG)
  {
#if (defined CPRIME_TARGET_I386 && !defined CPRIME_TARGET_PE) \
 || (defined CPRIME_TARGET_ARM && !defined CPRIME_ARM_EABI)
    *a = 4;
#else
    *a = 8;
#endif
    return 8;
  }
  else if (bt == VT_INT || bt == VT_FLOAT)
  {
    *a = 4;
    return 4;
  }
  else if (bt == VT_SHORT)
  {
    *a = 2;
    return 2;
  }
  else if (bt == VT_QLONG || bt == VT_QFLOAT)
  {
    *a = 8;
    return 16;
  }
  else
  {
    // char, void, function, _Bool
    *a = 1;
    return 1;
  }
}

/* push type size as known at runtime time on top of value stack. Put
   alignment at 'a' */
static void vpush_type_size(CType *type, int *a)
{
  if (type->t & VT_VLA)
  {
    type_size(&type->ref->type, a);
    vset(&int_type, VT_LOCAL | VT_LVAL, type->ref->c);
  }
  else
  {
    int size = type_size(type, a);
    if (size < 0)
      cprime_error("unknown type size");
    vpushs(size);
  }
}

// Return The Pointed Type Of T
static inline CType *pointed_type(CType *type)
{
  return &type->ref->type;
}

// Modify Type So That Its It Is A Pointer To Type.
ST_FUNC void mk_pointer(CType *type)
{
  Sym *s;
  s = sym_push(SYM_FIELD, type, 0, -1);
  type->t = VT_PTR | (type->t &VT_STORAGE);
  type->ref = s;
}

static void mk_reference(CType *type)
{
  mk_pointer(type);
  type->t |= VT_REFERENCE;
}

static int is_reference_type(CType *type)
{
  return (type->t & (VT_BTYPE | VT_REFERENCE)) == (VT_PTR | VT_REFERENCE);
}

static void decay_reference_type(CType *type)
{
  type->t &= ~(VT_REFERENCE | VT_RVALUE_REFERENCE);
}

static void maybe_indir_reference(void)
{
  if (is_reference_type(&vtop->type))
  {
    decay_reference_type(&vtop->type);
    indir();
  }
}

/* return true if type1 and type2 are exactly the same (including
   qualifiers).
*/
static int is_compatible_types(CType *type1, CType *type2)
{
  return compare_types(type1, type2, 0);
}

/* return true if type1 and type2 are the same (ignoring qualifiers).
*/
static int is_compatible_unqualified_types(CType *type1, CType *type2)
{
  return compare_types(type1, type2, 1);
}

static void cast_error(CType *st, CType *dt)
{
  if (getenv("CPC_TRACE_CAST_ERROR"))
    fprintf(stderr, "CPC_CAST_ERROR func=%s tok=%s macro=%p src=%04x/%s dst=%04x/%s func_ret=%04x/%s\n",
            funcname ? funcname : "<none>", get_tok_str(tok, &tokc),
            (void *)macro_stack,
            st->t, get_tok_str(get_struct_type_name_tok(st), NULL),
            dt->t, get_tok_str(get_struct_type_name_tok(dt), NULL),
            func_vt.t, get_tok_str(get_struct_type_name_tok(&func_vt), NULL));
  type_incompatibility_error(st, dt, "cannot convert '%s' to '%s'");
}

// Verify Type Compatibility To Store Vtop In 'Dt' Type
static void verify_assign_cast(CType *dt)
{
  CType *st, *type1, *type2;
  int dbt, sbt, qualwarn, lvl;

  st = &vtop->type; // Source Type
  dbt = dt->t &VT_BTYPE;
  sbt = st->t &VT_BTYPE;
  if (dt->t & VT_CONSTANT)
    cprime_warning("assignment of read-only location");
  switch (dbt)
  {
  case VT_VOID:
    if (sbt != dbt)
      cprime_error("assignment to void expression");
    break;
  case VT_PTR:
    // Special Cases For Pointers
    // '0' Can Also Be A Pointer
    if (is_null_pointer(vtop))
      break;
    // Accept Implicit Pointer To Integer Cast With Warning
    if (is_integer_btype(sbt))
    {
      cprime_warning("assignment makes pointer from integer without a cast");
      break;
    }
    type1 = pointed_type(dt);
    if (sbt == VT_PTR)
      type2 = pointed_type(st);
    else if (sbt == VT_FUNC)
      type2 = st; // A Function Is Implicitly A Function Pointer
    else
      goto error;
    if (is_compatible_types(type1, type2))
      break;
    for (qualwarn = lvl = 0;; ++lvl)
    {
      if (((type2->t & VT_CONSTANT) && !(type1->t & VT_CONSTANT)) ||
          ((type2->t & VT_VOLATILE) && !(type1->t & VT_VOLATILE)))
        qualwarn = 1;
      dbt = type1->t & (VT_BTYPE | VT_LONG);
      sbt = type2->t & (VT_BTYPE | VT_LONG);
      if (dbt != VT_PTR || sbt != VT_PTR)
        break;
      type1 = pointed_type(type1);
      type2 = pointed_type(type2);
    }
    if (!is_compatible_unqualified_types(type1, type2))
    {
      if ((dbt == VT_VOID || sbt == VT_VOID) && lvl == 0)
      {
        // Void * Can Match Anything
      }
      else if (dbt == sbt
               && is_integer_btype(sbt & VT_BTYPE)
               && IS_ENUM(type1->t) + IS_ENUM(type2->t)
               + !!((type1->t ^type2->t) & VT_UNSIGNED) < 2)
      {
        /* Like GCC don't warn by default for merely changes
           in pointer target signedness.  Do warn for different
           base types, though, in particular for unsigned enums
           and signed int targets.  */
      }
      else
      {
        cprime_warning("assignment from incompatible pointer type");
        break;
      }
    }
    if (qualwarn)
      cprime_warning_c(warn_discarded_qualifiers)("assignment discards qualifiers from pointer target type");
    break;
  case VT_BYTE:
  case VT_SHORT:
  case VT_INT:
  case VT_LLONG:
    if (sbt == VT_PTR || sbt == VT_FUNC)
      cprime_warning("assignment makes integer from pointer without a cast");
    else if (sbt == VT_STRUCT)
      goto case_VT_STRUCT;
    // XXX: more tests
    break;
  case VT_STRUCT:
case_VT_STRUCT:
    if (!is_compatible_unqualified_types(dt, st))
    {
      if (getenv("CPC_TRACE_ASSIGN_CAST"))
        fprintf(stderr, "CPC_ASSIGN_CAST dst_t=%04x dst_s=%s src_t=%04x src_s=%s file=%s line=%d\n",
                dt->t, get_tok_str(get_struct_type_name_tok(dt), NULL),
                st->t, get_tok_str(get_struct_type_name_tok(st), NULL),
                file ? file->filename : "<no file>", file ? file->line_num : 0);
      if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
          && get_struct_type_name_tok(dt)
          && get_struct_type_name_tok(st))
        fprintf(stderr, "CPC_STRUCT_CAST_CHECK dst=%s src=%s\n",
                get_tok_str(get_struct_type_name_tok(dt), NULL),
                get_tok_str(get_struct_type_name_tok(st), NULL));
      if (same_template_family_compatible_elements(dt, st))
        break;
error:
      cast_error(st, dt);
    }
    break;
  }
}

static void gen_assign_cast(CType *dt)
{
  if ((dt->t & VT_BTYPE) == VT_STRUCT
      && (vtop->type.t & VT_BTYPE) == VT_STRUCT
      && !is_compatible_unqualified_types(dt, &vtop->type)
      && same_template_family_instantiations(dt, &vtop->type)
      && try_materialize_same_template_family_conversion(dt))
    return;
  verify_assign_cast(dt);
  gen_cast(dt);
}

/* Find the global declaration through the token's symbol chain. */
static inline Sym *global_symbol_find(int v)
{
  Sym *s = sym_find(v);

  while (s && sym_scope_ex(s))
    s = s->prev_tok;
  return s;
}

static const char *cpc_vstore_context;

// Store Vtop In Lvalue Pushed On Stack
ST_FUNC void vstore(void)
{
  int sbt, dbt, ft, r, size, align, bit_size, bit_pos, delayed_cast;

  ft = vtop[-1].type.t;
  sbt = vtop->type.t &VT_BTYPE;
  dbt = ft &VT_BTYPE;
  if (getenv("CPC_TRACE_VSTORE")
      && ((vtop[-1].type.t & VT_BTYPE) == VT_STRUCT
          || (vtop->type.t & VT_BTYPE) == VT_STRUCT))
    fprintf(stderr, "CPC_VSTORE dst_t=%04x dst_s=%s dst_r=%04x src_t=%04x src_s=%s src_r=%04x depth=%d file=%s line=%d\n",
            vtop[-1].type.t, get_tok_str(get_struct_type_name_tok(&vtop[-1].type), NULL),
            vtop[-1].r, vtop->type.t,
            get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
            vtop->r, (int)(vtop - vstack),
            file ? file->filename : "<no file>", file ? file->line_num : 0);
  if (getenv("CPC_TRACE_VSTORE"))
    fprintf(stderr, "CPC_VSTORE_CONTEXT %s\n",
            cpc_vstore_context ? cpc_vstore_context : "<none>");
  verify_assign_cast(&vtop[-1].type);

  if (sbt == VT_STRUCT)
  {
    // if structure, only generate pointer
    // Structure Assignment : Generate Memcpy
    size = type_size(&vtop->type, &align);
    // Destination, Keep On Stack() As Result
    vpushv(vtop - 1);
#ifdef CONFIG_CPRIME_BCHECK
    if (vtop->r & VT_MUSTBOUND)
      gbound(); // Check Would Be Wrong After Gaddrof()
#endif
    vtop->type.t = VT_PTR;
    gaddrof();
    // Source
    vswap();
#ifdef CONFIG_CPRIME_BCHECK
    if (vtop->r & VT_MUSTBOUND)
      gbound();
#endif
    vtop->type.t = VT_PTR;
    gaddrof();

    if (struct_needs_memberwise_assignment(&vtop[-2].type))
    {
      SValue dst_ptr = vtop[-1];
      SValue src_ptr = vtop[0];
      CType assigned_struct_type = vtop[-2].type;
      vtop -= 3;
      assign_struct_memberwise_from_base_ptr(&assigned_struct_type,
                                             &dst_ptr, &src_ptr, 0);
      vpushi(0);
      return;
    }

#ifdef CPRIME_TARGET_NATIVE_STRUCT_COPY
    if (1
#ifdef CONFIG_CPRIME_BCHECK
        && !cprime_state->do_bounds_check
#endif
       )
      gen_struct_copy(size);
    else
#endif
    {
      // Type Size
      vpushi(size);
      // Use memmove, rather than memcpy, as dest and src may be same:
#ifdef CPRIME_ARM_EABI
      if (!(align & 7))
        vpush_helper_func(TOK_memmove8);
      else if (!(align & 3))
        vpush_helper_func(TOK_memmove4);
      else
#endif
        vpush_helper_func(TOK_memmove);
      vrott(4);
      gfunc_call(3);
    }

  }
  else if (ft & VT_BITFIELD)
  {
    // Bitfield Store Handling

    // save lvalue as expression result (example: s.b = s.a = n;)
    vdup(), vtop[-1] = vtop[-2];

    bit_pos = BIT_POS(ft);
    bit_size = BIT_SIZE(ft);
    // Remove Bit Field Info To Avoid Loops
    vtop[-1].type.t = ft & ~VT_STRUCT_MASK;

    if (dbt == VT_BOOL)
    {
      gen_cast(&vtop[-1].type);
      vtop[-1].type.t = (vtop[-1].type.t & ~VT_BTYPE) | (VT_BYTE | VT_UNSIGNED);
    }
    r = adjust_bf(vtop - 1, bit_pos, bit_size);
    if (dbt != VT_BOOL)
    {
      gen_cast(&vtop[-1].type);
      dbt = vtop[-1].type.t &VT_BTYPE;
    }
    if (r == VT_STRUCT)
      store_packed_bf(bit_pos, bit_size);
    else
    {
      unsigned long long mask = (1ULL << bit_size) - 1;
      if (dbt != VT_BOOL)
      {
        // Mask Source
        if (dbt == VT_LLONG)
          vpushll(mask);
        else
          vpushi((unsigned)mask);
        gen_op('&');
      }
      // Shift Source
      vpushi(bit_pos);
      gen_op(TOK_SHL);
      vswap();
      // Duplicate Destination
      vdup();
      vrott(3);
      // Load Destination, Mask And Or With Source
      if (dbt == VT_LLONG)
        vpushll(~(mask << bit_pos));
      else
        vpushi(~((unsigned)mask << bit_pos));
      gen_op('&');
      gen_op('|');
      // Store Result
      vstore();
      // ... And Discard
      vpop();
    }
  }
  else if (dbt == VT_VOID)
    --vtop;
  else
  {
    // Optimize Char/Short Casts
    delayed_cast = 0;
    if ((dbt == VT_BYTE || dbt == VT_SHORT)
        && is_integer_btype(sbt)
       )
    {
      if ((vtop->r & VT_MUSTCAST)
          && btype_size(dbt) > btype_size(sbt)
         )
        force_charshort_cast();
      delayed_cast = 1;
    }
    else
      gen_cast(&vtop[-1].type);

#ifdef CONFIG_CPRIME_BCHECK
    // Bound Check Case
    if (vtop[-1].r & VT_MUSTBOUND)
    {
      vswap();
      gbound();
      vswap();
    }
#endif

#ifdef CPRIME_TARGET_X86_64
    if (!nocode_wanted
        && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
        && !USING_TWO_WORDS(dbt)
        && store_immediate(vtop - 1, vtop->c.i))
    {
      vswap();
      vtop--;
      return;
    }
#endif

    gv(RC_TYPE(dbt)); // Generate Value

    if (delayed_cast)
    {
      vtop->r |= BFVAL(VT_MUSTCAST, (sbt == VT_LLONG) + 1);
      //cprime_warning("deley cast %x -> %x", sbt, dbt);
      vtop->type.t = ft &VT_TYPE;
    }

    // if lvalue was saved on stack, must read it
    if ((vtop[-1].r & VT_VALMASK) == VT_LLOCAL)
    {
      SValue sv;
      r = get_reg(RC_INT);
      sv.type.t = VT_PTRDIFF_T;
      sv.r = VT_LOCAL | VT_LVAL;
      sv.c.i = vtop[-1].c.i;
      sv.sym = NULL;
      load(r, &sv);
      vtop[-1].r = r | VT_LVAL;
    }

    r = vtop->r &VT_VALMASK;
    /* two word case handling :
       store second register at word + 4 (or +8 for x86-64)  */
    if (USING_TWO_WORDS(dbt))
    {
      int load_type = (dbt == VT_QFLOAT) ? VT_DOUBLE : VT_PTRDIFF_T;
      vtop[-1].type.t = load_type;
      store(r, vtop - 1);
      vswap();
      incr_offset(PTR_SIZE);
      vswap();
      // XXX: it works because r2 is spilled last !
      store(vtop->r2, vtop - 1);
    }
    else
    {
      // Single Word
      store(r, vtop - 1);
    }
    vswap();
    vtop--; // NOT vpop() because on x86 it would flush the fp stack
  }
}

// post defines POST/PRE add. c is the token ++ or --
ST_FUNC void inc(int post, int c)
{
  test_lvalue();
  vdup(); // Save Lvalue
  if (post)
  {
    gv_dup(); // Duplicate Value
    vrotb(3);
    vrotb(3);
  }
  // Add Constant
  vpushi(c - TOK_MID);
  gen_op('+');
  vstore(); // Store Value
  if (post)
    vpop(); // if post op, return saved value
}

ST_FUNC CString *parse_mult_str (const char *msg)
{
  // Read The String
  if (tok != TOK_STR)
    expect(msg);
  cstr_reset(&initstr);
  while (tok == TOK_STR)
  {
    // XXX: add \0 handling too ?
    cstr_cat(&initstr, tokc.str.data, -1);
    next();
  }
  cstr_ccat(&initstr, '\0');
  return &initstr;
}

/* If I is >= 1 and a power of two, returns log2(i)+1.
   If I is 0 returns 0.  */
ST_FUNC int exact_log2p1(int i)
{
  int ret;
  if (!i)
    return 0;
  for (ret = 1; i >= 1 << 8; ret += 8)
    i >>= 8;
  if (i >= 1 << 4)
    ret += 4, i >>= 4;
  if (i >= 1 << 2)
    ret += 2, i >>= 2;
  if (i >= 1 << 1)
    ret++;
  return ret;
}

// Parse __attribute__((...)) GNUC extension.
static void parse_attribute(AttributeDef *ad)
{
  int t, n;
  char *astr;
  AttributeDef ad_tmp;

redo:
  /* C++11 attributes may prefix declarations in the same places as GNU
     attributes.  CPC does not consume their semantics yet, but it must parse
     and ignore the complete attribute-specifier sequence. */
  if (tok == '[')
  {
    next();
    skip('[');
    for (;;)
    {
      if (tok == TOK_EOF)
        expect("']]'");
      if (tok == ']')
      {
        next();
        if (tok == ']')
        {
          next();
          goto redo;
        }
        continue;
      }
      next();
    }
  }
  if (tok != TOK_ATTRIBUTE1 && tok != TOK_ATTRIBUTE2)
    return;
  if (NULL == ad) // Skip Over / Ignore Attributes
    ad = &ad_tmp;

  next();
  skip('(');
  skip('(');
  while (tok != ')')
  {
    if (tok < TOK_IDENT)
      expect("attribute name");
    t = tok;
    next();
    switch (t)
    {
    case TOK_CLEANUP1:
    case TOK_CLEANUP2:
    {
      Sym *s;

      skip('(');
      s = sym_find(tok);
      if (!s)
      {
        cprime_warning_c(warn_implicit_function_declaration)(
          "implicit declaration of function '%s'", get_tok_str(tok, &tokc));
        s = external_global_sym(tok, &func_old_type);
      }
      else if ((s->type.t & VT_BTYPE) != VT_FUNC)
        cprime_error("'%s' is not declared as function", get_tok_str(tok, &tokc));
      ad->cleanup_func = s;
      next();
      skip(')');
      break;
    }
    case TOK_CONSTRUCTOR1:
    case TOK_CONSTRUCTOR2:
      ad->f.func_ctor = 1;
      break;
    case TOK_DESTRUCTOR1:
    case TOK_DESTRUCTOR2:
      ad->f.func_dtor = 1;
      break;
    case TOK_ALWAYS_INLINE1:
    case TOK_ALWAYS_INLINE2:
      ad->f.func_alwinl = 1;
      break;
    case TOK_SECTION1:
    case TOK_SECTION2:
      skip('(');
      astr = parse_mult_str("section name")->data;
      ad->section = find_section(cprime_state, astr);
      skip(')');
      break;
    case TOK_ALIAS1:
    case TOK_ALIAS2:
      skip('(');
      astr = parse_mult_str("alias(\"target\")")->data;
      // Save String As Token, For Later
      ad->alias_target = tok_alloc_const(astr);
      skip(')');
      break;
    case TOK_VISIBILITY1:
    case TOK_VISIBILITY2:
      skip('(');
      astr = parse_mult_str("visibility(\"default|hidden|internal|protected\")")->data;
      if (!strcmp (astr, "default"))
        ad->a.visibility = STV_DEFAULT;
      else if (!strcmp (astr, "hidden"))
        ad->a.visibility = STV_HIDDEN;
      else if (!strcmp (astr, "internal"))
        ad->a.visibility = STV_INTERNAL;
      else if (!strcmp (astr, "protected"))
        ad->a.visibility = STV_PROTECTED;
      else
        expect("visibility(\"default|hidden|internal|protected\")");
      skip(')');
      break;
    case TOK_ALIGNED1:
    case TOK_ALIGNED2:
      if (tok == '(')
      {
        next();
        n = expr_const();
        if (n <= 0 || (n & (n - 1)) != 0)
          cprime_error("alignment must be a positive power of two");
        skip(')');
      }
      else
        n = MAX_ALIGN;
      ad->a.aligned = exact_log2p1(n);
      if (n != 1 << (ad->a.aligned - 1))
        cprime_error("alignment of %d is larger than implemented", n);
      break;
    case TOK_PACKED1:
    case TOK_PACKED2:
      ad->a.packed = 1;
      break;
    case TOK_WEAK1:
    case TOK_WEAK2:
      ad->a.weak = 1;
      break;
    case TOK_NODEBUG1:
    case TOK_NODEBUG2:
      ad->a.nodebug = 1;
      break;
    case TOK_USED1:
    case TOK_USED2:
    case TOK_UNUSED1:
    case TOK_UNUSED2:
      /* currently, no need to handle it because cpc does not
         track used/unused objects */
      break;
    case TOK_CONST1:
    case TOK_CONST2:
    case TOK_CONST3:
    case TOK_PURE1:
    case TOK_PURE2:
      // Ignored
      break;
    case TOK_NOINLINE:
      // Ignored
      break;
    case TOK_FORMAT1:
    case TOK_FORMAT2:
      // Ignored
      goto skip_param;
    case TOK_NORETURN1:
    case TOK_NORETURN2:
      ad->f.func_noreturn = 1;
      break;
    case TOK_CDECL1:
    case TOK_CDECL2:
    case TOK_CDECL3:
      ad->f.func_call = FUNC_CDECL;
      break;
    case TOK_STDCALL1:
    case TOK_STDCALL2:
    case TOK_STDCALL3:
      ad->f.func_call = FUNC_STDCALL;
      break;
#ifdef CPRIME_TARGET_I386
    case TOK_REGPARM1:
    case TOK_REGPARM2:
      skip('(');
      n = expr_const();
      if (n > 3)
        n = 3;
      else if (n < 0)
        n = 0;
      if (n > 0)
        ad->f.func_call = FUNC_FASTCALL1 + n - 1;
      skip(')');
      break;
    case TOK_FASTCALL1:
    case TOK_FASTCALL2:
    case TOK_FASTCALL3:
      ad->f.func_call = FUNC_FASTCALLW;
      break;
    case TOK_THISCALL1:
    case TOK_THISCALL2:
    case TOK_THISCALL3:
      ad->f.func_call = FUNC_THISCALL;
      break;
#endif
    case TOK_MODE:
      skip('(');
      switch (tok)
      {
      case TOK_MODE_DI:
        ad->attr_mode = VT_LLONG + 1;
        break;
      case TOK_MODE_QI:
        ad->attr_mode = VT_BYTE + 1;
        break;
      case TOK_MODE_HI:
        ad->attr_mode = VT_SHORT + 1;
        break;
      case TOK_MODE_SI:
      case TOK_MODE_word:
        ad->attr_mode = VT_INT + 1;
        break;
      default:
        cprime_warning("__mode__(%s) not supported\n", get_tok_str(tok, NULL));
        break;
      }
      next();
      skip(')');
      break;
    case TOK_DLLEXPORT:
      ad->a.dllexport = 1;
      break;
    case TOK_NODECORATE:
      ad->a.nodecorate = 1;
      break;
    case TOK_DLLIMPORT:
      ad->a.dllimport = 1;
      break;
    default:
      cprime_warning_c(warn_unsupported)("'%s' attribute ignored", get_tok_str(t, NULL));
      // Skip Parameters
skip_param:
      if (tok == '(')
      {
        int parenthesis = 0;
        do
        {
          if (tok == '(')
            parenthesis++;
          else if (tok == ')')
            parenthesis--;
          next();
        }
        while (parenthesis && tok != -1);
      }
      break;
    }
attr_done:
    if (tok != ',')
      break;
    next();
  }
  skip(')');
  skip(')');
  goto redo;
}

static int get_struct_type_name_tok(CType *type)
{
  Sym *s;
  int v;

  if ((type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  s = type->ref;
  if (!s)
    return 0;
  v = s->v & ~SYM_STRUCT;
  if (v < TOK_UIDENT || v >= SYM_FIRST_ANOM)
    return 0;
  return v;
}

static int make_member_func_tok(int struct_tok, int method_tok)
{
  char name[512];

  snprintf(name, sizeof(name), "%s_%s",
           get_tok_str(struct_tok, NULL),
           get_tok_str(method_tok, NULL));
  return tok_alloc_const(name);
}

static int make_member_func_tok_with_cv(int struct_tok, int method_tok,
                                        int cv_qualifiers)
{
  char name[512];

  snprintf(name, sizeof(name), "%s_%s",
           get_tok_str(struct_tok, NULL),
           get_tok_str(method_tok, NULL));
  if (cv_qualifiers & VT_CONSTANT)
    pstrcat(name, sizeof(name), "_const");
  return tok_alloc_const(name);
}

static int make_static_member_tok(int struct_tok, int member_tok)
{
  return make_member_func_tok(struct_tok, member_tok);
}

static int symbol_is_nonstatic_member_of_class(Sym *sym, int class_tok)
{
  CType ft, this_type;
  Sym *func_type, *this_param;

  if (!sym || !class_tok)
    return 0;
  ft = sym->type;
  if ((ft.t & VT_BTYPE) == VT_PTR)
    ft = *pointed_type(&ft);
  if ((ft.t & VT_BTYPE) != VT_FUNC || !ft.ref)
    return 0;
  func_type = ft.ref;
  this_param = func_type->next;
  if (!this_param)
    return 0;

  this_type = this_param->type;
  if (is_reference_type(&this_type))
    this_type = *pointed_type(&this_type);
  if ((this_type.t & VT_BTYPE) != VT_PTR)
    return 0;
  this_type = *pointed_type(&this_type);
  return get_struct_type_name_tok(&this_type) == class_tok;
}

static int class_tok_matches_unqualified_name(int class_tok, int name_tok)
{
  const char *class_name, *name, *tail;
  size_t class_len, name_len;
  int i, j;

  if (!class_tok || !name_tok)
    return 0;
  class_name = get_tok_str(class_tok, NULL);
  name = get_tok_str(name_tok, NULL);
  if (!strcmp(class_name, name))
    return 1;
  /* A class-template instance name ends in its argument spelling
     (HeapList__Material), so a raw suffix test would mistake Material for
     the injected class name and rewrite Material locals as HeapList.  Match
     instances against their template's source name instead. */
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == class_tok)
      {
        int base_tok = td->lookup_tok ? td->lookup_tok : td->name_tok;
        const char *base_name = get_tok_str(base_tok, NULL);
        name = get_tok_str(name_tok, NULL);
        if (!strcmp(base_name, name))
          return 1;
        class_len = strlen(base_name);
        name_len = strlen(name);
        if (class_len <= name_len)
          return 0;
        tail = base_name + class_len - name_len;
        return !strcmp(tail, name) && tail > base_name && tail[-1] == '_';
      }
  }
  class_len = strlen(class_name);
  name_len = strlen(name);
  if (class_len <= name_len)
    return 0;
  tail = class_name + class_len - name_len;
  return !strcmp(tail, name) && tail > class_name && tail[-1] == '_';
}

static int template_scalar_typedef_tok(const char *name, int type_flags)
{
  int type_tok = tok_alloc_const(name);
  Sym *s = global_symbol_find(type_tok);

  if (!s || !(s->type.t & VT_TYPEDEF))
  {
    s = global_identifier_push(type_tok, type_flags | VT_TYPEDEF, 0);
    s->type.ref = NULL;
  }
  return type_tok;
}

static Sym *template_stable_pointer_ref(CType *type)
{
  Sym *source;
  Sym *copy;
  CType pointee;

  if (!type || !type->ref)
    return NULL;
  source = type->ref;
  pointee = source->type;
  if ((pointee.t & VT_BTYPE) == VT_PTR && pointee.ref)
    pointee.ref = template_stable_pointer_ref(&pointee);
  copy = sym_push2(&global_stack, SYM_FIELD, pointee.t, source->c);
  copy->type.ref = pointee.ref;
  return copy;
}

static int template_ctype_typedef_tok(CType *type)
{
  static unsigned synthetic_type_counter;
  CType wanted;
  Sym *s;
  char name[512];
  int type_tok;

  if (!type)
    return 0;
  /* Template argument deduction observes the referred-to type of an lvalue
     or rvalue reference.  Merely clearing VT_REFERENCE leaves the wrapper's
     pointer node in place, turning T& for an int* argument into int** and,
     once its body is replayed, a cyclic pointer declaration. */
  wanted = is_reference_type(type) ? *pointed_type(type) : *type;
  wanted.t &= ~(VT_TYPEDEF | VT_REFERENCE | VT_RVALUE_REFERENCE);

  /* Prefer an existing spelling, especially for pointers to anonymous
     structs: their typedef is the only source-level name for the type. */
  for (s = local_stack; s; s = s->prev)
    if (s->type.t & VT_TYPEDEF)
    {
      CType candidate = s->type;
      candidate.t &= ~(VT_TYPEDEF | VT_REFERENCE | VT_RVALUE_REFERENCE);
      if (candidate.t == wanted.t
          && is_compatible_types(&candidate, &wanted))
        return s->v & ~SYM_FIELD;
    }
  type_tok = template_type_cache_lookup(&wanted);
  if (type_tok)
    return type_tok;
  for (s = global_stack; s; s = s->prev)
    if (s->type.t & VT_TYPEDEF)
    {
      CType candidate = s->type;
      candidate.t &= ~(VT_TYPEDEF | VT_REFERENCE | VT_RVALUE_REFERENCE);
      if (candidate.t == wanted.t
          && is_compatible_types(&candidate, &wanted))
        return s->v & ~SYM_FIELD;
    }

  pstrcpy(name, sizeof(name), "__cpc_template_type");
  if (!append_type_mangle(name, sizeof(name), &wanted))
    snprintf(name, sizeof(name), "__cpc_template_type_%u",
             ++synthetic_type_counter);
  type_tok = tok_alloc_const(name);
  s = global_symbol_find(type_tok);
  while (s)
  {
    CType candidate = s->type;
    candidate.t &= ~(VT_TYPEDEF | VT_REFERENCE | VT_RVALUE_REFERENCE);
    if ((s->type.t & VT_TYPEDEF)
        && candidate.t == wanted.t
        && is_compatible_types(&candidate, &wanted))
      break;
    snprintf(name, sizeof(name), "__cpc_template_type_%u",
             ++synthetic_type_counter);
    type_tok = tok_alloc_const(name);
    s = global_symbol_find(type_tok);
  }
  if (!s)
  {
    s = global_identifier_push(type_tok, wanted.t | VT_TYPEDEF, 0);
    /* Expression types commonly borrow their pointer nodes from the current
       local symbol stack.  A template argument survives until the end-of-TU
       body pass, after those nodes have been popped and reused.  Own a global
       copy of the pointer chain so delayed template bodies cannot observe a
       recycled, cyclic CType graph. */
    s->type.ref = ((wanted.t & VT_BTYPE) == VT_PTR)
                    ? template_stable_pointer_ref(&wanted)
                    : wanted.ref;
  }
  /* Cache only globally stable spellings.  Local aliases are still checked
     first on every call because their visibility ends with the local stack. */
  {
    CType stable_type = s->type;
    stable_type.t &= ~(VT_TYPEDEF | VT_REFERENCE | VT_RVALUE_REFERENCE);
    template_type_cache_note(&wanted, &stable_type, type_tok);
  }
  return type_tok;
}

static int template_unsigned_scalar_type_tok(CType *type)
{
  int bt = type->t & VT_BTYPE;

  if (!(type->t & VT_UNSIGNED))
    return 0;
  if (bt == VT_BYTE)
    return template_scalar_typedef_tok("__cpc_type_unsigned_char",
                                      VT_BYTE | VT_UNSIGNED);
  if (bt == VT_SHORT)
    return template_scalar_typedef_tok("__cpc_type_unsigned_short",
                                      VT_SHORT | VT_UNSIGNED);
  if (bt == VT_INT)
    return template_scalar_typedef_tok("__cpc_type_unsigned_int",
                                      VT_INT | VT_UNSIGNED);
  if (bt == VT_LLONG)
    return template_scalar_typedef_tok("__cpc_type_unsigned_long_long",
                                      VT_LLONG | VT_UNSIGNED);
  return 0;
}

#define DIF_FIRST     1
#define DIF_SIZE_ONLY 2
#define DIF_HAVE_ELEM 4
#define DIF_CLEAR     8

static void materialize_braced_temporary(CType *type)
{
  init_params p = {0};
  int size, align, addr, r2;
  SValue *temporary;

  size = type_size(type, &align);
  if (size < 0)
    cprime_error("initialization of incomplete temporary type");
  addr = get_temp_local_var(size, align, &r2);
  p.local_offset = addr + size;

  /* Keep the destination live while parsing its initializer.  Nested braced
     temporaries use get_temp_local_var() too, and that allocator determines
     liveness from the value stack.  Publishing the destination only after
     decl_initializer() allowed a nested aggregate to reuse and overwrite the
     outer object's storage. */
  vset(type, VT_LOCAL | VT_LVAL, addr);
  vtop->r2 = r2;
  temporary = vtop;
  decl_initializer(&p, type, addr, DIF_FIRST);
  if (vtop != temporary)
    cprime_internal_error("unbalanced braced temporary initializer");
}

#include "cprimegen_cpp_names_overload.inc"

#include "cprimegen_lifecycle.inc"

#include "cprimegen_templates.inc"

static void note_struct_member_init_list(int struct_tok)
{
  if (!struct_tok || struct_has_member_init_list(struct_tok))
    return;
  if (nb_member_init_list_struct_toks >= al_member_init_list_struct_toks)
  {
    al_member_init_list_struct_toks = al_member_init_list_struct_toks
                                      ? al_member_init_list_struct_toks * 2 : 4;
    member_init_list_struct_toks =
      cprime_realloc(member_init_list_struct_toks,
                  al_member_init_list_struct_toks * sizeof(int));
  }
  member_init_list_struct_toks[nb_member_init_list_struct_toks++] = struct_tok;
}

static void note_auto_return_member_tok(int member_tok)
{
  int i;

  if (!member_tok)
    return;
  for (i = 0; i < nb_auto_return_member_toks; ++i)
    if (auto_return_member_toks[i] == member_tok)
      return;
  if (nb_auto_return_member_toks >= al_auto_return_member_toks)
  {
    al_auto_return_member_toks = al_auto_return_member_toks
                                 ? al_auto_return_member_toks * 2 : 16;
    auto_return_member_toks =
      cprime_realloc(auto_return_member_toks,
                     al_auto_return_member_toks * sizeof(int));
  }
  auto_return_member_toks[nb_auto_return_member_toks++] = member_tok;
}

static void note_defaulted_member_func(int struct_tok, int method_tok,
                                       CType *func_type)
{
  int i;

  if (!struct_tok || !method_tok || !func_type)
    return;
  for (i = 0; i < nb_defaulted_member_funcs; ++i)
    if (defaulted_member_struct_toks[i] == struct_tok
        && defaulted_member_method_toks[i] == method_tok
        && same_lowered_member_func_signature(
             &defaulted_member_func_types[i], func_type))
      return;
  if (nb_defaulted_member_funcs >= al_defaulted_member_funcs)
  {
    al_defaulted_member_funcs = al_defaulted_member_funcs
                                ? al_defaulted_member_funcs * 2 : 16;
    defaulted_member_struct_toks =
      cprime_realloc(defaulted_member_struct_toks,
                     al_defaulted_member_funcs * sizeof(int));
    defaulted_member_method_toks =
      cprime_realloc(defaulted_member_method_toks,
                     al_defaulted_member_funcs * sizeof(int));
    defaulted_member_func_types =
      cprime_realloc(defaulted_member_func_types,
                     al_defaulted_member_funcs * sizeof(CType));
  }
  defaulted_member_struct_toks[nb_defaulted_member_funcs] = struct_tok;
  defaulted_member_method_toks[nb_defaulted_member_funcs] = method_tok;
  defaulted_member_func_types[nb_defaulted_member_funcs] = *func_type;
  nb_defaulted_member_funcs++;
}

static int is_defaulted_member_func(int struct_tok, int method_tok)
{
  int i;

  for (i = 0; i < nb_defaulted_member_funcs; ++i)
    if (defaulted_member_struct_toks[i] == struct_tok
        && defaulted_member_method_toks[i] == method_tok)
      return 1;
  return 0;
}

static int is_defaulted_lifecycle_constructor(CType *struct_type,
                                              Sym *func_sym)
{
  int i, struct_tok = get_struct_type_name_tok(struct_type);

  if (!struct_tok || !func_sym)
    return 0;
  for (i = 0; i < nb_defaulted_member_funcs; ++i)
  {
    CType lowered_type;
    if (defaulted_member_struct_toks[i] != struct_tok
        || defaulted_member_method_toks[i] != TOK_CONSTRUCTOR1)
      continue;
    lowered_type = make_lowered_member_func_type(
      struct_type, &defaulted_member_func_types[i]);
    if (same_lowered_member_func_signature(&lowered_type, &func_sym->type))
      return 1;
  }
  return 0;
}

static void queue_defaulted_assignment_body(CType *struct_type, int struct_tok,
                                            CType *func_type, int param_tok,
                                            int mangled_tok)
{
  Sym *member;
  PendingMemberFunc *pm;
  TokenString *str = tok_str_alloc();
  int is_move_assignment =
    (func_type->ref->next->type.t & VT_RVALUE_REFERENCE) != 0;

  if (!add_ctype_tokens(str, &func_type->ref->type))
    cprime_error("unsupported defaulted member return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  tok_str_add(str, ',');
  if (!add_ctype_tokens(str, &func_type->ref->next->type))
    cprime_error("unsupported defaulted member parameter type");
  tok_str_add(str, param_tok);
  tok_str_add(str, ')');
  tok_str_add(str, '{');
  for (member = struct_type->ref->next; member; member = member->next)
  {
    int name_tok, use_explicit_assignment = 0;
    if ((member->type.t & VT_BTYPE) == VT_FUNC
        || (member->type.t & VT_STATIC))
      continue;
    name_tok = member->v & ~SYM_FIELD;
    if (!name_tok)
      continue;
    if ((member->type.t & VT_BTYPE) == VT_STRUCT)
    {
      CType value_type = member->type;
      use_explicit_assignment =
        resolve_member_func_by_arg_types(&member->type,
                                         tok_alloc_const("operator="),
                                         &value_type, 1) != NULL;
    }
    tok_str_add(str, tok_alloc_const("this"));
    tok_str_add(str, TOK_ARROW);
    tok_str_add(str, name_tok);
    if (use_explicit_assignment)
    {
      tok_str_add(str, '.');
      tok_str_add(str, tok_alloc_const("operator="));
      tok_str_add(str, '(');
    }
    else
      tok_str_add(str, '=');
    if (use_explicit_assignment && is_move_assignment)
    {
      tok_str_add(str, tok_alloc_const("static_cast"));
      tok_str_add(str, TOK_LT);
      if (!add_ctype_tokens(str, &member->type))
        cprime_error("unsupported defaulted move-assignment member type");
      tok_str_add(str, TOK_LAND);
      tok_str_add(str, TOK_GT);
      tok_str_add(str, '(');
    }
    tok_str_add(str, param_tok);
    tok_str_add(str, '.');
    tok_str_add(str, name_tok);
    if (use_explicit_assignment && is_move_assignment)
      tok_str_add(str, ')');
    if (use_explicit_assignment)
      tok_str_add(str, ')');
    tok_str_add(str, ';');
  }
  tok_str_add(str, TOK_RETURN);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  tok_str_add(str, ';');
  tok_str_add(str, '}');
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->struct_tok = struct_tok;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void emit_defaulted_member_bodies(CType *struct_type)
{
  int struct_tok, method_tok, i;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    return;
  method_tok = tok_alloc_const("operator=");
  for (i = 0; i < nb_defaulted_member_funcs; ++i)
  {
    if (defaulted_member_struct_toks[i] != struct_tok
        || defaulted_member_method_toks[i] != method_tok)
      continue;
    if ((defaulted_member_func_types[i].t & VT_BTYPE) != VT_FUNC
        || !defaulted_member_func_types[i].ref
        || !defaulted_member_func_types[i].ref->next)
      continue;
    {
      CType *func_type = &defaulted_member_func_types[i];
      Sym *param = func_type->ref->next;
      int param_tok = param->v & ~SYM_FIELD;
      int mangled_tok =
        make_member_func_tok_for_type(struct_tok, method_tok, func_type);
      CType lowered_type =
        make_lowered_member_func_type(struct_type, func_type);
      int canonical_tok = make_member_func_tok(struct_tok, method_tok);

      external_global_sym(mangled_tok, &lowered_type);
      note_member_func_overload(struct_tok, method_tok, mangled_tok,
                                &lowered_type);
      queue_defaulted_assignment_body(struct_type, struct_tok, func_type,
                                      param_tok, mangled_tok);
      if (canonical_tok != mangled_tok
          && !pending_member_func_has_body_tok(canonical_tok))
        queue_defaulted_assignment_body(struct_type, struct_tok, func_type,
                                        param_tok, canonical_tok);
    }
    defaulted_member_struct_toks[i] = 0;
    defaulted_member_method_toks[i] = 0;
  }
}

static int member_is_auto_return_tok(int member_tok)
{
  int i;

  for (i = 0; i < nb_auto_return_member_toks; ++i)
    if (auto_return_member_toks[i] == member_tok)
      return 1;
  return 0;
}

static TokenString *parse_explicit_constructor_member_initializers(
  CType *struct_type, TokenString *initialized_fields,
  int *is_delegating_constructor)
{
  TokenString *prefix, *args;
  int this_tok = tok_alloc_const("this");

  if (tok != ':')
    return NULL;
  next();
  prefix = tok_str_alloc();

  for (;;)
  {
    int field_tok, storage_field_tok, dummy_ofs, skip_initializer_emit = 0;
    int is_delegating_initializer = 0;
    Sym *field;
    CType field_type;
    ClassBaseInfo *base_info;
    int field_struct_tok, level, saw_arg, arg_count;

    if (tok < TOK_UIDENT)
      cprime_error("member initializer name");
    field_tok = tok;
    storage_field_tok = field_tok;
    field = find_field_try(struct_type, field_tok, &dummy_ofs);
    base_info = NULL;
    if (!field)
    {
      int class_tok = get_struct_type_name_tok(struct_type);
      for (base_info = class_base_infos; base_info; base_info = base_info->next)
        if (base_info->class_tok == class_tok
            && (base_info->base_tok == field_tok
                || class_tok_matches_unqualified_name(base_info->base_tok,
                                                      field_tok)))
          break;
      if (base_info && base_info->field)
      {
        field = base_info->field;
        storage_field_tok = field->v & ~SYM_FIELD;
      }
    }
    if (field)
    {
      field_type = field->type;
      if (initialized_fields)
        tok_str_add(initialized_fields, storage_field_tok);
    }
    else
    {
      field_type.t = VT_VOID;
      field_type.ref = NULL;
      if (field_tok == get_struct_type_name_tok(struct_type)
          || class_tok_matches_unqualified_name(
               get_struct_type_name_tok(struct_type), field_tok))
        is_delegating_initializer = 1;
      else if (class_has_base(get_struct_type_name_tok(struct_type), field_tok)
          || find_class_template_def(find_current_namespace_tok(field_tok)))
        skip_initializer_emit = 1;
    }
    next();
    if (tok == TOK_LT || tok == '<')
    {
      int angle = 1;
      if (!field)
        skip_initializer_emit = 1;
      next();
      while (tok != TOK_EOF && angle > 0)
      {
        if (tok == TOK_LT || tok == '<')
          ++angle;
        else if (tok == TOK_GT || tok == '>')
          --angle;
        else if (tok == TOK_SAR)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
        if (angle > 0)
          next();
      }
      skip(TOK_GT);
    }
    skip('(');

    args = tok_str_alloc();
    level = 0;
    saw_arg = 0;
    arg_count = 0;
    while (tok != TOK_EOF)
    {
      if (tok == ')' && level == 0)
        break;
      if (tok == '(' || tok == '[')
        ++level;
      else if ((tok == ')' || tok == ']') && level > 0)
        --level;
      else if (tok == ',' && level == 0)
        ++arg_count;
      saw_arg = 1;
      tok_str_add_tok(args);
      next();
    }
    skip(')');

    field_struct_tok = field ? get_struct_type_name_tok(&field_type) : 0;
    if (is_delegating_initializer)
    {
      int ctor_func_tok;

      if (!saw_arg)
      {
        /* Resolving an instantiated template constructor while another
           constructor body for the same class is being replayed can recurse
           through the template worklist.  The zero-argument lifecycle name
           is canonical, so publish that call without re-entering overload
           resolution. */
        ctor_func_tok = make_member_func_tok(
          get_struct_type_name_tok(struct_type), TOK_CONSTRUCTOR1);
      }
      else
      {
        Sym *ctor_func;
        TokenString *call_args[32];
        CType call_arg_types[32];
        TokenString *parse_args;
        TokenString *saved_macro_stack;
        const int *saved_macro_ptr;
        int call_arg_count, saved_tok;
        CValue saved_tokc;

        ++arg_count;
        parse_args = tok_str_alloc();
        tok_str_append(parse_args, args);
        tok_str_add(parse_args, ')');
        tok_str_add(parse_args, TOK_EOF);
        saved_tok = tok;
        saved_tokc = tokc;
        saved_macro_stack = macro_stack;
        saved_macro_ptr = macro_ptr;
        begin_macro(parse_args, 1);
        next();
        call_arg_count = count_saved_call_args(call_args, 32);
        infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
        while (macro_stack && macro_stack != saved_macro_stack)
          end_macro();
        macro_ptr = saved_macro_ptr;
        tok = saved_tok;
        tokc = saved_tokc;
        ctor_func = resolve_member_func_by_arg_types(struct_type,
                                                     TOK_CONSTRUCTOR1,
                                                     call_arg_types,
                                                     call_arg_count);
        if (!ctor_func)
          ctor_func = resolve_member_func_by_arg_count(struct_type,
                                                       TOK_CONSTRUCTOR1,
                                                       arg_count);
        if (!ctor_func)
          cprime_error("no matching delegating constructor for '%s'",
                       get_tok_str(get_struct_type_name_tok(struct_type), NULL));
        ctor_func_tok = ctor_func->v & ~SYM_FIELD;
      }
      /* Keep the generated call globally qualified so pending-member
         rewriting does not turn it into a recursive `this->constructor(...)`
         member lookup.  Lifecycle call lowering supplies the current object. */
      tok_str_add(prefix, ':');
      tok_str_add(prefix, ':');
      tok_str_add(prefix, ctor_func_tok);
      tok_str_add(prefix, '(');
      if (saw_arg)
      {
        tok_str_append(prefix, args);
      }
      tok_str_add(prefix, ')');
      tok_str_add(prefix, ';');
    }
    if (is_delegating_initializer && is_delegating_constructor)
      *is_delegating_constructor = 1;
    else if (skip_initializer_emit)
    {
    }
    else if (field && (field_type.t & VT_BTYPE) == VT_STRUCT && field_struct_tok
        && field_type.ref && field_type.ref->a.lifecycle_ctor)
    {
      Sym *ctor_func;
      Sym *field_struct = struct_find(field_struct_tok);
      CType ctor_type = field_type;
      TokenString *call_args[32];
      CType call_arg_types[32];
      TokenString *parse_args;
      TokenString *saved_macro_stack;
      const int *saved_macro_ptr;
      int call_arg_count, saved_tok;
      CValue saved_tokc;
      if (saw_arg)
        ++arg_count;
      if (field_struct)
      {
        ctor_type.t = field_struct->type.t;
        ctor_type.ref = field_struct;
      }
      parse_args = tok_str_alloc();
      tok_str_append(parse_args, args);
      tok_str_add(parse_args, ')');
      tok_str_add(parse_args, TOK_EOF);
      saved_tok = tok;
      saved_tokc = tokc;
      saved_macro_stack = macro_stack;
      saved_macro_ptr = macro_ptr;
      begin_macro(parse_args, 1);
      next();
      call_arg_count = count_saved_call_args(call_args, 32);
      infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
      /* Token advancement can close the owned probe automatically.  Only
         unwind frames that remain, then restore the enclosing replay. */
      while (macro_stack && macro_stack != saved_macro_stack)
        end_macro();
      macro_ptr = saved_macro_ptr;
      tok = saved_tok;
      tokc = saved_tokc;
      ctor_func = resolve_member_func_by_arg_types(&ctor_type,
                                                   TOK_CONSTRUCTOR1,
                                                   call_arg_types,
                                                   call_arg_count);
      if (!ctor_func)
        ctor_func = resolve_member_func_by_arg_count(&ctor_type,
                                                     TOK_CONSTRUCTOR1,
                                                     arg_count);
      /* Construct class members through the ordinary placement-new path.
         Calling the resolved lifecycle symbol directly is incorrect for a
         `= default` copy/move constructor: its declaration-only stub has no
         user body, while placement construction expands the memberwise
         defaulted operation.  Keeping all member initializers on that path
         also gives explicit and synthesized construction identical overload
         and lifetime handling. */
      tok_str_add(prefix, tok_alloc_const("new"));
      tok_str_add(prefix, '(');
      tok_str_add(prefix, '&');
      tok_str_add(prefix, '(');
      tok_str_add(prefix, this_tok);
      tok_str_add(prefix, TOK_ARROW);
      tok_str_add(prefix, storage_field_tok);
      tok_str_add(prefix, ')');
      tok_str_add(prefix, ')');
      tok_str_add(prefix, field_struct_tok);
      tok_str_add(prefix, '(');
      if (saw_arg)
      {
        tok_str_append(prefix, args);
      }
      tok_str_add(prefix, ')');
      tok_str_add(prefix, ';');
    }
    else
    {
      tok_str_add(prefix, this_tok);
      tok_str_add(prefix, TOK_ARROW);
      tok_str_add(prefix, field_tok);
      tok_str_add(prefix, '=');
      tok_str_append(prefix, args);
      tok_str_add(prefix, ';');
    }
    tok_str_free(args);

    if (tok != ',')
      break;
    next();
  }

  return prefix;
}

static int constructor_field_was_explicitly_initialized(TokenString *fields,
                                                        int field_tok)
{
  int i;
  if (!fields)
    return 0;
  for (i = 0; i < fields->len; ++i)
    if (fields->str[i] == field_tok)
      return 1;
  return 0;
}

static TokenString *parse_constructor_member_initializers(CType *struct_type)
{
  TokenString *initialized_fields = tok_str_alloc();
  TokenString *prefix;
  Sym *field;
  int had_explicit_list = tok == ':';
  int is_delegating_constructor = 0;
  int this_tok = tok_alloc_const("this");

  prefix = parse_explicit_constructor_member_initializers(
    struct_type, initialized_fields, &is_delegating_constructor);

  /* In-class member initializers are part of every non-delegating
     constructor unless that constructor names the member explicitly.  Saved
     field tokens are replayed through the same lowering used by a source
     member-initializer list, which preserves constructor calls for class
     fields instead of degrading them to assignment. */
  for (field = struct_type && struct_type->ref ? struct_type->ref->next : NULL;
       field && !is_delegating_constructor; field = field->next)
  {
    TokenString *synthetic, *field_prefix;
    TokenString *saved_macro_stack;
    const int *saved_macro_ptr;
    int field_tok = field->v & ~SYM_FIELD;
    int saved_tok, i;
    CValue saved_tokc;
    int field_struct_tok = get_struct_type_name_tok(&field->type);
    int needs_default_init = (field->type.t & VT_BTYPE) == VT_STRUCT
                             && field_struct_tok
                             && field->type.ref;

    if (field_tok < TOK_UIDENT || field_tok >= SYM_FIRST_ANOM
        || constructor_field_was_explicitly_initialized(initialized_fields,
                                                        field_tok)
        || (!field->default_arg && !needs_default_init))
      continue;

    if (!field->default_arg && needs_default_init)
    {
      if (!prefix)
        prefix = tok_str_alloc();
      /* Every omitted class member is default-initialized, including classes
         whose default constructor is implicit.  Placement construction keeps
         explicit, defaulted, and implicit constructors on the same recursive
         initialization path; calling only a declared lifecycle symbol leaves
         implicit ownership state as uninitialized storage. */
      tok_str_add(prefix, tok_alloc_const("new"));
      tok_str_add(prefix, '(');
      tok_str_add(prefix, '&');
      tok_str_add(prefix, '(');
      tok_str_add(prefix, this_tok);
      tok_str_add(prefix, TOK_ARROW);
      tok_str_add(prefix, field_tok);
      tok_str_add(prefix, ')');
      tok_str_add(prefix, ')');
      tok_str_add(prefix, field_struct_tok);
      tok_str_add(prefix, '(');
      tok_str_add(prefix, ')');
      tok_str_add(prefix, ';');
      continue;
    }

    synthetic = tok_str_alloc();
    tok_str_add(synthetic, ':');
    tok_str_add(synthetic, field_tok);
    tok_str_add(synthetic, '(');
    if (field->default_arg)
      for (i = 0; i < field->default_arg->len; ++i)
      {
        int t = field->default_arg->str[i];
        if (t == TOK_EOF)
          break;
        if (TOK_HAS_VALUE(t))
          tok_str_add_record(synthetic, field->default_arg->str,
                             field->default_arg->len, &i);
        else
          tok_str_add(synthetic, t);
      }
    tok_str_add(synthetic, ')');
    tok_str_add(synthetic, TOK_EOF);

    saved_tok = tok;
    saved_tokc = tokc;
    saved_macro_stack = macro_stack;
    saved_macro_ptr = macro_ptr;
    begin_macro(synthetic, 1);
    next();
    field_prefix = parse_explicit_constructor_member_initializers(
      struct_type, NULL, NULL);
    while (macro_stack && macro_stack != saved_macro_stack)
      end_macro();
    macro_ptr = saved_macro_ptr;
    tok = saved_tok;
    tokc = saved_tokc;
    if (!prefix)
      prefix = tok_str_alloc();
    if (field_prefix)
    {
      tok_str_append(prefix, field_prefix);
      tok_str_free(field_prefix);
    }
  }

  tok_str_free(initialized_fields);
  if (!prefix && had_explicit_list)
    prefix = tok_str_alloc();
  return prefix;
}

static void push_saved_param_scope(TokenString *params, Sym **saved_ls,
                                   int *saved_scope)
{
  TokenString *parse_params;
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;
  int saved_tok, v;
  CValue saved_tokc;
  AttributeDef ad;
  CType param_type;

  *saved_ls = local_stack;
  *saved_scope = local_scope;
  local_stack = NULL;
  ++local_scope;
  sym_push2(&local_stack, SYM_FIELD, 0, 0);
  if (!params || params->len <= 1)
    return;

  parse_params = tok_str_alloc();
  {
    int pi;
    for (pi = 0; pi < params->len; ++pi)
    {
      if (TOK_HAS_VALUE(params->str[pi]))
      {
        tok_str_add_record(parse_params, params->str, params->len, &pi);
        continue;
      }
      tok_str_add(parse_params, params->str[pi]);
    }
  }
  saved_tok = tok;
  saved_tokc = tokc;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  begin_macro(parse_params, 1);
  next();
  while (tok != TOK_EOF)
  {
    int name_tok = 0;
    memset(&ad, 0, sizeof ad);
    if (!parse_btype(&param_type, &ad, 0))
      expect("parameter type");
    type_decl(&param_type, &ad, &name_tok,
              TYPE_DIRECT | TYPE_ABSTRACT | TYPE_PARAM);
    if (tok == '=')
    {
      TokenString *default_arg = NULL;
      next();
      skip_or_save_param_default(&default_arg);
      if (default_arg)
        tok_str_free(default_arg);
    }
    if (name_tok >= TOK_UIDENT)
    {
      convert_parameter_type(&param_type);
      sym_push(name_tok, &param_type, VT_LOCAL | VT_LVAL, 0);
    }
    if (tok == TOK_EOF)
      break;
    skip(',');
  }
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
}

static void pop_saved_param_scope(Sym *saved_ls, int saved_scope)
{
  sym_pop(&local_stack, NULL, 0);
  local_stack = saved_ls;
  local_scope = saved_scope;
}

static int try_parse_cpp_lifecycle_def(void)
{
  int class_tok, method_tok, saved_nb_pending_member_funcs;
  int paren_level;
  CType struct_type;
  Sym *s;
  TokenString *body = NULL, *params = NULL, *init_prefix = NULL;
  TokenString *replay;

  if (tok < TOK_UIDENT)
    return 0;

  replay = tok_str_alloc();
  tok_str_add2(replay, tok, &tokc);
  class_tok = find_current_namespace_tok(tok);
  if (!struct_find(class_tok))
  {
    tok_str_free(replay);
    return 0;
  }
  next();
  if (tok != ':')
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  tok_str_add(replay, tok);
  next();
  if (tok != ':')
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  tok_str_add(replay, tok);
  next();

  /* Walk nested class qualifiers before deciding whether the terminal name
     is a constructor/destructor.  Nested classes are registered under their
     qualified synthetic token (Outer_Inner), while source definitions retain
     the C++ spelling Outer::Inner::Inner(...). */
  while (tok >= TOK_UIDENT
         && !class_tok_matches_unqualified_name(class_tok, tok))
  {
    int nested_name_tok = tok;
    int nested_class_tok = make_static_member_tok(class_tok, nested_name_tok);
    CValue nested_tokc = tokc;
    Sym *nested_sym = struct_find(nested_class_tok);

    if (!nested_sym)
    {
      nested_sym = struct_find(nested_name_tok);
      if (nested_sym)
        nested_class_tok = nested_sym->v & ~SYM_STRUCT;
    }
    if (!nested_sym)
      break;
    next();
    if (tok != ':')
    {
      unget_tok(tok);
      tok = nested_name_tok;
      tokc = nested_tokc;
      break;
    }
    tok_str_add2(replay, nested_name_tok, &nested_tokc);
    tok_str_add(replay, tok);
    next();
    if (tok != ':')
    {
      restore_cpp_lifecycle_probe(replay);
      return 0;
    }
    tok_str_add(replay, tok);
    next();
    class_tok = nested_class_tok;
  }

  if (tok == '~')
  {
    next();
    if (tok != class_tok
        && !class_tok_matches_unqualified_name(class_tok, tok))
      cprime_error("destructor name must match class name");
    method_tok = TOK_DESTRUCTOR1;
    next();
  }
  else if (tok == class_tok
           || class_tok_matches_unqualified_name(class_tok, tok))
  {
    method_tok = TOK_CONSTRUCTOR1;
    next();
  }
  else if (tok == TOK_CONSTRUCTOR1 || tok == TOK_CONSTRUCTOR2
           || tok == TOK_DESTRUCTOR1 || tok == TOK_DESTRUCTOR2)
  {
    method_tok = tok;
    next();
  }
  else
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  tok_str_free(replay);

  s = struct_find(class_tok);
  if (!s)
    cprime_error("unknown class '%s'", get_tok_str(class_tok, NULL));
  struct_type.t = s->type.t;
  struct_type.ref = s;
  if (method_tok == TOK_CONSTRUCTOR1 && !s->a.lifecycle_ctor)
    cprime_error("constructor definition requires constructor() declaration");
  if (method_tok == TOK_DESTRUCTOR1 && !s->a.lifecycle_dtor)
    cprime_error("destructor definition requires destructor() declaration");

  skip('(');
  params = tok_str_alloc();
  paren_level = 0;
  while (tok != TOK_EOF)
  {
    if (tok == ')' && paren_level == 0)
      break;
    if (tok == '(')
      paren_level++;
    else if (tok == ')')
      paren_level--;
    tok_str_add2(params, tok, &tokc);
    next();
  }
  tok_str_add(params, TOK_EOF);
  skip(')');

  if (method_tok == TOK_CONSTRUCTOR1)
    struct_type.ref->a.lifecycle_ctor = 1;
  else if (method_tok == TOK_DESTRUCTOR1)
    struct_type.ref->a.lifecycle_dtor = 1;
  if (params->len <= 1)
    declare_lifecycle_func(&struct_type, method_tok);
  skip_member_func_cv_qualifiers();
  if (tok == ';')
  {
    next();
    return 1;
  }
  if (method_tok == TOK_CONSTRUCTOR1)
  {
    Sym *saved_ls;
    int saved_scope;
    push_saved_param_scope(params, &saved_ls, &saved_scope);
    init_prefix = parse_constructor_member_initializers(&struct_type);
    pop_saved_param_scope(saved_ls, saved_scope);
  }
  if (init_prefix)
    note_struct_member_init_list(class_tok);
  if (tok != '{')
    expect("function definition");

  saved_nb_pending_member_funcs = nb_pending_member_funcs;
  skip_or_save_block(&body);
  if (init_prefix)
  {
    TokenString *combined = tok_str_alloc();
    int i, inserted = 0;
    for (i = 0; i < body->len; ++i)
    {
      if (TOK_HAS_VALUE(body->str[i]))
      {
        tok_str_add_record(combined, body->str, body->len, &i);
        continue;
      }
      tok_str_add(combined, body->str[i]);
      if (!inserted && body->str[i] == '{')
      {
        tok_str_append(combined, init_prefix);
        inserted = 1;
      }
    }
    if (!inserted)
      cprime_error("constructor initializer requires function body");
    tok_str_free(body);
    tok_str_free(init_prefix);
    body = combined;
  }
  add_pending_lifecycle_func(&struct_type, method_tok, params, body, 0, 0);
  if (saved_nb_pending_member_funcs != nb_pending_member_funcs
      && !defer_pending_member_funcs)
    compile_pending_member_funcs(saved_nb_pending_member_funcs);
  if (tok == ';')
    next();
  return 1;
}

static CType make_func_type_from_saved_params(CType *ret_type, TokenString *params)
{
  int saved_tok, v;
  CValue saved_tokc;
  AttributeDef ad;
  CType func_type, param_type;
  Sym *fref, *param, *last;
  TokenString *parse_params;
  Sym *saved_ls;

  func_type.t = VT_FUNC;
  /* Saved-parameter signatures can be built while a local scope is live
     (member resolution inside a function body).  Keep the constructed
     parameter symbols on the global stack so an enclosing scope pop cannot
     unlink them (they carry the real parameter name tokens). */
  saved_ls = local_stack;
  local_stack = NULL;
  func_type.ref = fref = sym_push(SYM_FIELD, ret_type, 0, 0);
  /* Definitions parsed from function declarators carry FUNC_NEW for
     prototyped parameter lists; give the saved-parameter signature the same
     marker so a later definition is not rejected as a redefinition. */
  fref->f.func_type = FUNC_NEW;
  last = NULL;

  if (!params || params->len <= 1)
  {
    local_stack = saved_ls;
    return func_type;
  }

  parse_params = tok_str_alloc();
  {
    const int *p = params->str;
    const int *end = params->str + params->len;
    while (p < end)
    {
      CValue cv;
      int t;
      TOK_GET(&t, &p, &cv);
      tok_str_add2(parse_params, t, &cv);
    }
  }
  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(parse_params, 1);
  next();
  while (tok != TOK_EOF)
  {
    TokenString *default_arg = NULL;
    if (tok == TOK_DOTS)
    {
      fref->f.func_type = FUNC_ELLIPSIS;
      next();
      if (tok != TOK_EOF)
        cprime_error("variadic marker must end parameter list");
      break;
    }
    memset(&ad, 0, sizeof ad);
    if (!parse_btype(&param_type, &ad, 0))
      expect("parameter type");
    v = 0;
    type_decl(&param_type, &ad, &v,
              TYPE_DIRECT | TYPE_ABSTRACT | TYPE_PARAM);
    if (tok == '=')
    {
      next();
      skip_or_save_param_default(&default_arg);
      expand_saved_single_object_macro(&default_arg);
      qualify_saved_default_arg_current_class(&default_arg);
    }
    convert_parameter_type(&param_type);
    /* Saved-parameter signatures are replayed as function definitions;
       parameters must carry the local lvalue storage class so the
       replayed body (and inline emission) reads values instead of
       taking addresses.  Overload metadata also reuses this chain via
       use_overload_func_type(), so r=0 here would corrupt codegen. */
    param = sym_push(SYM_FIELD, &param_type, VT_LOCAL | VT_LVAL, 0);
    param->v = v;
    param->default_arg = default_arg;
    if (!last)
      fref->next = param;
    else
      last->next = param;
    last = param;
    if (tok == TOK_EOF)
      break;
    skip(',');
  }
  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;
  local_stack = saved_ls;
  return func_type;
}

static int make_type_from_saved_type_tokens(CType *type, TokenString *tokens)
{
  AttributeDef ad;
  int saved_tok, name_tok = 0;
  CValue saved_tokc;

  if (!tokens)
    return 0;
  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(tokens, 1);
  next();
  if (!parse_btype(type, &ad, 0))
  {
    end_macro();
    tok = saved_tok;
    tokc = saved_tokc;
    return 0;
  }
  type_decl(type, &ad, &name_tok, TYPE_ABSTRACT);
  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;
  return 1;
}

static int tok_str_add_integer_const_sym(TokenString *str, Sym *s)
{
  CValue cv;
  int const_tok;
  int bt;

  if (!str || !s || (s->r & VT_VALMASK) != VT_CONST
      || (!(s->type.t & VT_CONSTANT) && !IS_ENUM_VAL(s->type.t)))
    return 0;
  bt = s->type.t & VT_BTYPE;
  if (!is_integer_btype(bt))
    return 0;
  cv.i = IS_ENUM_VAL(s->type.t) ? s->enum_val : s->c;
  if (bt == VT_LLONG)
    const_tok = (s->type.t & VT_UNSIGNED) ? TOK_CULLONG : TOK_CLLONG;
  else if (bt == VT_LONG)
    const_tok = (s->type.t & VT_UNSIGNED) ? TOK_CULONG : TOK_CLONG;
  else
    const_tok = (s->type.t & VT_UNSIGNED) ? TOK_CUINT : TOK_CINT;
  tok_str_add2(str, const_tok, &cv);
  return 1;
}

static void skip_or_save_param_default(TokenString **str)
{
  int level = 0;

  if (str)
    *str = tok_str_alloc();
  while (tok != TOK_EOF)
  {
    if (level == 0 && (tok == ',' || tok == ')'))
      break;
    if (str)
    {
      if (nb_defining_class_stack > 0 && tok >= TOK_UIDENT)
      {
        int class_tok = defining_class_stack[nb_defining_class_stack - 1];
        Sym *static_member = find_static_member_by_class_try(class_tok, tok, NULL);
        if (static_member
            && (static_member->r & VT_VALMASK) == VT_CONST)
        {
          tok_str_add(*str, make_static_member_tok(class_tok, tok));
          next();
          continue;
        }
      }
      tok_str_add_tok(*str);
    }
    if (tok == '(' || tok == '{' || tok == '[')
      ++level;
    else if ((tok == ')' || tok == '}' || tok == ']') && level > 0)
      --level;
    next();
  }
  if (str)
    tok_str_add(*str, TOK_EOF);
}

static void expand_saved_single_object_macro(TokenString **str)
{
  TokenString *raw, *expanded;
  Sym *macro_sym, *nested_list = NULL;
  int i, macro_tok;

  if (!str || !(raw = *str))
    return;
  i = 0;
  while (i + 1 < raw->len && raw->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= raw->len || raw->str[i] < TOK_IDENT)
    return;
  macro_tok = raw->str[i++];
  while (i + 1 < raw->len && raw->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= raw->len || raw->str[i] != TOK_EOF)
    return;
  macro_sym = define_find(macro_tok);
  if (!macro_sym || (macro_sym->type.t & MACRO_FUNC))
    return;
  expanded = tok_str_alloc();
  macro_subst(expanded, &nested_list, raw->str);
  tok_str_add(expanded, TOK_EOF);
  tok_str_free(raw);
  *str = expanded;
}

static void qualify_saved_default_arg_current_class(TokenString **str)
{
  TokenString *raw, *qualified;
  int i, class_tok, needs_qualification = 0;

  if (!str || !(raw = *str) || nb_defining_class_stack <= 0)
    return;
  class_tok = defining_class_stack[nb_defining_class_stack - 1];
  for (i = 0; i < raw->len && raw->str[i] != TOK_EOF; ++i)
  {
    int t = raw->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(raw->str, raw->len, i);
      continue;
    }
    if (t >= TOK_UIDENT)
    {
      Sym *static_member = find_static_member_by_class_try(class_tok, t, NULL);
      if (static_member
          && (static_member->r & VT_VALMASK) == VT_CONST)
      {
        needs_qualification = 1;
        break;
      }
    }
  }
  if (!needs_qualification)
    return;

  qualified = tok_str_alloc();
  for (i = 0; i < raw->len; ++i)
  {
    int t = raw->str[i];
    if (TOK_HAS_VALUE(t))
    {
      tok_str_add_record(qualified, raw->str, raw->len, &i);
      continue;
    }
    if (t >= TOK_UIDENT)
    {
      Sym *static_member = find_static_member_by_class_try(class_tok, t, NULL);
      if (static_member
          && (static_member->r & VT_VALMASK) == VT_CONST)
      {
        tok_str_add(qualified, make_static_member_tok(class_tok, t));
        continue;
      }
    }
    tok_str_add(qualified, t);
  }
  tok_str_free(raw);
  *str = qualified;
}

static int parse_cpp_scoped_member_def_body(CType *ret_type, int class_tok)
{
  int method_tok, saved_tok, paren, i, mangled_tok, cv_qualifiers;
  int has_params = 0;
  int is_static_member_def = 0;
  static int scoped_member_this_counter;
  CValue saved_tokc;
  Sym *class_sym, *decl_field, *canonical_sym;
  CType *effective_ret_type;
  CType class_type, lowered_type, static_func_type, canonical_type;
  CType conversion_ret_type;
  int dummy_ofs;
  int this_tok, public_this_tok, has_conversion_ret_type = 0;
  TokenString *params, *body = NULL, *str;

  if (tok == TOK_OPERATOR)
  {
    method_tok = parse_cpp_operator_method_tok();
    has_conversion_ret_type =
      make_type_from_saved_type_tokens(&conversion_ret_type,
                                       last_cpp_conversion_operator_type_tokens);
  }
  else
  {
    if (tok < TOK_UIDENT)
      cprime_error("member function name");
    method_tok = tok;
    next();
  }
  this_tok = 0;
  public_this_tok = tok_alloc_const("this");
  mangled_tok = make_member_func_tok(class_tok, method_tok);
  skip('(');

  params = tok_str_alloc();
  if (tok != ')')
  {
    has_params = 1;
    paren = 0;
    for (;;)
    {
      if (tok == TOK_EOF)
        cprime_error("unexpected end of file in member function parameters");
      if (tok == ')' && paren == 0)
        break;
      if (tok == '(')
        ++paren;
      else if (tok == ')')
        --paren;
      tok_str_add_tok(params);
      next();
    }
  }
  skip(')');
  tok_str_add(params, TOK_EOF);
  cv_qualifiers = skip_member_func_cv_qualifiers();

  if (tok == '{')
    skip_or_save_block(&body);
  else if (tok != ';')
    expect("function definition");

  class_sym = struct_find(class_tok);
  effective_ret_type = ret_type;
  decl_field = NULL;
  canonical_sym = NULL;
  if (!class_sym)
    cprime_error("member function definition requires declared class '%s'",
              get_tok_str(class_tok, NULL));
  class_type.t = class_sym->type.t;
  class_type.ref = class_sym;
  if (has_conversion_ret_type)
    ret_type = &conversion_ret_type;
  static_func_type = make_func_type_from_saved_params(ret_type, params);
  static_func_type.t |= cv_qualifiers;
  if (cv_qualifiers & VT_CONSTANT)
    class_type.t |= VT_CONSTANT;
  canonical_sym = resolve_member_func_by_param_signature(&class_type, method_tok,
                                                         &static_func_type);
  if (canonical_sym)
    mangled_tok = canonical_sym->v;
  if (!canonical_sym)
  {
    int static_base_tok = make_static_member_tok(class_tok, method_tok);
    mangled_tok = make_static_member_func_tok_for_type(static_base_tok,
                                                       &static_func_type);
    canonical_sym = sym_find(mangled_tok);
    if (!canonical_sym)
      canonical_sym = global_symbol_find(mangled_tok);
    if (canonical_sym)
      is_static_member_def = 1;
  }
  if (!canonical_sym)
    canonical_sym = resolve_member_func_by_arg_count(&class_type, method_tok,
                                                     count_param_tokens(params));
  if (canonical_sym)
    mangled_tok = canonical_sym->v;
  if (!canonical_sym)
    canonical_sym = global_symbol_find(mangled_tok);

  decl_field = find_field_try(&class_type, method_tok, &dummy_ofs);
  if (!decl_field)
    decl_field = find_field_try(&class_type, method_tok | SYM_FIELD, &dummy_ofs);
  if (!canonical_sym && decl_field && (decl_field->type.t & VT_BTYPE) == VT_FUNC)
  {
    lowered_type = make_lowered_member_func_type(&class_type, &decl_field->type);
    canonical_sym = external_global_sym(mangled_tok, &lowered_type);
  }

  if (!canonical_sym)
    cprime_error("member function definition requires class member declaration '%s'",
              get_tok_str(method_tok, NULL));
  if ((canonical_sym->type.t & VT_BTYPE) != VT_FUNC || !canonical_sym->type.ref)
    cprime_error("member function target '%s' is not declared as function",
              get_tok_str(mangled_tok, NULL));
  if (!compare_types(&canonical_sym->type.ref->type, ret_type, 1))
    cprime_error("incompatible return type for member function definition '%s'",
              get_tok_str(mangled_tok, NULL));
  effective_ret_type = &canonical_sym->type.ref->type;
  canonical_type = canonical_sym->type;
  if (!is_static_member_def)
  {
    char this_name[256];
    snprintf(this_name, sizeof this_name, "__cprime_this_%s_%d",
             get_tok_str(mangled_tok, NULL), scoped_member_this_counter++);
    this_tok = tok_alloc_const(this_name);
  }

  str = tok_str_alloc();
  if (!add_ctype_tokens(str, effective_ret_type))
    cprime_error("unsupported member function return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  if (!is_static_member_def)
  {
    tok_str_add(str, TOK_STRUCT);
    tok_str_add(str, class_tok);
    if (static_func_type.t & VT_CONSTANT)
      tok_str_add(str, TOK_CONST1);
    tok_str_add(str, '*');
    tok_str_add(str, this_tok);
  }
  if (has_params)
  {
    Sym *def_param = static_func_type.ref
                       ? static_func_type.ref->next : NULL;
    Sym *canonical_param = canonical_sym->type.ref
                             ? canonical_sym->type.ref->next : NULL;
    if (!is_static_member_def && canonical_param)
      canonical_param = canonical_param->next; /* implicit this */
    if (!is_static_member_def && def_param)
      tok_str_add(str, ',');
    for (i = 0; def_param; def_param = def_param->next, ++i)
    {
      CType param_type = canonical_param
                          ? canonical_param->type : def_param->type;
      if (i)
        tok_str_add(str, ',');
      if (!add_ctype_declarator_tokens(str, &param_type,
                                       def_param->v & ~SYM_FIELD))
        cprime_error("unsupported scoped member parameter type");
      if (canonical_param)
        canonical_param = canonical_param->next;
    }
    if (static_func_type.ref
        && static_func_type.ref->f.func_type == FUNC_ELLIPSIS)
    {
      if (!is_static_member_def || static_func_type.ref->next)
        tok_str_add(str, ',');
      tok_str_add(str, TOK_DOTS);
    }
  }
  tok_str_add(str, ')');
  if (body)
  {
    for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
    {
      int prev_i, next_i;

      if (TOK_HAS_VALUE(body->str[i]))
      {
        tok_str_add_record(str, body->str, body->len, &i);
        continue;
      }
      prev_i = tok_str_prev_token_index(body->str, body->len, i);
      next_i = tok_str_next_non_linenum_index(body->str, body->len, i + 1);
      if (!is_static_member_def && body->str[i] == TOK_OPERATOR
          && (prev_i < 0 || (body->str[prev_i] != '.'
                             && body->str[prev_i] != TOK_ARROW))
          && i + 3 < body->len && body->str[i + 1] == '['
          && body->str[i + 2] == ']' && body->str[i + 3] == '(')
      {
        tok_str_add(str, this_tok);
        tok_str_add(str, TOK_ARROW);
      }
      if (body->str[i] >= TOK_UIDENT
          && (prev_i < 0 || body->str[prev_i] != ':')
          && (next_i >= body->len || body->str[next_i] != ':')
          && class_tok_matches_unqualified_name(class_tok, body->str[i]))
      {
        tok_str_add(str, class_tok);
        continue;
      }
      if (!is_static_member_def && body->str[i] >= TOK_UIDENT
          && next_i < body->len
          && (body->str[next_i] == TOK_LT || body->str[next_i] == '<')
          && (prev_i < 0 || (body->str[prev_i] != '.'
                             && body->str[prev_i] != TOK_ARROW
                             && body->str[prev_i] != ':'))
          && class_or_inst_has_member_template_name(class_tok, body->str[i]))
      {
        tok_str_add(str, this_tok);
        tok_str_add(str, TOK_ARROW);
      }
      if (body->str[i] >= TOK_UIDENT
          && (prev_i < 0 || (body->str[prev_i] != '.'
                              && body->str[prev_i] != TOK_ARROW
                              && (body->str[prev_i] != ':'
                                  || tok_str_prev_token_index(body->str,
                                                              body->len,
                                                              prev_i) < 0
                                  || body->str[tok_str_prev_token_index(
                                       body->str, body->len, prev_i)] != ':')))
          )
      {
        int field_ofs, field_owner_tok = 0;
        Sym *static_member =
          find_static_member_by_class_try(class_tok, body->str[i], NULL);
        if (static_member
            && ((static_member->type.t & VT_BTYPE) != VT_FUNC))
        {
          /* Integral in-class constants have no required storage definition
             when used as values.  Preserve that C++ behavior in replayed
             bodies instead of emitting a relocation to a declaration-only
             static data symbol (for example std::wstring::npos). */
          if ((static_member->r & VT_VALMASK) != VT_CONST
              || !tok_str_add_integer_const_sym(str, static_member))
            tok_str_add(str, make_static_member_tok(class_tok, body->str[i]));
          continue;
        }
        if (!is_static_member_def
            && !saved_params_contains_name(params, body->str[i])
            && !body_identifier_is_decl_target(body, i, prev_i, next_i)
            && find_field_try_with_owner(&class_type, body->str[i],
                                         &field_ofs, &field_owner_tok))
        {
          if (field_owner_tok && field_owner_tok != class_tok)
          {
            tok_str_add(str, '(');
            tok_str_add(str, '(');
            tok_str_add(str, TOK_STRUCT);
            tok_str_add(str, field_owner_tok);
            tok_str_add(str, '*');
            tok_str_add(str, ')');
            tok_str_add(str, this_tok);
            tok_str_add(str, ')');
            tok_str_add(str, TOK_ARROW);
          }
          else
          {
            tok_str_add(str, this_tok);
            tok_str_add(str, TOK_ARROW);
          }
        }
      }
      if (body->str[i] >= TOK_UIDENT && i + 3 < body->len
          && body->str[i + 1] == ':' && body->str[i + 2] == ':'
          && body->str[i + 3] >= TOK_UIDENT
          && struct_find(body->str[i])
          && class_has_static_member_func(body->str[i], body->str[i + 3]))
      {
        tok_str_add(str, make_static_member_tok(body->str[i], body->str[i + 3]));
        i += 3;
        continue;
      }
      if (body->str[i] >= TOK_UIDENT && next_i < body->len
          && body->str[next_i] == '('
          && (prev_i < 0 || (body->str[prev_i] != '.'
                             && body->str[prev_i] != TOK_ARROW
                             && body->str[prev_i] != ':')))
      {
        int call_arg_count = count_call_arg_tokens_in_str(body, next_i);
        int static_tok = make_static_member_tok(class_tok, body->str[i]);
        if (call_arg_count >= 0
            && is_static_member_def
            && class_has_static_member_func(class_tok, body->str[i]))
        {
          tok_str_add(str, class_tok);
          tok_str_add(str, ':');
          tok_str_add(str, ':');
        }
        else if (call_arg_count >= 0
                 && !is_static_member_def
                 && free_func_overload_exists_for_arg_count(static_tok,
                                                            call_arg_count)
                 && !member_overload_exists_for_call(&class_type,
                                                     body->str[i],
                                                     call_arg_count))
        {
          /* Static overloads declared on the class are replayed as free
             functions.  Qualify them before considering instance members so
             nested static calls do not receive an implicit this argument. */
          tok_str_add(str, class_tok);
          tok_str_add(str, ':');
          tok_str_add(str, ':');
        }
        else if (!is_static_member_def
                 && type_has_member_func_name(&class_type, body->str[i]))
        {
          int owner_tok = member_func_owner_for_call(&class_type, body->str[i],
                                                     call_arg_count);
          if (!owner_tok)
          {
            int member_ofs, field_owner_tok = 0;
            Sym *member_field =
              find_field_try_with_owner(&class_type, body->str[i],
                                        &member_ofs, &field_owner_tok);
            if (member_field
                && ((member_field->type.t & VT_BTYPE) == VT_FUNC))
              owner_tok = field_owner_tok ? field_owner_tok : class_tok;
          }
          if (owner_tok == class_tok)
          {
            tok_str_add(str, this_tok);
            tok_str_add(str, TOK_ARROW);
          }
          else if (owner_tok)
          {
            tok_str_add(str, '(');
            tok_str_add(str, '(');
            tok_str_add(str, TOK_STRUCT);
            tok_str_add(str, owner_tok);
            tok_str_add(str, '*');
            tok_str_add(str, ')');
            tok_str_add(str, this_tok);
            tok_str_add(str, ')');
            tok_str_add(str, TOK_ARROW);
          }
        }
        else if (class_has_static_member_func(class_tok, body->str[i]))
        {
          /* Unqualified static member calls in an out-of-class body must be
             qualified as Class::Member so they resolve to the static member
             instead of an implicit global function. */
          tok_str_add(str, class_tok);
          tok_str_add(str, ':');
          tok_str_add(str, ':');
        }
      }
      if (body->str[i] == public_this_tok)
        tok_str_add(str, this_tok);
      else
        tok_str_add(str, body->str[i]);
    }
    if (tok == ';')
      next();
  }
  else
  {
    tok_str_add(str, ';');
    if (tok == ';')
      next();
  }
  tok_str_add(str, TOK_EOF);

  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(str, 1);
  next();
  decl(VT_CONST);
  end_macro();
  canonical_sym = sym_find(mangled_tok);
  if (!canonical_sym)
    canonical_sym = global_symbol_find(mangled_tok);
  if (canonical_sym)
  {
    CType *overload_type = find_overload_func_type_by_mangled(mangled_tok);
    if (overload_type)
      use_overload_func_type(canonical_sym, overload_type);
    preserve_func_default_args(&canonical_sym->type, &canonical_type);
  }
  tok = saved_tok;
  tokc = saved_tokc;
  return 1;
}

static int try_parse_cpp_scoped_member_def(CType *ret_type)
{
  int class_tok, member_tok, saved_tok;
  CValue saved_tokc;
  Sym *decl_sym;
  TokenString *replay, *str;

  if (tok < TOK_UIDENT)
    return 0;

  replay = tok_str_alloc();
  tok_str_add2(replay, tok, &tokc);
  class_tok = find_current_namespace_tok(tok);
  if (!struct_find(class_tok))
  {
    tok_str_free(replay);
    return 0;
  }
  next();
  if (tok != ':')
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  tok_str_add(replay, tok);
  next();
  if (tok != ':')
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  tok_str_free(replay);
  next();

  while (tok >= TOK_UIDENT)
  {
    int nested_name_tok = tok;
    int nested_class_tok = make_static_member_tok(class_tok, nested_name_tok);
    CValue nested_tokc = tokc;
    Sym *nested_sym = struct_find(nested_class_tok);

    if (!nested_sym)
    {
      nested_sym = struct_find(nested_name_tok);
      if (nested_sym)
        nested_class_tok = nested_sym->v & ~SYM_STRUCT;
    }
    if (!nested_sym)
      break;
    next();
    if (tok != ':')
    {
      unget_tok(tok);
      tok = nested_name_tok;
      tokc = nested_tokc;
      break;
    }
    next();
    if (tok != ':')
      cprime_error("':' expected");
    next();
    class_tok = nested_class_tok;
  }

  if (tok >= TOK_UIDENT)
  {
    replay = tok_str_alloc();
    tok_str_add2(replay, tok, &tokc);
    member_tok = tok;
    next();
    if (tok != '(')
    {
      member_tok = make_static_member_tok(class_tok, member_tok);
      decl_sym = sym_find(member_tok);
      if (!decl_sym)
        decl_sym = global_symbol_find(member_tok);
      if (!decl_sym)
        cprime_error("static data member definition requires class member declaration '%s'",
                  get_tok_str(member_tok, NULL));

      str = tok_str_alloc();
      if (!add_ctype_tokens(str, ret_type))
        cprime_error("unsupported static data member type");
      tok_str_add(str, member_tok);
      while (tok != TOK_EOF)
      {
        tok_str_add_tok(str);
        if (tok == ';')
        {
          next();
          break;
        }
        next();
      }
      tok_str_add(str, TOK_EOF);
      saved_tok = tok;
      saved_tokc = tokc;
      begin_macro(str, 1);
      next();
      decl(VT_CONST);
      end_macro();
      tok = saved_tok;
      tokc = saved_tokc;
      tok_str_free(replay);
      return 1;
    }
    restore_cpp_lifecycle_probe(replay);
  }

  return parse_cpp_scoped_member_def_body(ret_type, class_tok);
}

static int try_parse_cpp_scoped_member_def_after_declarator(CType *ret_type,
                                                           int class_tok)
{
  if (tok != ':' || class_tok < TOK_UIDENT)
    return 0;
  class_tok = find_current_namespace_tok(class_tok);
  next();
  if (tok != ':')
    cprime_error("':' expected");
  next();
  while (tok >= TOK_UIDENT)
  {
    int nested_name_tok = tok;
    int nested_class_tok = make_static_member_tok(class_tok, nested_name_tok);
    CValue nested_tokc = tokc;
    Sym *nested_sym = struct_find(nested_class_tok);

    if (!nested_sym)
    {
      nested_sym = struct_find(nested_name_tok);
      if (nested_sym)
        nested_class_tok = nested_sym->v & ~SYM_STRUCT;
    }
    if (!nested_sym)
      break;
    next();
    if (tok != ':')
    {
      unget_tok(tok);
      tok = nested_name_tok;
      tokc = nested_tokc;
      break;
    }
    next();
    if (tok != ':')
      cprime_error("':' expected");
    next();
    class_tok = nested_class_tok;
  }
  return parse_cpp_scoped_member_def_body(ret_type, class_tok);
}

static int try_rewrite_cpp_scoped_static_data_after_declarator(CType *type,
                                                              AttributeDef *ad,
                                                              int *pv)
{
  int class_tok, member_tok;
  Sym *class_sym, *decl_sym;
  TokenString *replay;

  class_tok = *pv;
  if (tok != ':' || class_tok < TOK_UIDENT)
    return 0;
  class_tok = find_current_namespace_tok(class_tok);
  replay = tok_str_alloc();
  tok_str_add(replay, tok);
  next();
  if (tok != ':')
    cprime_error("':' expected");
  tok_str_add(replay, tok);
  next();
  if (tok == TOK_OPERATOR)
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  if (tok < TOK_UIDENT)
    cprime_error("static data member name");
  member_tok = tok;
  tok_str_add2(replay, tok, &tokc);
  next();
  {
    int nested_class_tok = make_static_member_tok(class_tok, member_tok);
    Sym *nested_sym = struct_find(nested_class_tok);
    if (!nested_sym)
      nested_sym = struct_find(member_tok);
    if (nested_sym && tok == ':')
    {
      /* This is Outer::Inner::Method after the declarator, not a static data
         definition named Inner.  Leave the qualifier for the scoped-member
         parser below. */
      restore_cpp_lifecycle_probe(replay);
      return 0;
    }
  }
  if (tok == '(')
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  tok_str_free(replay);

  class_sym = struct_find(class_tok);
  if (!class_sym)
    cprime_error("static data member definition requires declared class '%s'",
              get_tok_str(class_tok, NULL));
  member_tok = make_static_member_tok(class_tok, member_tok);
  decl_sym = sym_find(member_tok);
  if (!decl_sym)
    decl_sym = global_symbol_find(member_tok);
  if (!decl_sym)
    cprime_error("static data member definition requires class member declaration '%s'",
              get_tok_str(member_tok, NULL));
  if (tok == '[')
  {
    int dummy_v = 0;
    int pushed_class = 0;
    /* The declarator-id in an out-of-class static member definition enters
       class scope.  Parse array bounds only after replacing Class::member,
       while making enum constants and other static members visible. */
    if (nb_defining_class_stack
          < (int)(sizeof(defining_class_stack) / sizeof(defining_class_stack[0])))
    {
      defining_class_stack[nb_defining_class_stack++] = class_tok;
      pushed_class = 1;
    }
    type_decl(type, ad, &dummy_v, TYPE_ABSTRACT);
    if (pushed_class)
      --nb_defining_class_stack;
  }
  type->t &= ~VT_STATIC;
  *pv = member_tok;
  return 1;
}

static Sym *find_field (CType *type, int v, int *cumofs)
{
  Sym *s = type->ref;
  int v1 = v | SYM_FIELD;
  if (!(v & SYM_FIELD))   // Top-Level Call
  {
    if ((type->t & VT_BTYPE) != VT_STRUCT)
    {
      if (getenv("CPC_DUMP_FIELD_FAIL"))
        fprintf(stderr,
                "CPC_FIELD_NONSTRUCT func=%s t=%04x ref=%s field=%s\n",
                funcname ? funcname : "<none>", type->t,
                type->ref ? get_tok_str(type->ref->v & ~SYM_STRUCT, NULL)
                          : "<null>",
                v >= TOK_IDENT ? get_tok_str(v, NULL) : "<token>");
      expect("struct or union");
    }
    if (v < TOK_UIDENT)
      expect("field name");
    if (s->c < 0)
      cprime_error("dereferencing incomplete type '%s'",
                get_tok_str(s->v & ~SYM_STRUCT, 0));
  }
  while ((s = s->next) != NULL)
  {
    if (s->v == v1)
    {
      *cumofs = s->c;
      return s;
    }
    if ((s->type.t & VT_BTYPE) == VT_STRUCT
        && s->v >= (SYM_FIRST_ANOM | SYM_FIELD))
    {
      // Try To Find Field In Anonymous Sub-Struct/Union
      Sym *ret = find_field (&s->type, v1, cumofs);
      if (ret)
      {
        *cumofs += s->c;
        return ret;
      }
    }
  }
  if (!(v & SYM_FIELD))
  {
    if (getenv("CPC_DUMP_FIELD_FAIL"))
    {
      fprintf(stderr, "CPC_FIELD_FAIL func=%s type=%s t=%04x field=%s\n",
              funcname ? funcname : "<none>",
              type->ref ? get_tok_str(type->ref->v & ~SYM_STRUCT, NULL)
                        : "<null>",
              type->t, get_tok_str(v, NULL));
      if (type->ref)
      {
        Sym *d = type->ref->next;
        int n = 0;
        while (d && n++ < 32)
        {
          fprintf(stderr, "  member v=%s raw=%x type=%04x\n",
                  get_tok_str(d->v & ~SYM_FIELD, NULL), d->v, d->type.t);
          d = d->next;
        }
      }
    }
    cprime_error("field not found: %s", get_tok_str(v, NULL));
  }
  return s;
}

static int saved_params_contains_name(TokenString *params, int name_tok)
{
  int i;

  if (!params || name_tok < TOK_UIDENT)
    return 0;
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    int next_i;
    if (TOK_HAS_VALUE(params->str[i]))
    {
      i += tok_str_value_extra_words(params->str, params->len, i);
      continue;
    }
    if ((params->str[i] & ~SYM_FIELD) != (name_tok & ~SYM_FIELD))
      continue;
    next_i = tok_str_next_non_linenum_index(params->str, params->len, i + 1);
    if (next_i >= params->len || params->str[next_i] == TOK_EOF
        || params->str[next_i] == ',' || params->str[next_i] == '=')
      return 1;
  }
  return 0;
}

static int tok_is_decl_type_name(int t)
{
  Sym *s;

  if (t < TOK_UIDENT)
    return 0;
  if (struct_find(t))
    return 1;
  s = sym_find(t);
  if (!s)
    s = global_symbol_find(t);
  return s && ((s->type.t & VT_TYPEDEF) || (s->type.t & VT_ENUM));
}

static int token_can_start_parameter_declaration(int t)
{
  switch (t)
  {
  case TOK_VOID:
  case TOK_CHAR:
  case TOK_SHORT:
  case TOK_INT:
  case TOK_LONG:
  case TOK_BOOL:
  case TOK_BOOL2:
  case TOK_FLOAT:
  case TOK_DOUBLE:
  case TOK_ENUM:
  case TOK_STRUCT:
  case TOK_CLASS:
  case TOK_UNION:
  case TOK__Atomic:
  case TOK_CONST1:
  case TOK_CONST2:
  case TOK_CONST3:
  case TOK_VOLATILE1:
  case TOK_VOLATILE2:
  case TOK_VOLATILE3:
  case TOK_SIGNED1:
  case TOK_SIGNED2:
  case TOK_SIGNED3:
  case TOK_UNSIGNED:
  case TOK_AUTO:
  case TOK_DECLTYPE:
  case TOK_REGISTER:
    return 1;
  default:
    return t >= TOK_UIDENT
           && tok_is_decl_type_name(find_current_namespace_tok(t));
  }
}

static int local_paren_starts_direct_initializer(void)
{
  TokenString *replay;
  int is_initializer;

  if (tok != '(')
    return 0;
  replay = tok_str_alloc();
  tok_str_add2(replay, tok, &tokc);
  next();
  /* Empty parentheses and a leading type form a block-scope function
     declaration (the classic most-vexing parse).  An expression starter is
     direct initialization, including scalar declarations such as
     `const char* text("value")`. */
  is_initializer = tok != ')' && !token_can_start_parameter_declaration(tok);
  restore_cpp_lifecycle_probe(replay);
  return is_initializer;
}

static int body_identifier_is_decl_target(TokenString *body, int i,
                                          int prev_i, int next_i)
{
  int prev_prev_i;

  if (!body || next_i >= body->len || body->str[next_i] != '=')
    return 0;
  if (prev_i >= 0 && tok_is_decl_type_name(body->str[prev_i]))
    return 1;
  if (prev_i >= 0 && body->str[prev_i] == '*')
  {
    prev_prev_i = tok_str_prev_token_index(body->str, body->len, prev_i);
    if (prev_prev_i >= 0 && tok_is_decl_type_name(body->str[prev_prev_i]))
      return 1;
  }
  return 0;
}

static void note_class_base(int class_tok, int base_tok)
{
  ClassBaseInfo *info;

  if (!class_tok || !base_tok)
    return;
  for (info = class_base_infos; info; info = info->next)
    if (info->class_tok == class_tok && info->base_tok == base_tok)
      return;
  info = cprime_mallocz(sizeof(*info));
  info->class_tok = class_tok;
  info->base_tok = base_tok;
  info->next = class_base_infos;
  class_base_infos = info;
}

static int class_has_base(int class_tok, int base_tok)
{
  ClassBaseInfo *info;

  for (info = class_base_infos; info; info = info->next)
    if (info->class_tok == class_tok
        && (info->base_tok == base_tok || class_has_base(info->base_tok, base_tok)))
      return 1;
  return 0;
}

static int class_subobject_offset(int class_tok, int base_tok, int *offset)
{
  ClassBaseInfo *base;

  if (class_tok == base_tok)
  {
    *offset = 0;
    return 1;
  }
  for (base = class_base_infos; base; base = base->next)
    if (base->class_tok == class_tok && base->field)
    {
      int nested;
      if (class_subobject_offset(base->base_tok, base_tok, &nested))
      {
        *offset = base->field->c + nested;
        return 1;
      }
    }
  return 0;
}

static int make_class_type_from_tok(CType *type, int class_tok)
{
  Sym *class_sym = struct_find(class_tok);

  if (!class_sym)
    return 0;
  type->t = class_sym->type.t;
  type->ref = class_sym;
  return 1;
}

static Sym *find_base_field_try(int class_tok, int v, int *cumofs,
                                int *owner_tok)
{
  ClassBaseInfo *info;

  for (info = class_base_infos; info; info = info->next)
  {
    CType base_type;
    Sym *ret;

    if (info->class_tok != class_tok)
      continue;
    if (!make_class_type_from_tok(&base_type, info->base_tok))
      continue;
    ret = find_field_try(&base_type, v, cumofs);
    if (ret)
    {
      if (owner_tok)
        *owner_tok = info->base_tok;
      return ret;
    }
    ret = find_base_field_try(info->base_tok, v, cumofs, owner_tok);
    if (ret)
      return ret;
  }
  return NULL;
}

// Non-Throwing Field Lookup Used By Syntax Sugar Probes
static Sym *find_field_try(CType *type, int v, int *cumofs)
{
  Sym *s = type->ref;
  int v1 = (v &SYM_FIELD) ? v : (v | SYM_FIELD);

  if (!s)
    return NULL;

  while ((s = s->next) != NULL)
  {
    if (s->v == v1)
    {
      *cumofs = s->c;
      return s;
    }
    if ((s->type.t & VT_BTYPE) == VT_STRUCT
        && s->v >= (SYM_FIRST_ANOM | SYM_FIELD))
    {
      Sym *ret = find_field_try(&s->type, v1, cumofs);
      if (ret)
      {
        *cumofs += s->c;
        return ret;
      }
    }
  }
  {
    int class_tok = get_struct_type_name_tok(type);
    if (class_tok)
      return find_base_field_try(class_tok, v1, cumofs, NULL);
  }
  return NULL;
}

static Sym *find_field_try_with_owner(CType *type, int v, int *cumofs,
                                      int *owner_tok)
{
  Sym *s = type->ref;
  int v1 = (v &SYM_FIELD) ? v : (v | SYM_FIELD);

  if (owner_tok)
    *owner_tok = get_struct_type_name_tok(type);
  if (!s)
    return NULL;

  while ((s = s->next) != NULL)
  {
    if (s->v == v1)
    {
      *cumofs = s->c;
      return s;
    }
    if ((s->type.t & VT_BTYPE) == VT_STRUCT
        && s->v >= (SYM_FIRST_ANOM | SYM_FIELD))
    {
      Sym *ret = find_field_try(&s->type, v1, cumofs);
      if (ret)
      {
        *cumofs += s->c;
        return ret;
      }
    }
  }
  {
    int class_tok = get_struct_type_name_tok(type);
    if (class_tok)
      return find_base_field_try(class_tok, v1, cumofs, owner_tok);
  }
  return NULL;
}

static Sym *find_static_member_by_class_try(int class_tok, int member_tok,
                                            int *owner_tok)
{
  ClassBaseInfo *info;
  Sym *s;
  int static_tok;

  if (!class_tok)
    return NULL;
  static_tok = make_static_member_tok(class_tok, member_tok);
  s = sym_find(static_tok);
  if (!s)
    s = global_symbol_find(static_tok);
  if (s)
  {
    if (owner_tok)
      *owner_tok = class_tok;
    return s;
  }
  for (info = class_base_infos; info; info = info->next)
  {
    if (info->class_tok != class_tok)
      continue;
    s = find_static_member_by_class_try(info->base_tok, member_tok,
                                        owner_tok);
    if (s)
      return s;
  }
  return NULL;
}

static int class_has_static_member_func(int class_tok, int member_tok)
{
  TemplateDef *td;
  int static_tok = make_static_member_tok(class_tok, member_tok);

  /* Static member functions are registered as free-function overloads by
     declare_static_member_func; instance members are not.  Symbol lookup is
     not usable here because instance members share the same plain mangled
     token (Class_Member). */
  if (has_free_func_overload(static_tok))
    return 1;
  /* Static member templates declared inline in the class body (e.g.
     template<typename U> static M<U> Make(...)) are captured in
     template_member_defs but never reach declare_static_member_func, so
     also recognize them through the template class's member table. */
  td = find_class_template_def_for_class_tok(class_tok);
  if (td)
  {
    TemplateMemberDef *md;
    for (md = template_member_candidates(td->name_tok, member_tok);
         md; md = md->bucket_next)
    {
      int j, saw_static = 0, scan_limit;
      if (md->class_tok != td->name_tok
          || template_member_def_method_tok(md) != member_tok)
        continue;
      /* The captured declaration may contain stray keyword tokens inside
         its body (template capture artifacts), so only scan the signature
         portion before the body or a terminating semicolon. */
      scan_limit = md->def_str->len;
      for (j = 0; j + 1 < md->def_str->len; ++j)
        if (md->def_str->str[j] == '{' || md->def_str->str[j] == ';')
        {
          scan_limit = j;
          break;
        }
      for (j = 0; j + 1 < scan_limit; ++j)
        if (md->def_str->str[j] == TOK_STATIC
            || (md->def_str->str[j] >= TOK_UIDENT
                && !strcmp(get_tok_str(md->def_str->str[j], NULL), "static")))
          saw_static = 1;
      if (saw_static)
        return 1;
    }
  }
  return 0;
}

static Sym *find_static_member_try(CType *type, int member_tok, int *owner_tok)
{
  return find_static_member_by_class_try(get_struct_type_name_tok(type),
                                         member_tok, owner_tok);
}

static void check_fields (CType *type, int check)
{
  Sym *s = type->ref;

  while ((s = s->next) != NULL)
  {
    int v = s->v & ~SYM_FIELD;
    if (v < SYM_FIRST_ANOM)
    {
      TokenSym *ts = table_ident[v - TOK_IDENT];
      if (check && (ts->tok & SYM_FIELD))
        cprime_error("duplicate member '%s'", get_tok_str(v, NULL));
      if (check)
        ts->tok |= SYM_FIELD;
      else
        ts->tok &= ~SYM_FIELD;
    }
    else if ((s->type.t & VT_BTYPE) == VT_STRUCT
             && !get_struct_type_name_tok(&s->type))
      check_fields (&s->type, check);
  }
}

static void struct_layout(CType *type, AttributeDef *ad)
{
  int size, align, maxalign, offset, c, bit_pos, bit_size;
  int packed, a, bt, prevbt, prev_bit_size;
  int pcc = !cprime_state->ms_bitfields;
  int pragma_pack = *cprime_state->pack_stack_ptr;
  Sym *f;

  maxalign = 1;
  offset = 0;
  c = 0;
  bit_pos = 0;
  prevbt = VT_STRUCT; // Make It Never Match
  prev_bit_size = 0;

  // #define BF_DEBUG

  for (f = type->ref->next; f; f = f->next)
  {
    if (f->type.t & VT_BITFIELD)
      bit_size = BIT_SIZE(f->type.t);
    else
      bit_size = -1;
    size = type_size(&f->type, &align);
    a = f->a.aligned ? 1 << (f->a.aligned - 1) : 0;
    packed = 0;

    if (pcc && bit_size == 0)
    {
      // in pcc mode, packing does not affect zero-width bitfields

    }
    else
    {
      // in pcc mode, attribute packed overrides if set.
      if (pcc && (f->a.packed || ad->a.packed))
        align = packed = 1;

      // pragma pack overrides align if lesser and packs bitfields always
      if (pragma_pack)
      {
        packed = 1;
        if (pragma_pack < align)
          align = pragma_pack;
        // In Pcc Mode Pragma Pack Also Overrides Individual Align
        if (pcc && pragma_pack < a)
          a = 0;
      }
    }
    // Some Individual Align Was Specified
    if (a)
      align = a;

    if (type->ref->type.t == VT_UNION)
    {
      if (pcc && bit_size >= 0)
        size = (bit_size + 7) >> 3;
      offset = 0;
      if (size > c)
        c = size;

    }
    else if (bit_size < 0)
    {
      if (pcc)
        c += (bit_pos + 7) >> 3;
      c = (c + align - 1) & -align;
      offset = c;
      if (size > 0)
        c += size;
      bit_pos = 0;
      prevbt = VT_STRUCT;
      prev_bit_size = 0;

    }
    else
    {
      /* A bit-field.  Layout is more complicated.  There are two
         options: PCC (GCC) compatible and MS compatible */
      if (pcc)
      {
        /* In PCC layout a bit-field is placed adjacent to the
                       preceding bit-fields, except if:
                       - it has zero-width
                       - an individual alignment was given
                       - it would overflow its base type container and
                         there is no packing */
        if (bit_size == 0)
        {
new_field:
          c = (c + ((bit_pos + 7) >> 3) + align - 1) & -align;
          bit_pos = 0;
        }
        else if (f->a.aligned)
          goto new_field;
        else if (!packed)
        {
          int a8 = align * 8;
          int ofs = ((c * 8 + bit_pos) % a8 + bit_size + a8 - 1) / a8;
          if (ofs > size / align)
            goto new_field;
        }

        // in pcc mode, long long bitfields have type int if they fit
        if (size == 8 && bit_size <= 32)
          f->type.t = (f->type.t & ~VT_BTYPE) | VT_INT, size = 4;

        while (bit_pos >= align * 8)
          c += align, bit_pos -= align * 8;
        offset = c;

        /* In PCC layout named bit-fields influence the alignment
           of the containing struct using the base types alignment,
           except for packed fields (which here have correct align).  */
        if (f->v & SYM_FIRST_ANOM
            // && bit_size // ??? gcc on ARM/rpi does that
           )
          align = 1;

      }
      else
      {
        bt = f->type.t &VT_BTYPE;
        if ((bit_pos + bit_size > size * 8)
            || (bit_size > 0) == (bt != prevbt)
           )
        {
          c = (c + align - 1) & -align;
          offset = c;
          bit_pos = 0;
          /* In MS bitfield mode a bit-field run always uses
             at least as many bits as the underlying type.
             To start a new run it's also required that this
             or the last bit-field had non-zero width.  */
          if (bit_size || prev_bit_size)
            c += size;
        }
        /* In MS layout the records alignment is normally
           influenced by the field, except for a zero-width
           field at the start of a run (but by further zero-width
           fields it is again).  */
        if (bit_size == 0 && prevbt != bt)
          align = 1;
        prevbt = bt;
        prev_bit_size = bit_size;
      }

      f->type.t = (f->type.t & ~(0x3f << VT_STRUCT_SHIFT))
                  | (bit_pos << VT_STRUCT_SHIFT);
      bit_pos += bit_size;
    }
    if (align > maxalign)
      maxalign = align;

#ifdef BF_DEBUG
    printf("set field %s offset %-2d size %-2d align %-2d",
           get_tok_str(f->v & ~SYM_FIELD, NULL), offset, size, align);
    if (f->type.t & VT_BITFIELD)
    {
      printf(" pos %-2d bits %-2d",
             BIT_POS(f->type.t),
             BIT_SIZE(f->type.t)
            );
    }
    printf("\n");
#endif

    f->c = offset;
    f->r = 0;
  }

  if (pcc)
    c += (bit_pos + 7) >> 3;

  // Store Size And Alignment
  a = bt = ad->a.aligned ? 1 << (ad->a.aligned - 1) : 1;
  if (a < maxalign)
    a = maxalign;
  type->ref->r = a;
  if (pragma_pack && pragma_pack < maxalign && 0 == pcc)
  {
    /* can happen if individual align for some member was given.  In
       this case MSVC ignores maxalign when aligning the size */
    a = pragma_pack;
    if (a < bt)
      a = bt;
  }
  c = (c + a - 1) & -a;
  type->ref->c = c;

#ifdef BF_DEBUG
  printf("struct size %-2d align %-2d\n\n", c, a), fflush(stdout);
#endif

  // Check Whether We Can Access Bitfields By Their Type
  for (f = type->ref->next; f; f = f->next)
  {
    int s, px, cx, c0;
    CType t;

    if (0 == (f->type.t & VT_BITFIELD))
      continue;
    f->type.ref = f;
    f->auxtype = -1;
    bit_size = BIT_SIZE(f->type.t);
    if (bit_size == 0)
      continue;
    bit_pos = BIT_POS(f->type.t);
    size = type_size(&f->type, &align);

    if (bit_pos + bit_size <= size * 8 && f->c + size <= c
#ifdef CPRIME_TARGET_ARM
        && !(f->c & (align - 1))
#endif
       )
      continue;

    // Try To Access The Field Using A Different Type
    c0 = -1, s = align = 1;
    t.t = VT_BYTE;
    for (;;)
    {
      px = f->c * 8 + bit_pos;
      cx = (px >> 3) & -align;
      px = px - (cx << 3);
      if (c0 == cx)
        break;
      s = (px + bit_size + 7) >> 3;
      if (s > 4)
        t.t = VT_LLONG;
      else if (s > 2)
        t.t = VT_INT;
      else if (s > 1)
        t.t = VT_SHORT;
      else
        t.t = VT_BYTE;
      s = type_size(&t, &align);
      c0 = cx;
    }

    if (px + bit_size <= s * 8 && cx + s <= c
#ifdef CPRIME_TARGET_ARM
        && !(cx & (align - 1))
#endif
       )
    {
      // Update Offset And Bit Position
      f->c = cx;
      bit_pos = px;
      f->type.t = (f->type.t & ~(0x3f << VT_STRUCT_SHIFT))
                  | (bit_pos << VT_STRUCT_SHIFT);
      if (s != size)
        f->auxtype = t.t;
#ifdef BF_DEBUG
      printf("FIX field %s offset %-2d size %-2d align %-2d "
             "pos %-2d bits %-2d\n",
             get_tok_str(f->v & ~SYM_FIELD, NULL),
             cx, s, align, px, bit_size);
#endif
    }
    else
    {
      // Fall Back To Load/Store Single-Byte Wise
      f->auxtype = VT_STRUCT;
#ifdef BF_DEBUG
      printf("FIX field %s : load byte-wise\n",
             get_tok_str(f->v & ~SYM_FIELD, NULL));
#endif
    }
  }
}

// Does 'n' fit into integer type 't' ?
static int in_range(long long n, int t)
{
  unsigned long long m = (1ULL << (btype_size(t &VT_BTYPE) * 8 - 1)) - 1;
  if (t & VT_UNSIGNED)
    return n <= (m << 1) + 1;
  return n >= -(long long)m - 1 && n <= (long long)m;
}

// enum/struct/union declaration. u is VT_ENUM/VT_STRUCT/VT_UNION
static void struct_decl(CType *type, int u, int is_class_tag)
{
  int v, c, size, align, flexible;
  int has_base_classes = 0;
  int bit_size, bsize, bt, ut;
  int member_decl_is_auto;
  Sym *s, *ss, **ps;
  AttributeDef ad, ad1;
  CType type1, btype;

  memset(&ad, 0, sizeof ad);
  next();
  parse_attribute(&ad);

  v = 0;
  if (tok >= TOK_IDENT) // Struct/Enum Tag
  {
    v = tok;
    next();
    if (is_cpp_translation_unit() && nb_defining_class_stack
        && !is_class_template_instantiation_tok(v)
        && !is_current_class_qualified_nested_tok(v))
    {
      if (tok != '{' && tok != ':' && tok != ';')
        v = find_current_class_nested_type_tok(v);
      else
        v = make_current_class_nested_tok(v);
    }
    else if (nb_namespace_stack)
    {
      if (tok != '{' && tok != ':' && tok != ';')
        v = find_current_namespace_tok(v);
      else
        v = make_current_namespace_tok(v);
    }
  }

  bt = ut = 0;
  if (u == VT_ENUM)
  {
    ut = VT_INT;
    if (tok == ':')   // C2x enum : <type> ...
    {
      next();
      if (!parse_btype(&btype, &ad1, 0)
          || !is_integer_btype(btype.t & VT_BTYPE))
        expect("enum type");
      bt = ut = btype.t & (VT_BTYPE | VT_LONG | VT_UNSIGNED | VT_DEFSIGN);
    }
  }

  if (v)
  {
    // Struct Already Defined ? Return It
    s = struct_find(v);
    if (s && (s->sym_scope == local_scope || (tok != '{' && tok != ';')))
    {
      if (u == s->type.t)
        goto do_decl;
      if (u == VT_ENUM && IS_ENUM(s->type.t)) // XXX: check integral types
        goto do_decl;
      cprime_error("redeclaration of '%s'", get_tok_str(v, NULL));
    }
  }
  else
  {
    if (tok != '{')
      expect("struct/union/enum name");
    v = anon_sym++;
  }
  // Record the original enum/struct/union token.
  type1.t = u | ut;
  type1.ref = NULL;
  // We Put An Undefined Size For Struct/Union
  s = sym_push(v | SYM_STRUCT, &type1, 0, bt ? 0 : -1);
  if (is_class_tag)
    s->a.is_class_tag = 1;
  s->r = 0; // Default Alignment Is Zero As Gcc
do_decl:
  type->t = s->type.t;
  type->ref = s;

  if (u == VT_STRUCT && tok == ':')
  {
    has_base_classes = 1;
    next();
    for (;;)
    {
      int base_tok;

      if (tok == tok_public || tok == tok_protected || tok == tok_private)
        next();
      if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "virtual"))
        next();
      if (tok < TOK_UIDENT)
        expect("base class name");
      base_tok = tok;
      base_tok = find_current_namespace_tok(base_tok);
      next();
      if (tok == TOK_LT || tok == '<')
      {
        TemplateDef *base_td = find_class_template_def(base_tok);
        if (base_td && base_td->is_class)
        {
          TemplateArgList base_args;
          parse_template_type_args(&base_args);
          base_tok = instantiate_template_if_needed(base_td, &base_args);
          compile_pending_template_specs_without_member_flush();
        }
        else
        {
          int angle = 1;
          next();
          while (tok != TOK_EOF && angle > 0)
          {
            if (tok == TOK_LT || tok == '<')
              ++angle;
            else if (tok == TOK_GT || tok == '>')
              --angle;
            else if (tok == TOK_SAR)
            {
              --angle;
              if (angle > 0)
                --angle;
            }
            if (angle > 0)
              next();
          }
          skip(TOK_GT);
        }
      }
      note_class_base(s->v & ~SYM_STRUCT, base_tok);
      if (tok != ',')
        break;
      next();
    }
  }

  if (tok == '{')
  {
    int saved_nb_pending_member_funcs = nb_pending_member_funcs;
    next();
    if (s->c != -1
        && !(u == VT_ENUM && s->c == 0)) // Not Yet Defined Typed Enum
      cprime_error("struct/union/enum '%s' already defined",
                   get_tok_str(s->v & ~SYM_STRUCT, NULL));
    s->c = -2;
    if (u == VT_STRUCT && nb_defining_class_stack
        < (int)(sizeof(defining_class_stack) / sizeof(defining_class_stack[0])))
      defining_class_stack[nb_defining_class_stack++] = s->v & ~SYM_STRUCT;
    // Cannot Be Empty
    // Non Empty Enums Are Not Allowed
    ps = &s->next;
    if (u == VT_STRUCT && has_base_classes)
    {
      /* Represent each C++ base subobject in the ordinary aggregate layout.
         The inheritance table remains responsible for name and overload
         lookup, while these anonymous fields give derived objects the
         storage, alignment, and copy/destruction traversal of their bases. */
      ClassBaseInfo *base_info;
      ClassBaseInfo *bases[32];
      int nb_bases = 0, base_i;

      for (base_info = class_base_infos; base_info;
           base_info = base_info->next)
        if (base_info->class_tok == (s->v & ~SYM_STRUCT))
        {
          if (nb_bases >= (int)(sizeof(bases) / sizeof(bases[0])))
            cprime_error("too many base classes for '%s'",
                         get_tok_str(s->v & ~SYM_STRUCT, NULL));
          bases[nb_bases++] = base_info;
        }
      /* note_class_base prepends entries; restore declaration order. */
      for (base_i = nb_bases - 1; base_i >= 0; --base_i)
      {
        CType base_type;
        Sym *base_field;
        if (!make_class_type_from_tok(&base_type, bases[base_i]->base_tok)
            || !base_type.ref || base_type.ref->c < 0)
          cprime_error("base class '%s' is incomplete",
                       get_tok_str(bases[base_i]->base_tok, NULL));
        base_field = sym_push((anon_sym++) | SYM_FIELD, &base_type, 0, 0);
        bases[base_i]->field = base_field;
        *ps = base_field;
        ps = &base_field->next;
      }
    }
    if (u == VT_ENUM)
    {
      long long ll = 0, pl = 0, nl = 0;
      CType t;
      t.ref = s;
      s->sym_scope = local_scope; // Anonymous Symbol Won'T Have Set
      // Enum Symbols Have Static Storage
      t.t = VT_INT | VT_STATIC | VT_ENUM_VAL;
      if (bt)
        t.t = bt | VT_STATIC | VT_ENUM_VAL;
      for (;;)
      {
        v = tok;
        if (v < TOK_UIDENT)
          expect("identifier");
        next();
        if (tok == '=')
        {
          next();
          ll = expr_const64();
        }
        if (bt && !in_range(ll, t.t))
          cprime_error("enumerator '%s' out of range of its type",
                    get_tok_str(v, NULL));
        if (nb_defining_class_stack > 0)
        {
          int class_tok = defining_class_stack[nb_defining_class_stack - 1];
          int static_tok = make_static_member_tok(class_tok, v);
          ss = sym_push(static_tok, &t, VT_CONST, 0);
        }
        else
          ss = sym_push(v, &t, VT_CONST, 0);
        ss->enum_val = ll;
        *ps = ss, ps = &ss->next;
        if (ll < nl)
          nl = ll;
        if (ll > pl)
          pl = ll;
        if (tok != ',')
          break;
        next();
        ll++;
        // NOTE: we accept a trailing comma
        if (tok == '}')
          break;
      }
      skip('}');

      if (bt)
      {
        t.t = bt;
        s->c = 2;
        goto enum_done;
      }

      // Set Integral Type Of The Enum
      t.t = VT_INT;
      if (nl >= 0)
      {
        if (pl != (unsigned)pl)
          t.t = (LONG_SIZE == 8 ? VT_LLONG | VT_LONG : VT_LLONG);
        t.t |= VT_UNSIGNED;
      }
      else if (pl != (int)pl || nl != (int)nl)
        t.t = (LONG_SIZE == 8 ? VT_LLONG | VT_LONG : VT_LLONG);

      // Set Type For Enum Members
      for (ss = s->next; ss; ss = ss->next)
      {
        ll = ss->enum_val;
        if (ll == (int)ll) // default is int if it fits
          continue;
        if (t.t & VT_UNSIGNED)
        {
          ss->type.t |= VT_UNSIGNED;
          if (ll == (unsigned)ll)
            continue;
        }
        ss->type.t = (ss->type.t & ~VT_BTYPE)
                     | (LONG_SIZE == 8 ? VT_LLONG | VT_LONG : VT_LLONG);
      }
      s->c = 1;
enum_done:
      s->type.t = type->t = t.t | VT_ENUM;

    }
    else
    {
      c = 0;
      flexible = 0;
      while (tok != '}')
      {
        int member_func_body = 0;
        int lifecycle_body = 0;
        int lifecycle_tok = 0;
        int lifecycle_saw_paren = 0;
        int lifecycle_explicit = 0;
        int lifecycle_default_suffix = 0;
        int member_decl_is_virtual = 0;
        TokenString *body = NULL, *lifecycle_params = NULL;
        TokenString *lifecycle_init_prefix = NULL;

        while (tok == TOK_LINENUM)
          next();

        if (is_cpp_translation_unit() && tok >= TOK_UIDENT
            && !strcmp(get_tok_str(tok, NULL), "virtual"))
        {
          member_decl_is_virtual = 1;
          next();
        }

        if (tok == tok_public || tok == tok_protected || tok == tok_private)
        {
          int label_tok = tok;
          next();
          if (tok == ':')
          {
            next();
            continue;
          }
          unget_tok(label_tok);
        }

        if (is_template_keyword_tok(tok))
        {
          int class_tok = get_struct_type_name_tok(type);
          TemplateDef *td = find_class_template_def_for_class_tok(class_tok);
          TokenString *member_str = skip_or_save_template_member_decl(1);
          if (member_str && token_string_template_is_friend_decl(member_str))
            tok_str_free(member_str);
          else if (member_str && td && td->nb_type_params > 0)
            add_template_member_def(td->name_tok, td->type_param_toks[0],
                                    NULL, 0, NULL, 0, member_str);
          else if (member_str && class_tok)
            add_template_member_def(class_tok, 0, NULL, 0,
                                    NULL, 0, member_str);
          else if (member_str)
            tok_str_free(member_str);
          continue;
        }

        if (tok == tok_explicit)
        {
          lifecycle_explicit = 1;
          next();
          if (!class_tok_matches_unqualified_name(
                get_struct_type_name_tok(type), tok)
              && tok != TOK_OPERATOR
              && tok != '~')
            cprime_error("explicit is only supported on constructors and conversion operators");
        }

        if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "friend"))
        {
          int fi, skipped_friend = 0, friend_has_body = 0;
          int class_tok = get_struct_type_name_tok(type);
          TokenString *friend_src = skip_or_save_template_member_decl(1);
          TokenString *friend_decl = tok_str_alloc();
          if (friend_src)
          {
            const int *fp = friend_src->str;
            const int *fend = friend_src->str + friend_src->len;
            while (fp < fend)
            {
              int ft;
              CValue ignored;
              TOK_GET(&ft, &fp, &ignored);
              if (ft == '{')
                friend_has_body = 1;
            }
          }
          for (fi = 0; friend_src && fi < friend_src->len; ++fi)
          {
            int ft = friend_src->str[fi];
            if (TOK_HAS_VALUE(ft))
            {
              tok_str_add_record(friend_decl, friend_src->str,
                                 friend_src->len, &fi);
              continue;
            }
            if (!skipped_friend && ft >= TOK_UIDENT
                && !strcmp(get_tok_str(ft, NULL), "friend"))
            {
              skipped_friend = 1;
              if (friend_has_body)
                tok_str_add(friend_decl, TOK_INLINE1);
              continue;
            }
            tok_str_add(friend_decl, ft);
          }
          if (!friend_src || !skipped_friend)
            tok_str_free(friend_decl);
          else
            queue_pending_template_spec(friend_decl, class_tok);
          if (friend_src)
            tok_str_free(friend_src);
          continue;
        }

        if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "using"))
        {
          int alias_tok, dummy_v = 0;
          Sym *alias_sym;

          next();
          if (tok < TOK_UIDENT)
            expect("using alias name");
          alias_tok = tok;
          next();
          skip('=');
          memset(&ad1, 0, sizeof ad1);
          if (!parse_btype(&type1, &ad1, 0))
            expect("using alias type");
          type_decl(&type1, &ad1, &dummy_v, TYPE_ABSTRACT);
          type1.t |= VT_TYPEDEF;
          alias_sym = sym_find(alias_tok);
          if (alias_sym && alias_sym->sym_scope == local_scope)
          {
            if (!is_compatible_types(&alias_sym->type, &type1)
                || !(alias_sym->type.t & VT_TYPEDEF))
              cprime_error("incompatible redefinition of '%s'",
                        get_tok_str(alias_tok, NULL));
            alias_sym->type = type1;
          }
          else
            alias_sym = sym_push(alias_tok, &type1, 0, 0);
          alias_sym->a = ad1.a;
          if ((type1.t & VT_BTYPE) == VT_FUNC)
            merge_funcattr(&alias_sym->type.ref->f, &ad1.f);
          skip(';');
          continue;
        }

        if (tok == TOK_TYPEDEF)
        {
          if (!parse_btype(&btype, &ad1, 0))
            expect("typedef declaration");
          while (1)
          {
            type1 = btype;
            type_decl(&type1, &ad1, &v, TYPE_DIRECT);
            if (v >= TOK_UIDENT && !strcmp(get_tok_str(v, NULL), "wchar_t"))
              type1.t |= VT_WCHAR_T;
            ss = sym_find(v);
            if (ss && ss->sym_scope == local_scope)
            {
              if (!is_compatible_types(&ss->type, &type1)
                  || !(ss->type.t & VT_TYPEDEF))
                cprime_error("incompatible redefinition of '%s'",
                          get_tok_str(v, NULL));
              ss->type = type1;
            }
            else
              ss = sym_push(v, &type1, 0, 0);
            ss->a = ad1.a;
            if ((type1.t & VT_BTYPE) == VT_FUNC)
              merge_funcattr(&ss->type.ref->f, &ad1.f);
            if (tok == ';' || tok == TOK_EOF)
              break;
            skip(',');
          }
          skip(';');
          continue;
        }

        if (tok == TOK_OPERATOR)
        {
          int method_tok, paren_level = 0, cv_qualifiers, default_suffix;
          CType conversion_ret_type, func_type;
          TokenString *params;

          method_tok = parse_cpp_operator_method_tok();
          if (!make_type_from_saved_type_tokens(
                &conversion_ret_type, last_cpp_conversion_operator_type_tokens))
            cprime_error("unsupported conversion operator type");
          skip('(');
          params = tok_str_alloc();
          while (tok != TOK_EOF)
          {
            if (tok == ')' && paren_level == 0)
              break;
            if (tok == '(')
              ++paren_level;
            else if (tok == ')' && paren_level > 0)
              --paren_level;
            tok_str_add_tok(params);
            next();
          }
          tok_str_add(params, TOK_EOF);
          skip(')');
          cv_qualifiers = skip_member_func_cv_qualifiers();
          default_suffix = skip_defaulted_or_deleted_member_suffix();
          func_type = make_func_type_from_saved_params(&conversion_ret_type,
                                                       params);
          func_type.t |= cv_qualifiers;
          if (member_decl_is_virtual
              || member_overrides_virtual_method(type, method_tok,
                                                 &func_type))
            note_virtual_method(type, method_tok, &func_type,
                                default_suffix == 3);
          if (tok == '=')
          {
            next();
            skip_initializer_expression();
          }
          if (tok == '{')
          {
            skip_or_save_block(&body);
            add_pending_member_func(type, method_tok, &func_type, body);
          }
          if (tok == ';')
            next();
          else if (!body)
            expect("';'");
          if (!body)
          {
            Sym *decl_sym = declare_member_func(type, method_tok, &func_type);
            if (default_suffix == 1 && decl_sym)
              note_defaulted_member_func(get_struct_type_name_tok(type),
                                         method_tok, &func_type);
          }
          continue;
        }

        if ((tok >= TOK_UIDENT
             && class_tok_matches_unqualified_name(
                  get_struct_type_name_tok(type), tok))
            || tok == '~')
        {
          int class_tok = get_struct_type_name_tok(type);
          if (tok == '~')
          {
            if (lifecycle_explicit)
              cprime_error("explicit is only supported on constructors");
            next();
            if (tok < TOK_UIDENT
                || !class_tok_matches_unqualified_name(class_tok, tok))
              cprime_error("destructor name must match class name");
            lifecycle_tok = TOK_DESTRUCTOR1;
            next();
          }
          else
          {
            lifecycle_tok = TOK_CONSTRUCTOR1;
            next();
            /*
             * Disambiguate constructor syntax from methods that return the
             * enclosing class type, e.g. `V3 add(V3 b);`.
             */
            if (tok != '(')
            {
              unget_tok(tok);
              tok = class_tok;
              lifecycle_tok = 0;
            }
          }
          if (lifecycle_tok && tok == '(')
          {
            int paren_level = 0;
            next();
            lifecycle_params = tok_str_alloc();
            while (tok != TOK_EOF)
            {
              if (tok == ')' && paren_level == 0)
                break;
              if (tok == '(')
                paren_level++;
              else if (tok == ')')
                paren_level--;
              tok_str_add_tok(lifecycle_params);
              next();
            }
            tok_str_add(lifecycle_params, TOK_EOF);
            skip(')');
            lifecycle_saw_paren = 1;
            if (lifecycle_tok == TOK_CONSTRUCTOR1)
              type->ref->a.lifecycle_ctor = 1;
            else if (lifecycle_tok == TOK_DESTRUCTOR1)
              type->ref->a.lifecycle_dtor = 1;
            if (lifecycle_params->len <= 1)
              declare_lifecycle_func(type, lifecycle_tok);
            else
              add_pending_lifecycle_decl(type, lifecycle_tok, lifecycle_params);
            if (lifecycle_tok == TOK_CONSTRUCTOR1)
            {
              Sym *saved_param_ls;
              int saved_param_scope;
              push_saved_param_scope(lifecycle_params, &saved_param_ls,
                                     &saved_param_scope);
              lifecycle_init_prefix = parse_constructor_member_initializers(type);
              pop_saved_param_scope(saved_param_ls, saved_param_scope);
            }
            if (lifecycle_init_prefix)
              note_struct_member_init_list(get_struct_type_name_tok(type));
            skip_member_func_cv_qualifiers();
            lifecycle_default_suffix = skip_defaulted_or_deleted_member_suffix();
            if (lifecycle_default_suffix == 1 && tok != '{')
            {
              body = tok_str_alloc();
              tok_str_add(body, '{');
              tok_str_add(body, '}');
              tok_str_add(body, TOK_EOF);
              add_pending_lifecycle_func(type, lifecycle_tok, lifecycle_params,
                                         body, 1, 1);
              lifecycle_body = 1;
            }
            else if (lifecycle_default_suffix == 2)
            {
              lifecycle_body = 1;
              if (tok != ';')
                cprime_error("deleted member function declaration");
            }
            if (tok == '{')
            {
              skip_or_save_block(&body);
              if (lifecycle_init_prefix)
              {
                TokenString *combined = tok_str_alloc();
                int i, inserted = 0;
                for (i = 0; i < body->len; ++i)
                {
                  if (TOK_HAS_VALUE(body->str[i]))
                  {
                    tok_str_add_record(combined, body->str, body->len, &i);
                    continue;
                  }
                  tok_str_add(combined, body->str[i]);
                  if (!inserted && body->str[i] == '{')
                  {
                    tok_str_append(combined, lifecycle_init_prefix);
                    inserted = 1;
                  }
                }
                if (!inserted)
                  cprime_error("constructor initializer requires function body");
                tok_str_free(body);
                tok_str_free(lifecycle_init_prefix);
                body = combined;
                lifecycle_init_prefix = NULL;
              }
              add_pending_lifecycle_func(type, lifecycle_tok, lifecycle_params,
                                         body, 1, 0);
              lifecycle_body = 1;
            }
          }
          if (lifecycle_tok && !lifecycle_saw_paren)
            cprime_error("invalid inline constructor/destructor syntax");
          if (lifecycle_body)
          {
            if (tok == ';')
              next();
            continue;
          }
          if (tok == ';')
          {
            next();
            continue;
          }
        }

        if (tok == tok_mutable)
          next();

        if (!parse_btype(&btype, &ad1, 0))
        {
          if (tok == TOK_STATIC_ASSERT)
          {
            do_Static_assert();
            continue;
          }
          skip(';');
          continue;
        }
        member_decl_is_auto = last_decl_was_auto;
        while (1)
        {
          TokenString *member_default_init = NULL;
          if (flexible)
            cprime_error("flexible array member '%s' not at the end of struct",
                      get_tok_str(v, NULL));
          bit_size = -1;
          v = 0;
          type1 = btype;
          if (tok != ':')
          {
          if (tok != ';')
          {
            if (class_tok_matches_unqualified_name(
                  get_struct_type_name_tok(type), tok))
            {
              int class_name_tok = tok;
              next();
              if (tok == ':')
              {
                next();
                if (tok != ':')
                  cprime_error("':' expected");
                next();
              }
              else
              {
                unget_tok(tok);
                tok = class_name_tok;
              }
            }
            if (tok == TOK_OPERATOR)
            {
              int op_tok = parse_cpp_operator_method_tok();
              unget_tok(tok);
              tok = op_tok;
            }
              type_decl(&type1, &ad1, &v, TYPE_DIRECT);
          }
            if (v == 0)
            {
              if (tok == ';' && IS_ENUM(type1.t))
                break;
              if ((type1.t & VT_BTYPE) != VT_STRUCT)
                expect("identifier");
              else
              {
                int v = btype.ref->v;
                if ((v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
                {
                  if (cprime_state->ms_extensions == 0)
                    expect("identifier");
                }
              }
            }
            if ((type1.t & VT_BTYPE) == VT_FUNC)
            {
              int default_suffix;
              type1.t |= skip_member_func_cv_qualifiers();
              default_suffix = skip_defaulted_or_deleted_member_suffix();
              if (!(type1.t & VT_STATIC)
                  && (member_decl_is_virtual
                      || member_overrides_virtual_method(type, v, &type1)))
                note_virtual_method(type, v, &type1, default_suffix == 3);
              if (type1.t & VT_STATIC)
              {
                if (tok == '{')
                {
                  skip_or_save_block(&body);
                  add_pending_static_member_func(type, v, &type1, body,
                                                 member_decl_is_auto);
                  member_func_body = 1;
                  break;
                }
                if (tok == ';')
                {
                  declare_static_member_func(type, v, &type1);
                  member_func_body = 1;
                  break;
                }
                cprime_error("invalid type for '%s'",
                          get_tok_str(v, NULL));
              }
              if (tok == '{')
              {
                skip_or_save_block(&body);
                add_pending_member_func(type, v, &type1, body);
                member_func_body = 1;
                break;
              }
              if (tok == ';')
              {
                Sym *decl_sym = declare_member_func(type, v, &type1);
                if (member_decl_is_auto && decl_sym)
                  note_auto_return_member_tok(decl_sym->v);
                if (default_suffix == 1 && decl_sym)
                  note_defaulted_member_func(get_struct_type_name_tok(type), v,
                                             &type1);
                member_func_body = 1;
                break;
              }
              cprime_error("invalid type for '%s'",
                        get_tok_str(v, NULL));
            }
            if (type1.t & VT_STATIC)
            {
              CType static_type = type1;
              int class_tok = get_struct_type_name_tok(type);
              int static_tok;
              int r = 0;
              int has_const_init = 0;
              int64_t const_init = 0;

              if (!class_tok)
                cprime_error("static data members require a named class");
              if (tok == '=')
              {
                next();
                if ((static_type.t & VT_CONSTANT)
                    && is_integer_btype(static_type.t & VT_BTYPE))
                {
                  const_init = expr_const64();
                  has_const_init = 1;
                }
                else
                  skip_initializer_expression();
              }
              static_tok = make_static_member_tok(class_tok, v);
              static_type.t = (static_type.t & ~VT_STATIC) | VT_EXTERN;
              if (has_const_init)
              {
                Sym *static_sym = external_sym(static_tok, &static_type,
                                               VT_CONST, &ad1);
                static_sym->c = const_init;
              }
              else
              {
              if (!(static_type.t & VT_ARRAY))
                r |= VT_LVAL;
              external_sym(static_tok, &static_type, r, &ad1);
              }
              if (tok == ';' || tok == TOK_EOF)
                break;
              skip(',');
              continue;
            }
            if (tok == '=')
            {
              next();
              skip_or_save_member_initializer(&member_default_init);
            }
            if (type_size(&type1, &align) < 0)
            {
              if ((u == VT_STRUCT) && (type1.t & VT_ARRAY) && c)
                flexible = 1;
              else
                cprime_error("field '%s' has incomplete type",
                          get_tok_str(v, NULL));
            }
            if ((type1.t & VT_BTYPE) == VT_FUNC ||
                (type1.t & VT_BTYPE) == VT_VOID ||
                (type1.t & VT_STORAGE))
              cprime_error("invalid type for '%s'",
                        get_tok_str(v, NULL));
          }
          if (tok == ':')
          {
            next();
            bit_size = expr_const();
            // XXX: handle v = 0 case for messages
            if (bit_size < 0)
              cprime_error("negative width in bit-field '%s'",
                        get_tok_str(v, NULL));
            if (v && bit_size == 0)
              cprime_error("zero width for bit-field '%s'",
                        get_tok_str(v, NULL));
            parse_attribute(&ad1);
          }
          size = type_size(&type1, &align);
          if (bit_size >= 0)
          {
            bt = type1.t &VT_BTYPE;
            if (bt != VT_INT &&
                bt != VT_BYTE &&
                bt != VT_SHORT &&
                bt != VT_BOOL &&
                bt != VT_LLONG)
              cprime_error("bitfields must have scalar type");
            bsize = size * 8;
            if (bit_size > bsize)
            {
              cprime_error("width of '%s' exceeds its type",
                        get_tok_str(v, NULL));
            }
            else if (bit_size == bsize
                     && !*cprime_state->pack_stack_ptr
                     && !ad.a.packed && !ad1.a.packed)
            {
              // No Need For Bit Fields
              ;
            }
            else if (bit_size == 64)
              cprime_error("field width 64 not implemented");
            else
            {
              type1.t = (type1.t & ~VT_STRUCT_MASK)
                        | VT_BITFIELD
                        | ((unsigned)bit_size << (VT_STRUCT_SHIFT + 6));
            }
          }
          if (v != 0 || (type1.t & VT_BTYPE) == VT_STRUCT)
          {
            /* Remember we've seen a real field to check
            for placement of flexible array member. */
            c = 1;
          }
          /* If member is a struct or bit-field, enforce
             placing into the struct (as anonymous).  */
          if (v == 0 &&
              ((type1.t & VT_BTYPE) == VT_STRUCT ||
               bit_size >= 0))
            v = anon_sym++;
          if (v)
          {
            ss = sym_push(v | SYM_FIELD, &type1, 0, 0);
            ss->a = ad1.a;
            ss->default_arg = member_default_init;
            member_default_init = NULL;
            *ps = ss;
            ps = &ss->next;
          }
          if (member_default_init)
            tok_str_free(member_default_init);
          if (tok == ';' || tok == TOK_EOF)
            break;
          skip(',');
        }
        if (member_func_body)
        {
          if (tok == ';')
            next();
          continue;
        }
        skip(';');
      }
      skip('}');
      parse_attribute(&ad);
      if (ad.cleanup_func)
        cprime_warning("attribute '__cleanup__' ignored on type");
      if (u == VT_STRUCT)
      {
        int class_tok = get_struct_type_name_tok(type);
        VirtualMethodInfo *vm;
        int needs_vptr = 0;
        for (vm = virtual_method_infos; vm; vm = vm->next)
          if (vm->class_tok == class_tok && vm->root_tok == class_tok)
          {
            needs_vptr = 1;
            break;
          }
        if (needs_vptr)
        {
          CType vptr_type;
          int dummy_offset;
          if (!find_field_try(type, virtual_vptr_field_tok(class_tok),
                              &dummy_offset))
          {
            Sym *vptr_field;
            vptr_type.t = VT_VOID;
            vptr_type.ref = NULL;
            mk_pointer(&vptr_type);
            vptr_field = sym_push(virtual_vptr_field_tok(class_tok)
                                   | SYM_FIELD, &vptr_type, 0, 0);
            *ps = vptr_field;
            ps = &vptr_field->next;
          }
        }
      }
      check_fields(type, 1);
      check_fields(type, 0);
      struct_layout(type, &ad);
      if (u == VT_STRUCT)
        emit_virtual_tables_for_class(get_struct_type_name_tok(type));
      emit_defaulted_member_bodies(type);
      if (saved_nb_pending_member_funcs != nb_pending_member_funcs
          && !defer_pending_member_funcs)
        compile_pending_member_funcs(saved_nb_pending_member_funcs);
    }
    if (u == VT_STRUCT && nb_defining_class_stack > 0
        && defining_class_stack[nb_defining_class_stack - 1]
           == (s->v & ~SYM_STRUCT))
      --nb_defining_class_stack;
    /* Friend declarations and other interface work recorded while parsing a
       class must become ordinary global declarations before a later function
       body can trigger overload resolution.  Draining while any class is
       still active makes decl() inherit that suspended class context, so wait
       for the completed outermost class to leave the defining stack. */
    if (is_cpp_translation_unit() && nb_defining_class_stack == 0
        && compiled_template_specs < nb_pending_template_specs)
      compile_pending_template_specs_without_member_flush();
    if (debug_modes)
      cprime_debug_fix_forw(cprime_state, type);
  }
}

static void sym_to_attr(AttributeDef *ad, Sym *s)
{
  merge_symattr(&ad->a, &s->a);
  merge_funcattr(&ad->f, &s->f);
}

/* Add type qualifiers to a type. If the type is an array then the qualifiers
   are added to the element type, copied because it could be a typedef. */
static void parse_btype_qualify(CType *type, int qualifiers)
{
  while (type->t & VT_ARRAY)
  {
    type->ref = sym_push(SYM_FIELD, &type->ref->type, 0, type->ref->c);
    type = &type->ref->type;
  }
  type->t |= qualifiers;
}

/* return 0 if no type declaration. otherwise, return the basic type
   and skip it.
 */
static int parse_btype(CType *type, AttributeDef *ad, int ignore_label)
{
  int t, u, bt, st, type_found, typespec_found, g, n;
  Sym *s;
  CType type1;

  memset(ad, 0, sizeof(AttributeDef));
  last_decl_was_auto = 0;
  last_btype_was_typedef = 0;
  last_btype_was_decltype = 0;
  type_found = 0;
  typespec_found = 0;
  t = VT_INT;
  if (pending_cpp_extern_linkage)
  {
    t |= VT_EXTERN;
    pending_cpp_extern_linkage = 0;
  }
  bt = st = -1;
  type->ref = NULL;

  while (1)
  {


    if (is_cpp_translation_unit() && tok >= TOK_UIDENT
        && !strcmp(get_tok_str(tok, NULL), "virtual"))
    {
      next();
      continue;
    }
    if (tok == tok_constexpr)
    {
      t |= VT_INLINE;
      next();
      continue;
    }
    if (is_cpp_translation_unit() && tok >= TOK_UIDENT
        && !strcmp(get_tok_str(tok, NULL), "thread_local"))
    {
      /* The PE backend does not yet emit TLS objects.  Preserve static
         storage duration so template bodies using `static thread_local`
         remain well-formed; access is process-wide until TLS lowering is
         implemented. */
      t |= VT_STATIC;
      next();
      continue;
    }
    switch (tok)
    {
    case TOK_EXTENSION:
      // Currently, We Really Ignore Extension
      next();
      continue;

    // Basic Types
    case TOK_CHAR:
      u = VT_BYTE;
basic_type:
      next();
basic_type1:
      if (u == VT_SHORT || u == VT_LONG)
      {
        if (st != -1 || (bt != -1 && bt != VT_INT))
tmbt: cprime_error("too many basic types");
        st = u;
      }
      else
      {
        if (bt != -1 || (st != -1 && u != VT_INT))
          goto tmbt;
        bt = u;
      }
      if (u != VT_INT)
        t = (t & ~(VT_BTYPE | VT_LONG)) | u;
      typespec_found = 1;
      break;
    case TOK_VOID:
      u = VT_VOID;
      goto basic_type;
    case TOK_SHORT:
      u = VT_SHORT;
      goto basic_type;
    case TOK_INT:
      u = VT_INT;
      goto basic_type;
    case TOK_ALIGNAS:
    {
      int n;
      AttributeDef ad1;
      next();
      skip('(');
      memset(&ad1, 0, sizeof(AttributeDef));
      if (parse_btype(&type1, &ad1, 0))
      {
        type_decl(&type1, &ad1, &n, TYPE_ABSTRACT);
        if (ad1.a.aligned)
          n = 1 << (ad1.a.aligned - 1);
        else
          type_size(&type1, &n);
      }
      else
      {
        n = expr_const();
        if (n < 0 || (n & (n - 1)) != 0)
          cprime_error("alignment must be a positive power of two");
      }
      skip(')');
      ad->a.aligned = exact_log2p1(n);
    }
    continue;
    case TOK_LONG:
      if ((t & VT_BTYPE) == VT_DOUBLE)
        t = (t & ~(VT_BTYPE | VT_LONG)) | VT_LDOUBLE;
      else if ((t & (VT_BTYPE | VT_LONG)) == VT_LONG)
        t = (t & ~(VT_BTYPE | VT_LONG)) | VT_LLONG;
      else
      {
        u = VT_LONG;
        goto basic_type;
      }
      next();
      break;
    case TOK_BOOL:
    case TOK_BOOL2:
      u = VT_BOOL;
      goto basic_type;
    case TOK_COMPLEX:
      cprime_error("_Complex is not yet supported");
    case TOK_FLOAT:
      u = VT_FLOAT;
      goto basic_type;
    case TOK_DOUBLE:
      if ((t & (VT_BTYPE | VT_LONG)) == VT_LONG)
        t = (t & ~(VT_BTYPE | VT_LONG)) | VT_LDOUBLE;
      else
      {
        u = VT_DOUBLE;
        goto basic_type;
      }
      next();
      break;
    case TOK_ENUM:
      struct_decl(&type1, VT_ENUM, 0);
basic_type2:
      u = type1.t;
      type->ref = type1.ref;
      goto basic_type1;
    case TOK_STRUCT:
      struct_decl(&type1, VT_STRUCT, 0);
      goto basic_type2;
    case TOK_CLASS:
      struct_decl(&type1, VT_STRUCT, 1);
      goto basic_type2;
    case TOK_UNION:
      struct_decl(&type1, VT_UNION, 0);
      goto basic_type2;

    // Type Modifiers
    case TOK__Atomic:
      next();
      type->t = t;
      parse_btype_qualify(type, VT_ATOMIC);
      t = type->t;
      if (tok == '(')
      {
        parse_expr_type(&type1);
        // Remove All Storage Modifiers Except Typedef
        type1.t &= ~(VT_STORAGE & ~VT_TYPEDEF);
        if (type1.ref)
          sym_to_attr(ad, type1.ref);
        goto basic_type2;
      }
      break;
    case TOK_CONST1:
    case TOK_CONST2:
    case TOK_CONST3:
      type->t = t;
      parse_btype_qualify(type, VT_CONSTANT);
      t = type->t;
      next();
      break;
    case TOK_VOLATILE1:
    case TOK_VOLATILE2:
    case TOK_VOLATILE3:
      type->t = t;
      parse_btype_qualify(type, VT_VOLATILE);
      t = type->t;
      next();
      break;
    case TOK_SIGNED1:
    case TOK_SIGNED2:
    case TOK_SIGNED3:
      if ((t & (VT_DEFSIGN | VT_UNSIGNED)) == (VT_DEFSIGN | VT_UNSIGNED))
        cprime_error("signed and unsigned modifier");
      t |= VT_DEFSIGN;
      next();
      typespec_found = 1;
      break;
    case TOK_REGISTER:
    case TOK_AUTO:
      if (tok == TOK_AUTO)
        last_decl_was_auto = 1;
    case TOK_RESTRICT1:
    case TOK_RESTRICT2:
    case TOK_RESTRICT3:
      next();
      break;
    case TOK_UNSIGNED:
      if ((t & (VT_DEFSIGN | VT_UNSIGNED)) == VT_DEFSIGN)
        cprime_error("signed and unsigned modifier");
      t |= VT_DEFSIGN | VT_UNSIGNED;
      next();
      typespec_found = 1;
      break;

    // Storage
    case TOK_EXTERN:
      g = VT_EXTERN;
      goto storage;
    case TOK_STATIC:
      g = VT_STATIC;
      goto storage;
    case TOK_TYPEDEF:
      g = VT_TYPEDEF;
      goto storage;
storage:
      if (t & (VT_EXTERN | VT_STATIC | VT_TYPEDEF) & ~g)
        cprime_error("multiple storage classes");
      t |= g;
      next();
      if (g == VT_EXTERN && tok == TOK_STR)
        parse_mult_str("linkage string");
      break;
    case TOK_INLINE1:
    case TOK_INLINE2:
    case TOK_INLINE3:
    case TOK_INLINE4:
      t |= VT_INLINE;
      next();
      break;
    case TOK_NORETURN3:
      next();
      ad->f.func_noreturn = 1;
      break;
    // GNUC attribute
    case TOK_ATTRIBUTE1:
    case TOK_ATTRIBUTE2:
      parse_attribute(ad);
      if (ad->attr_mode)
      {
        u = ad->attr_mode - 1;
        t = (t & ~(VT_BTYPE | VT_LONG)) | u;
      }
      continue;
    /* C++11 attribute-specifier-seq before a declaration. */
    case '[':
      if (!is_cpp_translation_unit() || type_found)
        goto the_end;
      next();
      if (tok != '[')
      {
        unget_tok(tok);
        tok = '[';
        goto the_end;
      }
      unget_tok(tok);
      tok = '[';
      parse_attribute(ad);
      continue;
    // GNUC typeof
    case TOK_TYPEOF1:
    case TOK_TYPEOF2:
    case TOK_TYPEOF3:
      next();
      parse_expr_type(&type1);
      // Remove All Storage Modifiers Except Typedef
      type1.t &= ~(VT_STORAGE & ~VT_TYPEDEF);
      if (type1.ref)
      {
        sym_to_attr(ad, type1.ref);
        if (type1.t & VT_ARRAY)
          type1.t |= VT_BT_ARRAY;
      }
      goto basic_type2;
    case TOK_DECLTYPE:
      next();
      last_btype_was_decltype = 1;
      parse_decltype_type(&type1);
      last_btype_was_decltype = 1;
      type1.t &= ~(VT_STORAGE & ~VT_TYPEDEF);
      if (type1.ref)
      {
        sym_to_attr(ad, type1.ref);
        if (type1.t & VT_ARRAY)
          type1.t |= VT_BT_ARRAY;
      }
      goto basic_type2;
    case TOK_THREAD_LOCAL:
      cprime_error("_Thread_local is not implemented");
    default:
      if (!typespec_found)
      {
        if (tok >= TOK_UIDENT)
        {
          TemplateDef *class_td = find_class_template_def(find_current_namespace_tok(tok));
          if (!class_td)
            class_td = find_class_template_def(tok);
          if (class_td && class_td->is_class)
          {
            int original_tok = tok;
            next();
            if (tok == TOK_LT || tok == '<')
            {
              int inst_tok;
              TemplateArgList class_args;
              parse_template_type_args(&class_args);
              inst_tok = instantiate_template_if_needed(class_td, &class_args);
              compile_pending_template_specs_without_member_flush();
              s = struct_find(inst_tok);
              if (!s)
                cprime_error("template class instantiation failed for '%s'",
                             get_tok_str(class_td->name_tok, NULL));
              t &= ~(VT_BTYPE | VT_LONG);
              u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
              type->t = s->type.t | u;
              type->ref = s;
              if (t)
                parse_btype_qualify(type, t);
              t = type->t;
              typespec_found = 1;
              st = bt = -2;
              break;
            }
            unget_tok(tok);
            tok = original_tok;
          }
        }
        if (tok >= TOK_UIDENT && is_cpp_translation_unit()
            && is_namespace_tok(tok))
        {
          int first_tok = tok, qtok, parts[16], nb_parts = 0;
          TokenString *replay = tok_str_alloc();
          tok_str_add2(replay, tok, &tokc);
          next();
          if (tok == ':')
          {
            parts[nb_parts++] = first_tok;
            while (tok == ':')
            {
              tok_str_add(replay, tok);
              next();
              if (tok != ':')
              {
                restore_cpp_lifecycle_probe(replay);
                goto the_end;
              }
              tok_str_add(replay, tok);
              next();
              if (tok < TOK_UIDENT)
                cprime_error("qualified type name");
              if (nb_parts >= (int)(sizeof(parts) / sizeof(parts[0])))
                cprime_error("qualified name too deep");
              parts[nb_parts++] = tok;
              tok_str_add2(replay, tok, &tokc);
              next();
            }
            qtok = make_namespace_tok_from_parts(parts, nb_parts);
            s = struct_find(qtok);
            if (s
                && (((s->type.t & VT_BTYPE) == VT_STRUCT)
                    || ((s->type.t & VT_BTYPE) == VT_UNION)
                    || IS_ENUM(s->type.t)))
            {
              tok_str_free(replay);
              t &= ~(VT_BTYPE | VT_LONG);
              u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
              type->t = s->type.t | u;
              type->ref = s;
              if (t)
                parse_btype_qualify(type, t);
              t = type->t;
              typespec_found = 1;
              st = bt = -2;
              break;
            }
            s = sym_find(qtok);
            if (!s)
              s = global_symbol_find(qtok);
            if (s && (s->type.t & VT_TYPEDEF))
            {
              tok_str_free(replay);
              t &= ~(VT_BTYPE | VT_LONG);
              u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
              type->t = (s->type.t & ~VT_TYPEDEF) | u;
              type->ref = s->type.ref;
              if (t)
                parse_btype_qualify(type, t);
              t = type->t;
              typespec_found = 1;
              st = bt = -2;
              break;
            }
            {
              TemplateDef *td = find_class_template_def(qtok);
              if (td && td->is_class && (tok == TOK_LT || tok == '<'))
              {
                int mangled_tok;
                TemplateArgList args;
                tok_str_free(replay);
                parse_template_type_args(&args);
                mangled_tok = instantiate_template_if_needed(td, &args);
                compile_pending_template_specs_without_member_flush();
                s = struct_find(mangled_tok);
                if (!s)
                  cprime_error("template class instantiation failed for '%s'",
                            get_tok_str(td->name_tok, NULL));
                t &= ~(VT_BTYPE | VT_LONG);
                u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
                type->t = s->type.t | u;
                type->ref = s;
                if (t)
                  parse_btype_qualify(type, t);
                t = type->t;
                typespec_found = 1;
                st = bt = -2;
                break;
              }
            }
            restore_cpp_lifecycle_probe(replay);
            goto the_end;
          }
          tok_str_free(replay);
          unget_tok(tok);
          tok = first_tok;
        }

        TemplateDef *td = find_class_template_def(find_current_namespace_tok(tok));
        if (td && td->is_class)
        {
          int template_tok = tok;
          int mangled_tok;
          TemplateArgList args;

          next();
          if (tok != TOK_LT && tok != '<')
          {
            unget_tok(template_tok);
            goto the_end;
          }
          parse_template_type_args(&args);
          mangled_tok = instantiate_template_if_needed(td, &args);
          compile_pending_template_specs_without_member_flush();
          s = struct_find(mangled_tok);
          if (!s)
            cprime_error("template class instantiation failed for '%s'",
                      get_tok_str(td->name_tok, NULL));

          t &= ~(VT_BTYPE | VT_LONG);
          u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
          type->t = s->type.t | u;
          type->ref = s;
          if (t)
            parse_btype_qualify(type, t);
          t = type->t;
          typespec_found = 1;
          st = bt = -2;
          break;
        }

        /*
         * cprime class/struct names should be usable directly as type names
         * (C++-style), not only via explicit typedef aliases.
         */
        n = find_current_class_nested_type_tok(tok);
        if (n == tok)
          n = find_current_namespace_tok(tok);
        s = struct_find(n);
        if (!s)
        {
          TemplateDef *class_td = find_class_template_def(n);
          if (!class_td)
            class_td = find_class_template_def(tok);
          if (class_td && class_td->is_class)
          {
            int original_tok = tok;
            next();
            if (tok == TOK_LT || tok == '<')
            {
              int inst_tok;
              TemplateArgList class_args;
              parse_template_type_args(&class_args);
              inst_tok = instantiate_template_if_needed(class_td, &class_args);
              compile_pending_template_specs_without_member_flush();
              s = struct_find(inst_tok);
              if (!s)
                cprime_error("template class instantiation failed for '%s'",
                             get_tok_str(class_td->name_tok, NULL));
              n = inst_tok;
            }
            else
            {
              unget_tok(tok);
              tok = original_tok;
            }
          }
        }
        if (s
            && (((s->type.t & VT_BTYPE) == VT_STRUCT)
                  || ((s->type.t & VT_BTYPE) == VT_UNION)
                  || IS_ENUM(s->type.t)))
        {
          int source_name_tok = tok;
          next();
          if (tok == TOK_LT || tok == '<')
          {
            int level = 1;
            next();
            while (tok != TOK_EOF && level > 0)
            {
              if (tok == TOK_LT || tok == '<')
                level++;
              else if (tok == TOK_GT || tok == TOK_SAR)
                level--;
              next();
            }
          }
          if (ignore_label && tok == '=')
          {
            unget_tok(source_name_tok);
            goto the_end;
          }
          if (tok == ':')
          {
            int class_tok = n;
            TokenString *replay = tok_str_alloc();
            tok_str_add(replay, source_name_tok);
            tok_str_add(replay, tok);
            next();
            if (tok == ':')
            {
              tok_str_add(replay, tok);
              next();
              if (tok >= TOK_UIDENT)
              {
                int nested_tok = make_static_member_tok(class_tok, tok);
                Sym *nested_sym = struct_find(nested_tok);
                if (!nested_sym)
                  nested_sym = struct_find(tok);
                if (nested_sym
                    && (((nested_sym->type.t & VT_BTYPE) == VT_STRUCT)
                        || ((nested_sym->type.t & VT_BTYPE) == VT_UNION)
                        || IS_ENUM(nested_sym->type.t)))
                {
                  tok_str_free(replay);
                  next();
                  s = nested_sym;
                  t &= ~(VT_BTYPE | VT_LONG);
                  u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
                  type->t = s->type.t | u;
                  type->ref = s;
                  if (t)
                    parse_btype_qualify(type, t);
                  t = type->t;
                  typespec_found = 1;
                  st = bt = -2;
                  break;
                }
              }
            }
            restore_cpp_lifecycle_probe(replay);
            goto the_end;
          }

          t &= ~(VT_BTYPE | VT_LONG);
          u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
          type->t = s->type.t | u;
          type->ref = s;
          if (t)
            parse_btype_qualify(type, t);
          t = type->t;
          typespec_found = 1;
          st = bt = -2;
          break;
        }
      }
      if (typespec_found)
        goto the_end;
      n = find_current_namespace_tok(tok);
      {
        TemplateDef *class_td = find_class_template_def(n);
        if (!class_td)
          class_td = find_class_template_def(tok);
        if (class_td && class_td->is_class)
        {
          int original_tok = tok;
          next();
          if (tok == TOK_LT || tok == '<')
          {
            int inst_tok;
            TemplateArgList class_args;
            parse_template_type_args(&class_args);
            inst_tok = instantiate_template_if_needed(class_td, &class_args);
            compile_pending_template_specs_without_member_flush();
            s = struct_find(inst_tok);
            if (!s)
              cprime_error("template class instantiation failed for '%s'",
                           get_tok_str(class_td->name_tok, NULL));
            t &= ~(VT_BTYPE | VT_LONG);
            u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
            type->t = s->type.t | u;
            type->ref = s;
            if (t)
              parse_btype_qualify(type, t);
            t = type->t;
            typespec_found = 1;
            st = bt = -2;
            break;
          }
          unget_tok(tok);
          tok = original_tok;
        }
      }
      s = sym_find(n);
      if (!s || !(s->type.t & VT_TYPEDEF))
      {
        Sym *global_typedef = global_symbol_find(n);
        if ((!global_typedef || !(global_typedef->type.t & VT_TYPEDEF))
            && n != tok)
          global_typedef = global_symbol_find(tok);
        if (global_typedef && (global_typedef->type.t & VT_TYPEDEF))
          s = global_typedef;
      }
      if (!s || !(s->type.t & VT_TYPEDEF))
        goto the_end;

      n = tok, next();
      if ((tok == ':' || tok == '=') && ignore_label)
      {
        /* A typedef name can be shadowed by a local object.  At statement
           start, `name = value` is therefore an expression, not a new
           declaration using the outer typedef (the ':' case is a label). */
        unget_tok(n);
        goto the_end;
      }

      t &= ~(VT_BTYPE | VT_LONG);
      u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
      type->t = (s->type.t & ~VT_TYPEDEF) | u;
      type->ref = s->type.ref;
      last_btype_was_typedef = 1;
      if (t)
        parse_btype_qualify(type, t);
      t = type->t;
      if (t & VT_ARRAY)
        t |= VT_BT_ARRAY;
      // Get Attributes From Typedef
      sym_to_attr(ad, s);
      typespec_found = 1;
      st = bt = -2;
      break;
    }
    type_found = 1;
  }
the_end:
  if (cprime_state->char_is_unsigned)
  {
    if ((t & (VT_DEFSIGN | VT_BTYPE)) == VT_BYTE)
      t |= VT_UNSIGNED;
  }
  // VT_LONG is used just as a modifier for VT_INT / VT_LLONG
  bt = t & (VT_BTYPE | VT_LONG);
  if (bt == VT_LONG)
    t |= LONG_SIZE == 8 ? VT_LLONG : VT_INT;
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
  if (bt == VT_LDOUBLE)
    t = (t & ~(VT_BTYPE | VT_LONG)) | (VT_DOUBLE | VT_LONG);
#endif
  type->t = t;
  return type_found;
}

/* convert a function parameter type (array to pointer and function to
   function pointer) */
static inline void convert_parameter_type(CType *pt)
{
  /* remove const and volatile qualifiers (XXX: const could be used
     to indicate a const function parameter */
  pt->t &= ~(VT_CONSTANT | VT_VOLATILE);
  // array must be transformed to pointer according to ANSI C
  pt->t &= ~(VT_ARRAY | VT_VLA);
  if ((pt->t & VT_BTYPE) == VT_FUNC)
    mk_pointer(pt);
}

ST_FUNC CString *parse_asm_str(void)
{
  skip('(');
  return parse_mult_str("string constant");
}

// Parse an asm label and return the token
static int asm_label_instr(void)
{
  int v;
  char *astr;

  next();
  astr = parse_asm_str()->data;
  skip(')');
#ifdef ASM_DEBUG
  printf("asm_alias: \"%s\"\n", astr);
#endif
  v = tok_alloc_const(astr);
  return v;
}

static int post_type(CType *type, AttributeDef *ad, int storage, int td)
{
  int n, l, t1, arg_size, align;
  Sym **plast, *s, *first, **ps, *sr;
  AttributeDef ad1;
  CType pt;
  TokenString *vla_array_tok = NULL;
  int *vla_array_str = NULL;

  if (tok == '(')
  {
    // function type, or recursive declarator (return if so)
    next();
    if (TYPE_DIRECT == (td & (TYPE_DIRECT | TYPE_ABSTRACT)))
      return 0;

    // We Push A Anonymous Symbol Which Will Contain The Function Prototype
    // It Also Serves As A Boundary For The Function Parameter Scope
    ps = local_stack ? &local_stack : &global_stack;
    ++local_scope;
    sr = sym_push2(ps, SYM_FIELD, 0, 0);

    if (tok == ')')
      l = 0;
    else if (parse_btype(&pt, &ad1, 0))
      l = FUNC_NEW;
    else if (td & (TYPE_DIRECT | TYPE_ABSTRACT))
    {
      sym_pop(ps, sr->prev, 0);
      --local_scope;
      merge_attr (ad, &ad1);
      return 0;
    }
    else
      l = FUNC_OLD;

    first = NULL;
    plast = &first;
    arg_size = 0;
    if (l)
    {
      for (;;)
      {
        TokenString *default_arg = NULL;
        // Read Param Name And Compute Offset
        if (l != FUNC_OLD)
        {
          if ((pt.t & VT_BTYPE) == VT_VOID && tok == ')')
            break;
          n = 0;
          type_decl(&pt, &ad1, &n, TYPE_DIRECT | TYPE_ABSTRACT | TYPE_PARAM);
          if ((pt.t & VT_BTYPE) == VT_VOID)
            cprime_error("parameter declared as void");
          if (n == 0)
            n = (anon_sym++) | SYM_FIELD;
          else if (td & TYPE_PARAM)
            n |= SYM_FIELD;
          if (tok == '=')
          {
            next();
            skip_or_save_param_default(&default_arg);
            expand_saved_single_object_macro(&default_arg);
            qualify_saved_default_arg_current_class(&default_arg);
          }
        }
        else
        {
          n = tok;
          pt.t = VT_INT | VT_EXTERN; // Default Type
          pt.ref = NULL;
          next();
        }
        if (n < TOK_UIDENT)
          expect("identifier");
        convert_parameter_type(&pt);
        arg_size += (type_size(&pt, &align) + PTR_SIZE - 1) / PTR_SIZE;
        /* these symbols may be evaluated for VLArrays (see below, under
           nocode_wanted) Example: int func(int a, int b[++a]); */
        {
          Sym *visible_param = n >= TOK_UIDENT ? sym_find(n) : NULL;
          Sym *scope_param;
          int visible_in_current_params = 0;
          for (scope_param = *ps;
               scope_param && scope_param != sr;
               scope_param = scope_param->prev)
            if (scope_param == visible_param)
            {
              visible_in_current_params = 1;
              break;
            }
          int restore_visible = visible_param
                                && !visible_in_current_params
                                && sym_scope_ex(visible_param) == local_scope;
          if (restore_visible)
            sym_link(visible_param, 0);
          s = sym_push(n, &pt, VT_LOCAL | VT_LVAL, 0);
          if (restore_visible)
            s->prev_tok = visible_param;
        }
        s->default_arg = default_arg;
        *plast = s;
        plast = &s->next;
        if (tok == ')')
          break;
        skip(',');
        if (l == FUNC_NEW && tok == TOK_DOTS)
        {
          l = FUNC_ELLIPSIS;
          next();
          break;
        }
        if (l == FUNC_NEW && !parse_btype(&pt, &ad1, 0))
          cprime_error("invalid type");
      }
    }
    else
      // if no parameters, then old type prototype
      l = FUNC_OLD;
    skip(')');
    /* NOTE: const is ignored in returned type as it has a special
       meaning in gcc / C++ */
    type->t &= ~VT_CONSTANT;
    /* some ancient pre-K&R C allows a function to return an array
       and the array brackets to be put after the arguments, such
       that "int c()[]" means something like "int[] c()" */
    if (tok == '[')
    {
      next();
      skip(']'); // Only Handle Simple "[]"
      mk_pointer(type);
    }
    ad->f.func_args = arg_size;
    ad->f.func_type = l;
    sr->type = *type, s = sr;
    s->a = ad->a;
    s->f = ad->f;
    s->next = first;
    type->t = VT_FUNC;
    type->ref = s;
    // Unlink Parameter Symbols From The Token Table, Keep On Stack
    sym_pop(ps, sr, 1);
    --local_scope;

  }
  else if (tok == '[')
  {
    int saved_nocode_wanted = nocode_wanted;
    // Array Definition
    next();
    n = -1;
    t1 = 0;
    if (td & TYPE_PARAM) while (1)
      {
        /* XXX The optional type-quals and static should only be accepted
           in parameter decls.  The '*' as well, and then even only
           in prototypes (not function defs).  */
        switch (tok)
        {
        case TOK_RESTRICT1: case TOK_RESTRICT2: case TOK_RESTRICT3:
        case TOK_CONST1:
        case TOK_VOLATILE1:
        case TOK_STATIC:
        case '*':
          next();
          continue;
        default:
          break;
        }
        if (tok != ']')
        {
          /* Code generation is not done now but has to be done
             at start of function. Save code here for later use. */
          nocode_wanted = 1;
          skip_or_save_block(&vla_array_tok);
          unget_tok(0);
          vla_array_str = vla_array_tok->str;
          begin_macro(vla_array_tok, 2);
          next();
          gexpr();
          end_macro();
          next();
          goto check;
        }
        break;

      }
    else if (tok != ']')
    {
      if (!local_stack || (storage & VT_STATIC))
        vpushi(expr_const());
      else
      {
        /* VLAs (which can only happen with local_stack && !VT_STATIC)
           length must always be evaluated, even under nocode_wanted,
           so that its size slot is initialized (e.g. under sizeof
           or typeof).  */
        nocode_wanted = 0;
        ++integral_constant_expression_wanted;
        gexpr();
        --integral_constant_expression_wanted;
      }
check:
      if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST)
      {
        n = vtop->c.i;
        if (n < 0)
          cprime_error("invalid array size");
      }
      else
      {
        if (!is_integer_btype(vtop->type.t & VT_BTYPE))
          cprime_error("size of variable length array should be an integer");
        n = 0;
        t1 = VT_VLA;
      }
    }
    skip(']');
    // Parse Next Post Type
    post_type(type, ad, storage, (td & ~(TYPE_DIRECT | TYPE_ABSTRACT)) | TYPE_NEST);

    if ((type->t & VT_BTYPE) == VT_FUNC)
      cprime_error("declaration of an array of functions");
    if ((type->t & VT_BTYPE) == VT_VOID
        || (!(td & TYPE_PARAM) && type_size(type, &align) < 0))
      cprime_error("declaration of an array of incomplete type elements");

    t1 |= type->t &VT_VLA;

    if (t1 & VT_VLA)
    {
      if (n < 0)
      {
        if  (td & TYPE_NEST)
          cprime_error("need explicit inner array size in VLAs");
      }
      else
      {
        loc -= type_size(&int_type, &align);
        loc &= -align;
        n = loc;

        vpush_type_size(type, &align);
        gen_op('*');
        vset(&int_type, VT_LOCAL | VT_LVAL, n);
        vswap();
        vstore();
      }
    }
    if (n != -1)
      vpop();
    nocode_wanted = saved_nocode_wanted;

    /* we push an anonymous symbol which will contain the array
       element type */
    s = sym_push(SYM_FIELD, type, 0, n);
    type->t = (t1 ? VT_VLA : VT_ARRAY) | VT_PTR;
    type->ref = s;

    if (vla_array_str)
    {
      // For Function Args, The Top Dimension Is Converted To Pointer
      if ((t1 & VT_VLA) && (td & TYPE_NEST))
        s->vla_array_str = vla_array_str;
      else
        tok_str_free_str(vla_array_str);
    }
  }
  return 1;
}

/* Parse a type declarator (except basic type), and return the type
   in 'type'. 'td' is a bitmask indicating which kind of type decl is
   expected. 'type' should contain the basic type. 'ad' is the
   attribute definition of the basic type. It can be modified by
   type_decl().  If this (possibly abstract) declarator is a pointer chain
   it returns the innermost pointed to type (equals *type, but is a different
   pointer), otherwise returns type itself, that's used for recursive calls.  */
static CType *type_decl(CType *type, AttributeDef *ad, int *v, int td)
{
  CType *post, *ret;
  int qualifiers, storage, local_ctor_init;

  // Recursive Type, Remove Storage Bits First, Apply Them Later Again
  local_ctor_init = td & TYPE_LOCAL_CTOR_INIT;
  storage = type->t &VT_STORAGE;
  type->t &= ~VT_STORAGE;
  post = ret = type;

  while (tok == '*')
  {
    qualifiers = 0;
redo:
    next();
    switch (tok)
    {
    case TOK__Atomic:
      qualifiers |= VT_ATOMIC;
      goto redo;
    case TOK_CONST1:
    case TOK_CONST2:
    case TOK_CONST3:
      qualifiers |= VT_CONSTANT;
      goto redo;
    case TOK_VOLATILE1:
    case TOK_VOLATILE2:
    case TOK_VOLATILE3:
      qualifiers |= VT_VOLATILE;
      goto redo;
    case TOK_RESTRICT1:
    case TOK_RESTRICT2:
    case TOK_RESTRICT3:
      goto redo;
    // XXX: clarify attribute handling
    case TOK_ATTRIBUTE1:
    case TOK_ATTRIBUTE2:
      parse_attribute(ad);
      break;
    }
    mk_pointer(type);
    type->t |= qualifiers;
    if (ret == type)
      // Innermost Pointed To Type Is The One For The First Derivation
      ret = pointed_type(type);
  }

  if (tok == '&' || tok == TOK_LAND)
  {
    int is_rvalue_ref = tok == TOK_LAND;
    next();
    mk_reference(type);
    if (is_rvalue_ref)
      type->t |= VT_RVALUE_REFERENCE;
    if (ret == type)
      ret = pointed_type(type);
  }

  if (tok == '(')
  {
    /* This is possibly a parameter type list for abstract declarators
       ('int ()'), use post_type for testing this.  */
    if (!post_type(type, ad, 0, td))
    {
      /* It's not, so it's a nested declarator, and the post operations
         apply to the innermost pointed to type (if any).  */
      /* XXX: this is not correct to modify 'ad' at this point, but
         the syntax is not clear */
      parse_attribute(ad);
      post = type_decl(type, ad, v, td);
      skip(')');
    }
    else
      goto abstract;
  }
  else if (tok == TOK_OPERATOR && (td & TYPE_DIRECT))
  {
    *v = parse_cpp_operator_method_tok();
  }
  else if (tok >= TOK_IDENT && (td & TYPE_DIRECT))
  {
    // Type Identifier
    *v = tok;
    next();
    if (tok == ':')
      try_rewrite_namespace_qualified_declarator(v);
    if (local_ctor_init && tok == '('
        && local_paren_starts_direct_initializer())
      goto done;
  }
  else
  {
abstract:
    if (!(td & TYPE_ABSTRACT))
      expect("identifier");
    *v = 0;
  }
  post_type(post, ad, post != ret ? 0 : storage,
            td & ~(TYPE_DIRECT | TYPE_ABSTRACT | TYPE_LOCAL_CTOR_INIT));
done:
  parse_attribute(ad);
  type->t |= storage;
  return ret;
}

// Indirection With Full Error Checking And Bound Check
ST_FUNC void indir(void)
{
  if ((vtop->type.t & VT_BTYPE) != VT_PTR)
  {
    if ((vtop->type.t & VT_BTYPE) == VT_FUNC)
      return;
    expect("pointer");
  }
  if (vtop->r & VT_LVAL)
    gv(RC_INT);
  vtop->type = *pointed_type(&vtop->type);
  // Arrays and functions are never lvalues
  if (!(vtop->type.t & (VT_ARRAY | VT_VLA))
      && (vtop->type.t & VT_BTYPE) != VT_FUNC)
  {
    vtop->r |= VT_LVAL;
    // if bound checking, the referenced pointer must be checked
#ifdef CONFIG_CPRIME_BCHECK
    if (cprime_state->do_bounds_check)
      vtop->r |= VT_MUSTBOUND;
#endif
  }
}

/* Convert an already-materialized derived-class pointer to the address and
   type of its base subobject.  CPrime lays base subobjects out as anonymous
   aggregate fields, so a multiple-inheritance base may require a byte
   adjustment rather than just suppressing a pointer-type warning. */
static int try_adjust_derived_pointer_to_base(CType *target_pointer)
{
  CType *target, *source;
  int target_tok, source_tok, base_offset;

  if (!target_pointer || (target_pointer->t & VT_BTYPE) != VT_PTR
      || (vtop->type.t & VT_BTYPE) != VT_PTR)
    return 0;
  target = pointed_type(target_pointer);
  source = pointed_type(&vtop->type);
  target_tok = get_struct_type_name_tok(target);
  source_tok = get_struct_type_name_tok(source);
  if (!target_tok || !source_tok || !class_has_base(source_tok, target_tok)
      || !class_subobject_offset(source_tok, target_tok, &base_offset))
    return 0;

  vtop->type = char_pointer_type;
  if (base_offset)
  {
    vpushi(base_offset);
    gen_op('+');
  }
  vtop->type = *target_pointer;
  return 1;
}

// pass a parameter to a function and do type checking and casting
static void gfunc_param_typed(Sym *func, Sym *arg)
{
  int func_type;
  CType type;

  func_type = func->f.func_type;
  if (func_type == FUNC_OLD ||
      (func_type == FUNC_ELLIPSIS && arg == NULL))
  {
    // default casting : only need to convert float to double
    if ((vtop->type.t & VT_BTYPE) == VT_FLOAT)
      gen_cast_s(VT_DOUBLE);
    else if (vtop->type.t & VT_BITFIELD)
    {
      type.t = vtop->type.t & (VT_BTYPE | VT_UNSIGNED);
      type.ref = vtop->type.ref;
      gen_cast(&type);
    }
    else if (vtop->r & VT_MUSTCAST)
      force_charshort_cast();
  }
  else if (arg == NULL)
    cprime_error("too many arguments to function");
  else if (is_reference_type(&arg->type))
  {
    CType *param_pointed;
    type = arg->type;
    decay_reference_type(&type);
    param_pointed = pointed_type(&arg->type);
    if (getenv("CPC_TRACE_PARAM_CAST"))
      fprintf(stderr, "CPC_PARAM_CAST func_tok=%s arg_tok=%s ref param_t=%04x param_s=%s val_t=%04x val_s=%s file=%s line=%d\n",
              func && func->v ? get_tok_str(func->v, NULL) : "<anon>",
              arg && arg->v ? get_tok_str(arg->v, NULL) : "<anon>",
              param_pointed->t, get_tok_str(get_struct_type_name_tok(param_pointed), NULL),
              vtop->type.t, get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
              file ? file->filename : "<no file>", file ? file->line_num : 0);
    if (!(arg->type.t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && (param_pointed->t & VT_BTYPE) == VT_STRUCT
        && !is_compatible_unqualified_types(param_pointed, &vtop->type)
        && !same_concrete_template_instantiation(param_pointed,
                                                 &vtop->type)
        && class_has_single_arg_constructor_for(param_pointed, &vtop->type))
    {
      CType temp_type = *param_pointed;
      temp_type.t &= ~VT_CONSTANT;
      if (try_materialize_constructor_conversion(&temp_type))
      {
        mk_pointer(&vtop->type);
        gaddrof();
        type.t &= ~VT_CONSTANT;
        gen_assign_cast(&type);
        return;
      }
    }
    if (is_reference_type(&vtop->type))
    {
      decay_reference_type(&vtop->type);
      type.t &= ~VT_CONSTANT;
      if (try_adjust_derived_pointer_to_base(&type))
        return;
      gen_assign_cast(&type);
      return;
    }
    if (!(vtop->type.t & VT_ARRAY) && !(vtop->r & VT_LVAL)
        && ((pointed_type(&arg->type)->t & VT_CONSTANT)
            || (arg->type.t & VT_RVALUE_REFERENCE)))
    {
      CType storage_type = *pointed_type(&arg->type);
      SValue value;
      int align, size, r2, addr;

      storage_type.t &= ~VT_CONSTANT;
      gen_assign_cast(&storage_type);
      size = type_size(&storage_type, &align);
      addr = get_temp_local_var(size, align, &r2);
      value = *vtop;
      vtop--;
      vset(&storage_type, VT_LOCAL | VT_LVAL, addr);
      vtop->r2 = r2;
      vpushv(&value);
      cpc_vstore_context = "gfunc_param_ref_temp";
      vstore();
      cpc_vstore_context = NULL;
      vpop();
      vset(&storage_type, VT_LOCAL | VT_LVAL, addr);
      vtop->r2 = r2;
    }
    else if (!(vtop->type.t & VT_ARRAY))
      test_lvalue();
    type.t &= ~VT_CONSTANT;
    mk_pointer(&vtop->type);
    gaddrof();
    if (try_adjust_derived_pointer_to_base(&type))
      return;
    gen_assign_cast(&type);
  }
  else
  {
    type = arg->type;
    type.t &= ~VT_CONSTANT; // need to do that to avoid false warning
    if (getenv("CPC_TRACE_PARAM_CAST"))
      fprintf(stderr, "CPC_PARAM_CAST func_tok=%s arg_tok=%s val param_t=%04x param_s=%s val_t=%04x val_s=%s file=%s line=%d\n",
              func && func->v ? get_tok_str(func->v, NULL) : "<anon>",
              arg && arg->v ? get_tok_str(arg->v, NULL) : "<anon>",
              type.t, get_tok_str(get_struct_type_name_tok(&type), NULL),
              vtop->type.t, get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
              file ? file->filename : "<no file>", file ? file->line_num : 0);
    if (!try_adjust_derived_pointer_to_base(&type))
      gen_assign_cast(&type);
  }
}

// Parse An Expression And Return Its Type Without Any Side Effect.
static void expr_type(CType *type, void (*expr_fn)(void))
{
  nocode_wanted++;
  expr_fn();
  *type = vtop->type;
  vpop();
  nocode_wanted--;
}

/* parse an expression of the form '(type)' or '(expr)' and return its
   type */
static void parse_expr_type(CType *type)
{
  int n;
  AttributeDef ad;

  skip('(');
  if (parse_btype(type, &ad, 0))
    type_decl(type, &ad, &n, TYPE_ABSTRACT);
  else
    expr_type(type, gexpr);
  skip(')');
}

static int make_type_from_type_arg_tok(CType *type, int type_tok)
{
  Sym *s;

  type->ref = NULL;
  switch (type_tok)
  {
  case TOK_CHAR:
    type->t = VT_BYTE;
    return 1;
  case TOK_INT:
    type->t = VT_INT;
    return 1;
  case TOK_SHORT:
    type->t = VT_SHORT;
    return 1;
  case TOK_LONG:
    type->t = VT_INT | VT_LONG;
    return 1;
  case TOK_FLOAT:
    type->t = VT_FLOAT;
    return 1;
  case TOK_DOUBLE:
    type->t = VT_DOUBLE;
    return 1;
  case TOK_BOOL:
  case TOK_BOOL2:
    type->t = VT_BOOL;
    return 1;
  default:
    s = sym_find(type_tok);
    if (!s)
      s = global_symbol_find(type_tok);
    if (s && (s->type.t & VT_TYPEDEF))
    {
      *type = s->type;
      type->t &= ~VT_TYPEDEF;
      return 1;
    }
    return make_class_type_from_tok(type, type_tok);
  }
}


static int standard_type_trait_value(int trait_tok, TemplateArgList *args)
{
  const char *name = get_tok_str(trait_tok, NULL);
  CType type;
  int bt;

  if (!args || args->nb < 1
      || !make_type_from_type_arg_tok(&type, args->toks[0]))
    return 0;
  bt = type.t & VT_BTYPE;
  if (strstr(name, "is_floating_point"))
    return bt == VT_FLOAT || bt == VT_DOUBLE || bt == VT_LDOUBLE;
  if (strstr(name, "is_unsigned"))
    return is_integer_btype(bt) && (type.t & VT_UNSIGNED);
  if (strstr(name, "is_signed"))
    return is_integer_btype(bt) && !(type.t & VT_UNSIGNED);
  if (strstr(name, "is_integral"))
    return is_integer_btype(bt);
  if (strstr(name, "is_same") && args->nb == 2)
  {
    CType other;
    return make_type_from_type_arg_tok(&other, args->toks[1])
           && is_compatible_unqualified_types(&type, &other);
  }
  return 0;
}

static int is_standard_type_trait_tok(int trait_tok)
{
  const char *name = get_tok_str(trait_tok, NULL);

  /* The ::value shortcut must only fire for the runtime's standard type
     traits.  A user class template with a static member named value (e.g.
     identity<int>::value()) is a function call and must fall through to the
     qualified static-member path. */
  return !strncmp(name, "std::is_", 8)
         || !strncmp(name, "__cpc_ns_std_is_", 16)
         || strstr(name, "is_floating_point")
         || strstr(name, "is_unsigned")
         || strstr(name, "is_signed")
         || strstr(name, "is_integral")
         || strstr(name, "is_same");
}

static void parse_decltype_type(CType *type)
{
  skip('(');
  if (tok >= TOK_UIDENT)
  {
    int name_tok = tok;
    next();
    if (tok == TOK_LT || tok == '<')
    {
      TemplateArgList args;
      parse_template_type_args(&args);
      if (args.nb == 1 && tok == '(')
      {
        next();
        skip(')');
        skip(')');
        if (!make_type_from_type_arg_tok(type, args.toks[0]))
          cprime_error("unsupported decltype template argument");
        return;
      }
    }
    else if (tok == '(')
    {
      Sym *s = sym_find(name_tok);
      if (!s)
        s = global_symbol_find(name_tok);
      next();
      if (s && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref
          && tok == ')')
      {
        next();
        skip(')');
        *type = s->type.ref->type;
        return;
      }
      unget_tok('(');
      unget_tok(name_tok);
    }
    else
      unget_tok(name_tok);
  }
  expr_type(type, gexpr);
  skip(')');
}

static void parse_type(CType *type)
{
  AttributeDef ad;
  int n;

  if (!parse_btype(type, &ad, 0))
    expect("type");
  type_decl(type, &ad, &n, TYPE_ABSTRACT);
}

static void parse_builtin_params(int nc, const char *args)
{
  char c, sep = '(';
  CType type;
  if (nc)
    nocode_wanted++;
  next();
  if (*args == 0)
    skip(sep);
  while ((c = *args++))
  {
    skip(sep);
    sep = ',';
    if (c == 't')
    {
      parse_type(&type);
      vpush(&type);
      continue;
    }
    expr_eq();
    type.ref = NULL;
    type.t = 0;
    switch (c)
    {
    case 'e':
      continue;
    case 'V':
      type.t = VT_CONSTANT;
    case 'v':
      type.t |= VT_VOID;
      mk_pointer (&type);
      break;
    case 'S':
      type.t = VT_CONSTANT;
    case 's':
      type.t |= char_type.t;
      mk_pointer (&type);
      break;
    case 'i':
      type.t = VT_INT;
      break;
    case 'l':
      type.t = VT_SIZE_T;
      break;
    default:
      break;
    }
    gen_assign_cast(&type);
  }
  skip(')');
  if (nc)
    nocode_wanted--;
}

static void parse_atomic(int atok)
{
  int size, align, arg, t, save = 0;
  CType *atom, *atom_ptr, ct = {0};
  SValue store;
  char buf[40];
  static const char *const templates[] =
  {
    /*
     * Each entry consists of callback and function template.
     * The template represents argument types and return type.
     *
     * ? void (return-only)
     * b bool
     * a atomic
     * A read-only atomic
     * p pointer to memory
     * v value
     * l load pointer
     * s save pointer
     * m memory model
     */

    // Keep In Order Of Appearance In Tcctok.H:
    /* __atomic_store */            "alm.?",
    /* __atomic_load */             "Asm.v",
    /* __atomic_exchange */         "alsm.v",
    /* __atomic_compare_exchange */ "aplbmm.b",
    /* __atomic_fetch_add */        "avm.v",
    /* __atomic_fetch_sub */        "avm.v",
    /* __atomic_fetch_or */         "avm.v",
    /* __atomic_fetch_xor */        "avm.v",
    /* __atomic_fetch_and */        "avm.v",
    /* __atomic_fetch_nand */       "avm.v",
    /* __atomic_and_fetch */        "avm.v",
    /* __atomic_sub_fetch */        "avm.v",
    /* __atomic_or_fetch */         "avm.v",
    /* __atomic_xor_fetch */        "avm.v",
    /* __atomic_and_fetch */        "avm.v",
    /* __atomic_nand_fetch */       "avm.v"
  };
  const char *template = templates[(atok - TOK___atomic_store)];

  atom = atom_ptr = NULL;
  size = 0; // Pacify Compiler
  next();
  skip('(');
  for (arg = 0;;)
  {
    expr_eq();
    switch (template[arg])
    {
    case 'a':
    case 'A':
      atom_ptr = &vtop->type;
      if ((atom_ptr->t & VT_BTYPE) != VT_PTR)
        expect("pointer");
      atom = pointed_type(atom_ptr);
      size = type_size(atom, &align);
      if (size > 8
          || (size & (size - 1))
          || (atok > TOK___atomic_compare_exchange
              && (0 == btype_size(atom->t & VT_BTYPE)
                  || (atom->t & VT_BTYPE) == VT_PTR)))
        expect("integral or integer-sized pointer target type");
      // GCC does not care either:
      /* if (!(atom->t & VT_ATOMIC))
          cprime_warning("pointer target declaration is missing '_Atomic'"); */
      break;

    case 'p':
      if ((vtop->type.t & VT_BTYPE) != VT_PTR
          || type_size(pointed_type(&vtop->type), &align) != size)
        cprime_error("pointer target type mismatch in argument %d", arg + 1);
      gen_assign_cast(atom_ptr);
      break;
    case 'v':
      gen_assign_cast(atom);
      break;
    case 'l':
      indir();
      gen_assign_cast(atom);
      break;
    case 's':
      save = 1;
      indir();
      store = *vtop;
      vpop();
      break;
    case 'm':
      gen_assign_cast(&int_type);
      break;
    case 'b':
      ct.t = VT_BOOL;
      gen_assign_cast(&ct);
      break;
    }
    if ('.' == template[++arg])
      break;
    skip(',');
  }
  skip(')');

  ct.t = VT_VOID;
  switch (template[arg + 1])
  {
  case 'b':
    ct.t = VT_BOOL;
    break;
  case 'v':
    ct = *atom;
    break;
  }

  sprintf(buf, "%s_%d", get_tok_str(atok, 0), size);
  vpush_helper_func(tok_alloc_const(buf));
  vrott(arg - save + 1);
  gfunc_call(arg - save);

  vpush(&ct);
  PUT_R_RET(vtop, ct.t);
  t = ct.t &VT_BTYPE;
  if (t == VT_BYTE || t == VT_SHORT || t == VT_BOOL)
  {
#ifdef PROMOTE_RET
    vtop->r |= BFVAL(VT_MUSTCAST, 1);
#else
    vtop->type.t = VT_INT;
#endif
  }
  gen_cast(&ct);
  if (save)
  {
    vpush(&ct);
    *vtop = store;
    vswap();
    cpc_vstore_context = "expr_eq";
    vstore();
    cpc_vstore_context = NULL;
  }
}

static Sym *find_cpp_this_symbol(void)
{
  Sym *s = sym_find(tok_alloc_const("this"));
  if (s && (s->type.t & VT_BTYPE) == VT_PTR)
    return s;
  for (s = local_stack; s; s = s->prev)
    if ((s->type.t & VT_BTYPE) == VT_PTR && s->v >= TOK_UIDENT
        && !strncmp(get_tok_str(s->v & ~SYM_FIELD, NULL),
                    "__cprime_this_", 14))
      return s;
  return NULL;
}

ST_FUNC void unary(void)
{
  int n, t, align, size, r;
  int explicit_global_scope = 0;
  CType type;
  Sym *s;
  AttributeDef ad;

  // Generate Line Number Info
  if (debug_modes)
    cprime_debug_line(cprime_state), cprime_tcov_check_line (cprime_state, 1);

  type.ref = NULL;
  /* XXX: GCC 2.95.3 does not generate a table although it should be
     better here */
tok_next:
  /* A leading C++ global-scope qualifier does not change expression lookup
     in CPC's flat global symbol table.  Consume it before parsing the name. */
  if (tok == ':')
  {
    explicit_global_scope = 1;
    next();
    if (tok != ':')
      cprime_error("':' expected in global scope qualifier");
    next();
    goto tok_next;
  }
  if (tok >= TOK_UIDENT
      && (!strcmp(get_tok_str(tok, NULL), "static_cast")
          || !strcmp(get_tok_str(tok, NULL), "reinterpret_cast")
          || !strcmp(get_tok_str(tok, NULL), "const_cast")
          || !strcmp(get_tok_str(tok, NULL), "dynamic_cast")))
  {
    CType cast_type;
    AttributeDef cast_ad;
    int cast_name = tok;
    int cast_v = 0;

    next();
    skip(TOK_LT);
    memset(&cast_ad, 0, sizeof(cast_ad));
    if (!parse_btype(&cast_type, &cast_ad, 0))
      cprime_error("type expected in '%s'", get_tok_str(cast_name, NULL));
    type_decl(&cast_type, &cast_ad, &cast_v, TYPE_ABSTRACT);
    if (tok == TOK_SAR)
    {
      tok = TOK_GT;
      unget_tok(TOK_GT);
    }
    skip(TOK_GT);
    skip('(');
    expr_eq();
    skip(')');
    if (is_reference_type(&cast_type))
    {
      CType source_type = vtop->type;
      CType target_type = *pointed_type(&cast_type);

      if (is_reference_type(&source_type))
        source_type = *pointed_type(&source_type);
      if (!is_compatible_unqualified_types(&source_type, &target_type))
        cast_error(&source_type, &cast_type);
      if (!is_reference_type(&vtop->type))
      {
        test_lvalue();
        mk_pointer(&vtop->type);
        gaddrof();
      }
      vtop->type = cast_type;
    }
    else
      gen_cast(&cast_type);
    goto unary_post;
  }
  if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "delete"))
  {
    Sym *free_sym;
    next();
    if (tok == '[')
    {
      next();
      skip(']');
    }
    unary();
    if ((vtop->type.t & VT_BTYPE) != VT_PTR)
      cprime_error("delete requires a pointer operand");
    free_sym = sym_find(tok_alloc_const("free"));
    if (!free_sym)
      free_sym = global_symbol_find(tok_alloc_const("free"));
    if (!free_sym)
      free_sym = external_global_sym(tok_alloc_const("free"), &func_old_type);
    vpushsym(&free_sym->type, free_sym);
    vswap();
    gfunc_param_typed(free_sym->type.ref,
                      free_sym->type.ref ? free_sym->type.ref->next : NULL);
    gfunc_call(1);
    type.t = VT_VOID;
    type.ref = NULL;
    vpush(&type);
    goto unary_post;
  }
  switch (tok)
  {
  case TOK_EXTENSION:
    next();
    goto tok_next;
  case TOK_OPERATOR:
    t = parse_cpp_operator_method_tok();
    if (tok == '(')
    {
      Sym *this_sym = find_cpp_this_symbol();
      if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR))
      {
        int this_r = this_sym->r;
        TokenString *replay = tok_str_alloc();
        if ((this_r & VT_VALMASK) < VT_CONST
            && (this_r & VT_VALMASK) != VT_LLOCAL)
          this_r = (this_r & ~VT_VALMASK) | VT_LOCAL;
        vset(&this_sym->type, this_r, this_sym->c);
        vtop->sym = this_sym;
        tok_str_add(replay, TOK_ARROW);
        tok_str_add(replay, t);
        tok_str_add(replay, '(');
        tok_str_add(replay, 0);
        begin_macro(replay, 1);
        next();
        break;
      }
    }
    unget_tok(tok);
    tok = t;
    goto tok_identifier;
  case TOK_LCHAR:
#ifdef CPRIME_TARGET_PE
    t = VT_SHORT | VT_UNSIGNED;
    goto push_tokc;
#endif
  case TOK_CINT:
  case TOK_CCHAR:
    t = VT_INT;
push_tokc:
    type.t = t;
    vsetc(&type, VT_CONST, &tokc);
    next();
    break;
  case TOK_VOID:
  case TOK_CHAR:
  case TOK_SHORT:
  case TOK_INT:
  case TOK_LONG:
  case TOK_FLOAT:
  case TOK_DOUBLE:
  case TOK_BOOL:
  case TOK_BOOL2:
  case TOK_UNSIGNED:
  {
    CType cast_type;
    AttributeDef cast_ad;
    memset(&cast_ad, 0, sizeof(cast_ad));
    if (parse_btype(&cast_type, &cast_ad, 0) && tok == '(')
    {
      next();
      if (tok == ')')
      {
        CValue zval;
        memset(&zval, 0, sizeof(zval));
        vsetc(&cast_type, VT_CONST, &zval);
      }
      else
        expr_eq();
      skip(')');
      if ((cast_type.t & VT_BTYPE) == VT_BOOL
          && (vtop->type.t & VT_BTYPE) == VT_STRUCT)
        try_call_cpp_bool_conversion_operator();
      gen_cast(&cast_type);
      break;
    }
    if (getenv("CPC_TRACE_EXPR_ERROR"))
      fprintf(stderr, "CPC_EXPR_ERROR func=%s tok=%s file=%s line=%d macro=%p\n",
              funcname ? funcname : "<none>", get_tok_str(tok, &tokc),
              file ? file->filename : "<none>", file ? file->line_num : 0,
              (void *)macro_stack);
    cprime_error("expression expected before '%s'", get_tok_str(tok, &tokc));
  }
  case TOK_CUINT:
    t = VT_INT | VT_UNSIGNED;
    goto push_tokc;
  case TOK_CLLONG:
    t = VT_LLONG;
    goto push_tokc;
  case TOK_CULLONG:
    t = VT_LLONG | VT_UNSIGNED;
    goto push_tokc;
  case TOK_CFLOAT:
    t = VT_FLOAT;
    goto push_tokc;
  case TOK_CDOUBLE:
    t = VT_DOUBLE;
    goto push_tokc;
  case TOK_CLDOUBLE:
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
    t = VT_DOUBLE | VT_LONG;
#else
    t = VT_LDOUBLE;
#endif
    goto push_tokc;
  case TOK_CLONG:
    t = (LONG_SIZE == 8 ? VT_LLONG : VT_INT) | VT_LONG;
    goto push_tokc;
  case TOK_CULONG:
    t = (LONG_SIZE == 8 ? VT_LLONG : VT_INT) | VT_LONG | VT_UNSIGNED;
    goto push_tokc;
  case TOK_TRUE:
  case TOK_FALSE:
    vpushi(tok == TOK_TRUE ? 1 : 0);
    vtop->type.t = VT_BOOL;
    next();
    break;
  case TOK___FUNCTION__:
    if (!non_iso)
      goto tok_identifier;
  // Fall Thru
  case TOK___FUNC__:
    tok = TOK_STR;
    cstr_reset(&tokcstr);
    cstr_cat(&tokcstr, funcname, 0);
    tokc.str.size = tokcstr.size;
    tokc.str.data = tokcstr.data;
    goto case_TOK_STR;
  case TOK_LSTR:
#ifdef CPRIME_TARGET_PE
    t = VT_SHORT | VT_UNSIGNED;
#else
    t = VT_INT;
#endif
    goto str_init;
  case TOK_STR:
case_TOK_STR:
    // String Parsing
    t = char_type.t;
str_init:
    if (cprime_state->warn_write_strings & WARN_ON)
      t |= VT_CONSTANT;
    type.t = t;
    mk_pointer(&type);
    type.t |= VT_ARRAY;
    memset(&ad, 0, sizeof(AttributeDef));
    ad.section = rodata_section;
    decl_initializer_alloc(&type, &ad, VT_CONST, 2, 0, NULL, 0, 0);
    break;
  case TOK_SOTYPE:
  case '(':
    t = tok;
    next();
    if (tok >= TOK_UIDENT)
    {
      int first_tok = tok;
      CValue first_tokc = tokc;
      Sym *type_sym = struct_find(first_tok);
      int first_tok_is_typedef = 0;
      TokenString *replay = tok_str_alloc();

      if (!type_sym)
      {
        Sym *alias_sym = sym_find(first_tok);
        if (!alias_sym)
          alias_sym = global_symbol_find(first_tok);
        if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
            && ((alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
          type_sym = alias_sym->type.ref;
        if (alias_sym && (alias_sym->type.t & VT_TYPEDEF))
          first_tok_is_typedef = 1;
      }
      tok_str_add2(replay, first_tok, &first_tokc);
      next();
      if ((type_sym || first_tok_is_typedef) && tok == '(')
      {
        restore_cpp_lifecycle_probe(replay);
        gexpr();
        skip(')');
        break;
      }
      restore_cpp_lifecycle_probe(replay);
    }
    if (parse_btype(&type, &ad, 0))
    {
      int decltype_cast_type = n || last_btype_was_decltype;
      if (tok == ':')
      {
        next();
        if (tok != ':')
          cprime_error("':' expected");
        next();
        if (tok < TOK_UIDENT || strcmp(get_tok_str(tok, NULL), "value"))
          cprime_error("static template value expected");
        next();
        skip(')');
        vpushi(0);
        break;
      }
      if (!decltype_cast_type)
        type_decl(&type, &ad, &n, TYPE_ABSTRACT);
      skip(')');
      // check ISOC99 compound literal
      if (tok == '{')
      {
        // Data Is Allocated Locally By Default
        if (global_expr)
          r = VT_CONST;
        else
          r = VT_LOCAL;
        // All Except Arrays Are Lvalues
        if (!(type.t & VT_ARRAY))
          r |= VT_LVAL;
        memset(&ad, 0, sizeof(AttributeDef));
        decl_initializer_alloc(&type, &ad, r, 1, 0, NULL, 0, 0);
      }
      else if (t == TOK_SOTYPE)     // From Sizeof/Alignof (...)
      {
        vpush(&type);
        return;
      }
      else
      {
        unary();
        gen_cast(&type);
      }
    }
    else if (tok == '{')
    {
      int saved_nocode_wanted = nocode_wanted;
      if (CONST_WANTED && !NOEVAL_WANTED)
        expect("constant");
      if (0 == local_scope)
        cprime_error("statement expression outside of function");
      // Save All Registers
      save_regs(0);
      /* statement expression : we do not accept break/continue
         inside as GCC does.  We do retain the nocode_wanted state,
      as statement expressions can't ever be entered from the
      outside, so any reactivation of code emission (from labels
      or loop heads) can be disabled again after the end of it. */
      // Default Return Value Is (Void)
      vpushi(0), vtop->type.t = VT_VOID;
      block(STMT_EXPR);
      /* If the statement expr can be entered, then we retain the current
         nocode_wanted state (from e.g. a 'return 0;' in the stmt-expr).
         If it can't be entered then the state is that from before the
         statement expression.  */
      if (saved_nocode_wanted)
        nocode_wanted = saved_nocode_wanted;
      skip(')');
    }
    else
    {
      gexpr();
      skip(')');
    }
    break;
  case '*':
    next();
    unary();
    if (try_call_cpp_unary_operator('*'))
      break;
    indir();
    break;
  case '&':
    next();
    ++suppress_integral_constexpr_fold;
    unary();
    --suppress_integral_constexpr_fold;
    /* functions names must be treated as function pointers,
       except for unary '&' and sizeof. Since we consider that
       functions are not lvalues, we only have to handle it
       there and in function calls. */
    // Arrays Can Also Be Used Although They Are Not Lvalues
    if ((vtop->type.t & VT_BTYPE) != VT_FUNC &&
        !(vtop->type.t & (VT_ARRAY | VT_VLA)))
      test_lvalue();
    if (vtop->sym)
      vtop->sym->a.addrtaken = 1;
    mk_pointer(&vtop->type);
    gaddrof();
    break;
  case '!':
    next();
    unary();
    if (try_call_cpp_unary_operator('!'))
      break;
    gen_test_zero(TOK_EQ);
    break;
  case '~':
    next();
    unary();
    vpushi(-1);
    gen_op('^');
    break;
  case '+':
    next();
    unary();
    if ((vtop->type.t & VT_BTYPE) == VT_PTR)
      cprime_error("pointer not accepted for unary plus");
    /* In order to force cast, we add zero, except for floating point
    where we really need an noop (otherwise -0.0 will be transformed
    into +0.0).  */
    if (!is_float(vtop->type.t))
    {
      vpushi(0);
      gen_op('+');
    }
    break;
  case TOK_SIZEOF:
  case TOK_ALIGNOF1:
  case TOK_ALIGNOF2:
  case TOK_ALIGNOF3:
    t = tok;
    next();
    if (tok == '(')
      tok = TOK_SOTYPE;
    expr_type(&type, unary);
    if (t == TOK_SIZEOF)
    {
      vpush_type_size(&type, &align);
      gen_cast_s(VT_SIZE_T);
    }
    else
    {
      type_size(&type, &align);
      s = NULL;
      if (vtop[1].r & VT_SYM)
        s = vtop[1].sym; // Hack: Accessing Previous Vtop
      if (s && s->a.aligned)
        align = 1 << (s->a.aligned - 1);
      vpushs(align);
    }
    break;

  case TOK_builtin_expect:
    // __Builtin_Expect Is A No-Op For Now
    parse_builtin_params(0, "ee");
    vpop();
    break;
  case TOK_builtin_types_compatible_p:
    parse_builtin_params(0, "tt");
    vtop[-1].type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    vtop[0].type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    n = is_compatible_types(&vtop[-1].type, &vtop[0].type);
    vtop -= 2;
    vpushi(n);
    break;
  case TOK_builtin_choose_expr:
  {
    int64_t c;
    next();
    skip('(');
    c = expr_const64();
    skip(',');
    if (!c)
      nocode_wanted++;
    expr_eq();
    if (!c)
    {
      vpop();
      nocode_wanted--;
    }
    skip(',');
    if (c)
      nocode_wanted++;
    expr_eq();
    if (c)
    {
      vpop();
      nocode_wanted--;
    }
    skip(')');
  }
  break;
  case TOK_builtin_constant_p:
    parse_builtin_params(1, "e");
    n = 1;
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) != VT_CONST
        || ((vtop->r & VT_SYM) && vtop->sym->a.addrtaken)
       )
      n = 0;
    vtop--;
    vpushi(n);
    break;
  case TOK_builtin_unreachable:
    parse_builtin_params(0, ""); // Just Skip '()'
    type.t = VT_VOID;
    vpush(&type);
    CODE_OFF();
    break;
  case TOK_builtin_frame_address:
  case TOK_builtin_return_address:
  {
    int tok1 = tok;
    int level;
    next();
    skip('(');
    level = expr_const();
    if (level < 0)
      cprime_error("%s only takes positive integers", get_tok_str(tok1, 0));
    skip(')');
    type.t = VT_VOID;
    mk_pointer(&type);
    vset(&type, VT_LOCAL, 0);       // Local Frame
    while (level--)
    {
#ifdef CPRIME_TARGET_RISCV64
      vpushi(2 * PTR_SIZE);
      gen_op('-');
#endif
      mk_pointer(&vtop->type);
      indir();                    // -> Parent Frame
    }
    if (tok1 == TOK_builtin_return_address)
    {
      // Assume Return Address Is Just Above Frame Pointer On Stack
#ifdef CPRIME_TARGET_ARM
      vpushi(2 * PTR_SIZE);
      gen_op('+');
#elif defined CPRIME_TARGET_RISCV64
      vpushi(PTR_SIZE);
      gen_op('-');
#else
      vpushi(PTR_SIZE);
      gen_op('+');
#endif
      mk_pointer(&vtop->type);
      indir();
    }
  }
  break;
#ifdef CPRIME_TARGET_RISCV64
  case TOK_builtin_va_start:
    parse_builtin_params(0, "ee");
    r = vtop->r &VT_VALMASK;
    if (r == VT_LLOCAL)
      r = VT_LOCAL;
    if (r != VT_LOCAL)
      cprime_error("__builtin_va_start expects a local variable");
    gen_va_start();
    vstore();
    break;
#endif
#ifdef CPRIME_TARGET_X86_64
#ifdef CPRIME_TARGET_PE
  case TOK_builtin_va_start:
    parse_builtin_params(0, "ee");
    r = vtop->r &VT_VALMASK;
    if (r == VT_LLOCAL)
      r = VT_LOCAL;
    if (r != VT_LOCAL)
      cprime_error("__builtin_va_start expects a local variable");
    vtop->r = r;
    vtop->type = char_pointer_type;
    vtop->c.i += 8;
    vstore();
    break;
#else
  case TOK_builtin_va_arg_types:
    parse_builtin_params(0, "t");
    vpushi(classify_x86_64_va_arg(&vtop->type));
    vswap();
    vpop();
    break;
#endif
#endif

#ifdef CPRIME_TARGET_ARM64
  case TOK_builtin_va_start:
  {
    parse_builtin_params(0, "ee");
    // Xx Check Types
    gen_va_start();
    vpushi(0);
    vtop->type.t = VT_VOID;
    break;
  }
  case TOK_builtin_va_arg:
  {
    parse_builtin_params(0, "et");
    type = vtop->type;
    vpop();
    // Xx Check Types
    gen_va_arg(&type);
    vtop->type = type;
    break;
  }
  case TOK___arm64_clear_cache:
  {
    parse_builtin_params(0, "ee");
    gen_clear_cache();
    vpushi(0);
    vtop->type.t = VT_VOID;
    break;
  }
#endif

  // Atomic Operations
  case TOK___atomic_store:
  case TOK___atomic_load:
  case TOK___atomic_exchange:
  case TOK___atomic_compare_exchange:
  case TOK___atomic_fetch_add:
  case TOK___atomic_fetch_sub:
  case TOK___atomic_fetch_or:
  case TOK___atomic_fetch_xor:
  case TOK___atomic_fetch_and:
  case TOK___atomic_fetch_nand:
  case TOK___atomic_add_fetch:
  case TOK___atomic_sub_fetch:
  case TOK___atomic_or_fetch:
  case TOK___atomic_xor_fetch:
  case TOK___atomic_and_fetch:
  case TOK___atomic_nand_fetch:
    parse_atomic(tok);
    break;

  // Pre Operations
  case TOK_INC:
  case TOK_DEC:
    t = tok;
    next();
    unary();
    if (try_call_cpp_unary_operator(t))
      break;
    inc(0, t);
    break;
  case '-':
    next();
    unary();
    if (try_call_cpp_unary_minus_operator())
      break;
    if (is_float(vtop->type.t))
      gen_opif(TOK_NEG);
    else
    {
      vpushi(0);
      vswap();
      gen_op('-');
    }
    break;
  case TOK_LAND:
    if (!non_iso)
      goto tok_identifier;
    next();
    // Allow To Take The Address Of A Label
    if (tok < TOK_UIDENT)
      expect("label identifier");
    s = label_find(tok);
    if (!s)
      s = label_push(&global_label_stack, tok, LABEL_FORWARD);
    else
    {
      if (s->r == LABEL_DECLARED)
        s->r = LABEL_FORWARD;
    }
    if ((s->type.t & VT_BTYPE) != VT_PTR)
    {
      s->type.t = VT_VOID;
      mk_pointer(&s->type);
      s->type.t |= VT_STATIC;
    }
    vpushsym(&s->type, s);
    next();
    break;

  case TOK_GENERIC:
  {
    CType controlling_type;
    int has_default = 0;
    int has_match = 0;
    int learn = 0;
    TokenString *str = NULL;
    int saved_nocode_wanted = nocode_wanted;
    nocode_wanted &= ~CONST_WANTED_MASK;

    next();
    skip('(');
    expr_type(&controlling_type, expr_eq);
    convert_parameter_type (&controlling_type);

    nocode_wanted = saved_nocode_wanted;

    for (;;)
    {
      learn = 0;
      skip(',');
      if (tok == TOK_DEFAULT)
      {
        if (has_default)
          cprime_error("too many 'default'");
        has_default = 1;
        if (!has_match)
          learn = 1;
        next();
      }
      else
      {
        int v;
        parse_btype(&type, &ad, 0);
        type_decl(&type, &ad, &v, TYPE_ABSTRACT);
        if (compare_types(&controlling_type, &type, 0))
        {
          if (has_match)
            cprime_error("type match twice");
          has_match = 1;
          learn = 1;
        }
      }
      skip(':');
      if (learn)
      {
        if (str)
          tok_str_free(str);
        skip_or_save_block(&str);
      }
      else
        skip_or_save_block(NULL);
      if (tok == ')')
        break;
    }
    if (!str)
    {
      char buf[60];
      type_to_str(buf, sizeof buf, &controlling_type, NULL);
      cprime_error("type '%s' does not match any association", buf);
    }
    begin_macro(str, 1);
    next();
    expr_eq();
    if (tok != TOK_EOF)
      expect(",");
    end_macro();
    next();
    break;
  }
  // Special Qnan , Snan And Infinity Values
  case TOK___NAN__:
    n = 0x7fc00000;
special_math_val:
    vpushi(n);
    vtop->type.t = VT_FLOAT;
    next();
    break;
  case TOK___SNAN__:
    n = 0x7f800001;
    goto special_math_val;
  case TOK___INF__:
    n = 0x7f800000;
    goto special_math_val;

  default:
tok_identifier:
    {
      int template_direct_call;
      int template_direct_constexpr = 0;
      int64_t template_direct_constexpr_value = 0;
      int qualified_instance_class_tok = 0;
      int qualified_instance_member_tok = 0;
      int unqualified_call_tok = 0;
      CType template_call_type;
      if (tok < TOK_UIDENT)
      {
        if (getenv("CPC_TRACE_EXPR_ERROR"))
        {
          TokenString *trace_macro = macro_stack;
          fprintf(stderr, "CPC_EXPR_ERROR func=%s tok=%s file=%s line=%d macro=%p\n",
                  funcname ? funcname : "<none>", get_tok_str(tok, &tokc),
                  file ? file->filename : "<none>",
                  file ? file->line_num : 0, (void *)macro_stack);
          while (trace_macro)
          {
            const int *tp = trace_macro->str;
            const int *te = tp + trace_macro->len;
            CValue tv;
            int tt, tc = 0;
            fprintf(stderr, "  macro=%p alloc=%d len=%d toks=",
                    (void *)trace_macro, trace_macro->alloc, trace_macro->len);
            while (tp < te && tc++ < 16)
            {
              TOK_GET(&tt, &tp, &tv);
              if (!tt || tt == TOK_EOF)
                break;
              fprintf(stderr, "%s%s", tc > 1 ? " " : "",
                      get_tok_str(tt, &tv));
            }
            fprintf(stderr, "\n");
            trace_macro = trace_macro->prev;
          }
        }
        cprime_error("expression expected before '%s'", get_tok_str(tok, &tokc));
      }
      t = tok;
      unqualified_call_tok = t;
      next();
      /* Class-template substitution normalizes an operator declaration and
         its body references to a single identifier token (for example
         `operator[]`).  Preserve the implicit-object call semantics for a
         bare call in that replayed body, just as the raw TOK_OPERATOR path
         above does before substitution. */
      if (tok == '(' && !strcmp(get_tok_str(t, NULL), "operator[]"))
      {
        Sym *this_sym = find_cpp_this_symbol();
        if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR))
        {
          int this_r = this_sym->r;
          TokenString *replay = tok_str_alloc();
          if ((this_r & VT_VALMASK) < VT_CONST
              && (this_r & VT_VALMASK) != VT_LLOCAL)
            this_r = (this_r & ~VT_VALMASK) | VT_LOCAL;
          vset(&this_sym->type, this_r, this_sym->c);
          vtop->sym = this_sym;
          tok_str_add(replay, TOK_ARROW);
          tok_str_add(replay, t);
          tok_str_add(replay, '(');
          tok_str_add(replay, 0);
          begin_macro(replay, 1);
          next();
          break;
        }
      }
      if ((!strncmp(get_tok_str(t, NULL), "std::is_", 8)
           || !strncmp(get_tok_str(t, NULL), "__cpc_ns_std_is_", 16))
          && (tok == TOK_LT || tok == '<'))
      {
        TemplateArgList trait_args;
        parse_template_type_args(&trait_args);
        if (tok == ':')
        {
          next();
          if (tok != ':')
            cprime_error("':' expected");
          next();
          if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "value"))
          {
            next();
            vpushi(standard_type_trait_value(t, &trait_args));
            break;
          }
          cprime_error("type trait value expected");
        }
      }
      {
        TemplateDef *td = find_class_template_def(
          explicit_global_scope ? t : find_current_namespace_tok(t));
        if (getenv("CPC_TRACE_NUMERIC_LIMITS")
            && strstr(get_tok_str(t, NULL), "numeric_limits"))
          fprintf(stderr, "CPC_NUM_LIMITS_LOOKUP t=%s ns=%s td=%s tok=%s\n",
                  get_tok_str(t, NULL),
                  get_tok_str(find_current_namespace_tok(t), NULL),
                  td ? get_tok_str(td->name_tok, NULL) : "<none>",
                  get_tok_str(tok, &tokc));
        if (td && td->is_class && (tok == TOK_LT || tok == '<'))
        {
          TemplateArgList args;
          int qualified_member_tok = 0;
          parse_template_type_args(&args);
          if (getenv("CPC_TRACE_NUMERIC_LIMITS")
              && strstr(get_tok_str(t, NULL), "numeric_limits"))
            fprintf(stderr, "CPC_NUM_LIMITS_ARGS t=%s arg0=%s tok=%s\n",
                    get_tok_str(t, NULL),
                    args.nb > 0 ? get_tok_str(args.toks[0], NULL) : "<none>",
                    get_tok_str(tok, &tokc));
          if (tok == ':')
          {
            next();
            if (tok != ':')
              cprime_error("':' expected");
            next();
            if (tok >= TOK_UIDENT
                && !strcmp(get_tok_str(tok, NULL), "value"))
            {
              next();
              if (tok != '(')
              {
                /* Static data member (or standard type trait): fold the
                   value directly. */
                vpushi(standard_type_trait_value(t, &args));
                break;
              }
              /* Static member function call (e.g. identity<int>::value()):
                 rewind '::value(' so the general Class<T>::member path below
                 resolves it as a static member function. */
              unget_tok(tok_alloc_const("value"));
              unget_tok(':');
              unget_tok(':');
            }
            else
            {
              if (is_standard_type_trait_tok(t))
                cprime_error("type trait value expected");
              qualified_member_tok = tok;
            }
          }
          t = instantiate_template_if_needed(td, &args);
          if (getenv("CPC_TRACE_NUMERIC_LIMITS"))
            fprintf(stderr, "CPC_NUM_LIMITS_INST inst=%s has_struct=%d member=%s\n",
                    get_tok_str(t, NULL), struct_find(t) != NULL,
                    qualified_member_tok ? get_tok_str(qualified_member_tok, NULL) : "<none>");
          compile_pending_template_specs_without_member_flush();
          if (qualified_member_tok)
          {
            qualified_instance_class_tok = t;
            qualified_instance_member_tok = qualified_member_tok;
            instantiate_static_template_member_for_call(t,
                                                        qualified_member_tok);
            t = make_static_member_tok(t, qualified_member_tok);
            next();
          }
        }
      }
      if (tok == ':')
      {
        int class_tok, parts[16], nb_parts = 0;
        int handled_scoped_enum = 0;
        Sym *class_alias_sym;
        if (!explicit_global_scope)
          t = find_current_namespace_tok(t);
        class_tok = t;
        class_alias_sym = sym_find(t);
        if (!class_alias_sym)
          class_alias_sym = global_symbol_find(t);
        if (class_alias_sym
            && (class_alias_sym->type.t & VT_TYPEDEF)
            && ((class_alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
        {
          int alias_struct_tok = get_struct_type_name_tok(&class_alias_sym->type);
          if (alias_struct_tok)
            class_tok = alias_struct_tok;
        }
        if (class_alias_sym && IS_ENUM(class_alias_sym->type.t))
        {
          TokenString *replay = tok_str_alloc();
          tok_str_add(replay, tok);
          next();
          if (tok != ':')
          {
            restore_cpp_lifecycle_probe(replay);
          }
          else
          {
            int enum_member_tok, scoped_enum_member_tok;
            Sym *enum_member_sym;
            tok_str_free(replay);
            next();
            if (tok < TOK_UIDENT)
              cprime_error("enum member name");
            enum_member_tok = tok;
            scoped_enum_member_tok = make_static_member_tok(class_tok,
                                                            enum_member_tok);
            enum_member_sym = sym_find(scoped_enum_member_tok);
            if (!enum_member_sym)
              enum_member_sym = global_symbol_find(scoped_enum_member_tok);
            if (enum_member_sym)
              t = scoped_enum_member_tok;
            else
              t = enum_member_tok;
            next();
            handled_scoped_enum = 1;
          }
        }
        if (!handled_scoped_enum && struct_find(class_tok))
        {
          TokenString *replay = tok_str_alloc();
          tok_str_add(replay, tok);
          next();
          if (tok != ':')
          {
            restore_cpp_lifecycle_probe(replay);
          }
          else
          {
            tok_str_free(replay);
            next();
            if (tok < TOK_UIDENT)
              cprime_error("static data member name");
            {
              int static_member_tok = tok;
              qualified_instance_class_tok = class_tok;
              qualified_instance_member_tok = static_member_tok;
              t = make_static_member_tok(class_tok, static_member_tok);
              instantiate_static_template_member_for_call(class_tok,
                                                          static_member_tok);
              next();
            }
          }
        }
        else if (!handled_scoped_enum)
        {
          TokenString *replay = tok_str_alloc();
          parts[nb_parts++] = t;
          tok_str_add(replay, tok);
          next();
          if (tok != ':')
          {
            restore_cpp_lifecycle_probe(replay);
          }
          else
          {
            tok_str_free(replay);
            next();
            if (tok < TOK_UIDENT)
              cprime_error("qualified name");
            if (nb_parts >= (int)(sizeof(parts) / sizeof(parts[0])))
              cprime_error("qualified name too deep");
            parts[nb_parts++] = tok;
            next();
            while (tok == ':')
            {
              next();
              if (tok != ':')
                cprime_error("':' expected");
              next();
              if (tok < TOK_UIDENT)
                cprime_error("qualified name");
              if (nb_parts >= (int)(sizeof(parts) / sizeof(parts[0])))
                cprime_error("qualified name too deep");
              parts[nb_parts++] = tok;
              next();
            }
            t = make_namespace_tok_from_parts(parts, nb_parts);
          }
        }
      }
      if ((!strncmp(get_tok_str(t, NULL), "std::is_", 8)
           || !strncmp(get_tok_str(t, NULL), "__cpc_ns_std_is_", 16))
          && (tok == TOK_LT || tok == '<'))
      {
        TemplateArgList trait_args;
        parse_template_type_args(&trait_args);
        if (tok == ':')
        {
          next();
          if (tok != ':')
            cprime_error("':' expected");
          next();
          if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "value"))
          {
            next();
            vpushi(standard_type_trait_value(t, &trait_args));
            break;
          }
          cprime_error("type trait value expected");
        }
      }
      {
        TemplateDef *td = find_class_template_def(t);
        if (getenv("CPC_TRACE_NUMERIC_LIMITS")
            && strstr(get_tok_str(t, NULL), "numeric_limits"))
          fprintf(stderr, "CPC_NUM_LIMITS_Q_LOOKUP t=%s td=%s tok=%s\n",
                  get_tok_str(t, NULL),
                  td ? get_tok_str(td->name_tok, NULL) : "<none>",
                  get_tok_str(tok, &tokc));
        if (td && td->is_class && (tok == TOK_LT || tok == '<'))
        {
          TemplateArgList args;
          int qualified_member_tok = 0;
          parse_template_type_args(&args);
          if (getenv("CPC_TRACE_NUMERIC_LIMITS")
              && strstr(get_tok_str(t, NULL), "numeric_limits"))
            fprintf(stderr, "CPC_NUM_LIMITS_Q_ARGS t=%s arg0=%s tok=%s\n",
                    get_tok_str(t, NULL),
                    args.nb > 0 ? get_tok_str(args.toks[0], NULL) : "<none>",
                    get_tok_str(tok, &tokc));
          if (tok == ':')
          {
            next();
            if (tok != ':')
              cprime_error("':' expected");
            next();
            if (tok >= TOK_UIDENT
                && !strcmp(get_tok_str(tok, NULL), "value"))
            {
              next();
              if (tok != '(')
              {
                vpushi(standard_type_trait_value(t, &args));
                break;
              }
              unget_tok(tok_alloc_const("value"));
              unget_tok(':');
              unget_tok(':');
            }
            else if (is_standard_type_trait_tok(t))
              cprime_error("static template value expected");
            else
              qualified_member_tok = tok;
          }
          t = instantiate_template_if_needed(td, &args);
          compile_pending_template_specs_without_member_flush();
          if (getenv("CPC_TRACE_NUMERIC_LIMITS"))
            fprintf(stderr, "CPC_NUM_LIMITS_Q_INST inst=%s has_struct=%d member=%s\n",
                    get_tok_str(t, NULL), struct_find(t) != NULL,
                    qualified_member_tok ? get_tok_str(qualified_member_tok, NULL) : "<none>");
          if (qualified_member_tok)
          {
            qualified_instance_class_tok = t;
            qualified_instance_member_tok = qualified_member_tok;
            instantiate_static_template_member_for_call(t,
                                                        qualified_member_tok);
            t = make_static_member_tok(t, qualified_member_tok);
            next();
          }
        }
      }
      if (qualified_instance_member_tok && tok == ':')
      {
        TokenString *replay = tok_str_alloc();
        tok_str_add(replay, tok);
        next();
        if (tok == ':')
        {
          next();
          if (tok == qualified_instance_member_tok)
            next();
          tok_str_free(replay);
        }
        else
          restore_cpp_lifecycle_probe(replay);
      }
      /* Functional construction and braced temporaries inspect the class
         table before ordinary expression symbol lookup below.  Resolve an
         unqualified name through the current/imported namespace here as
         well, so `using namespace ns; Type(args)` names the same class as a
         declaration or a qualified expression does. */
      if (!explicit_global_scope)
        t = find_current_namespace_tok(t);
      if (tok == ':' && struct_find(t))
      {
        int class_tok = t;
        Sym *class_alias_sym;
        class_alias_sym = sym_find(t);
        if (!class_alias_sym)
          class_alias_sym = global_symbol_find(t);
        if (class_alias_sym
            && (class_alias_sym->type.t & VT_TYPEDEF)
            && ((class_alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
        {
          int alias_struct_tok = get_struct_type_name_tok(&class_alias_sym->type);
          if (alias_struct_tok)
            class_tok = alias_struct_tok;
        }
        next();
        if (tok != ':')
          cprime_error("':' expected");
        next();
        if (tok < TOK_UIDENT)
          cprime_error("static data member name");
        {
          int static_member_tok = tok;
          qualified_instance_class_tok = class_tok;
          qualified_instance_member_tok = static_member_tok;
          t = make_static_member_tok(class_tok, static_member_tok);
          instantiate_static_template_member_for_call(class_tok,
                                                      static_member_tok);
        }
        next();
      }
      {
        TemplateDef *td = find_class_template_def(t);
        if (td && td->is_class && (tok == TOK_LT || tok == '<'))
        {
          TemplateArgList args;
          parse_template_type_args(&args);
          t = instantiate_template_if_needed(td, &args);
          compile_pending_template_specs_without_member_flush();
        }
      }
      if (tok == '{' && struct_find(t))
      {
        Sym *class_sym = struct_find(t);
        type.t = class_sym->type.t;
        type.ref = class_sym;
        materialize_braced_temporary(&type);
        break;
      }
      n = 0;
      template_direct_call = 0;
      if (tok == TOK_LT || tok == '<')
      {
        TemplateDef *td = find_function_template_def(t);
        if (td)
        {
          TemplateArgList explicit_args;
          CType inferred_return_type;
          int has_inferred_return_type = 0;
          int explicit_call_has_typed_arg;

          parse_template_type_args(&explicit_args);
          if (tok != '(')
            cprime_error("explicit template instantiation must be followed by call");
          if (try_eval_integer_log2_template_call(td, &explicit_args,
                                                  &template_direct_constexpr_value))
            template_direct_constexpr = 1;
          explicit_call_has_typed_arg = td->func_min_args > 0;
          if (token_string_is_auto_type_token_prefix(td->def_str)
              && template_return_ctype_from_struct_tok(&inferred_return_type,
                infer_template_return_struct_tok(td, &explicit_args)))
            has_inferred_return_type = 1;
          t = instantiate_template_if_needed(td, &explicit_args);
          compile_pending_template_specs_without_member_flush();
          s = sym_find(t);
          if (!s)
            s = external_helper_sym(t);
          if (!has_inferred_return_type && s
              && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref
              && (s->type.ref->type.t & VT_BTYPE) == VT_STRUCT)
          {
            inferred_return_type = s->type.ref->type;
            has_inferred_return_type = 1;
          }
          if (s && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref)
          {
            if (has_inferred_return_type)
              template_call_type =
                make_template_func_type_with_return(&s->type,
                                                    &inferred_return_type);
            else
              template_call_type = s->type;
          }
          else if (has_inferred_return_type)
            template_call_type =
              make_template_func_type_from_return(&inferred_return_type,
                                                  explicit_call_has_typed_arg);
          else
            template_call_type = make_template_func_type(explicit_args.toks[0],
                                                         explicit_call_has_typed_arg);
          template_direct_call = 1;
          n = 1;
        }
      }
      if (!strcmp(get_tok_str(t, NULL), "new")
          && try_parse_cpp_placement_new_after_name())
        break;
      if (!strcmp(get_tok_str(t, NULL), "static_assert") && tok == '(')
      {
        int assertion_value;
        next();
        assertion_value = expr_const();
        if (tok == ',')
        {
          next();
          skip_or_save_block(NULL);
        }
        skip(')');
        if (!assertion_value && !defer_pending_member_funcs
            && !compiling_non_lifecycle_template_member_body)
          cprime_error("static assertion failed in '%s'",
                       funcname ? funcname : "<global>");
        vpushi(0);
        break;
      }
      /* A class name can also have function-template metadata recorded under
         the same token for its templated constructors.  In an expression such
         as Class(arg), the class functional construction takes precedence. */
      if (tok == '(')
      {
        Sym *type_sym = struct_find(t);
        if (type_sym && IS_ENUM(type_sym->type.t)
            && try_parse_cpp_functional_type_cast(t))
          break;
        if (type_sym && (type_sym->type.t & VT_BTYPE) == VT_STRUCT
            && try_parse_cpp_functional_constructor(t))
          break;
      }
      {
        TemplateDef *td = find_function_template_def(t);
        if (td && tok == '(' && !has_free_func_overload(t))
        {
          int type_tok = 0, inferred_call;
          int prefer_concrete_call = 0;
          int legacy_explicit_call = 0;
          CType inferred_return_type;
          int has_inferred_return_type = 0;
          next();
          inferred_call = 0;
          if (tok == TOK_CHAR || tok == TOK_INT || tok == TOK_LONG
              || tok == TOK_FLOAT || tok == TOK_DOUBLE)
          {
            int candidate_type_tok = tok;
            CValue candidate_type_tokc = tokc;
            next();
            if (tok == ')')
            {
              next();
              if (tok != '(')
                cprime_error("explicit template instantiation must be followed by call");
              type_tok = candidate_type_tok;
              legacy_explicit_call = 1;
            }
            else
            {
              unget_tok(tok);
              tok = candidate_type_tok;
              tokc = candidate_type_tokc;
            }
          }
          if (!legacy_explicit_call)
          {
            TokenString *call_args[32];
            CType call_arg_types[32];
            int call_arg_count;
            TemplateArgList inferred_args;
            unget_tok('(');
            call_arg_count = probe_template_call_args(call_args, call_arg_types, 32);
            if (free_func_has_exact_concrete_match(t, call_arg_types,
                                                   call_arg_count))
              prefer_concrete_call = 1;
            else
            {
              td = find_function_template_for_call(t, call_arg_types,
                                                   call_arg_count);
              if (!td)
                cprime_error("no matching function template '%s'",
                             get_tok_str(t, NULL));
              if (!infer_template_args_from_call(td, call_arg_types,
                                                 call_arg_count,
                                                 &inferred_args))
                cprime_error("conflicting deductions for function template '%s'",
                             get_tok_str(t, NULL));
              type_tok = inferred_args.toks[0];
              if (token_string_is_auto_type_token_prefix(td->def_str)
                  && template_return_ctype_from_struct_tok(&inferred_return_type,
                    infer_template_return_struct_tok(td, &inferred_args)))
                has_inferred_return_type = 1;
              if (getenv("CPC_DUMP_TEMPLATE_CALL")
                  && (strstr(get_tok_str(t, NULL), "clClamp")
                      || strstr(get_tok_str(t, NULL), "clMin")
                      || strstr(get_tok_str(t, NULL), "clMax")
                      || strstr(get_tok_str(t, NULL), "clLerp")))
                fprintf(stderr, "CPC_TEMPLATE_CALL %s type_arg=%s has_ret=%d ret=%s\n",
                        get_tok_str(t, NULL), get_tok_str(type_tok, NULL),
                        has_inferred_return_type,
                        has_inferred_return_type
                          && get_struct_type_name_tok(&inferred_return_type)
                          ? get_tok_str(get_struct_type_name_tok(&inferred_return_type), NULL)
                          : "<none>");
              t = instantiate_template_if_needed(td, &inferred_args);
              compile_pending_template_specs_without_member_flush();
              inferred_call = 1;
            }
          }
          if (!prefer_concrete_call)
          {
            if (tok != '(')
              cprime_error("template instantiation must be followed by call");
            if (!inferred_call)
            {
              TemplateArgList args;
              template_arg_list_one(&args, type_tok);
              if (token_string_is_auto_type_token_prefix(td->def_str)
                  && template_return_ctype_from_struct_tok(&inferred_return_type,
                    infer_template_return_struct_tok(td, &args)))
                has_inferred_return_type = 1;
              t = instantiate_template_if_needed(td, &args);
              compile_pending_template_specs_without_member_flush();
            }
            s = sym_find(t);
            if (!s)
              s = external_helper_sym(t);
            if (s && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref)
            {
              if (has_inferred_return_type)
                template_call_type =
                  make_template_func_type_with_return(&s->type,
                                                      &inferred_return_type);
              else
                template_call_type = s->type;
            }
            else if (has_inferred_return_type)
              template_call_type =
                make_template_func_type_from_return(&inferred_return_type,
                                                    inferred_call);
            else
              template_call_type = make_template_func_type(type_tok, inferred_call);
            template_direct_call = 1;
            n = 1;
          }
        }
      }
      if (!n)
      {
        int lookup_tok = t;
        s = explicit_global_scope ? global_symbol_find(t)
                                  : find_namespace_or_plain_symbol(&t);
        if (!s && qualified_instance_member_tok)
        {
          Sym *enum_member_sym = sym_find(qualified_instance_member_tok);
          if (!enum_member_sym)
            enum_member_sym = global_symbol_find(qualified_instance_member_tok);
          if (enum_member_sym && (enum_member_sym->type.t & VT_ENUM_VAL))
          {
            s = enum_member_sym;
            t = qualified_instance_member_tok;
          }
        }
        if (!s && nb_defining_class_stack > 0)
        {
          int class_i;
          for (class_i = nb_defining_class_stack - 1;
               class_i >= 0 && !s; --class_i)
          {
            int class_tok = defining_class_stack[class_i];
            int static_tok = make_static_member_tok(class_tok, lookup_tok);
            s = sym_find(static_tok);
            if (!s)
              s = global_symbol_find(static_tok);
            if (s)
              t = static_tok;
          }
        }
      }
      if ((!s || symbol_is_nonstatic_member_of_class(s, qualified_instance_class_tok))
          && tok == '(' && qualified_instance_class_tok
          && qualified_instance_member_tok)
      {
        Sym *this_sym = find_cpp_this_symbol();
        CType explicit_class_type;
        if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR)
            && make_class_type_from_tok(&explicit_class_type,
                                        qualified_instance_class_tok))
        {
          CType *this_target = pointed_type(&this_sym->type);
          explicit_class_type.t |= this_target->t & (VT_CONSTANT | VT_VOLATILE);
          if (resolve_member_func_by_arg_count(&explicit_class_type,
                                               qualified_instance_member_tok,
                                               0)
              || resolve_member_func(&explicit_class_type,
                                     qualified_instance_member_tok))
          {
            int this_r = this_sym->r;
            TokenString *replay = tok_str_alloc();
            CType this_as_explicit = explicit_class_type;
            CType this_as_explicit_ptr;
            this_as_explicit_ptr = this_as_explicit;
            mk_pointer(&this_as_explicit_ptr);
            if ((this_r & VT_VALMASK) < VT_CONST
                && (this_r & VT_VALMASK) != VT_LLOCAL)
              this_r = (this_r & ~VT_VALMASK) | VT_LOCAL;
            vset(&this_as_explicit_ptr, this_r, this_sym->c);
            vtop->sym = this_sym;
            /* An explicitly qualified base call is statically dispatched in
               C++.  Carry that fact through the short `this->member` replay
               so the ordinary member-call lowering does not consult the
               dynamic vtable and recurse into the override. */
            explicitly_qualified_nonvirtual_class_tok =
              qualified_instance_class_tok;
            explicitly_qualified_nonvirtual_method_tok =
              qualified_instance_member_tok;
            tok_str_add(replay, TOK_ARROW);
            tok_str_add(replay, qualified_instance_member_tok);
            tok_str_add(replay, '(');
            tok_str_add(replay, 0);
            begin_macro(replay, 1);
            next();
            break;
          }
        }
      }
      /* Class scope precedes namespace/global scope for an unqualified call.
         Replay may already have published a placeholder global with the same
         spelling, so do this lookup even when ordinary symbol lookup found
         something. */
      if (!explicit_global_scope && tok == '(' && is_cpp_translation_unit())
      {
        Sym *this_sym = find_cpp_this_symbol();
        if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR))
        {
          CType *this_target = pointed_type(&this_sym->type);
          int this_class_tok = get_struct_type_name_tok(this_target);
          if (type_has_member_func_name(this_target, unqualified_call_tok)
              && (!s || !symbol_is_nonstatic_member_of_class(s,
                                                              this_class_tok)))
          {
            int this_r = this_sym->r;
            TokenString *replay = tok_str_alloc();
            if ((this_r & VT_VALMASK) < VT_CONST
                && (this_r & VT_VALMASK) != VT_LLOCAL)
              this_r = (this_r & ~VT_VALMASK) | VT_LOCAL;
            vset(&this_sym->type, this_r, this_sym->c);
            vtop->sym = this_sym;
            tok_str_add(replay, TOK_ARROW);
            tok_str_add(replay, unqualified_call_tok);
            tok_str_add(replay, '(');
            tok_str_add(replay, 0);
            begin_macro(replay, 1);
            next();
            break;
          }
        }
      }
      /* C++ functional cast: typedef-name(expr) */
      if (s && (s->type.t & VT_TYPEDEF)
          && ((s->type.t & VT_BTYPE) == VT_STRUCT)
          && tok == '(')
      {
        int alias_struct_tok = get_struct_type_name_tok(&s->type);
        if (alias_struct_tok && try_parse_cpp_functional_constructor(alias_struct_tok))
          break;
      }
      if (s && (s->type.t & VT_TYPEDEF) && tok == '(')
      {
        if (try_parse_cpp_functional_type_cast(t))
          break;
      }
      if (try_parse_cpp_functional_type_cast(t))
        break;
      if (try_parse_cpp_functional_constructor(t))
        break;
      if (template_direct_constexpr && tok == '(')
      {
        next();
        skip(')');
        vpushi((int)template_direct_constexpr_value);
        break;
      }
      if (template_direct_call)
      {
        CValue cval;
        if (s && ((s->type.t & VT_BTYPE) == VT_FUNC)
            && s->type.ref
            && ((s->type.ref->type.t & VT_BTYPE) == VT_STRUCT))
          template_call_type = s->type;
        cval.i = 0;
        if (s)
          s->type = template_call_type;
        vsetc(&template_call_type, VT_CONST | VT_SYM, &cval);
        vtop->sym = s;
        break;
      }
      if (!s || IS_ASM_SYM(s))
      {
        const char *name = get_tok_str(t, NULL);
        /* `nullptr` is a C++ core literal, not an ordinary identifier.  The
           runtime headers also define it as zero for legacy paths, but saved
           overload-call arguments can be replayed after that macro context
           has been consumed.  Recognize the literal directly so overload
           probing and emission see the same null pointer constant. */
        if (!strcmp(name, "nullptr"))
        {
          vpushi(0);
          break;
        }
        if (tok == '(')
        {
          Sym *this_sym = find_cpp_this_symbol();
          if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR))
          {
            CType *this_target = pointed_type(&this_sym->type);
            if (type_has_member_func_name(this_target, t))
            {
              int this_r = this_sym->r;
              TokenString *replay = tok_str_alloc();
              if ((this_r & VT_VALMASK) < VT_CONST
                  && (this_r & VT_VALMASK) != VT_LLOCAL)
                this_r = (this_r & ~VT_VALMASK) | VT_LOCAL;
              vset(&this_sym->type, this_r, this_sym->c);
              vtop->sym = this_sym;
              tok_str_add(replay, TOK_ARROW);
              tok_str_add(replay, t);
              tok_str_add(replay, '(');
              tok_str_add(replay, 0);
              begin_macro(replay, 1);
              next();
              break;
            }
          }
        }
        if (tok != '(')
        {
          Sym *this_sym = find_cpp_this_symbol();
          if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR))
          {
            CType *this_target = pointed_type(&this_sym->type);
            if ((this_target->t & VT_BTYPE) == VT_STRUCT)
            {
              Sym *static_member = find_static_member_try(this_target, t, NULL);
              if (static_member
                  && (static_member->r & VT_VALMASK) == VT_CONST)
              {
                CValue cval;
                CType value_type = static_member->type;
                cval.i = static_member->c;
                if (IS_ENUM_VAL(static_member->type.t))
                  cval.i = static_member->enum_val;
                value_type.t &= ~VT_STORAGE;
                if (IS_ENUM_VAL(static_member->type.t)
                    || (static_member->type.t & VT_CONSTANT))
                  vsetc(&value_type, VT_CONST, &cval);
                else
                {
                  vsetc(&static_member->type,
                        static_member->r | VT_LVAL, &cval);
                  if (static_member->r & VT_SYM)
                    vtop->c.i = 0;
                }
                vtop->sym = static_member;
                maybe_indir_reference();
                break;
              }
              int cumofs, qualifiers;
              Sym *field = find_field_try(this_target, t, &cumofs);
              if (field)
              {
                int this_r = this_sym->r;
                qualifiers = this_target->t & (VT_CONSTANT | VT_VOLATILE);
                if ((this_r & VT_VALMASK) < VT_CONST
                    && (this_r & VT_VALMASK) != VT_LLOCAL)
                  this_r = (this_r & ~VT_VALMASK) | VT_LOCAL;
                vset(&this_sym->type, this_r, this_sym->c);
                vtop->sym = this_sym;
                indir();
                gaddrof();
                vtop->type = char_pointer_type;
                vpushi(cumofs);
                gen_op('+');
                vtop->type = field->type;
                vtop->type.t |= qualifiers;
                if (!(vtop->type.t & VT_ARRAY))
                {
                  vtop->r |= VT_LVAL;
#ifdef CONFIG_CPRIME_BCHECK
                  if (cprime_state->do_bounds_check)
                    vtop->r |= VT_MUSTBOUND;
#endif
                }
                maybe_indir_reference();
                break;
              }
            }
          }
          if (getenv("CPC_TRACE_UNDECLARED"))
          {
            Sym *trace_sym;
            int trace_count = 0;
            fprintf(stderr,
                    "CPC_UNDECLARED name=%s func=%s file=%s line=%d locals=",
                    name, funcname ? funcname : "<none>",
                    file ? file->filename : "<none>",
                    file ? file->line_num : 0);
            for (trace_sym = local_stack; trace_sym && trace_count < 24;
                 trace_sym = trace_sym->prev, ++trace_count)
              fprintf(stderr, "%s%s", trace_count ? "," : "",
                      get_tok_str(trace_sym->v & ~SYM_FIELD, NULL));
            fprintf(stderr, "\n");
          }
          cprime_error("'%s' undeclared", name);
        }
        /* for simple function calls, we tolerate undeclared
           external reference to int() function */
        cprime_warning_c(warn_implicit_function_declaration)(
          "implicit declaration of function '%s'", name);
        s = external_global_sym(t, &func_old_type);
      }
      if (s && tok == '(')
      {
        Sym *this_sym = find_cpp_this_symbol();
        if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR)
            && symbol_is_nonstatic_member_of_class(
                 s, get_struct_type_name_tok(pointed_type(&this_sym->type))))
        {
          int this_r = this_sym->r;
          TokenString *replay = tok_str_alloc();
          if ((this_r & VT_VALMASK) < VT_CONST
              && (this_r & VT_VALMASK) != VT_LLOCAL)
            this_r = (this_r & ~VT_VALMASK) | VT_LOCAL;
          vset(&this_sym->type, this_r, this_sym->c);
          vtop->sym = this_sym;
          tok_str_add(replay, TOK_ARROW);
          tok_str_add(replay, t);
          tok_str_add(replay, '(');
          tok_str_add(replay, 0);
          begin_macro(replay, 1);
          next();
          break;
        }
      }

      r = s->r;
      /* A symbol that has a register is a local register variable,
         which starts out as VT_LOCAL value.  */
      if ((r & VT_VALMASK) < VT_CONST
          && (r & VT_VALMASK) != VT_LLOCAL)
        r = (r & ~VT_VALMASK) | VT_LOCAL;

      if (getenv("CPC_TRACE_STATIC_CALL")
          && strstr(get_tok_str(t, NULL), getenv("CPC_TRACE_STATIC_CALL")))
        fprintf(stderr, "CPC_STATIC_CALL_PUSH t=%s tok=%s s=%s type_t=%04x ret_t=%04x ret_s=%s r=%04x\n",
                get_tok_str(t, NULL), get_tok_str(tok, &tokc),
                s ? get_tok_str(s->v, NULL) : "<none>",
                s ? s->type.t : 0,
                s && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref
                  ? s->type.ref->type.t : 0,
                s && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref
                  ? get_tok_str(get_struct_type_name_tok(&s->type.ref->type), NULL)
                  : "<none>",
                r);
      {
        CType symbol_value_type = s->type;
        CValue symbol_value;
        symbol_value.i = s->c;
        if ((s->type.t & VT_CONSTANT)
            && (((r & VT_VALMASK) == VT_CONST && (s->type.t & VT_EXTERN))
                || (s->a.integral_constexpr
                    && (CONST_WANTED || integral_constant_expression_wanted)
                    && !suppress_integral_constexpr_fold))
            && is_integer_btype(s->type.t & VT_BTYPE))
        {
          /* An in-class integral constant is a value even though its merged
             declaration carries extern/symbol bookkeeping.  Treating that
             bookkeeping as a relocation leaves declaration-only constants
             such as `wstring::npos` as an uninitialized register. */
          r = (r & ~(VT_VALMASK | VT_SYM | VT_LVAL)) | VT_CONST;
          symbol_value_type.t &= ~VT_STORAGE;
          if (s->a.integral_constexpr)
            symbol_value.i = s->const_value;
        }
        vsetc(&symbol_value_type, r, &symbol_value);
      }
      /* Point to s as backpointer (even without r&VT_SYM).
      Will be used by at least the x86 inline asm parser for
      regvars.  */
      vtop->sym = s;

      if (r & VT_SYM)
      {
        vtop->c.i = 0;
#ifdef CPRIME_TARGET_PE
        if (s->a.dllimport)
        {
          mk_pointer(&vtop->type);
          vtop->r |= VT_LVAL;
          indir();
        }
#endif
      }
      else if (r == VT_CONST && IS_ENUM_VAL(s->type.t))
        vtop->c.i = s->enum_val;
      maybe_indir_reference();
      break;
    }
  }

unary_post:
  // Post Operations
  while (1)
  {
    if (tok == TOK_INC || tok == TOK_DEC)
    {
      inc(1, tok);
      next();
    }
    else if (tok == '.' || tok == TOK_ARROW)
    {
      int qualifiers, cumofs, v, is_arrow, nb_args;
      int explicit_member_type_tok = 0;
      Sym *field, *func_sym, *func_type, *sa;
      VirtualMethodInfo *virtual_method;
      Sym instantiated_call_target;
      CType ft;
      SValue ret;
      int ret_nregs, ret_align, regsize, variadic, has_sret_stack_arg, orig_ret_nregs;
      // Field
      is_arrow = tok == TOK_ARROW;
      if (is_arrow)
        indir();
      qualifiers = vtop->type.t & (VT_CONSTANT | VT_VOLATILE);
      test_lvalue();
      // Expect Pointer On Structure
      next();
      skip_line_markers();
      if (tok == '~')
      {
        int dtor_name;
        CType dtor_type = vtop->type;

        next();
        dtor_name = tok;
        if (!is_pseudo_destructor_type_tok(dtor_name))
          expect("destructor name");
        next();
        if (tok == TOK_LT || tok == '<')
        {
          int angle = 1;
          next();
          while (tok != TOK_EOF && angle > 0)
          {
            if (tok == TOK_LT || tok == '<')
              ++angle;
            else if (tok == TOK_GT || tok == '>')
              --angle;
            else if (tok == TOK_SAR)
            {
              --angle;
              if (angle > 0)
                --angle;
            }
            if (angle > 0)
              next();
          }
          skip(TOK_GT);
        }
        skip('(');
        skip(')');
        call_explicit_destructor(&dtor_type, dtor_name);
        continue;
      }
      if (tok == TOK_OPERATOR)
        v = parse_cpp_operator_method_tok();
      else
      {
        v = tok;
        if (v < TOK_UIDENT)
          cprime_error("field name expected after member access (got '%s')",
                       get_tok_str(tok, &tokc));
        next();
      }
      if ((tok == TOK_LT || tok == '<')
          && (vtop->type.t & VT_BTYPE) == VT_STRUCT
          && class_or_inst_has_member_template_name(
               get_struct_type_name_tok(&vtop->type), v))
      {
        TemplateArgList explicit_member_args;
        parse_template_type_args(&explicit_member_args);
        if (explicit_member_args.nb > 0)
          explicit_member_type_tok = explicit_member_args.toks[0];
      }
      field = NULL;
      if ((vtop->type.t & VT_BTYPE) == VT_STRUCT)
      {
        int owner_tok = 0;
        field = find_field_try_with_owner(&vtop->type, v, &cumofs,
                                          &owner_tok);
        if (field && owner_tok)
        {
          Sym *owner_sym = struct_find(owner_tok);
          if (owner_sym)
          {
            vtop->type.t = owner_sym->type.t
                           | (vtop->type.t & (VT_CONSTANT | VT_VOLATILE));
            vtop->type.ref = owner_sym;
          }
        }
      }
      if (tok == '(' && (vtop->type.t & VT_BTYPE) == VT_STRUCT)
      {
        int recv_struct_tok = get_struct_type_name_tok(&vtop->type);
        int dispatch_static = 1;

        if (recv_struct_tok
            && class_has_static_member_func(recv_struct_tok, v)
            && type_has_member_func_name(&vtop->type, v))
        {
          TokenString *probe_args[32];
          CType probe_types[32];
          int probe_count, pi;

          probe_count = probe_template_call_args(probe_args, probe_types, 32);
          dispatch_static = !resolve_member_func_by_arg_types(&vtop->type, v,
                                                              probe_types,
                                                              probe_count);
          for (pi = 0; pi < probe_count; ++pi)
            tok_str_free(probe_args[pi]);
        }
        if (recv_struct_tok
            && class_has_static_member_func(recv_struct_tok, v)
            && dispatch_static)
        {
          int static_tok = make_static_member_tok(recv_struct_tok, v);
          Sym *static_func = sym_find(static_tok);
          if (!static_func)
            static_func = global_symbol_find(static_tok);
          if (static_func && (static_func->type.t & VT_BTYPE) == VT_FUNC)
          {
            vpop();
            vpushsym(&static_func->type, static_func);
            continue;
          }
        }
      }
      if (tok == '(' && (vtop->type.t & VT_BTYPE) == VT_STRUCT
          && (!field || ((field->type.t & VT_BTYPE) == VT_FUNC)))
      {
        const char *name = get_tok_str(v, NULL);
        TokenString *call_args[32];
        CType call_arg_types[32];
        int call_arg_count, ai, instantiated_member_tok;

        next();
        call_arg_count = count_saved_call_args(call_args, 32);
        infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
        if (type_is_std_initializer_list(&vtop->type)
            && call_arg_count == 0
            && !strcmp(name, "size"))
        {
          s = vtop->type.ref ? vtop->type.ref->next : NULL;
          if (s)
            s = s->next;
          if (!s)
            cprime_error("initializer_list layout missing length field");
          cumofs = s->c;
          gaddrof();
          vtop->type = char_pointer_type;
          vpushi(cumofs);
          gen_op('+');
          vtop->type = s->type;
          vtop->type.t |= qualifiers;
          vtop->r |= VT_LVAL;
          maybe_indir_reference();
          next();
          continue;
        }
        if (type_is_std_initializer_list(&vtop->type)
            && call_arg_count == 0
            && !strcmp(name, "begin"))
        {
          s = vtop->type.ref ? vtop->type.ref->next : NULL;
          if (!s)
            cprime_error("initializer_list layout missing array field");
          cumofs = s->c;
          gaddrof();
          vtop->type = char_pointer_type;
          vpushi(cumofs);
          gen_op('+');
          vtop->type = s->type;
          vtop->type.t |= qualifiers;
          if (!(vtop->type.t & VT_ARRAY))
            vtop->r |= VT_LVAL;
          maybe_indir_reference();
          next();
          continue;
        }
        last_instantiated_member_func_tok = 0;
        {
          int saved_explicit_member_arg_tok = explicit_member_template_arg_tok;
          explicit_member_template_arg_tok = explicit_member_type_tok;
          func_sym = resolve_member_func_by_arg_types(&vtop->type, v,
                                                      call_arg_types,
                                                      call_arg_count);
          explicit_member_template_arg_tok = saved_explicit_member_arg_tok;
        }
        instantiated_member_tok = last_instantiated_member_func_tok;
        if (!func_sym)
          func_sym = resolve_member_field_func_by_arg_types(&vtop->type, v,
                                                            call_arg_types,
                                                            call_arg_count);
        if (!func_sym
            && member_overload_count_for_call(&vtop->type, v,
                                              call_arg_count) == 1)
          func_sym = resolve_member_func_by_arg_count(&vtop->type, v, call_arg_count);
        if (!func_sym
            && !member_overload_exists_for_call(&vtop->type, v,
                                                call_arg_count))
          func_sym = resolve_member_func_by_arg_count(&vtop->type, v, call_arg_count);
        if (!func_sym)
          func_sym = resolve_member_func(&vtop->type, v);
        if (!func_sym)
        {
          int recv_struct_tok = get_struct_type_name_tok(&vtop->type);
          if (recv_struct_tok
              && class_or_inst_has_member_template_name(recv_struct_tok, v))
            func_sym = external_global_sym(make_member_func_tok(recv_struct_tok, v),
                                           &func_old_type);
        }
        if (!func_sym)
          func_sym = sym_find(v);
        if (!func_sym || IS_ASM_SYM(func_sym))
        {
          cprime_warning_c(warn_implicit_function_declaration)(
            "implicit declaration of function '%s'", name);
          func_sym = external_global_sym(v, &func_old_type);
        }
        if (instantiated_member_tok
            && template_member_func_body_materialized(instantiated_member_tok))
        {
          Sym *instantiated_sym = global_symbol_find(instantiated_member_tok);
          if (!instantiated_sym)
            instantiated_sym = sym_find(instantiated_member_tok);
          /* Member-template replay publishes a concrete body target before
             the ordinary candidate scan.  Prefer that target over the
             declaration-only base placeholder; otherwise the call keeps a
             relocation such as List_X_EmplaceBack while the emitted body is
             List_X_EmplaceBack_h..._Arg.

             Finish ordinary call-interface resolution first.  A lowered
             variadic specialization can retain an incomplete parameter list,
             while the declaration fallback still describes a callable
             variadic interface.  A short-lived symbol combines that interface
             with the concrete body name without corrupting either overload
             entry. */
          if (instantiated_sym)
          {
            if (!member_func_matches_arg_types(instantiated_sym,
                                               call_arg_types,
                                               call_arg_count))
            {
              instantiated_call_target = *func_sym;
              instantiated_call_target.v = instantiated_sym->v;
              func_sym = &instantiated_call_target;
            }
            else
              func_sym = instantiated_sym;
          }
        }

        ft = func_sym->type;
        if ((ft.t & VT_BTYPE) == VT_PTR)
          ft = *pointed_type(&ft);
        if ((ft.t & VT_BTYPE) != VT_FUNC)
          cprime_error("function pointer expected for member '%s' in '%s' (type %04x)",
                       name, funcname ? funcname : "<global>", ft.t);
        func_type = ft.ref;
        {
          int receiver_class_tok = get_struct_type_name_tok(&vtop->type);
          if (explicitly_qualified_nonvirtual_class_tok == receiver_class_tok
              && explicitly_qualified_nonvirtual_method_tok == v)
          {
            virtual_method = NULL;
            explicitly_qualified_nonvirtual_class_tok = 0;
            explicitly_qualified_nonvirtual_method_tok = 0;
          }
          else
            virtual_method = find_virtual_method_for_symbol(
              receiver_class_tok, func_sym->v);
        }
        ret.r2 = VT_CONST;
        nb_args = regsize = 0;
        has_sret_stack_arg = 0;
        if ((func_type->type.t & VT_BTYPE) == VT_STRUCT)
        {
          variadic = (func_type->f.func_type == FUNC_ELLIPSIS);
          ret_nregs = gfunc_sret(&func_type->type, variadic, &ret.type,
                                 &ret_align, &regsize);
          if (ret_nregs <= 0)
          {
            size = type_size(&func_type->type, &align);
#ifdef CPRIME_TARGET_ARM64
            if (size < 16)
              while (size & (size - 1))
                size = (size | (size - 1)) + 1;
#endif
            loc = (loc - size) & -align;
            ret.type = func_type->type;
            ret.r = VT_LOCAL | VT_LVAL;
#ifdef CONFIG_CPRIME_BCHECK
            if (cprime_state->do_bounds_check)
              --loc;
#endif
            ret.c.i = loc;
            if (ret_nregs < 0)
              ;
            else
              has_sret_stack_arg = 1;
          }
        }
        else
        {
          ret_nregs = 1;
          ret.type = func_type->type;
        }
        if (ret_nregs > 0)
        {
          ret.c.i = 0;
          PUT_R_RET(&ret, ret.type.t);
        }
        orig_ret_nregs = ret_nregs;

        // Implicit Receiver Argument: &Obj
        mk_pointer(&vtop->type);
        gaddrof();
        gv(RC_INT);
        save_lvalues();
        if (has_sret_stack_arg)
        {
          vseti(VT_LOCAL, ret.c.i);
          nb_args++;
        }

        if (virtual_method)
          push_virtual_call_target(get_struct_type_name_tok(
                                     pointed_type(&vtop->type)),
                                   virtual_method, func_sym);
        else
          vpushsym(&func_sym->type, func_sym);
        if (has_sret_stack_arg)
        {
          vrott(3);
          vswap();
        }
        else
          vswap();

        sa = func_type->next;
        gfunc_param_typed(func_type, sa);
        if (sa)
          sa = sa->next;
        nb_args++;

        for (ai = 0; ai < call_arg_count; ++ai)
        {
          emit_saved_arg_for_param(call_args[ai], sa);
          gfunc_param_typed_with_conversions(func_type, sa);
          nb_args++;
          if (sa)
            sa = sa->next;
        }
        while (sa)
        {
          emit_default_arg(func_type, sa);
          nb_args++;
          sa = sa->next;
        }

        next();
        vcheck_cmp();
        gfunc_call(nb_args);
        drop_leaked_call_target(func_sym);
        if (ret_nregs < 0)
        {
          vsetc(&ret.type, ret.r, &ret.c);
#ifdef CPRIME_TARGET_RISCV64
          arch_transfer_ret_regs(1);
#endif
        }
        else
        {
          n = ret_nregs;
          while (n > 1)
          {
            int rc = reg_classes[ret.r] & ~(RC_INT | RC_FLOAT);
            rc <<= --n;
            for (r = 0; r < NB_REGS; ++r)
              if (reg_classes[r] & rc)
                break;
            vsetc(&ret.type, r, &ret.c);
            }
            vsetc(&ret.type, ret.r, &ret.c);
            vtop->r2 = ret.r2;

          if (((func_type->type.t & VT_BTYPE) == VT_STRUCT) && ret_nregs)
          {
            int addr, offset;

            size = type_size(&func_type->type, &align);
            size = (size + regsize - 1) & -regsize;
            if (ret_align > align)
              align = ret_align;
            loc = (loc - size) & -align;
            addr = loc;
            offset = 0;
            for (;;)
            {
              vset(&ret.type, VT_LOCAL | VT_LVAL, addr + offset);
              vswap();
              cpc_vstore_context = "overload_packed_struct_ret";
              vstore();
              cpc_vstore_context = NULL;
              vtop--;
              if (--ret_nregs == 0)
                break;
              offset += regsize;
            }
            vset(&func_type->type, VT_LOCAL | VT_LVAL, loc);
          }

          t = vtop->type.t & VT_BTYPE;
          if (t == VT_BYTE || t == VT_SHORT || t == VT_BOOL)
          {
#ifdef PROMOTE_RET
            vtop->r |= BFVAL(VT_MUSTCAST, 1);
#else
            vtop->type.t = VT_INT;
#endif
          }
        }
        if (func_type->f.func_noreturn)
        {
          if (debug_modes)
            cprime_tcov_block_end(cprime_state, -1);
          CODE_OFF();
        }
        maybe_indir_reference();
      }
      else
      {
        Sym *static_member = NULL;
        if ((vtop->type.t & VT_BTYPE) == VT_STRUCT)
          static_member = find_static_member_try(&vtop->type, v, NULL);
        if (static_member
            && (static_member->r & VT_VALMASK) == VT_CONST
            && (IS_ENUM_VAL(static_member->type.t)
                || (static_member->type.t & VT_CONSTANT)))
        {
          CValue cval;
          CType value_type = static_member->type;
          vpop();
          cval.i = IS_ENUM_VAL(static_member->type.t)
                     ? static_member->enum_val : static_member->c;
          value_type.t &= ~VT_STORAGE;
          vsetc(&value_type, VT_CONST, &cval);
          vtop->sym = static_member;
          maybe_indir_reference();
          continue;
        }
        if (field)
          s = field;
        else
          s = find_field(&vtop->type, v, &cumofs);
        // Add Field Offset To Pointer
        gaddrof();
        vtop->type = char_pointer_type; // Change Type To 'Char *'
        vpushi(cumofs);
        gen_op('+');
        // Change Type To Field Type, And Set To Lvalue
        vtop->type = s->type;
        vtop->type.t |= qualifiers;
        // An Array Is Never An Lvalue
        if (!(vtop->type.t & VT_ARRAY))
        {
          vtop->r |= VT_LVAL;
#ifdef CONFIG_CPRIME_BCHECK
          // if bound checking, the referenced pointer must be checked
          if (cprime_state->do_bounds_check)
            vtop->r |= VT_MUSTBOUND;
#endif
        }
        maybe_indir_reference();
      }
    }
    else if (tok == '[')
    {
      next();
      gexpr();
      if (!try_call_cpp_index_operator())
      {
        gen_op('+');
        indir();
      }
      skip(']');
    }
    else if (tok == '(')
    {
      SValue ret;
      Sym *sa;
      int nb_args, ret_nregs, ret_align, regsize, variadic;
      int saved_call_arg_count, ai, overload_name_tok;
      TokenString *call_args[32];
      CType call_arg_types[32];
      TokenString *p, *p2;

      if ((vtop->type.t & VT_BTYPE) == VT_STRUCT
          && try_call_cpp_call_operator())
        continue;

      // Function Call
      saved_call_arg_count = -1;
      overload_name_tok = 0;
      if (vtop->sym && has_free_func_overload(vtop->sym->v))
      {
        Sym *func_sym;
        int template_deduction_failed;

        overload_name_tok = vtop->sym->v;
        next();
        saved_call_arg_count = count_saved_call_args(call_args, 32);
        infer_saved_arg_types(call_args, call_arg_types, saved_call_arg_count);
        template_deduction_failed =
          instantiate_function_templates_for_call(overload_name_tok,
                                                  call_arg_types,
                                                  saved_call_arg_count);
        if (getenv("CPC_DUMP_FREE_RESOLVE")
            && overload_name_tok >= TOK_UIDENT)
        {
          int di;
          fprintf(stderr, "CPC_FREE_CALL name=%s argc=%d",
                  get_tok_str(overload_name_tok, NULL), saved_call_arg_count);
          for (di = 0; di < saved_call_arg_count; ++di)
            fprintf(stderr, " arg%d_t=%04x arg%d_s=%s",
                    di, call_arg_types[di].t, di,
                    get_tok_str(get_struct_type_name_tok(&call_arg_types[di]), NULL));
          fprintf(stderr, "\n");
        }
        func_sym = resolve_free_func_by_arg_types(overload_name_tok,
                                                  call_arg_types,
                                                  saved_call_arg_count,
                                                  template_deduction_failed);
        if (!func_sym)
          func_sym = resolve_free_func_by_arg_count(overload_name_tok,
                                                    saved_call_arg_count,
                                                    template_deduction_failed);
        if (!func_sym)
          cprime_error("no matching overloaded function '%s'",
                    get_tok_str(overload_name_tok, NULL));
        vtop->type = func_sym->type;
        vtop->r = func_sym->r;
        vtop->r2 = VT_CONST;
        vtop->c.i = func_sym->c;
        vtop->sym = func_sym;
        if (func_sym->r & VT_SYM)
          vtop->c.i = 0;
      }
      if (vtop->sym)
      {
        CType *overload_type = find_overload_func_type_by_mangled(vtop->sym->v);
        if (overload_type
            && same_func_param_signature(&vtop->sym->type, overload_type))
        {
          use_overload_func_type(vtop->sym, overload_type);
          vtop->type = vtop->sym->type;
        }
      }
      if ((vtop->type.t & VT_BTYPE) != VT_FUNC)
      {
        // Pointer Test (No Array Accepted)
        if ((vtop->type.t & (VT_BTYPE | VT_ARRAY)) == VT_PTR)
        {
          if (vtop->r & VT_LVAL)
            gv(RC_INT);
          vtop->type = *pointed_type(&vtop->type);
          if ((vtop->type.t & VT_BTYPE) != VT_FUNC)
            goto error_func;
        }
        else
        {
error_func:
          cprime_error("function pointer expected in '%s' for '%s' (type %04x)",
                       funcname ? funcname : "<global>",
                       vtop->sym ? get_tok_str(vtop->sym->v, NULL) : "<expression>",
                       vtop->type.t);
        }
      }
      else
      {
        vtop->r &= ~VT_LVAL; // No Lvalue
      }
      // Get Return Type
      s = vtop->type.ref;
      if (saved_call_arg_count < 0)
        next();
      sa = s->next; // First Parameter
      nb_args = regsize = 0;
      ret.r2 = VT_CONST;
      // compute first implicit argument if a structure is returned
      if ((s->type.t & VT_BTYPE) == VT_STRUCT)
      {
        variadic = (s->f.func_type == FUNC_ELLIPSIS);
        ret_nregs = gfunc_sret(&s->type, variadic, &ret.type,
                               &ret_align, &regsize);
        if (ret_nregs <= 0)
        {
          // Get Some Space For The Returned Structure
          size = type_size(&s->type, &align);
#ifdef CPRIME_TARGET_ARM64
          /* On arm64, a small struct is return in registers.
             It is much easier to write it to memory if we know
             that we are allowed to write some extra bytes, so
             round the allocated space up to a power of 2: */
          if (size < 16)
            while (size & (size - 1))
              size = (size | (size - 1)) + 1;
#endif
          loc = (loc - size) & -align;
          ret.type = s->type;
          ret.r = VT_LOCAL | VT_LVAL;
          /* pass it as 'int' to avoid structure arg passing
             problems */
          vseti(VT_LOCAL, loc);
#ifdef CONFIG_CPRIME_BCHECK
          if (cprime_state->do_bounds_check)
            --loc;
#endif
          ret.c = vtop->c;
          if (ret_nregs < 0)
            vtop--;
          else
            nb_args++;
        }
      }
      else
      {
        ret_nregs = 1;
        ret.type = s->type;
      }

      if (ret_nregs > 0)
      {
        // Return In Register
        ret.c.i = 0;
        PUT_R_RET(&ret, ret.type.t);
      }

      p = NULL;
      if (saved_call_arg_count >= 0)
      {
        for (ai = 0; ai < saved_call_arg_count; ++ai)
        {
          emit_saved_arg_for_param(call_args[ai], sa);
          gfunc_param_typed_with_conversions(s, sa);
          nb_args++;
          if (sa)
            sa = sa->next;
        }
      }
      else if (tok != ')')
      {
        r = cprime_state->reverse_funcargs;
        for (;;)
        {
          if (r)
          {
            skip_or_save_block(&p2);
            p2->prev = p, p = p2;
          }
          else
          {
            expr_eq();
            gfunc_param_typed(s, sa);
          }
          nb_args++;
          if (sa)
            sa = sa->next;
          if (tok == ')')
            break;
          skip(',');
        }
      }
      while (sa)
      {
        emit_default_arg(s, sa);
        nb_args++;
        sa = sa->next;
      }

      if (p)   // With Reverse_Funcargs
      {
        for (n = 0; p; p = p2, ++n)
        {
          p2 = p, sa = s;
          do
          {
            sa = sa->next, p2 = p2->prev;
          }
          while (p2 && sa);
          p2 = p->prev;
          begin_macro(p, 1), next();
          expr_eq();
          gfunc_param_typed(s, sa);
          end_macro();
        }
        vrev(n);
      }

      next();
      vcheck_cmp(); // the generators don't like VT_CMP on vtop
      gfunc_call(nb_args);

      if (ret_nregs < 0)
      {
        vsetc(&ret.type, ret.r, &ret.c);
#ifdef CPRIME_TARGET_RISCV64
        arch_transfer_ret_regs(1);
#endif
      }
      else
      {
        // Return Value
        n = ret_nregs;
        while (n > 1)
        {
          int rc = reg_classes[ret.r] & ~(RC_INT | RC_FLOAT);
          /* We assume that when a structure is returned in multiple
             registers, their classes are consecutive values of the
             suite s(n) = 2^n */
          rc <<= --n;
          for (r = 0; r < NB_REGS; ++r)
            if (reg_classes[r] & rc)
              break;
          vsetc(&ret.type, r, &ret.c);
        }
        vsetc(&ret.type, ret.r, &ret.c);
        vtop->r2 = ret.r2;

        // Handle Packed Struct Return
        if (((s->type.t & VT_BTYPE) == VT_STRUCT) && ret_nregs)
        {
          int addr, offset;

          size = type_size(&s->type, &align);
          /* We're writing whole regs often, make sure there's enough
             space.  Assume register size is power of 2.  */
          size = (size + regsize - 1) & -regsize;
          if (ret_align > align)
            align = ret_align;
          loc = (loc - size) & -align;
          addr = loc;
          offset = 0;
          for (;;)
          {
            vset(&ret.type, VT_LOCAL | VT_LVAL, addr + offset);
            vswap();
            cpc_vstore_context = "func_packed_struct_ret";
            vstore();
            cpc_vstore_context = NULL;
            vtop--;
            if (--ret_nregs == 0)
              break;
            offset += regsize;
          }
          vset(&s->type, VT_LOCAL | VT_LVAL, addr);
        }

        /* Promote char/short return values. This is matters only
           for calling function that were not compiled by CPRIME and
           only on some architectures.  For those where it doesn't
           matter we expect things to be already promoted to int,
           but not larger.  */
        t = s->type.t &VT_BTYPE;
        if (t == VT_BYTE || t == VT_SHORT || t == VT_BOOL)
        {
#ifdef PROMOTE_RET
          vtop->r |= BFVAL(VT_MUSTCAST, 1);
#else
          vtop->type.t = VT_INT;
#endif
        }
      }
      if (s->f.func_noreturn)
      {
        if (debug_modes)
          cprime_tcov_block_end(cprime_state, -1);
        CODE_OFF();
      }
      maybe_indir_reference();
    }
    else
      break;
  }
}

#ifndef precedence_parser // Original Top-Down Parser 

static void expr_prod(void)
{
  int t;

  unary();
  while ((t = tok) == '*' || t == '/' || t == '%')
  {
    next();
    unary();
    gen_op(t);
  }
}

static void expr_sum(void)
{
  int t;

  expr_prod();
  while ((t = tok) == '+' || t == '-')
  {
    next();
    expr_prod();
    gen_op(t);
  }
}

static void expr_shift(void)
{
  int t;

  expr_sum();
  while ((t = tok) == TOK_SHL || t == TOK_SAR)
  {
    next();
    expr_sum();
    gen_op(t);
  }
}

static void expr_cmp(void)
{
  int t;

  expr_shift();
  while (((t = tok) >= TOK_ULE && t <= TOK_GT) ||
         t == TOK_ULT || t == TOK_UGE)
  {
    next();
    expr_shift();
    gen_op(t);
  }
}

static void expr_cmpeq(void)
{
  int t;

  expr_cmp();
  while ((t = tok) == TOK_EQ || t == TOK_NE)
  {
    next();
    expr_cmp();
    gen_op(t);
  }
}

static void expr_and(void)
{
  expr_cmpeq();
  while (tok == '&')
  {
    next();
    expr_cmpeq();
    gen_op('&');
  }
}

static void expr_xor(void)
{
  expr_and();
  while (tok == '^')
  {
    next();
    expr_and();
    gen_op('^');
  }
}

static void expr_or(void)
{
  expr_xor();
  while (tok == '|')
  {
    next();
    expr_xor();
    gen_op('|');
  }
}

static void expr_landor(int op);

static void expr_land(void)
{
  expr_or();
  if (tok == TOK_LAND)
    expr_landor(tok);
}

static void expr_lor(void)
{
  expr_land();
  if (tok == TOK_LOR)
    expr_landor(tok);
}

# define expr_landor_next(op) op == TOK_LAND ? expr_or() : expr_land()
#else // Defined Precedence_Parser 
# define expr_landor_next(op) unary(), expr_infix(precedence(op) + 1)
# define expr_lor() unary(), expr_infix(1)

static int precedence(int tok)
{
  switch (tok)
  {
  case TOK_LOR: return 1;
  case TOK_LAND: return 2;
  case '|': return 3;
  case '^': return 4;
  case '&': return 5;
  case TOK_EQ: case TOK_NE: return 6;
relat: case TOK_ULT: case TOK_UGE: return 7;
  case TOK_SHL: case TOK_SAR: return 8;
  case '+': case '-': return 9;
  case '*': case '/': case '%': return 10;
  default:
    if (tok >= TOK_ULE && tok <= TOK_GT)
      goto relat;
    return 0;
  }
}
static unsigned char prec[256];
static void init_prec(void)
{
  int i;
  for (i = 0; i < 256; i++)
    prec[i] = precedence(i);
}
#define precedence(i) ((unsigned)i < 256 ? prec[i] : 0)

static void expr_landor(int op);

static void expr_infix(int p)
{
  int t = tok, p2;
  while ((p2 = precedence(t)) >= p)
  {
    if (t == TOK_LOR || t == TOK_LAND)
      expr_landor(t);
    else
    {
      next();
      unary();
      if (precedence(tok) > p2)
        expr_infix(p2 + 1);
      gen_op(t);
    }
    t = tok;
  }
}
#endif

/* Assuming vtop is a value used in a conditional context
   (i.e. compared with zero) return 0 if it's false, 1 if
   true and -1 if it can't be statically determined.  */
static int condition_3way(void)
{
  int c = -1;
  try_call_cpp_bool_conversion_operator();
  if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
      (!(vtop->r & VT_SYM) || !vtop->sym->a.weak))
  {
    vdup();
    gen_cast_s(VT_BOOL);
    c = vtop->c.i;
    vpop();
  }
  return c;
}

static void expr_landor(int op)
{
  int t = 0, cc = 1, f = 0, i = op == TOK_LAND, c;
  for (;;)
  {
    c = f ? i : condition_3way();
    if (c < 0)
      save_regs(1), cc = 0;
    else if (c != i)
      nocode_wanted++, f = 1;
    if (tok != op)
      break;
    if (c < 0)
      t = gvtst(i, t);
    else
      vpop();
    next();
    expr_landor_next(op);
  }
  if (cc || f)
  {
    vpop();
    vpushi(i ^f);
    gsym(t);
    nocode_wanted -= f;
  }
  else
    gvtst_set(i, t);
}

static int is_cond_bool(SValue *sv)
{
  if ((sv->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
      && (sv->type.t & VT_BTYPE) == VT_INT)
    return (unsigned)sv->c.i < 2;
  if (sv->r == VT_CMP)
    return 1;
  return 0;
}

static void expr_cond(void)
{
  int tt, u, r1, r2, rc, t1, t2, islv, c, g;
  SValue sv;
  CType type;

  expr_lor();
  if (tok == '?')
  {
    next();
    c = condition_3way();
    g = (tok == ':' && non_iso);
    tt = 0;
    if (!g)
    {
      if (c < 0)
      {
        save_regs(1);
        tt = gvtst(1, 0);
      }
      else
        vpop();
    }
    else if (c < 0)
    {
      /* needed to avoid having different registers saved in
         each branch */
      save_regs(1);
      gv_dup();
      tt = gvtst(0, 0);
    }

    if (c == 0)
      nocode_wanted++;
    if (!g)
      gexpr();

    if ((vtop->type.t & VT_BTYPE) == VT_FUNC)
      mk_pointer(&vtop->type);
    sv = *vtop; // Save Value To Handle It Later
    vtop--; // no vpop so that FP stack is not flushed

    if (g)
      u = tt;
    else if (c < 0)
    {
      u = gjmp(0);
      gsym(tt);
    }
    else
      u = 0;

    if (c == 0)
      nocode_wanted--;
    if (c == 1)
      nocode_wanted++;
    skip(':');
    expr_cond();

    if ((vtop->type.t & VT_BTYPE) == VT_FUNC)
      mk_pointer(&vtop->type);

    // cast operands to correct type according to ISOC rules
    if (!combine_types(&type, &sv, vtop, '?'))
      type_incompatibility_error(&sv.type, &vtop->type,
                                 "type mismatch in conditional expression (have '%s' and '%s')");

    if (c < 0 && is_cond_bool(vtop) && is_cond_bool(&sv))
    {
      /* optimize "if (f ? a > b : c || d) ..." for example, where normally
         "a < b" and "c || d" would be forced to "(int)0/1" first, whereas
         this code jumps directly to the if's then/else branches. */
      t1 = gvtst(0, 0);
      t2 = gjmp(0);
      gsym(u);
      vpushv(&sv);
      // combine jump targets of 2nd op with VT_CMP of 1st op
      gvtst_set(0, t1);
      gvtst_set(1, t2);
      gen_cast(&type);
      //  cprime_warning("two conditions expr_cond");
      return;
    }

    /* keep structs lvalue by transforming `(expr ? a : b)` to `*(expr ? &a : &b)` so
       that `(expr ? a : b).mem` does not error  with "lvalue expected" */
    islv = (vtop->r &VT_LVAL) && (sv.r &VT_LVAL) && VT_STRUCT == (type.t &VT_BTYPE);

    // Now We Convert Second Operand
    if (c != 1)
    {
      gen_cast(&type);
      if (islv)
      {
        mk_pointer(&vtop->type);
        gaddrof();
      }
      else if (VT_STRUCT == (vtop->type.t & VT_BTYPE))
        gaddrof();
    }

    rc = RC_TYPE(type.t);
    /* for long longs, we use fixed registers to avoid having
       to handle a complicated move */
    if (USING_TWO_WORDS(type.t))
      rc = RC_RET(type.t);

    tt = r2 = 0;
    if (c < 0)
    {
      r2 = gv(rc);
      tt = gjmp(0);
    }
    gsym(u);
    if (c == 1)
      nocode_wanted--;

    /* this is horrible, but we must also convert first
       operand */
    if (c != 0)
    {
      *vtop = sv;
      gen_cast(&type);
      if (islv)
      {
        mk_pointer(&vtop->type);
        gaddrof();
      }
      else if (VT_STRUCT == (vtop->type.t & VT_BTYPE))
        gaddrof();
    }

    if (c < 0)
    {
      r1 = gv(rc);
      move_reg(r2, r1, islv ? VT_PTR : type.t);
      vtop->r = r2;
      gsym(tt);
    }

    if (islv)
      indir();
  }
}

typedef struct AssignmentLvalueGuard
{
  SValue *value;
  struct AssignmentLvalueGuard *prev;
} AssignmentLvalueGuard;

static AssignmentLvalueGuard *active_assignment_lvalues;

static int is_active_assignment_lvalue(SValue *value)
{
  AssignmentLvalueGuard *guard;
  for (guard = active_assignment_lvalues; guard; guard = guard->prev)
    if (guard->value == value)
      return 1;
  return 0;
}

static void expr_eq(void)
{
  int t;

  expr_cond();
  if ((t = tok) == '=' || TOK_ASSIGN(t))
  {
    AssignmentLvalueGuard assignment_guard;
    test_lvalue();
    assignment_guard.value = vtop;
    assignment_guard.prev = active_assignment_lvalues;
    active_assignment_lvalues = &assignment_guard;
    next();
    if (t == '=')
    {
      if (tok == '{' && ((vtop->type.t & VT_BTYPE) == VT_STRUCT))
      {
        CType rhs_type = vtop->type;
        rhs_type.t &= ~VT_LVAL;
        materialize_braced_temporary(&rhs_type);
        if (try_call_cpp_assignment_operator())
        {
          active_assignment_lvalues = assignment_guard.prev;
          return;
        }
        vstore();
        active_assignment_lvalues = assignment_guard.prev;
        return;
      }
      expr_eq();
      if (try_call_cpp_assignment_operator())
      {
        active_assignment_lvalues = assignment_guard.prev;
        return;
      }
    }
    else
    {
      vdup();
      expr_eq();
      if (try_call_cpp_compound_assign_operator(t))
      {
        vswap();
        vpop();
        active_assignment_lvalues = assignment_guard.prev;
        return;
      }
      gen_op(TOK_ASSIGN_OP(t));
    }
    vstore();
    active_assignment_lvalues = assignment_guard.prev;
  }
}

ST_FUNC void gexpr(void)
{
  expr_eq();
  if (tok == ',')
  {
    do
    {
      vpop();
      next();
      expr_eq();
    }
    while (tok == ',');

    // Convert Array & Function To Pointer
    convert_parameter_type(&vtop->type);

    // Make Builtin_Constant_P((1,2)) Return 0 (Like On Gcc)
    if ((vtop->r & VT_VALMASK) == VT_CONST && nocode_wanted && !CONST_WANTED)
      gv(RC_TYPE(vtop->type.t));
  }
}

// Parse A Constant Expression And Return Value In Vtop.
static void expr_const1(void)
{
  nocode_wanted += CONST_WANTED_BIT;
  expr_cond();
  nocode_wanted -= CONST_WANTED_BIT;
}

// Parse An Integer Constant And Return Its Value.
static inline int64_t expr_const64(void)
{
  int64_t c;
  expr_const1();
  if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
    expect("constant expression");
  c = vtop->c.i;
  vpop();
  return c;
}

/* parse an integer constant and return its value.
   Complain if it doesn't fit 32bit (signed or unsigned).  */
ST_FUNC int expr_const(void)
{
  int c;
  int64_t wc = expr_const64();
  c = wc;
  if (c != wc && (unsigned)c != wc)
    cprime_error("constant exceeds 32 bit");
  return c;
}

// -------------------------------------------------------------------------
// Return From Function

#ifndef CPRIME_TARGET_ARM64
static void gfunc_return(CType *func_type);

static Sym *resolve_copy_constructor_func(CType *type, CType *source_type)
{
  CType arg_type;
  int struct_tok;
  int template_ctor_tok;
  MemberFuncOverload *o;
  Sym *best = NULL, *resolved;
  int best_rank = 0x7fffffff;

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref || !type->ref->a.lifecycle_ctor)
    return NULL;

  arg_type = *source_type;
  struct_tok = get_struct_type_name_tok(type);
  /* Materialize any template-specialized constructor bodies before choosing
     between the copy and move self-type overloads below. */
  template_ctor_tok = instantiate_template_self_constructor_body(
    type, (arg_type.t & VT_RVALUE_REFERENCE) != 0);
  if (template_ctor_tok)
  {
    best = global_symbol_find(template_ctor_tok);
    if (!best)
      best = sym_find(template_ctor_tok);
    if (best)
      best_rank = 0;
  }
  resolved = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                              &arg_type, 1);
  if (!(arg_type.t & VT_RVALUE_REFERENCE) && resolved && resolved->type.ref)
  {
    Sym *param = resolved->type.ref->next;
    CType param_value, source_value;
    if (param)
      param = param->next; /* implicit this */
    if (param && !param->next)
    {
      param_value = param->type;
      param_value.t &= ~VT_RVALUE_REFERENCE;
      if (is_reference_type(&param_value))
        param_value = *pointed_type(&param_value);
      source_value = arg_type;
      source_value.t &= ~VT_RVALUE_REFERENCE;
      if (is_reference_type(&source_value))
        source_value = *pointed_type(&source_value);
      if ((param_value.t & VT_BTYPE) == VT_STRUCT
          && (source_value.t & VT_BTYPE) == VT_STRUCT
          && get_struct_type_name_tok(&param_value) == struct_tok
          && get_struct_type_name_tok(&source_value) == struct_tok)
      {
        best = resolved;
        best_rank = 0;
      }
    }
  }
  for (o = member_func_candidates(struct_tok, TOK_CONSTRUCTOR1);
       o; o = o->bucket_next)
  {
    Sym *candidate;
    Sym *param;
    CType param_value, source_value;
    int rank;
    if (o->struct_tok != struct_tok || o->method_tok != TOK_CONSTRUCTOR1
        || o->explicit_arg_count != 1)
      continue;
    candidate = sym_find(o->mangled_tok);
    if (!candidate)
      candidate = global_symbol_find(o->mangled_tok);
    if (!candidate || !o->func_type.ref)
      continue;
    param = o->func_type.ref->next;
    if (param)
      param = param->next; /* implicit this */
    if (!param || param->next)
      continue;
    param_value = param->type;
    param_value.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&param_value))
      param_value = *pointed_type(&param_value);
    source_value = arg_type;
    source_value.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&source_value))
      source_value = *pointed_type(&source_value);
    if ((param_value.t & VT_BTYPE) != VT_STRUCT
        || (source_value.t & VT_BTYPE) != VT_STRUCT
        || get_struct_type_name_tok(&param_value) != struct_tok
        || get_struct_type_name_tok(&source_value) != struct_tok)
      continue;
    rank = call_arg_match_rank(&param->type, &arg_type);
    if (rank < 0 || rank >= best_rank)
      continue;
    best = candidate;
    best_rank = rank;
  }
  if (!best || is_defaulted_lifecycle_constructor(type, best))
    return NULL;
  instantiate_template_member_body_for_func_tok(type, TOK_CONSTRUCTOR1,
                                                best->v);
  return best;
}

static int try_gfunc_return_copy_construct(CType *func_type)
{
  SValue source;
  CType source_value_type;
  CType ptr_type;
  Sym *ctor_func, *ctor_type, *sa;
  int ret_align, ret_nregs, regsize, size, align, addr, r2;

  if ((func_type->t & VT_BTYPE) != VT_STRUCT)
    return 0;

  source_value_type = vtop->type;
  source_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&source_value_type))
    source_value_type = *pointed_type(&source_value_type);
  ctor_func = resolve_copy_constructor_func(func_type, &vtop->type);
  if (!ctor_func
      && !((source_value_type.t & VT_BTYPE) == VT_STRUCT
           && get_struct_type_name_tok(func_type)
                == get_struct_type_name_tok(&source_value_type)))
    ctor_func = resolve_member_func_by_arg_types(func_type, TOK_CONSTRUCTOR1,
                                                 &vtop->type, 1);
  if (!ctor_func && !struct_needs_memberwise_copy(func_type))
    return 0;
  if (ctor_func
      && ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref))
    cprime_error("copy constructor target is not declared as function");

  ret_nregs = gfunc_sret(func_type, func_var, &ptr_type, &ret_align, &regsize);
  source = *vtop;
  vtop--;

  /* A returned member expression can hold its source address in a volatile
     register (for example `return m_fullPath`).  Keep it on the value stack
     while destination members are constructed so the normal call register
     saving materializes the address before any constructor can clobber it. */
  vpushv(&source);

  if (ret_nregs == 0)
  {
    SValue return_ptr;
    ptr_type = *func_type;
    mk_pointer(&ptr_type);
    vset(&ptr_type, VT_LOCAL | VT_LVAL, func_vc);
    return_ptr = *vtop;
    vtop--;
    call_lifecycle_constructor_members_base_ptr(func_type, &return_ptr, 0);
    vset(&ptr_type, VT_LOCAL | VT_LVAL, func_vc);
  }
  else
  {
    size = type_size(func_type, &align);
    addr = get_temp_local_var(size, align, &r2);
    vset(func_type, VT_LOCAL | VT_LVAL, addr);
    vtop->r2 = r2;
    call_lifecycle_constructor_members(func_type, VT_LOCAL | VT_LVAL, addr);
    mk_pointer(&vtop->type);
    gaddrof();
  }

  if (ctor_func)
  {
    SValue dst_ptr = *vtop;
    vtop--;
    source = *vtop;
    vtop--;
    vpushsym(&ctor_func->type, ctor_func);
    vpushv(&dst_ptr);
    ctor_type = ctor_func->type.ref;
    sa = ctor_type->next;
    gfunc_param_typed(ctor_type, sa);
    if (sa)
      sa = sa->next;

    vpushv(&source);
    gfunc_param_typed(ctor_type, sa);
    if (sa)
      sa = sa->next;
    if (sa)
      cprime_error("too few arguments to copy constructor");

    vcheck_cmp();
    gfunc_call(2);
  }
  else
  {
    SValue dst_ptr = *vtop;
    SValue src_ptr;
    vtop--;
    source = *vtop;
    vtop--;
    vpushv(&source);
    mk_pointer(&vtop->type);
    gaddrof();
    src_ptr = *vtop;
    vtop--;
    copy_construct_struct_memberwise_from_base_ptr(
      func_type, &dst_ptr, &src_ptr, 0,
      (source.type.t & VT_RVALUE_REFERENCE) != 0);
  }

  if (ret_nregs != 0)
  {
    vset(func_type, VT_LOCAL | VT_LVAL, addr);
    vtop->r2 = r2;
    gfunc_return(func_type);
  }

  return 1;
}

static int virtual_vptr_field_tok(int root_tok)
{
  char name[512];
  snprintf(name, sizeof(name), "__cprime_vptr_%s",
           get_tok_str(root_tok, NULL));
  return tok_alloc_const(name);
}

static int virtual_method_signatures_match(CType *left, CType *right)
{
  Sym *left_arg, *right_arg;

  if (!left || !right
      || (left->t & VT_BTYPE) != VT_FUNC
      || (right->t & VT_BTYPE) != VT_FUNC
      || !left->ref || !right->ref
      || (left->t & (VT_CONSTANT | VT_VOLATILE))
           != (right->t & (VT_CONSTANT | VT_VOLATILE)))
    return 0;
  left_arg = left->ref->next;
  right_arg = right->ref->next;
  while (left_arg && right_arg)
  {
    if (!is_compatible_types(&left_arg->type, &right_arg->type))
      return 0;
    left_arg = left_arg->next;
    right_arg = right_arg->next;
  }
  return !left_arg && !right_arg;
}

static VirtualMethodInfo *find_virtual_method_for_class(int class_tok,
                                                         int method_tok,
                                                         CType *func_type)
{
  VirtualMethodInfo *vm;
  ClassBaseInfo *base;

  for (vm = virtual_method_infos; vm; vm = vm->next)
    if (vm->class_tok == class_tok && vm->method_tok == method_tok
        && virtual_method_signatures_match(&vm->func_type, func_type))
      return vm;
  for (base = class_base_infos; base; base = base->next)
    if (base->class_tok == class_tok)
    {
      vm = find_virtual_method_for_class(base->base_tok, method_tok,
                                         func_type);
      if (vm)
        return vm;
    }
  return NULL;
}

static VirtualMethodInfo *find_virtual_method_for_symbol(int class_tok,
                                                         int mangled_tok)
{
  VirtualMethodInfo *vm;
  ClassBaseInfo *base;

  for (vm = virtual_method_infos; vm; vm = vm->next)
    if (vm->class_tok == class_tok && vm->mangled_tok == mangled_tok)
      return vm;
  for (base = class_base_infos; base; base = base->next)
    if (base->class_tok == class_tok)
    {
      vm = find_virtual_method_for_symbol(base->base_tok, mangled_tok);
      if (vm)
        return vm;
    }
  return NULL;
}

static int member_overrides_virtual_method(CType *class_type, int method_tok,
                                           CType *func_type)
{
  ClassBaseInfo *base;
  int class_tok;

  class_tok = get_struct_type_name_tok(class_type);
  if (!class_tok || !func_type || (func_type->t & VT_BTYPE) != VT_FUNC
      || !func_type->ref)
    return 0;
  for (base = class_base_infos; base; base = base->next)
    if (base->class_tok == class_tok
        && find_virtual_method_for_class(base->base_tok, method_tok,
                                         func_type))
      return 1;
  return 0;
}

static int virtual_root_slot_count(int root_tok)
{
  VirtualMethodInfo *vm;
  int count = 0;
  for (vm = virtual_method_infos; vm; vm = vm->next)
    if (vm->root_tok == root_tok && vm->slot >= count)
      count = vm->slot + 1;
  return count;
}

static void note_virtual_method(CType *class_type, int method_tok,
                                CType *func_type, int is_pure)
{
  VirtualMethodInfo *base_vm, *vm;
  int class_tok, arg_count = 0, mangled_tok;
  Sym *arg;

  class_tok = get_struct_type_name_tok(class_type);
  if (!class_tok || !func_type || (func_type->t & VT_BTYPE) != VT_FUNC
      || !func_type->ref)
    return;
  for (arg = func_type->ref->next; arg; arg = arg->next)
    ++arg_count;
  for (vm = virtual_method_infos; vm; vm = vm->next)
    if (vm->class_tok == class_tok && vm->method_tok == method_tok
        && virtual_method_signatures_match(&vm->func_type, func_type))
      return;

  base_vm = NULL;
  {
    ClassBaseInfo *base;
    for (base = class_base_infos; base && !base_vm; base = base->next)
      if (base->class_tok == class_tok)
        base_vm = find_virtual_method_for_class(base->base_tok, method_tok,
                                                func_type);
  }
  mangled_tok = make_member_func_tok_for_type(class_tok, method_tok,
                                               func_type);
  vm = cprime_mallocz(sizeof(*vm));
  vm->class_tok = class_tok;
  vm->method_tok = method_tok;
  vm->mangled_tok = mangled_tok;
  vm->explicit_arg_count = arg_count;
  vm->func_type = *func_type;
  vm->is_pure = is_pure != 0;
  if (base_vm)
  {
    vm->root_tok = base_vm->root_tok;
    vm->slot = base_vm->slot;
  }
  else
  {
    vm->root_tok = class_tok;
    vm->slot = virtual_root_slot_count(class_tok);
  }
  vm->next = virtual_method_infos;
  virtual_method_infos = vm;
}

static int virtual_vptr_offset(int class_tok, int root_tok)
{
  CType root_type;
  int subobject_offset, field_offset;

  if (!class_subobject_offset(class_tok, root_tok, &subobject_offset)
      || !make_class_type_from_tok(&root_type, root_tok)
      || !find_field_try(&root_type, virtual_vptr_field_tok(root_tok),
                         &field_offset))
    return -1;
  return subobject_offset + field_offset;
}

static VirtualMethodInfo *find_virtual_impl(int class_tok, int root_tok,
                                            int slot)
{
  VirtualMethodInfo *vm;
  ClassBaseInfo *base;

  for (vm = virtual_method_infos; vm; vm = vm->next)
    if (vm->class_tok == class_tok && vm->root_tok == root_tok
        && vm->slot == slot)
      return vm;
  for (base = class_base_infos; base; base = base->next)
    if (base->class_tok == class_tok)
    {
      vm = find_virtual_impl(base->base_tok, root_tok, slot);
      if (vm)
        return vm;
    }
  return NULL;
}

static VirtualTableInfo *find_virtual_table(int class_tok, int root_tok)
{
  VirtualTableInfo *vt;
  for (vt = virtual_table_infos; vt; vt = vt->next)
    if (vt->class_tok == class_tok && vt->root_tok == root_tok)
      return vt;
  return NULL;
}

static void emit_virtual_tables_for_class(int class_tok)
{
  VirtualMethodInfo *root_vm;

  for (root_vm = virtual_method_infos; root_vm; root_vm = root_vm->next)
  {
    VirtualTableInfo *vt;
    CType table_type;
    Sym *table_sym;
    int root_tok = root_vm->root_tok;
    int slots, slot, table_tok;
    unsigned long offset;
    char name[768];

    if (root_vm->class_tok != root_tok)
      continue;
    if (root_tok != class_tok && !class_has_base(class_tok, root_tok))
      continue;
    if (find_virtual_table(class_tok, root_tok))
      continue;
    slots = virtual_root_slot_count(root_tok);
    if (!slots || virtual_vptr_offset(class_tok, root_tok) < 0)
      continue;
    snprintf(name, sizeof(name), "__cprime_vtable_%s_as_%s",
             get_tok_str(class_tok, NULL), get_tok_str(root_tok, NULL));
    table_tok = tok_alloc_const(name);
    table_type = char_pointer_type;
    table_type.t |= VT_STATIC;
    table_sym = external_global_sym(table_tok, &table_type);
    table_sym->type.t = (table_sym->type.t & ~VT_EXTERN) | VT_STATIC;
    offset = section_add(data_section, slots * PTR_SIZE, PTR_SIZE);
    put_extern_sym(table_sym, data_section, offset, slots * PTR_SIZE);
    vt = cprime_mallocz(sizeof(*vt));
    vt->class_tok = class_tok;
    vt->root_tok = root_tok;
    vt->symbol_tok = table_tok;
    vt->sym = table_sym;
    vt->offset = offset;
    vt->next = virtual_table_infos;
    virtual_table_infos = vt;
  }
}

static void emit_virtual_table_relocations(void)
{
  VirtualTableInfo *vt;

  for (vt = virtual_table_infos; vt; vt = vt->next)
  {
    int slots = virtual_root_slot_count(vt->root_tok);
    int slot;
    if (!vt->referenced || vt->relocations_emitted)
      continue;
    for (slot = 0; slot < slots; ++slot)
    {
      VirtualMethodInfo *impl = find_virtual_impl(vt->class_tok,
                                                  vt->root_tok, slot);
      Sym *func_sym = impl ? global_symbol_find(impl->mangled_tok) : NULL;
      if (impl && impl->is_pure)
        continue;
      if (!func_sym && impl)
        func_sym = sym_find(impl->mangled_tok);
      if (func_sym)
        greloca(data_section, func_sym, vt->offset + slot * PTR_SIZE,
                R_DATA_PTR, 0);
    }
    vt->relocations_emitted = 1;
  }
}

static void initialize_virtual_tables_for_object(CType *type, int r,
                                                  int object_offset,
                                                  Sym *object_sym)
{
  VirtualTableInfo *vt;
  int class_tok = get_struct_type_name_tok(type);

  if (!class_tok || NODATA_WANTED)
    return;
  for (vt = virtual_table_infos; vt; vt = vt->next)
    if (vt->class_tok == class_tok)
    {
      int field_offset = virtual_vptr_offset(class_tok, vt->root_tok);
      if (field_offset < 0 || !vt->sym)
        continue;
      vt->referenced = 1;
      vset(type, r | VT_LVAL, object_offset);
      vtop->sym = object_sym;
      if (r & VT_SYM)
        vtop->c.i = 0;
      gaddrof();
      vtop->type = char_pointer_type;
      if (field_offset)
      {
        vpushi(field_offset);
        gen_op('+');
      }
      vtop->type = char_pointer_type;
      vtop->r |= VT_LVAL;
      vpushsym(&char_pointer_type, vt->sym);
      vstore();
      vpop();
    }
}

static void initialize_virtual_tables_for_pointer(CType *type,
                                                  SValue *base_ptr,
                                                  int object_offset)
{
  VirtualTableInfo *vt;
  SValue stable_base_ptr;
  int class_tok = get_struct_type_name_tok(type);

  if (!class_tok || !base_ptr || NODATA_WANTED)
    return;
  stable_base_ptr = *base_ptr;
  spill_pointer_svalue_to_local(&stable_base_ptr);
  for (vt = virtual_table_infos; vt; vt = vt->next)
    if (vt->class_tok == class_tok)
    {
      int field_offset = virtual_vptr_offset(class_tok, vt->root_tok);
      if (field_offset < 0 || !vt->sym)
        continue;
      vt->referenced = 1;
      vpushv(&stable_base_ptr);
      if (vtop->r & VT_LVAL)
        gv(RC_INT);
      vtop->type = char_pointer_type;
      if (object_offset + field_offset)
      {
        vpushi(object_offset + field_offset);
        gen_op('+');
      }
      vtop->type = char_pointer_type;
      vtop->r |= VT_LVAL;
      vpushsym(&char_pointer_type, vt->sym);
      vstore();
      vpop();
    }
}

static int ctype_from_mangled_scalar_template_arg(CType *type, const char *name);

static void gfunc_return(CType *func_type)
{
  CType scalar_template_ret;

  if ((func_type->t & VT_BTYPE) == VT_STRUCT
      && btype_is_arithmetic_scalar(vtop->type.t & VT_BTYPE)
      && ctype_from_mangled_scalar_template_arg(&scalar_template_ret,
                                                funcname))
    *func_type = scalar_template_ret;
  if (getenv("CPC_TRACE_RETURN"))
    fprintf(stderr, "CPC_RETURN func=%s ret_t=%04x ret_s=%s val_t=%04x val_s=%s val_r=%04x file=%s line=%d\n",
            funcname ? funcname : "<none>", func_type->t,
            get_tok_str(get_struct_type_name_tok(func_type), NULL),
            vtop->type.t, get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
            vtop->r, file ? file->filename : "<no file>",
            file ? file->line_num : 0);
  if ((func_type->t & VT_BTYPE) == VT_STRUCT)
  {
    CType type, ret_type;
    int ret_align, ret_nregs, regsize;
    ret_nregs = gfunc_sret(func_type, func_var, &ret_type,
                           &ret_align, &regsize);
    if (ret_nregs < 0)
    {
#ifdef CPRIME_TARGET_RISCV64
      arch_transfer_ret_regs(0);
#endif
    }
    else if (0 == ret_nregs)
    {
      /* if returning structure, must copy it to implicit
         first pointer arg location */
      type = *func_type;
      mk_pointer(&type);
      vset(&type, VT_LOCAL | VT_LVAL, func_vc);
      indir();
      vswap();
      // Copy Structure Value To Pointer
      cpc_vstore_context = "return_sret_copy";
      vstore();
      cpc_vstore_context = NULL;
    }
    else
    {
      // Returning Structure Packed Into Registers
      int size, addr, align, rc, n;
      size = type_size(func_type, &align);
      if (ret_nregs *regsize > size ||
          ((align & (ret_align - 1))
           && ((vtop->r & VT_VALMASK) < VT_CONST // Pointer To Struct
               || (vtop->c.i & (ret_align - 1))
              )))
      {
        if (ret_nregs *regsize > size)
          size = ret_nregs * regsize;
        if (ret_align > align)
          align = ret_align;
        loc = (loc - size) & -align;
        addr = loc;
        type = *func_type;
        if (getenv("CPC_TRACE_RETURN"))
          fprintf(stderr, "CPC_RETURN_PACKED_TEMP func=%s ret_t=%04x ret_s=%s val_t=%04x val_s=%s val_r=%04x\n",
                  funcname ? funcname : "<none>", func_type->t,
                  get_tok_str(get_struct_type_name_tok(func_type), NULL),
                  vtop->type.t,
                  get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
                  vtop->r);
        vset(&type, VT_LOCAL | VT_LVAL, addr);
        vswap();
        cpc_vstore_context = "return_packed_temp";
        vstore();
        cpc_vstore_context = NULL;
        vpop();
        vset(&ret_type, VT_LOCAL | VT_LVAL, addr);
      }
      vtop->type = ret_type;
      rc = RC_RET(ret_type.t);
      //printf("struct return: n:%d t:%02x rc:%02x\n", ret_nregs, ret_type.t, rc);
      for (n = ret_nregs; --n > 0;)
      {
        vdup();
        gv(rc);
        vswap();
        incr_offset(regsize);
        /* We assume that when a structure is returned in multiple
           registers, their classes are consecutive values of the
           suite s(n) = 2^n */
        rc <<= 1;
      }
      gv(rc);
      vtop -= ret_nregs - 1;
    }
  }
  else
    gv(RC_RET(func_type->t));
  vtop--; // NOT vpop() because on x86 it would flush the fp stack
}
#endif

static void check_func_return(void)
{
  if ((func_vt.t & VT_BTYPE) == VT_VOID)
    return;
  if (!strcmp (funcname, "main")
      && (func_vt.t & VT_BTYPE) == VT_INT)
  {
    // Main Returns 0 By Default
    vpushi(0);
    gen_assign_cast(&func_vt);
    gfunc_return(&func_vt);
  }
  else
    cprime_warning("function might return no value: '%s'", funcname);
}

static int ctype_from_mangled_scalar_template_arg(CType *type, const char *name)
{
  const char *p, *q;
  char arg[32];
  size_t n;

  if (!type || !name)
    return 0;
  p = strstr(name, "__");
  if (!p)
    return 0;
  p += 2;
  q = strstr(p, "__");
  if (!q)
    return 0;
  n = (size_t)(q - p);
  if (n == 0 || n >= sizeof(arg))
    return 0;
  memcpy(arg, p, n);
  arg[n] = '\0';
  type->ref = NULL;
  if (!strcmp(arg, "i32") || !strcmp(arg, "int"))
    type->t = VT_INT;
  else if (!strcmp(arg, "ui32"))
    type->t = VT_INT | VT_UNSIGNED;
  else if (!strcmp(arg, "i64"))
    type->t = VT_LLONG;
  else if (!strcmp(arg, "ui64"))
    type->t = VT_LLONG | VT_UNSIGNED;
  else if (!strcmp(arg, "float") || !strcmp(arg, "f32"))
    type->t = VT_FLOAT;
  else if (!strcmp(arg, "double") || !strcmp(arg, "f64"))
    type->t = VT_DOUBLE;
  else if (!strcmp(arg, "bool"))
    type->t = VT_BOOL;
  else
    return 0;
  return 1;
}

// -------------------------------------------------------------------------
// Switch/Case

static int case_cmp(uint64_t a, uint64_t b)
{
  if (cur_switch->sv.type.t & VT_UNSIGNED)
    return a < b ? -1 : a > b;
  else
    return (int64_t)a < (int64_t)b ? -1 : (int64_t)a > (int64_t)b;
}

static int case_cmp_qs(const void *pa, const void *pb)
{
  return case_cmp((*(struct case_t **)pa)->v1, (*(struct case_t **)pb)->v1);
}

static void case_sort(struct switch_t *sw)
{
  struct case_t **p;
  if (sw->n < 2)
    return;
  qsort(sw->p, sw->n, sizeof *sw->p, case_cmp_qs);
  p = sw->p;
  while (p < sw->p + sw->n - 1)
  {
    if (case_cmp(p[0]->v2, p[1]->v1) >= 0)
    {
      int l1 = p[0]->line, l2 = p[1]->line;
      // Using Special Format "%I:..." To Show Specific Line
      cprime_error("%i:duplicate case value", l1 > l2 ? l1 : l2);
    }
    else if (p[0]->v2 + 1 == p[1]->v1 && p[0]->ind == p[1]->ind)
    {
      // Treat "Case 1: Case 2: Case 3:" Like "Case 1 ... 3:
      p[1]->v1 = p[0]->v1;
      cprime_free(p[0]);
      memmove(p, p + 1, (--sw->n - (p - sw->p)) * sizeof *p);
    }
    else
      ++p;
  }
}

static void push_virtual_call_target(int receiver_class_tok,
                                     VirtualMethodInfo *vm, Sym *func_sym)
{
  CType func_ptr_type;
  int field_offset;

  if (!vm || !func_sym)
    cprime_internal_error("virtual call target is incomplete");
  field_offset = virtual_vptr_offset(receiver_class_tok, vm->root_tok);
  if (field_offset < 0)
    cprime_error("virtual base '%s' is not a subobject of '%s'",
                 get_tok_str(vm->root_tok, NULL),
                 get_tok_str(receiver_class_tok, NULL));

  /* Computing a nonzero vptr offset may update a register in place.  Preserve
     the unadjusted hidden receiver through the normal temporary allocator
     before duplicating it; a raw stack slot can collide with call-target
     spills when a statement performs consecutive virtual calls. */
  save_regs(0);

  /* Keep the receiver below the indirect target, matching the ordinary
     member-call stack shape. */
  vdup();
  vtop->type = char_pointer_type;
  if (vtop->r & VT_LVAL)
    gv(RC_INT);
  if (field_offset)
  {
    vpushi(field_offset);
    gen_op('+');
  }
  /* The hidden field contains the address of the first function slot. */
  vtop->type = char_pointer_type;
  vtop->r |= VT_LVAL;
  gv(RC_INT);
  vtop->type = char_pointer_type;
  if (vm->slot)
  {
    vpushi(vm->slot * PTR_SIZE);
    gen_op('+');
  }
  func_ptr_type = func_sym->type;
  mk_pointer(&func_ptr_type);
  vtop->type = func_ptr_type;
  vtop->r |= VT_LVAL;
}

static int gcase(struct case_t **base, int len, int dsym)
{
  struct case_t *p;
  int t, l2, e;

  t = vtop->type.t &VT_BTYPE;
  if (t != VT_LLONG)
    t = VT_INT;
  while (len)
  {
    // binary search while len > 8, else linear
    l2 = len > 8 ? len / 2 : 0;
    p = base[l2];
    vdup(), vpush64(t, p->v2);
    if (l2 == 0 && p->v1 == p->v2)
    {
      gen_op(TOK_EQ); // Jmp To Case When Equal
      gsym_addr(gvtst(0, 0), p->ind);
    }
    else
    {
      // Case V1 ... V2
      gen_op(TOK_GT); // jmp over when > V2
      if (len == 1) // Last Case Test Jumps To Default When False
        dsym = gvtst(0, dsym), e = 0;
      else
        e = gvtst(0, 0);
      vdup(), vpush64(t, p->v1);
      gen_op(TOK_GE); // jmp to case when >= V1
      gsym_addr(gvtst(0, 0), p->ind);
      dsym = gcase(base, l2, dsym);
      gsym(e);
    }
    ++l2, base += l2, len -= l2;
  }
  // Jump Automagically Will Suppress More Jumps
  return gjmp(dsym);
}

static void end_switch(void)
{
  struct switch_t *sw = cur_switch;
  dynarray_reset(&sw->p, &sw->n);
  cur_switch = sw->prev;
  cprime_free(sw);
}

// -------------------------------------------------------------------------
// __Attribute__((Cleanup(Fn)))

// Protect Symbol Lvalues From Further Modification
static void save_lvalues(void)
{
  SValue *sv = vtop;
  while (sv >= vstack)
  {
    if (sv->sym && (sv->r & VT_LVAL)
        && !is_active_assignment_lvalue(sv)
        && ((sv->type.t & VT_BTYPE) != VT_STRUCT))
    {
      int align, size = type_size(&sv->type, &align);
      int r2, l = get_temp_local_var(size, align, &r2);
      vset(&sv->type, VT_LOCAL | VT_LVAL, l), vtop->r2 = r2;
      vpushv(sv), *sv = vtop[-1], cpc_vstore_context = "save_lvalues",
        vstore(), cpc_vstore_context = NULL, --vtop;
    }
    --sv;
  }
}

#include "cprimegen_statements.inc"

#include "cprimegen_initializers.inc"

static void gen_function(Sym *sym)
{
  struct scope f = { 0 };

  cur_scope = root_scope = &f;
  nocode_wanted = 0;

  ind = cur_text_section->data_offset;
  if (sym->a.aligned)
  {
    size_t newoff = section_add(cur_text_section, 0,
                                1 << (sym->a.aligned - 1));
    gen_fill_nops(newoff - ind);
  }

  funcname = get_tok_str(sym->v, NULL);
  func_ind = ind;
  func_vt = sym->type.ref->type;
  /* A prior auto-return declaration may leave the merged definition with an
     extra function-type layer. C and C++ cannot return a function by value;
     the nested function's return type is the deduced result. */
  while ((func_vt.t & VT_BTYPE) == VT_FUNC && func_vt.ref)
    func_vt = func_vt.ref->type;
  func_var = sym->type.ref->f.func_type == FUNC_ELLIPSIS;

  // NOTE: we patch the symbol size later
  put_extern_sym(sym, cur_text_section, ind, 0);
  x86_64_asm_func_begin(sym);

  if (sym->type.ref->f.func_ctor)
    add_array (cprime_state, ".init_array", sym->c);
  if (sym->type.ref->f.func_dtor)
    add_array (cprime_state, ".fini_array", sym->c);

  // Put Debug Symbol
  cprime_debug_funcstart(cprime_state, sym);

  // Push A Dummy Symbol To Enable Local Sym Storage
  sym_push2(&local_stack, SYM_FIELD, 0, 0);
  // Push Parameters
  local_scope = 1;
  sym_push_params(sym->type.ref);

  local_scope = 0;
  rsym = 0;
  nb_temp_local_vars = 0;

  gfunc_prolog(sym);
  cprime_debug_prolog_epilog(cprime_state, 0);
  func_vla_arg(sym);
  block(0);
  if (f.stack_min < loc)
    loc = f.stack_min;
  gsym(rsym);
  nocode_wanted = 0;
  cprime_debug_end_scope(NULL, !func_var);
  cprime_debug_prolog_epilog(cprime_state, 1);
  gfunc_epilog();

  // End Of Function
  cprime_debug_funcend(cprime_state, ind - func_ind);

  // Patch Symbol Size
  elfsym(sym)->st_size = ind - func_ind;
  cur_text_section->data_offset = ind;

  /* Inline members of a local class cannot be left for the end-of-unit
     inline pass: their class layout belongs to this function's local symbol
     stack.  The enclosing function is fully emitted here, while those type
     symbols are still live, so lower the queued helpers before releasing the
     stack. */
  if (nb_pending_member_funcs)
  {
    ++compile_local_member_funcs_now;
    compile_pending_member_funcs(0);
    --compile_local_member_funcs_now;
  }

  sym_pop(&local_stack, NULL, 0);
  label_pop(&global_label_stack, NULL, 0);
  sym_pop(&all_cleanups, NULL, 0);
  local_scope = 0;

  // It's better to crash than to generate wrong code
  cur_text_section = NULL;
  funcname = ""; // For Safety
  func_vt.t = VT_VOID; // For Safety
  func_var = 0; // For Safety
  ind = 0; // For Safety
  func_ind = -1;
  nocode_wanted = DATA_ONLY_WANTED;
  check_vstack();

  // do this after funcend debug info
  next();
}

static void gen_inline_functions(CPRIMEState *s)
{
  Sym *sym;
  int inline_generated, i;
  int pass;
  struct InlineFunc *fn;

  cprime_open_bf(s, ":inline:", 0);
  // iterate while inline function are referenced
  pass = 0;
  do
  {
    inline_generated = 0;
    for (i = 0; i < s->nb_inline_fns; ++i)
    {
      fn = s->inline_fns[i];
      sym = fn->sym;
      if (sym && (sym->c || !(sym->type.t & VT_INLINE)))
      {
        if (fn->preserve_type)
          sym->type = fn->type;
        /* the function was used or forced (and then not internal):
           generate its code and convert it to a normal function */
        if ((sym->type.t & VT_BTYPE) == VT_FUNC && sym->type.ref
            && sym->type.ref->next
            && sym->type.ref->next->v == tok_alloc_const("this"))
        {
          char this_name[256];
          int synthetic_this_tok;
          snprintf(this_name, sizeof this_name, "__cprime_this_%s",
                   get_tok_str(sym->v, NULL));
          synthetic_this_tok = tok_alloc_const(this_name);
          if (token_string_contains_tok(fn->func_str, synthetic_this_tok))
            sym->type.ref->next->v = synthetic_this_tok;
        }
        fn->sym = NULL;
        cprimepp_putfile(fn->filename);
        if (getenv("CPC_TRACE_INLINE_BODY")
            && sym
            && strstr(get_tok_str(sym->v, NULL),
                      getenv("CPC_TRACE_INLINE_BODY")))
        {
          int bi;
          fprintf(stderr, "CPC_INLINE_BODY func=%s file=%s ret_t=%04x ret_s=%s toks=",
                  get_tok_str(sym->v, NULL),
                  fn->filename,
                  sym->type.ref ? sym->type.ref->type.t : 0,
                  sym->type.ref
                    ? get_tok_str(get_struct_type_name_tok(&sym->type.ref->type), NULL)
                    : "<none>");
          for (bi = 0; bi < fn->func_str->len && bi < 80; ++bi)
            fprintf(stderr, "%s%s", bi ? " " : "",
                    get_tok_str(fn->func_str->str[bi], NULL));
          fprintf(stderr, "\n");
        }
        begin_macro(fn->func_str, fn->stable_heap ? 3 : 1);
        next();
        cur_text_section = text_section;
        gen_function(sym);
        if (macro_stack == fn->func_str)
          end_macro();
        if (fn->stable_heap)
        {
          cprime_free(fn->func_str->str);
          cprime_free(fn->func_str);
          fn->func_str = NULL;
        }

        inline_generated = 1;
      }
    }
    /* Inline bodies emitted above can instantiate further template members
       (e.g. a static member body that constructs a class), queueing pending
       definitions after the end-of-TU flush already ran.  Flush before the
       next emission pass so bodies queued by an emitted inline function are
       themselves emitted (and can queue further members). */
    queue_demanded_template_member_bodies();
    if (nb_pending_member_funcs)
      compile_pending_member_funcs(0);
    /* Replayed member bodies also queue free-function/class template
       specializations; compile those before the next pass so bodies called
       from a member (e.g. clCopyAssign from a constructor) are emitted. */
    if (finalizing_template_bodies
        || compiled_template_specs < nb_pending_template_specs)
      compile_pending_template_specs();
  }
  while ((inline_generated || nb_pending_member_funcs
          || nb_pending_template_member_body_requests
          || compiled_template_specs < nb_pending_template_specs)
         && ++pass < 16);
  cprime_close();
}

static void free_inline_functions(CPRIMEState *s)
{
  int i;
  // Free Tokens Of Unused Inline Functions
  for (i = 0; i < s->nb_inline_fns; ++i)
  {
    struct InlineFunc *fn = s->inline_fns[i];
    if (fn->sym && fn->func_str)
    {
      if (fn->stable_heap)
      {
        cprime_free(fn->func_str->str);
        cprime_free(fn->func_str);
      }
      else
        tok_str_free(fn->func_str);
    }
  }
  dynarray_reset(&s->inline_fns, &s->nb_inline_fns);
}

static void do_Static_assert(void)
{
  int c;
  const char *msg;

  next();
  skip('(');
  c = expr_const();
  msg = "_Static_assert fail";
  if (tok == ',')
  {
    next();
    msg = parse_mult_str("string constant")->data;
  }
  skip(')');
  if (c == 0)
    cprime_error("%s", msg);
  skip(';');
}

#ifdef CPRIME_TARGET_PE
static void pe_check_linkage(CType *type, AttributeDef *ad)
{
  if (!ad->a.dllimport && !ad->a.dllexport)
    return;
  if (type->t & VT_STATIC)
    cprime_error("cannot have dll linkage with static");
  if (type->t & VT_TYPEDEF)
  {
    const char *m = ad->a.dllimport ? "im" : "ex";
    cprime_warning("'dll%sport' attribute ignored for typedef", m);
    ad->a.dllimport = 0;
    ad->a.dllexport = 0;
  }
  else if (ad->a.dllimport)
  {
    if ((type->t & VT_BTYPE) == VT_FUNC)
      ad->a.dllimport = 0;
    else
      type->t |= VT_EXTERN;
  }
}
#endif

/* 'l' is VT_LOCAL or VT_CONST to define default storage type
   or VT_CMP if parsing old style parameter list
   or VT_JMP if parsing c99 for decl: for (int i = 0, ...) */
static int decl(int l)
{
  int v, has_init, has_ctor_init, has_direct_init, has_paren_init;
  int local_static_dynamic_init;
  int r, oldint, btype_is_auto, btype_was_typedef;
  CType type, btype;
  Sym *sym, *sa;
  TokenString *init_str, *copy_ctor_init;
  AttributeDef ad, adbase;
  ObjSym *esym;

  while (1)
  {

    oldint = 0;
    if (tok >= TOK_UIDENT
        && !strcmp(get_tok_str(tok, NULL), "static_assert"))
    {
      int assertion_value;
      next();
      skip('(');
      assertion_value = expr_const();
      if (tok == ',')
      {
        next();
        skip_or_save_block(NULL);
      }
      skip(')');
      skip(';');
      if (!assertion_value && !defer_pending_member_funcs
          && !compiling_non_lifecycle_template_member_body)
        cprime_error("static assertion failed in '%s'",
                     funcname ? funcname : "<global>");
      continue;
    }
    if (tok == TOK_EXTERN)
    {
      next();
      if (tok == TOK_STR)
      {
        parse_mult_str("linkage string");
        if (tok == '{')
        {
          if (l != VT_CONST)
            cprime_error("linkage block cannot appear here");
          next();
          while (tok != '}')
          {
            if (tok == TOK_EOF)
              cprime_error("unexpected end of file in linkage block");
            decl(l);
          }
          next();
          continue;
        }
        pending_cpp_extern_linkage = 1;
      }
      else
        unget_tok(TOK_EXTERN);
    }
    if (l == VT_CONST && tok >= 0 && !strcmp(get_tok_str(tok, NULL), "template"))
    {
      parse_template_decl();
      continue;
    }
    if (l == VT_CONST && try_parse_using_namespace_directive())
      continue;
    if (l == VT_CONST && tok == TOK_NAMESPACE)
    {
      parse_namespace_decl();
      continue;
    }
    if (l == VT_CONST && try_parse_cpp_lifecycle_def())
      continue;
    if (!parse_btype(&btype, &adbase, l == VT_LOCAL))
    {
      if (l == VT_JMP)
        return 0;
      // skip redundant ';' if not in old parameter decl scope
      if (tok == ';' && l != VT_CMP)
      {
        next();
        continue;
      }
      if (l == VT_CONST && tok == '}')
        break;
      if (tok == TOK_STATIC_ASSERT)
      {
        do_Static_assert();
        continue;
      }
      if (l != VT_CONST)
        break;
      if (tok == TOK_ASM1 || tok == TOK_ASM2 || tok == TOK_ASM3)
      {
        // Global Asm Block
        asm_global_instr();
        continue;
      }
      if (tok >= TOK_UIDENT)
      {
        /* special test for old K&R protos without explicit int
           type. Only accepted when defining global data */
        btype.t = VT_INT;
        oldint = 1;
      }
      else
      {
        if (tok != TOK_EOF)
          expect("declaration");
        break;
      }
    }
    btype_is_auto = last_decl_was_auto;
    btype_was_typedef = last_btype_was_typedef;

    if (l == VT_CONST && try_parse_cpp_scoped_member_def(&btype))
      continue;

    if (tok == ';')
    {
      if ((btype.t & VT_BTYPE) == VT_STRUCT
          && (btype.ref->v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
        ; // Struct Decl With Named Tag
      else if (IS_ENUM(btype.t))
        ; // Enum Decl
      else
        cprime_warning("useless type defines no instances");
      if (l == VT_JMP)
        return 1;
      next();
      continue;
    }

    while (1)   // Iterate Thru Each Declaration
    {
      type = btype;
      ad = adbase;
      last_btype_was_typedef = btype_was_typedef;
      type_decl(&type, &ad, &v,
                l == VT_CMP ? TYPE_DIRECT | TYPE_PARAM
                            : TYPE_DIRECT | ((l == VT_LOCAL || l == VT_JMP)
                                             ? TYPE_LOCAL_CTOR_INIT : 0));
      if (l == VT_CONST
          && try_rewrite_namespace_qualified_declarator(&v))
        ;
      if (l == VT_CONST
          && try_rewrite_cpp_scoped_static_data_after_declarator(&type, &ad,
                                                                 &v))
        ;
      if (l == VT_CONST
          && try_parse_cpp_scoped_member_def_after_declarator(&type, v))
        break;
      if (l == VT_CONST)
        v = make_current_namespace_tok(v);
      //ptype("decl", &type, v);
      if ((type.t & VT_BTYPE) == VT_FUNC)
      {
        if ((type.t & VT_STATIC) && (l != VT_CONST))
          cprime_error("function without file scope cannot be static");
        /* if old style function prototype, we accept a
           declaration list */
        sym = type.ref;
        if (sym->f.func_type == FUNC_OLD && l == VT_CONST)
        {
          func_vt = type;
          ++local_scope;
          decl(VT_CMP);
          --local_scope;
        }
        if ((type.t & (VT_EXTERN | VT_INLINE)) == (VT_EXTERN | VT_INLINE))
        {
          /* always_inline functions must be handled as if they
             don't generate multiple global defs, even if extern
             inline, i.e. GNU inline semantics for those.  Rewrite
             them into static inline.  */
          if (cprime_state->classic_inline || sym->f.func_alwinl)
            type.t = (type.t & ~VT_EXTERN) | VT_STATIC;
          else
            type.t &= ~VT_INLINE; // Always Compile Otherwise
        }

      }
      else if (oldint)
        cprime_warning("type defaults to int");

      if (non_iso && (tok == TOK_ASM1 || tok == TOK_ASM2 || tok == TOK_ASM3))
      {
        ad.asm_label = asm_label_instr();
        // Parse One Last Attribute List, After Asm Label
        parse_attribute(&ad);
#if 0
        /* gcc does not allow __asm__("label") with function definition,
           but why not ... */
        if (tok == '{')
          expect(";");
#endif
      }

#ifdef CPRIME_TARGET_PE
      pe_check_linkage(&type, &ad);
#endif
      if (tok == '{' && (type.t & VT_BTYPE) == VT_FUNC)
      {
        int repeated_inline_definition = 0;
        if (l != VT_CONST)
          cprime_error("cannot use local functions");

        // Apply Post-Declaraton Attributes
        merge_funcattr(&type.ref->f, &ad.f);
        // Put Function Symbol
        type.t &= ~VT_EXTERN;
        v = prepare_free_func_declarator(v, &type);
        {
          Sym *previous = sym_find(v);
          if (!previous)
            previous = global_symbol_find(v);
          repeated_inline_definition = previous
            && (previous->type.t & VT_BTYPE) == VT_FUNC
            && (previous->type.t & VT_INLINE)
            && (type.t & VT_INLINE)
            && !(previous->type.t & VT_EXTERN);
        }
        sym = external_sym(v, &type, 0, &ad);

        /* reject abstract declarators in function definition
           make old-style float params double */
        for (sa = sym->type.ref; (sa = sa->next) != NULL;)
        {
          if (!(sa->v & ~SYM_FIELD))
            expect("identifier");
          if (sa->type.t == VT_FLOAT
              && sym->type.ref->f.func_type == FUNC_OLD)
            sa->type.t = VT_DOUBLE;
        }

        /* static inline functions are just recorded as a kind
           of macro. Their code will be emitted at the end of
           the compilation unit only if they are used */
        if (sym->type.t & VT_INLINE)
        {
          struct InlineFunc *fn;
          if (repeated_inline_definition)
          {
            skip_or_save_block(NULL);
            break;
          }
          fn = cprime_malloc(sizeof *fn + strlen(file->filename));
          strcpy(fn->filename, file->filename);
          fn->sym = sym;
          fn->type = sym->type;
          fn->stable_heap = 0;
          {
            Sym *first_arg = sym->type.ref ? sym->type.ref->next : NULL;
            const char *first_name = first_arg
              ? get_tok_str(first_arg->v, NULL) : "";
            fn->preserve_type = !is_member_func_mangled_tok(sym->v)
              && strcmp(first_name, "this")
              && strncmp(first_name, "__cprime_this_", 14);
          }
          if (getenv("CPC_TRACE_INLINE_QUEUE")
              && (strstr(file->filename, "clVector2.inl")
                  || (sym && strstr(get_tok_str(sym->v, NULL), "clMaxComponent"))))
            fprintf(stderr, "CPC_INLINE_QUEUE func=%s ret_t=%04x ret_s=%s decl_v=%s type_t=%04x\n",
                    get_tok_str(sym->v, NULL),
                    sym->type.ref ? sym->type.ref->type.t : 0,
                    sym->type.ref
                      ? get_tok_str(get_struct_type_name_tok(&sym->type.ref->type), NULL)
                      : "<none>",
                    get_tok_str(v, NULL), type.t);
          dynarray_add(&cprime_state->inline_fns,
                       &cprime_state->nb_inline_fns, fn);
          skip_or_save_block(&fn->func_str);
          if (compiling_pending_template_specs && fn->func_str)
          {
            TokenString *stable = cprime_malloc(sizeof(*stable));
            memset(stable, 0, sizeof(*stable));
            stable->len = fn->func_str->len;
            stable->allocated_len = stable->len;
            stable->last_line_num = fn->func_str->last_line_num;
            stable->str = cprime_malloc(stable->len * sizeof(int));
            memcpy(stable->str, fn->func_str->str,
                   stable->len * sizeof(int));
            /* The token allocator may still have a suspended call argument
               referring to the transient object.  Leave that allocation to
               its caller and retain an independently owned inline body. */
            fn->func_str = stable;
            fn->stable_heap = 1;
          }
        }
        else
        {
          // Compute Text Section
          cur_text_section = ad.section;
          if (!cur_text_section)
            cur_text_section = text_section;
          else if (cur_text_section->sh_num > bss_section->sh_num)
            cur_text_section->sh_flags = text_section->sh_flags;
          gen_function(sym);
        }
        break;
      }
      else
      {
        has_init = 0;
        has_ctor_init = 0;
        has_direct_init = 0;
        has_paren_init = 0;
        local_static_dynamic_init = 0;
        init_str = NULL;
        copy_ctor_init = NULL;
        if (l == VT_CMP)
        {
          // Find Parameter In Function Parameter List
          for (sym = func_vt.ref->next; sym; sym = sym->next)
            if ((sym->v & ~SYM_FIELD) == v)
              goto found;
          cprime_error("declaration for parameter '%s' but no such parameter",
                    get_tok_str(v, NULL));
found:
          if (type.t & VT_STORAGE) // 'Register' Is Okay
            cprime_error("storage class specified for '%s'",
                      get_tok_str(v, NULL));
          if (!(sym->type.t & VT_EXTERN))
            cprime_error("redefinition of parameter '%s'",
                      get_tok_str(v, NULL));
          convert_parameter_type(&type);
          sym->type = type;
        }
        else if (type.t & VT_TYPEDEF)
        {
          // Save Typedefed Type
          // XXX: test storage specifiers ?
          if (v >= TOK_UIDENT && !strcmp(get_tok_str(v, NULL), "wchar_t"))
            type.t |= VT_WCHAR_T;
          sym = sym_find(v);
          if (sym && sym->sym_scope == local_scope)
          {
            if (!is_compatible_types(&sym->type, &type)
                || !(sym->type.t & VT_TYPEDEF))
              cprime_error("incompatible redefinition of '%s'",
                        get_tok_str(v, NULL));
            sym->type = type;
          }
          else
            sym = sym_push(v, &type, 0, 0);
          sym->a = ad.a;
          if ((type.t & VT_BTYPE) == VT_FUNC)
            merge_funcattr(&sym->type.ref->f, &ad.f);
          if (debug_modes)
            cprime_debug_typedef (cprime_state, sym);
        }
        else if ((type.t & VT_BTYPE) == VT_VOID
                 && !(type.t & VT_EXTERN))
          cprime_error("declaration of void object");
        else
        {
          r = 0;
          if ((type.t & VT_BTYPE) == VT_FUNC)
          {
            // External Function Definition
            // Specific Case For Func_Call Attribute
            merge_funcattr(&type.ref->f, &ad.f);
          }
          else if (!(type.t & VT_ARRAY))
          {
            // not lvalue if array
            r |= VT_LVAL;
          }

          if (tok == '=')
            has_init = 1;
          else if (tok == '{')
            has_init = has_direct_init = 1;
          else if (tok == '(' && l != VT_CONST
                   && (type.t & VT_BTYPE) == VT_STRUCT
                   && type.ref)
            has_ctor_init = 1;
          else if (tok == '(' && l != VT_CONST)
          {
            has_init = has_direct_init = has_paren_init = 1;
            next();
          }

          if (((type.t & VT_EXTERN) && (!has_init || l != VT_CONST))
              || (type.t & VT_BTYPE) == VT_FUNC
              /* as with GCC, uninitialized global arrays with no size
                 are considered extern: */
              || ((type.t & VT_ARRAY) && !has_init
                  && l == VT_CONST && type.ref->c < 0)
             )
          {
            // External Variable Or Function
            type.t |= VT_EXTERN;
            if ((type.t & VT_BTYPE) == VT_FUNC && l == VT_CONST)
              v = prepare_free_func_declarator(v, &type);
            external_sym(v, &type, r, &ad);
          }
          else
          {
            if (l == VT_CONST || (type.t & VT_STATIC))
              r |= VT_CONST;
            else
              r |= VT_LOCAL;
            type.t &= ~VT_EXTERN;
            if (btype_is_auto)
            {
              TokenString *auto_init_str = NULL;
              TokenString *auto_macro_stack;
              TokenString *auto_macro_parent;
              const int *auto_macro_parent_ptr;
              CType inferred_type;
              int auto_ref_kind;
              int saved_tok;
              CValue saved_tokc;
              if (l == VT_CONST || (type.t & VT_STATIC) || !has_init
                  || has_direct_init || has_ctor_init || tok != '=')
                cprime_error("unsupported auto declaration");
              auto_ref_kind = type.t & (VT_REFERENCE | VT_RVALUE_REFERENCE);
              next();
              skip_or_save_block(&auto_init_str);
              saved_tok = tok;
              saved_tokc = tokc;
              infer_expr_type_from_tokens(auto_init_str, &inferred_type);
              if (is_reference_type(&inferred_type))
              {
                decay_reference_type(&inferred_type);
                inferred_type = *pointed_type(&inferred_type);
              }
              type = inferred_type;
              if (auto_ref_kind & VT_RVALUE_REFERENCE)
              {
                mk_reference(&type);
                type.t = (type.t & ~VT_REFERENCE) | VT_RVALUE_REFERENCE;
              }
              else if (auto_ref_kind & VT_REFERENCE)
                mk_reference(&type);
              type.t &= ~(VT_EXTERN | VT_TYPEDEF);
              auto_macro_parent = macro_stack;
              auto_macro_parent_ptr = macro_ptr;
              begin_macro(auto_init_str, 1);
              auto_macro_stack = macro_stack;
              next();
              decl_initializer_alloc(&type, &ad, r, 1, 0, NULL, v, l);
              while (macro_stack && macro_stack != auto_macro_stack
                     && macro_stack != auto_macro_parent)
                end_macro();
              if (macro_stack == auto_macro_stack)
              {
                end_macro();
              }
              /* Whether the initializer stopped inside its replay or consumed
                 it through TOK_EOF, end_macro() owns the buffer.  Restore the
                 suspended caller cursor because EOF lookahead may already
                 have fetched one token from the parent frame. */
              if (macro_stack == auto_macro_parent)
                auto_init_str = NULL;
              macro_ptr = auto_macro_parent_ptr;
              tok = saved_tok;
              tokc = saved_tokc;
              if (auto_init_str)
                tok_str_free(auto_init_str);
              auto_init_str = NULL;
              goto after_decl_initializer_alloc;
            }
            if (has_init)
            {
              if (!has_direct_init)
                next();
              if (l != VT_CONST && tok == '{'
                  && (type.t & VT_BTYPE) == VT_STRUCT
                  && resolve_initializer_list_constructor(&type))
              {
                skip_or_save_block(&copy_ctor_init);
                has_init = 0;
              }
              else if (can_lower_local_static_dynamic_init(&type, l,
                                                           has_init))
              {
                skip_or_save_block(&init_str);
                has_init = 0;
                local_static_dynamic_init = 1;
              }
              else if (l != VT_CONST
                  && (type.t & VT_BTYPE) == VT_STRUCT
                  && tok != '{'
                  && type.ref && type.ref->a.lifecycle_ctor)
              {
                skip_or_save_block(&copy_ctor_init);
                has_init = 0;
              }
              else if (can_lower_global_dynamic_init(&type, l, has_init))
              {
                skip_or_save_block(&init_str);
                has_init = 0;
              }
            }
            else if (l == VT_CONST)
              // Uninitialized Global Variables May Be Overridden
              type.t |= VT_EXTERN;
            decl_initializer_alloc(&type, &ad, r, has_init, has_ctor_init,
                                   copy_ctor_init, v, l);
            if (has_paren_init)
              skip(')');
after_decl_initializer_alloc:
            if (copy_ctor_init)
            {
              tok_str_free(copy_ctor_init);
              copy_ctor_init = NULL;
            }
            if (init_str)
            {
              if (local_static_dynamic_init)
                emit_local_static_dynamic_init(v, &type, init_str);
              else
                add_pending_global_dynamic_init(v, &type, init_str);
              tok_str_free(init_str);
              init_str = NULL;
            }
          }

          if (ad.alias_target && l == VT_CONST)
          {
            /* Aliases need to be emitted when their target symbol
               is emitted, even if perhaps unreferenced.
               We only support the case where the base is already
               defined, otherwise we would need deferring to emit
               the aliases until the end of the compile unit.  */
            esym = elfsym(sym_find(ad.alias_target));
            if (!esym)
              cprime_error("unsupported forward __alias__ attribute");
            put_extern_sym2(sym_find(v), esym->st_shndx,
                            esym->st_value, esym->st_size, 1);
          }
        }
        if (tok != ',')
        {
          if (l == VT_JMP)
            return has_init ? v : 1;
          skip(';');
          break;
        }
        next();
      }
    }
  }
  return 0;
}

// -------------------------------------------------------------------------
#undef gjmp_addr
#undef gjmp
// -------------------------------------------------------------------------









