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
} *cur_scope, *loop_scope, *root_scope;

typedef struct
{
  Section *sec;
  int local_offset;
  Sym *flex_array_ref;
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
static inline int64_t expr_const64(void);
static void vpush64(int ty, unsigned long long v);
static void vpush(CType *type);
static void save_lvalues(void);
static void gfunc_param_typed(Sym *func, Sym *arg);
static int gvtst(int inv, int t);
static void type_to_str(char *buf, int buf_size, CType *type, const char *varstr);
static inline void convert_parameter_type(CType *pt);
static void gen_inline_functions(CPRIMEState *s);
static void free_inline_functions(CPRIMEState *s);
static void skip_or_save_block(TokenString **str);
static void skip_or_save_param_default(TokenString **str);
static void expand_saved_single_object_macro(TokenString **str);
static void gv_dup(void);
static void compile_pending_member_funcs(int start);
static int get_temp_local_var(int size, int align, int *r2);
static void cast_error(CType *st, CType *dt);
static void end_switch(void);
static void do_Static_assert(void);
static void parse_template_decl(void);
static void compile_pending_template_specs(void);
static int struct_has_member_init_list(int struct_tok);
static int token_string_contains_tok(TokenString *str, int needle);
static Sym *resolve_member_func(CType *type, int method_tok);
static int member_func_explicit_arg_count(CType *lowered_type);
static Sym *resolve_member_func_by_arg_count(CType *type, int method_tok,
                                              int explicit_arg_count);
static Sym *resolve_initializer_list_constructor(CType *type);
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
static int type_is_std_initializer_list(CType *type);
static int same_lowered_member_func_signature(CType *type1, CType *type2);
static int type_has_member_func_name(CType *type, int member_tok);
static int class_or_inst_has_member_template_name(int class_tok,
                                                  int member_tok);
static int is_lifecycle_member_tok(int t);
static int template_return_ctype_from_struct_tok(CType *ret_type, int struct_tok);
static Sym *resolve_free_func_by_arg_types(int name_tok, CType *arg_types,
                                           int explicit_arg_count);
static Sym *resolve_free_func_by_arg_count(int name_tok,
                                           int explicit_arg_count);
static Sym *declare_static_member_func(CType *struct_type, int method_tok,
                                       CType *func_type);
static Sym *find_field_try(CType *type, int v, int *cumofs);
static Sym *find_static_member_try(CType *type, int member_tok, int *owner_tok);
static Sym *find_static_member_by_class_try(int class_tok, int member_tok,
                                            int *owner_tok);
static int class_has_static_member_func(int class_tok, int member_tok);
static void instantiate_static_template_member_for_call(int class_mangled_tok,
                                                        int member_tok);
static int try_call_cpp_binary_operator(int op);
static int try_call_cpp_unary_minus_operator(void);
static int try_call_cpp_unary_operator(int op);
static int try_call_cpp_index_operator(void);
static int try_call_cpp_assignment_operator(void);
static int try_call_cpp_compound_assign_operator(int op);
static CType make_lowered_member_func_type(CType *struct_type, CType *func_type);
static int struct_needs_memberwise_assignment(CType *type);
static void assign_struct_memberwise_from_base_ptr(CType *type, SValue *dst_ptr,
                                                   SValue *src_ptr,
                                                   int base_offset);
static Sym *resolve_copy_constructor_func(CType *type, CType *source_type);
static int struct_needs_memberwise_copy(CType *type);
static void copy_construct_struct_memberwise_from_base_ptr(CType *type,
                                                           SValue *dst_ptr,
                                                           SValue *src_ptr,
                                                           int base_offset);
static void restore_cpp_lifecycle_probe(TokenString *replay);
static int is_namespace_tok(int ns_tok);
static void note_namespace_tok(int ns_tok);
static void note_struct_member_init_list(int struct_tok);
static void note_auto_return_member_tok(int member_tok);
static int member_is_auto_return_tok(int member_tok);
static TokenString *parse_constructor_member_initializers(CType *struct_type);
static int is_template_keyword_tok(int t);
static CType make_func_type_from_saved_params(CType *ret_type,
                                              TokenString *params);

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
  int *inst_name_toks;
  int nb_inst, al_inst;
} TemplateDef;

typedef struct TemplateArgList
{
  int toks[16];
  int nb;
} TemplateArgList;

typedef struct TemplateMemberDef
{
  int class_tok;
  int type_param_tok;
  TokenString *def_str;
  int *inst_type_toks;
  int *inst_pack_toks;
  int nb_inst, al_inst;
  int *inst_class_toks;
  CType *inst_ret_types;
  int nb_inst_ret_types, al_inst_ret_types;
} TemplateMemberDef;

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
  struct ClassBaseInfo *next;
} ClassBaseInfo;

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
static void instantiate_template_member_if_needed(TemplateMemberDef *md,
                                                  int type_tok,
                                                  int class_mangled_tok,
                                                  int member_type_arg_tok);
static int template_member_def_method_tok(TemplateMemberDef *md);
static int token_string_has_initializer_list(TokenString *str);
static TemplateDef *find_class_template_def_for_class_tok(int class_tok);
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
static int template_lookup_inst(TemplateDef *td, TemplateArgList *args);
static void note_template_inst(TemplateDef *td, TemplateArgList *args,
                               int mangled_tok);
static void infer_saved_arg_types(TokenString **args, CType *types, int nb_args);
static void tok_str_append_without_eof(TokenString *dst, TokenString *src);
static int template_type_tok_from_ctype(CType *type);

static void infer_expr_type_from_tokens(TokenString *expr, CType *type)
{
  int saved_nocode_wanted = nocode_wanted;
  TokenString macro;
  TokenString *expr_macro_stack;

  macro = *expr;
  nocode_wanted++;
  begin_macro(&macro, 0);
  expr_macro_stack = macro_stack;
  next();
  expr_eq();
  *type = vtop->type;
  vpop();
  if (macro_stack == expr_macro_stack)
    end_macro();
  nocode_wanted = saved_nocode_wanted;
}

static TemplateDef **template_defs;
static int nb_template_defs;
static TemplateMemberDef **template_member_defs;
static int nb_template_member_defs;
static TemplateAliasInst *template_alias_insts;
static int nb_template_alias_insts;
static int al_template_alias_insts;
static TokenString **pending_template_specs;
static int nb_pending_template_specs;
static int suppress_template_member_flush;
static int pending_cpp_extern_linkage;
static int range_for_temp_counter;
static int compiled_template_specs;
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
static int defining_class_stack[32];
static int nb_defining_class_stack;
static int compiling_non_lifecycle_template_member_body;
static int last_decl_was_auto;
static int last_btype_was_typedef;
static int last_btype_was_decltype;
static int last_instantiated_member_func_tok;
static ClassBaseInfo *class_base_infos;

static int al_namespace_toks;

typedef struct PendingMemberFunc
{
  TokenString *str;
  int struct_tok;
  int is_template_member;
  int is_lifecycle_member;
  int is_static_member;
} PendingMemberFunc;

static PendingMemberFunc **pending_member_funcs;
static int nb_pending_member_funcs;
static int defer_pending_member_funcs;
static int compiling_pending_member_funcs;
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
} MemberFuncOverload;

typedef struct FreeFuncOverload
{
  int name_tok;
  int mangled_tok;
  int explicit_arg_count;
  int min_arg_count;
  CType func_type;
  struct FreeFuncOverload *next;
} FreeFuncOverload;

static MemberFuncOverload *member_func_overloads;
static FreeFuncOverload *free_func_overloads;
static int *defaulted_member_struct_toks;
static int *defaulted_member_method_toks;
static CType *defaulted_member_func_types;
static int nb_defaulted_member_funcs;
static int al_defaulted_member_funcs;
static int tok_public;
static int tok_protected;
static int tok_private;
static int tok_explicit;
static int tok_constexpr;

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

static int pending_member_func_has_body_tok(int func_tok)
{
  int i, j, has_tok, has_body;

  for (i = 0; i < nb_pending_member_funcs; ++i)
  {
    PendingMemberFunc *pm = pending_member_funcs[i];
    if (!pm || !pm->str)
      continue;
    has_tok = 0;
    has_body = 0;
    for (j = 0; j < pm->str->len; ++j)
    {
      if (pm->str->str[j] == func_tok)
        has_tok = 1;
      else if (pm->str->str[j] == '{')
        has_body = 1;
    }
    if (has_tok && has_body)
      return 1;
  }
  return 0;
}

static void free_template_state(void)
{
  int i;
  MemberFuncOverload *o;
  FreeFuncOverload *fo;

  /* Compiled pending specs are freed by end_macro() and their slots are
     NULLed during compile_pending_template_specs().  Entries that were
     queued but never compiled still own tokstr allocations and must be
     released here before the backing arena is torn down. */
  for (i = compiled_template_specs; i < nb_pending_template_specs; ++i)
    if (pending_template_specs[i])
      tok_str_free(pending_template_specs[i]);
  cprime_free(pending_template_specs);
  pending_template_specs = NULL;
  nb_pending_template_specs = 0;

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

  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (!md)
      continue;
    tok_str_free(md->def_str);
    cprime_free(md);
  }
  cprime_free(template_member_defs);
  template_member_defs = NULL;
  nb_template_member_defs = 0;

  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td)
      continue;
    tok_str_free(td->def_str);
    cprime_free(td->type_param_toks);
    cprime_free(td->inst_type_toks);
    cprime_free(td->inst_name_toks);
    cprime_free(td);
  }
  cprime_free(template_defs);
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

  while (member_func_overloads)
  {
    o = member_func_overloads;
    member_func_overloads = o->next;
    cprime_free(o);
  }
  while (free_func_overloads)
  {
    fo = free_func_overloads;
    free_func_overloads = fo->next;
    cprime_free(fo);
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
  compile_pending_template_specs();
  if (nb_pending_member_funcs)
    compile_pending_member_funcs(0);
  gen_inline_functions(s1);
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
    if ((v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
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
        const char *new_cmp;
        type_to_str(old_type_buf, sizeof(old_type_buf), &sym->type, NULL);
        type_to_str(new_type_buf, sizeof(new_type_buf), type, NULL);
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
    temp_var = &arr_temp_local_vars[i];
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

  if ((t1 & VT_ARRAY)
      && !(type1->ref->c < 0
           || type2->ref->c < 0
           || type1->ref->c == type2->ref->c))
    return 0;

  // Test More Complicated Cases
  bt1 = t1 &VT_BTYPE;
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
  else if (bt1 == VT_STRUCT)
  {
    /* C++-style member operators take precedence for class/struct lhs. */
    if (try_call_cpp_binary_operator(op))
      return;
  }
  else if (!combine_types(&combtype, vtop - 1, vtop, op_class))
  {
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
  verify_assign_cast(dt);
  gen_cast(dt);
}

// Store Vtop In Lvalue Pushed On Stack
ST_FUNC void vstore(void)
{
  int sbt, dbt, ft, r, size, align, bit_size, bit_pos, delayed_cast;

  ft = vtop[-1].type.t;
  sbt = vtop->type.t &VT_BTYPE;
  dbt = ft &VT_BTYPE;
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

static int make_current_class_nested_tok(int name_tok)
{
  if (nb_defining_class_stack <= 0 || name_tok < TOK_UIDENT)
    return name_tok;
  return make_static_member_tok(defining_class_stack[nb_defining_class_stack - 1],
                                name_tok);
}

static int find_current_class_nested_type_tok(int name_tok)
{
  int nested_tok;

  if (nb_defining_class_stack <= 0 || name_tok < TOK_UIDENT)
    return name_tok;
  nested_tok = make_current_class_nested_tok(name_tok);
  if (struct_find(nested_tok))
    return nested_tok;
  return name_tok;
}

static int make_namespace_tok_from_parts(const int *parts, int nb_parts)
{
  char name[512];
  int i;

  if (nb_parts <= 0)
    return 0;
  pstrcpy(name, sizeof(name), "__cpc_ns");
  for (i = 0; i < nb_parts; ++i)
  {
    pstrcat(name, sizeof(name), "_");
    pstrcat(name, sizeof(name), get_tok_str(parts[i], NULL));
  }
  return tok_alloc_const(name);
}

static int make_current_namespace_tok(int name_tok)
{
  int parts[17], i;
  const char *name = get_tok_str(name_tok, NULL);

  if (!nb_namespace_stack || name_tok < TOK_UIDENT)
    return name_tok;
  /* Replayed member bodies carry already-mangled names (e.g.
     __cpc_ns_std_W_f for a class inside a namespace).  Prefixing again
     would produce __cpc_ns_std___cpc_ns_std_W_f and leave the definition
     unreferenced by the call.  Compiler-generated namespace tokens are
     reserved (__cpc_ns...), so an existing prefix means fully qualified. */
  if (!strncmp(name, "__cpc_ns", 8))
    return name_tok;
  if (nb_namespace_stack >= (int)(sizeof(parts) / sizeof(parts[0])))
    cprime_error("namespace nesting too deep");
  for (i = 0; i < nb_namespace_stack; ++i)
    parts[i] = namespace_stack[i];
  parts[nb_namespace_stack] = name_tok;
  return make_namespace_tok_from_parts(parts, nb_namespace_stack + 1);
}

static int find_current_namespace_tok(int name_tok)
{
  int t;

  if (!nb_namespace_stack || name_tok < TOK_UIDENT)
    return name_tok;
  t = make_current_namespace_tok(name_tok);
  if (sym_find(t) || sym_find2(global_stack, t) || struct_find(t))
    return t;
  return name_tok;
}

static Sym *find_namespace_or_plain_symbol(int *ptok)
{
  int t = *ptok;
  Sym *s;

  s = sym_find(t);
  if (!s)
  {
    t = find_current_namespace_tok(t);
    if (t != *ptok)
      s = sym_find(t);
  }
  if (!s)
    s = sym_find2(global_stack, t);
  if (s)
    *ptok = t;
  return s;
}

static int try_rewrite_namespace_qualified_declarator(int *pv)
{
  int parts[16], nb_parts = 0;
  TokenString *replay;

  if (*pv < TOK_UIDENT || tok != ':' || struct_find(*pv)
      || !is_namespace_tok(*pv))
    return 0;
  parts[nb_parts++] = *pv;
  for (;;)
  {
    replay = tok_str_alloc();
    tok_str_add(replay, tok);
    next();
    if (tok != ':')
    {
      restore_cpp_lifecycle_probe(replay);
      break;
    }
    tok_str_free(replay);
    next();
    if (tok < TOK_UIDENT)
      cprime_error("qualified declaration name");
    if (nb_parts >= (int)(sizeof(parts) / sizeof(parts[0])))
      cprime_error("qualified name too deep");
    parts[nb_parts++] = tok;
    next();
    if (tok != ':')
      break;
  }
  if (nb_parts == 1)
    return 0;
  *pv = make_namespace_tok_from_parts(parts, nb_parts);
  return 1;
}

static int is_namespace_tok(int ns_tok)
{
  int i;

  for (i = 0; i < nb_namespace_toks; ++i)
    if (namespace_toks[i] == ns_tok)
      return 1;
  return 0;
}

static void note_namespace_tok(int ns_tok)
{
  if (is_namespace_tok(ns_tok))
    return;
  if (nb_namespace_toks >= al_namespace_toks)
  {
    al_namespace_toks = al_namespace_toks ? al_namespace_toks * 2 : 16;
    namespace_toks = cprime_realloc(namespace_toks,
                                    al_namespace_toks * sizeof(*namespace_toks));
  }
  namespace_toks[nb_namespace_toks++] = ns_tok;
}

static int append_type_mangle(char *name, int name_size, CType *type)
{
  int bt = type->t & VT_BTYPE;
  CType pt;
  char part[128];

  if (bt == VT_PTR)
  {
    pt = *pointed_type(type);
    if (!append_type_mangle(name, name_size, &pt))
      return 0;
    if (type->t & VT_RVALUE_REFERENCE)
      pstrcat(name, name_size, "_rref");
    else
      pstrcat(name, name_size, (type->t & VT_REFERENCE) ? "_ref" : "_ptr");
    return 1;
  }

  if (bt == VT_VOID)
    pstrcpy(part, sizeof(part), "void");
  else if (bt == VT_BYTE)
  {
    if (type->t & VT_UNSIGNED)
      pstrcpy(part, sizeof(part), "uchar");
    else if (type->t & VT_DEFSIGN)
      pstrcpy(part, sizeof(part), "schar");
    else
      pstrcpy(part, sizeof(part), "char");
  }
  else if (bt == VT_SHORT)
  {
    if (type->t & VT_WCHAR_T)
      pstrcpy(part, sizeof(part), "wchar");
    else
      pstrcpy(part, sizeof(part), (type->t & VT_UNSIGNED) ? "ushort" : "short");
  }
  else if (bt == VT_INT)
  {
    if (type->t & VT_LONG)
      pstrcpy(part, sizeof(part), (type->t & VT_UNSIGNED) ? "ulong" : "long");
    else
      pstrcpy(part, sizeof(part), (type->t & VT_UNSIGNED) ? "uint" : "int");
  }
  else if (bt == VT_BOOL)
    pstrcpy(part, sizeof(part), "bool");
  else if (bt == VT_LLONG)
    pstrcpy(part, sizeof(part), (type->t & VT_UNSIGNED) ? "ullong" : "llong");
  else if (bt == VT_FLOAT)
    pstrcpy(part, sizeof(part), "float");
  else if (bt == VT_DOUBLE)
    pstrcpy(part, sizeof(part), "double");
  else if (bt == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
    if (!struct_tok)
      return 0;
    snprintf(part, sizeof(part), "struct_%s", get_tok_str(struct_tok, NULL));
  }
  else
    return 0;

  pstrcat(name, name_size, "_");
  pstrcat(name, name_size, part);
  return 1;
}

static int make_member_func_tok_for_type(int struct_tok, int method_tok,
                                          CType *func_type)
{
  char name[512];
  Sym *arg;
  int base_tok;

  snprintf(name, sizeof(name), "%s_%s",
           get_tok_str(struct_tok, NULL),
           get_tok_str(method_tok, NULL));
  if (func_type && (func_type->t & VT_CONSTANT))
    pstrcat(name, sizeof(name), "_const");
  if (!func_type || !func_type->ref || !func_type->ref->next)
    return tok_alloc_const(name);
  base_tok = tok_alloc_const(name);
  if (!sym_find(base_tok) && !sym_find2(global_stack, base_tok))
    return base_tok;
  for (arg = func_type->ref->next; arg; arg = arg->next)
    if (!append_type_mangle(name, sizeof(name), &arg->type))
      cprime_error("unsupported overloaded member function parameter type");
  return tok_alloc_const(name);
}

static int make_lifecycle_func_tok_for_type(int struct_tok, int method_tok,
                                            CType *lowered_type,
                                            int prefer_existing)
{
  char name[512];
  Sym *arg;
  int same_count = 0, other_count = 0, same_mangled_tok = 0;
  int explicit_arg_count = member_func_explicit_arg_count(lowered_type);
  MemberFuncOverload *o;

  snprintf(name, sizeof(name), "%s_%s",
           get_tok_str(struct_tok, NULL),
           get_tok_str(method_tok, NULL));
  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && o->explicit_arg_count == explicit_arg_count)
    {
      same_count++;
      if (!same_mangled_tok)
        same_mangled_tok = o->mangled_tok;
    }
    else if (o->struct_tok == struct_tok && o->method_tok == method_tok)
      other_count++;
  if (same_count == 1 && prefer_existing)
    return same_mangled_tok;
  if (!lowered_type || !lowered_type->ref || !lowered_type->ref->next)
    return tok_alloc_const(name);
  if (!same_count && !other_count && !sym_find(tok_alloc_const(name))
      && !sym_find2(global_stack, tok_alloc_const(name)))
    return tok_alloc_const(name);
  arg = lowered_type->ref->next;
  if (arg)
    arg = arg->next;
  for (; arg; arg = arg->next)
    if (!append_type_mangle(name, sizeof(name), &arg->type))
      cprime_error("unsupported overloaded lifecycle parameter type");
  return tok_alloc_const(name);
}

static int member_func_explicit_arg_count(CType *lowered_type)
{
  int n = 0;
  Sym *arg;

  if (!lowered_type || !lowered_type->ref)
    return 0;
  arg = lowered_type->ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  for (; arg; arg = arg->next)
    ++n;
  return n;
}

static int member_func_min_arg_count(CType *lowered_type)
{
  int n = 0;
  Sym *arg;

  if (!lowered_type || !lowered_type->ref)
    return 0;
  arg = lowered_type->ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      ++n;
  return n;
}

static void note_member_func_overload(int struct_tok, int method_tok,
                                      int mangled_tok, CType *lowered_type)
{
  MemberFuncOverload *o;
  int is_const = 0;
  Sym *this_arg;

  if (lowered_type && lowered_type->ref)
  {
    this_arg = lowered_type->ref->next;
    if (this_arg && (this_arg->type.t & VT_BTYPE) == VT_PTR)
      is_const = pointed_type(&this_arg->type)->t & VT_CONSTANT;
  }

  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && o->mangled_tok == mangled_tok)
    {
      /* Raw lifecycle registration establishes the mangled name before the
         lowered function type exists. Complete that entry once parameter
         defaults and the implicit-this type are available. */
      if (lowered_type && lowered_type->ref)
      {
        o->explicit_arg_count = member_func_explicit_arg_count(lowered_type);
        o->min_arg_count = member_func_min_arg_count(lowered_type);
        o->is_const = is_const != 0;
        o->func_type = *lowered_type;
      }
      return;
    }
  o = cprime_mallocz(sizeof(*o));
  o->struct_tok = struct_tok;
  o->method_tok = method_tok;
  o->mangled_tok = mangled_tok;
  o->explicit_arg_count = member_func_explicit_arg_count(lowered_type);
  o->min_arg_count = member_func_min_arg_count(lowered_type);
  o->is_const = is_const != 0;
  if (lowered_type)
    o->func_type = *lowered_type;
  o->next = member_func_overloads;
  member_func_overloads = o;
}

static int func_explicit_arg_count(CType *func_type)
{
  int n = 0;
  Sym *arg;

  if (!func_type || !func_type->ref)
    return 0;
  for (arg = func_type->ref->next; arg; arg = arg->next)
    ++n;
  return n;
}

static int func_min_arg_count(CType *func_type)
{
  int n = 0;
  Sym *arg;

  if (!func_type || !func_type->ref)
    return 0;
  for (arg = func_type->ref->next; arg; arg = arg->next)
    if (!arg->default_arg)
      ++n;
  return n;
}

static int same_func_param_signature(CType *func_type1, CType *func_type2)
{
  Sym *a, *b;

  if (!func_type1 || !func_type1->ref || !func_type2 || !func_type2->ref)
    return 0;
  if (func_type1->ref->f.func_type == FUNC_OLD
      || func_type2->ref->f.func_type == FUNC_OLD)
    return func_explicit_arg_count(func_type1) == func_explicit_arg_count(func_type2);
  a = func_type1->ref->next;
  b = func_type2->ref->next;
  for (;;)
  {
    if (!a || !b)
      return a == b;
    if (!is_compatible_unqualified_types(&a->type, &b->type))
      return 0;
    a = a->next;
    b = b->next;
  }
}

static Sym *find_global_symbol(int tok)
{
  Sym *s = sym_find(tok);
  while (s && s->sym_scope)
    s = s->prev_tok;
  return s;
}

static void note_free_func_overload(int name_tok, int mangled_tok,
                                    CType *func_type)
{
  FreeFuncOverload *o;

  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == name_tok && o->mangled_tok == mangled_tok)
      return;
  o = cprime_mallocz(sizeof(*o));
  o->name_tok = name_tok;
  o->mangled_tok = mangled_tok;
  o->explicit_arg_count = func_explicit_arg_count(func_type);
  o->min_arg_count = func_min_arg_count(func_type);
  if (func_type)
    o->func_type = *func_type;
  o->next = free_func_overloads;
  free_func_overloads = o;
}

static int has_free_func_overload(int name_tok)
{
  FreeFuncOverload *o;

  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == name_tok)
      return 1;
  return 0;
}

static int is_member_func_mangled_tok(int tok)
{
  MemberFuncOverload *o;

  for (o = member_func_overloads; o; o = o->next)
    if (o->mangled_tok == tok)
      return 1;
  return 0;
}

static int has_suffix(const char *s, const char *suffix)
{
  size_t n, m;

  if (!s || !suffix)
    return 0;
  n = strlen(s);
  m = strlen(suffix);
  return n >= m && !strcmp(s + n - m, suffix);
}

static int is_cpp_translation_unit(void)
{
  BufferedFile *root_file = NULL;
  const char *filename;

  if (cprime_state && cprime_state->include_stack[0])
    root_file = cprime_state->include_stack[0];
  if (!root_file)
    root_file = file;
  filename = root_file ? root_file->filename : NULL;
  return has_suffix(filename, ".cpp")
         || has_suffix(filename, ".cxx")
         || has_suffix(filename, ".cc")
         || has_suffix(filename, ".C");
}

static int make_free_func_tok_for_type(int name_tok, CType *func_type)
{
  char name[512];
  Sym *arg, *base;

  if (!func_type || (func_type->t & VT_BTYPE) != VT_FUNC || !func_type->ref)
    return name_tok;
  if (!is_cpp_translation_unit())
    return name_tok;
  {
    const char *fn = get_tok_str(name_tok, NULL);
    if (fn[0] == '_' && fn[1] == '_')
      return name_tok;
  }
  if (is_member_func_mangled_tok(name_tok))
    return name_tok;

  base = find_global_symbol(name_tok);
  if (!base && !has_free_func_overload(name_tok))
    return name_tok;
  if (base && (base->type.t & VT_BTYPE) == VT_FUNC
      && same_func_param_signature(&base->type, func_type))
    return name_tok;
  if (base && (base->type.t & VT_BTYPE) == VT_FUNC)
    note_free_func_overload(name_tok, name_tok, &base->type);

  pstrcpy(name, sizeof(name), get_tok_str(name_tok, NULL));
  if (!func_type->ref->next)
    pstrcat(name, sizeof(name), "_void");
  for (arg = func_type->ref->next; arg; arg = arg->next)
    if (!append_type_mangle(name, sizeof(name), &arg->type))
      cprime_error("unsupported overloaded function parameter type");
  return tok_alloc_const(name);
}

static int prepare_free_func_declarator(int name_tok, CType *func_type)
{
  int mangled_tok;
  Sym *base;

  base = find_global_symbol(name_tok);
  if (base && (base->type.t & VT_BTYPE) == VT_FUNC
      && same_func_param_signature(&base->type, func_type))
    preserve_func_default_args(func_type, &base->type);
  mangled_tok = make_free_func_tok_for_type(name_tok, func_type);
  if (mangled_tok != name_tok)
  {
    base = find_global_symbol(mangled_tok);
    if (base && (base->type.t & VT_BTYPE) == VT_FUNC
        && same_func_param_signature(&base->type, func_type))
      preserve_func_default_args(func_type, &base->type);
  }
  if (mangled_tok != name_tok)
    note_free_func_overload(name_tok, mangled_tok, func_type);
  return mangled_tok;
}

static void emit_default_arg(Sym *func_type, Sym *arg)
{
  TokenString macro;

  if (!arg || !arg->default_arg)
  {
    cprime_error("too few arguments to function");
  }
  macro = *arg->default_arg;
  begin_macro(&macro, 0);
  next();
  expr_eq();
  end_macro();
  gfunc_param_typed(func_type, arg);
}

static void drop_leaked_call_target(Sym *func_sym)
{
  if (vtop >= vstack && func_sym
      && vtop->sym == func_sym
      && (vtop->type.t & VT_BTYPE) == VT_FUNC
      && is_compatible_types(&vtop->type, &func_sym->type))
    vpop();
}

static int count_saved_call_args(TokenString **args, int max_args)
{
  int n = 0;

  if (tok == ')')
    return 0;
  for (;;)
  {
    if (n >= max_args)
      cprime_error("too many overloaded call arguments");
    skip_or_save_block(&args[n++]);
    if (tok == ')')
      break;
    skip(',');
  }
  return n;
}

static int count_saved_braced_ctor_args(TokenString **args, int max_args)
{
  int n = 0;

  skip('{');
  if (tok == '}')
  {
    next();
    return 0;
  }
  for (;;)
  {
    if (n >= max_args)
      cprime_error("too many overloaded call arguments");
    skip_or_save_block(&args[n++]);
    if (tok == '}')
      break;
    skip(',');
  }
  skip('}');
  return n;
}

static void skip_line_markers(void)
{
  while (tok == TOK_LINENUM)
    next();
}

static int probe_template_call_args(TokenString **args, CType *types,
                                    int max_args)
{
  TokenString *replay = tok_str_alloc();
  int n = 0;

  if (tok != '(')
    cprime_error("template call probe requires argument list");
  tok_str_add2(replay, tok, &tokc);
  next();
  if (tok != ')')
  {
    for (;;)
    {
      if (n >= max_args)
        cprime_error("too many template call arguments");
      skip_or_save_block(&args[n]);
      tok_str_append_without_eof(replay, args[n]);
      n++;
      if (tok == ')')
        break;
      tok_str_add2(replay, tok, &tokc);
      skip(',');
    }
  }
  tok_str_add2(replay, tok, &tokc);
  next();
  if (tok != TOK_EOF)
    tok_str_add2(replay, tok, &tokc);
  tok_str_add(replay, 0);
  infer_saved_arg_types(args, types, n);
  begin_macro(replay, 1);
  next();
  return n;
}

static int saved_arg_is_single_cchar(TokenString *arg)
{
  int i = 0;

  if (!arg || !arg->str)
    return 0;
  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  if (i + 1 >= arg->len || arg->str[i] != TOK_CCHAR)
    return 0;
  i += 2; /* token plus literal payload */
  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  return i < arg->len && arg->str[i] == TOK_EOF;
}

static int saved_arg_is_functional_temporary(TokenString *arg)
{
  int i = 0;

  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= arg->len || arg->str[i] < TOK_UIDENT)
    return 0;
  ++i;
  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  return i < arg->len && arg->str[i] == '(';
}

static int saved_arg_first_real_index(TokenString *arg)
{
  int i = 0;

  while (arg && i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  return i;
}

static int saved_arg_is_braced(TokenString *arg)
{
  int i = saved_arg_first_real_index(arg);
  return arg && i < arg->len && arg->str[i] == '{';
}

static int saved_arg_functional_temporary_type(TokenString *arg, CType *type)
{
  Sym *s;
  int type_tok, struct_tok;
  int i = saved_arg_first_real_index(arg);

  if (!arg || i >= arg->len || arg->str[i] < TOK_UIDENT)
    return 0;
  type_tok = arg->str[i++];
  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= arg->len || arg->str[i] != '(')
    return 0;

  s = sym_find(type_tok);
  if (!s)
    s = sym_find2(global_stack, type_tok);
  if (s && (s->type.t & VT_TYPEDEF)
      && ((s->type.t & VT_BTYPE) == VT_STRUCT))
  {
    type->t = s->type.t & ~VT_TYPEDEF;
    type->ref = s->type.ref;
    type->t |= VT_RVALUE_REFERENCE;
    return 1;
  }

  s = struct_find(type_tok);
  if (!s || (s->type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  type->t = s->type.t | VT_RVALUE_REFERENCE;
  type->ref = s;
  struct_tok = get_struct_type_name_tok(type);
  return struct_tok != 0;
}

static void infer_saved_arg_type(TokenString *arg, CType *type)
{
  SValue *saved_vtop = vtop;
  int saved_nocode_wanted = nocode_wanted;
  int is_single_cchar = saved_arg_is_single_cchar(arg);

  if (saved_arg_is_braced(arg))
  {
    type->t = VT_INT;
    type->ref = NULL;
    return;
  }
  if (saved_arg_functional_temporary_type(arg, type))
    return;

  nocode_wanted++;
  begin_macro(arg, 0);
  next();
  expr_eq();
  *type = vtop->type;
  if (!(vtop->r & VT_LVAL)
      || (((type->t & VT_BTYPE) == VT_STRUCT)
          && saved_arg_is_functional_temporary(arg)))
    type->t |= VT_RVALUE_REFERENCE;
  end_macro();
  if (is_single_cchar)
    type->t = VT_BYTE;
  vtop = saved_vtop;
  nocode_wanted = saved_nocode_wanted;
}

static void infer_saved_arg_types(TokenString **args, CType *types, int nb_args)
{
  int i;

  for (i = 0; i < nb_args; ++i)
    infer_saved_arg_type(args[i], &types[i]);
}

static Sym *resolve_free_func_by_arg_count(int name_tok, int explicit_arg_count)
{
  FreeFuncOverload *o;
  Sym *s, *match = NULL;
  int matches = 0;

  for (o = free_func_overloads; o; o = o->next)
  {
    if (o->name_tok != name_tok || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      continue;
    s = find_global_symbol(o->mangled_tok);
    if (!s || (s->type.t & VT_BTYPE) != VT_FUNC)
      continue;
    use_overload_func_type(s, &o->func_type);
    match = s;
    matches++;
  }
  if (matches > 1)
    cprime_error("ambiguous overloaded function '%s'",
              get_tok_str(name_tok, NULL));
  return match;
}

static int btype_is_arithmetic_scalar(int bt)
{
  return bt >= VT_BYTE && bt <= VT_LLONG
         || bt >= VT_FLOAT && bt <= VT_LDOUBLE
         || bt == VT_BOOL;
}

static int call_arg_matches_param_type(CType *param_type, CType *arg_type)
{
  CType decayed_arg, arg_value_type;
  CType param_value_type;
  CType *param_pointed, *arg_pointed;

  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  param_value_type = *param_type;
  param_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&param_value_type))
    param_value_type = *pointed_type(&param_value_type);
  if (is_reference_type(param_type))
  {
    if ((param_type->t & VT_RVALUE_REFERENCE)
        && !(arg_type->t & VT_RVALUE_REFERENCE))
      return 0;
    param_pointed = pointed_type(param_type);
    if (is_compatible_unqualified_types(param_pointed, &arg_value_type))
      return 1;
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && class_has_single_arg_constructor_for(param_pointed, &arg_value_type))
      return 1;
  }
  if (compare_types(param_type, &arg_value_type, 1))
    return 1;
  if (btype_is_arithmetic_scalar(param_value_type.t & VT_BTYPE)
      && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
    return 1;
  if ((param_type->t & VT_BTYPE) == VT_STRUCT
      && class_has_single_arg_constructor_for(param_type, &arg_value_type))
    return 1;
  if ((param_type->t & VT_BTYPE) == VT_PTR
      && (arg_value_type.t & VT_BTYPE) == VT_PTR)
  {
    param_pointed = pointed_type(param_type);
    arg_pointed = pointed_type(&arg_value_type);
    if (is_compatible_unqualified_types(param_pointed, arg_pointed)
        && !(arg_pointed->t & (VT_CONSTANT | VT_VOLATILE)
             & ~(param_pointed->t & (VT_CONSTANT | VT_VOLATILE))))
      return 1;
  }
  if ((param_type->t & VT_BTYPE) == VT_PTR
      && (arg_value_type.t & VT_BTYPE) == VT_PTR
      && (arg_value_type.t & VT_ARRAY))
  {
    decayed_arg = arg_value_type;
    decayed_arg.t &= ~VT_ARRAY;
    return call_arg_matches_param_type(param_type, &decayed_arg);
  }
  return 0;
}

static int call_arg_match_rank(CType *param_type, CType *arg_type)
{
  CType arg_value_type;
  CType param_value_type;
  CType *param_pointed;

  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  param_value_type = *param_type;
  param_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&param_value_type))
    param_value_type = *pointed_type(&param_value_type);
  if (is_reference_type(param_type))
  {
    if ((param_type->t & VT_RVALUE_REFERENCE)
        && !(arg_type->t & VT_RVALUE_REFERENCE))
      return -1;
    param_pointed = pointed_type(param_type);
    if (is_compatible_unqualified_types(param_pointed, &arg_value_type))
    {
      if ((param_type->t & VT_RVALUE_REFERENCE)
          && (arg_type->t & VT_RVALUE_REFERENCE))
        return 0;
      if ((arg_type->t & VT_RVALUE_REFERENCE)
          && (param_pointed->t & VT_CONSTANT))
        return 2;
      return 1;
    }
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && class_has_single_arg_constructor_for(param_pointed, &arg_value_type))
      return 4;
  }
  if (compare_types(param_type, &arg_value_type, 1))
    return 1;
  if (btype_is_arithmetic_scalar(param_value_type.t & VT_BTYPE)
      && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
    return 4;
  if ((param_type->t & VT_BTYPE) == VT_STRUCT
      && class_has_single_arg_constructor_for(param_type, &arg_value_type))
    return 4;
  return call_arg_matches_param_type(param_type, arg_type) ? 3 : -1;
}

static Sym *resolve_free_func_by_arg_types(int name_tok, CType *arg_types,
                                           int explicit_arg_count)
{
  FreeFuncOverload *o;
  Sym *s, *arg, *match = NULL;
  int i, ok, matches = 0;

  for (o = free_func_overloads; o; o = o->next)
  {
    if (o->name_tok != name_tok || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      continue;
    s = find_global_symbol(o->mangled_tok);
    if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
      continue;
    use_overload_func_type(s, &o->func_type);
    ok = 1;
    arg = s->type.ref->next;
    for (i = 0; i < explicit_arg_count; ++i)
    {
      if (!arg || !call_arg_matches_param_type(&arg->type, &arg_types[i]))
      {
        ok = 0;
        break;
      }
      arg = arg->next;
    }
    if (!ok)
      continue;
    for (; arg; arg = arg->next)
      if (!arg->default_arg)
      {
        ok = 0;
        break;
      }
    if (!ok)
      continue;
    match = s;
    matches++;
  }
  if (matches > 1)
    cprime_error("ambiguous overloaded function '%s'",
              get_tok_str(name_tok, NULL));
  return match;
}

static int count_param_tokens(TokenString *params)
{
  int i, n = 0, saw = 0, level = 0;

  if (!params)
    return 0;
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    int t = params->str[i];
    if (level == 0 && t == ',')
    {
      if (saw)
        ++n;
      saw = 0;
      continue;
    }
    if (t == '(' || t == '[')
      ++level;
    else if (t == ')' || t == ']')
      --level;
    else if (level == 0)
      saw = 1;
  }
  if (saw)
    ++n;
  return n;
}

static int count_call_arg_tokens_in_str(TokenString *str, int paren_index)
{
  int i, n = 0, saw = 0, level = 0;

  if (!str || paren_index < 0 || paren_index >= str->len
      || str->str[paren_index] != '(')
    return -1;
  for (i = paren_index + 1; i < str->len && str->str[i] != TOK_EOF; ++i)
  {
    int t = str->str[i];
    if (level == 0 && t == ')')
      return saw ? n + 1 : 0;
    if (level == 0 && t == ',')
    {
      ++n;
      saw = 0;
      continue;
    }
    if (t == '(' || t == '[')
      ++level;
    else if (t == ')' || t == ']')
      --level;
    else if (level == 0)
      saw = 1;
  }
  return -1;
}

static int make_lifecycle_func_tok_for_params(int struct_tok, int method_tok,
                                              TokenString *params,
                                              int prefer_existing)
{
  char name[512];
  int i, has_suffix = 0, param_count, same_count = 0, other_count = 0;
  int same_mangled_tok = 0;
  MemberFuncOverload *o;

  snprintf(name, sizeof(name), "%s_%s",
           get_tok_str(struct_tok, NULL),
           get_tok_str(method_tok, NULL));
  param_count = count_param_tokens(params);
  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && o->explicit_arg_count == param_count)
    {
      same_count++;
      if (!same_mangled_tok)
        same_mangled_tok = o->mangled_tok;
    }
    else if (o->struct_tok == struct_tok && o->method_tok == method_tok)
      other_count++;
  if (same_count == 1 && prefer_existing)
    return same_mangled_tok;
  if (!params || params->len <= 1)
  {
    if (!same_count && !other_count && !sym_find(tok_alloc_const(name))
        && !sym_find2(global_stack, tok_alloc_const(name)))
      return tok_alloc_const(name);
    pstrcat(name, sizeof(name), "_0args");
    return tok_alloc_const(name);
  }
  if (!same_count && !other_count && !sym_find(tok_alloc_const(name))
      && !sym_find2(global_stack, tok_alloc_const(name)))
    return tok_alloc_const(name);
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    if (params->str[i] >= TOK_IDENT && struct_find(params->str[i]))
    {
      pstrcat(name, sizeof(name), "_struct_");
      pstrcat(name, sizeof(name), get_tok_str(params->str[i], NULL));
      has_suffix = 1;
      if (i + 1 < params->len && params->str[i + 1] == '<')
      {
        int level = 1;
        i += 2;
        while (i < params->len && params->str[i] != TOK_EOF && level > 0)
        {
          if (params->str[i] == '<')
            level++;
          else if (params->str[i] == TOK_GT || params->str[i] == TOK_SAR)
            level--;
          i++;
        }
        i--;
      }
      continue;
    }
    switch (params->str[i])
    {
    case TOK_CONST1:
    case TOK_CONST2:
    case TOK_CONST3:
      pstrcat(name, sizeof(name), "_const");
      has_suffix = 1;
      break;
    case '&':
      pstrcat(name, sizeof(name), "_ref");
      has_suffix = 1;
      break;
    case TOK_LAND:
      pstrcat(name, sizeof(name), "_rref");
      has_suffix = 1;
      break;
    case TOK_INT:
      pstrcat(name, sizeof(name), "_int");
      has_suffix = 1;
      break;
    case TOK_FLOAT:
      pstrcat(name, sizeof(name), "_float");
      has_suffix = 1;
      break;
    case TOK_DOUBLE:
      pstrcat(name, sizeof(name), "_double");
      has_suffix = 1;
      break;
    case TOK_CHAR:
      pstrcat(name, sizeof(name), "_char");
      has_suffix = 1;
      break;
    case TOK_BOOL:
    case TOK_BOOL2:
      pstrcat(name, sizeof(name), "_bool");
      has_suffix = 1;
      break;
    case TOK_STRUCT:
      if (i + 1 < params->len)
      {
        pstrcat(name, sizeof(name), "_struct_");
        pstrcat(name, sizeof(name), get_tok_str(params->str[++i], NULL));
        has_suffix = 1;
      }
      break;
    case '*':
      pstrcat(name, sizeof(name), "_ptr");
      break;
    default:
      break;
    }
  }
  if (!has_suffix)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "_%dargs", count_param_tokens(params));
    pstrcat(name, sizeof(name), buf);
  }
  return tok_alloc_const(name);
}

static void note_raw_lifecycle_overload(int struct_tok, int method_tok,
                                        int mangled_tok, TokenString *params)
{
  MemberFuncOverload *o;

  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && o->mangled_tok == mangled_tok)
      return;
  o = cprime_mallocz(sizeof(*o));
  o->struct_tok = struct_tok;
  o->method_tok = method_tok;
  o->mangled_tok = mangled_tok;
  o->explicit_arg_count = count_param_tokens(params);
  o->next = member_func_overloads;
  member_func_overloads = o;
}

static const char *cpp_operator_name_from_tok(int op)
{
  switch (op)
  {
  case '+': return "operator+";
  case '-': return "operator-";
  case '*': return "operator*";
  case '/': return "operator/";
  case '%':
  case TOK_UMOD: return "operator%";
  case '&': return "operator&";
  case '|': return "operator|";
  case '^': return "operator^";
  case TOK_EQ: return "operator==";
  case TOK_NE: return "operator!=";
  case TOK_LT: return "operator<";
  case TOK_LE: return "operator<=";
  case TOK_GT: return "operator>";
  case TOK_GE: return "operator>=";
  case '=': return "operator=";
  case TOK_A_ADD: return "operator+=";
  case TOK_A_SUB: return "operator-=";
  case TOK_A_MUL: return "operator*=";
  case TOK_A_DIV: return "operator/=";
  case TOK_A_MOD: return "operator%=";
  case TOK_A_AND: return "operator&=";
  case TOK_A_OR: return "operator|=";
  case TOK_A_XOR: return "operator^=";
  case TOK_A_SHL: return "operator<<=";
  case TOK_A_SAR: return "operator>>=";
  case '!': return "operator!";
  case TOK_SHR:
  case TOK_SAR: return "operator>>";
  case TOK_SHL: return "operator<<";
  default:
    return NULL;
  }
}

static int parse_cpp_operator_method_tok(void)
{
  const char *name;

  if (tok != TOK_OPERATOR)
    cprime_error("operator");
  next();
  if (tok == '[')
  {
    next();
    skip(']');
    return tok_alloc_const("operator[]");
  }
  name = cpp_operator_name_from_tok(tok);
  if (!name)
    cprime_error("unsupported operator overload");
  next();
  return tok_alloc_const(name);
}

static int get_cpp_binary_operator_method_tok(int op)
{
  const char *name = cpp_operator_name_from_tok(op);
  return name ? tok_alloc_const(name) : 0;
}

static void finish_cpp_member_func_call(Sym *func_type, int nb_args)
{
  SValue ret;
  int ret_nregs, ret_align, regsize, variadic;
  int size, align, n, r, t;

  ret.r2 = VT_CONST;
  ret_align = regsize = 0;
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
      vseti(VT_LOCAL, loc);
#ifdef CONFIG_CPRIME_BCHECK
      if (cprime_state->do_bounds_check)
        --loc;
#endif
      ret.c = vtop->c;
      if (ret_nregs < 0)
        vtop--;
      else
      {
        vrott(nb_args + 1);
        nb_args++;
      }
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

  vcheck_cmp();
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
        vstore();
        vtop--;
        if (--ret_nregs == 0)
          break;
        offset += regsize;
      }
      vset(&func_type->type, VT_LOCAL | VT_LVAL, addr);
    }

    t = func_type->type.t & VT_BTYPE;
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

static int try_call_cpp_binary_operator(int op)
{
  int method_tok;
  Sym *func_sym, *func_type, *sa;
  CType saved_ret_type;

  if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
    return 0;

  method_tok = get_cpp_binary_operator_method_tok(op);
  if (!method_tok)
    return 0;
  instantiate_template_member_for_call(&vtop[-1].type, method_tok,
                                       &vtop->type, 1);
  func_sym = resolve_member_func_by_arg_types(&vtop[-1].type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
    func_sym = resolve_member_func(&vtop[-1].type, method_tok);
  if (!func_sym)
  {
    int lhs_struct_tok = get_struct_type_name_tok(&vtop[-1].type);
    int rhs_struct_tok = get_struct_type_name_tok(&vtop->type);
    int dummy_ofs;
    Sym *declared_op = NULL;
    int has_member_template_op = 0;
    if (lhs_struct_tok)
    {
      declared_op = find_field_try(&vtop[-1].type, method_tok, &dummy_ofs);
      if (!declared_op)
        declared_op = find_field_try(&vtop[-1].type, method_tok | SYM_FIELD,
                                     &dummy_ofs);
      if (declared_op && (declared_op->type.t & VT_BTYPE) != VT_FUNC)
        declared_op = NULL;
      has_member_template_op =
        class_or_inst_has_member_template_name(lhs_struct_tok, method_tok);
    }
    if (lhs_struct_tok
        && (op == '+' || op == '-' || op == '*' || op == '/'
            || op == TOK_SHL || op == TOK_SAR)
        && (lhs_struct_tok == rhs_struct_tok
            || declared_op || has_member_template_op
            || (rhs_struct_tok == 0
                && (vtop->type.t & VT_BTYPE) != VT_STRUCT)))
    {
      vpop();
      return 1;
    }
    return 0;
  }
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");

  func_type = func_sym->type.ref;
  saved_ret_type = func_type->type;
  if ((saved_ret_type.t & VT_BTYPE) != VT_STRUCT
      && (op == '+' || op == '-' || op == '*' || op == '/'
          || op == TOK_SHL || op == TOK_SAR))
  {
    vpop();
    return 1;
  }
  sa = func_type->next;
  if (!sa || !sa->next || sa->next->next)
    cprime_error("operator overload must have exactly one explicit parameter");

  /* Convert lhs/rhs into member call args: operator(this, rhs). */
  vswap();
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  vswap();
  save_lvalues();
  gfunc_param_typed(func_type, sa->next);
  vswap();
  gfunc_param_typed(func_type, sa);
  vswap();
  vpushsym(&func_sym->type, func_sym);
  vrott(3);
  finish_cpp_member_func_call(func_type, 2);
  return 1;
}

static int try_call_cpp_unary_operator(int op)
{
  int method_tok = get_cpp_binary_operator_method_tok(op);
  Sym *func_sym;
  Sym *func_type;
  Sym *sa;

  if (!method_tok)
    return 0;
  if ((vtop->type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  func_sym = resolve_member_func_by_arg_count(&vtop->type, method_tok, 0);
  if (!func_sym)
    func_sym = resolve_member_func(&vtop->type, method_tok);
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");
  func_type = func_sym->type.ref;
  sa = func_type->next;
  if (!sa || sa->next)
    cprime_error("unary operator overload must have no explicit parameters");
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  save_lvalues();
  vpushsym(&func_sym->type, func_sym);
  vswap();
  finish_cpp_member_func_call(func_type, 1);
  return 1;
}

static int try_call_cpp_unary_minus_operator(void)
{
  return try_call_cpp_unary_operator('-');
}

static int try_call_cpp_index_operator(void)
{
  int method_tok = tok_alloc_const("operator[]");
  Sym *func_sym;
  Sym *func_type;
  Sym *sa;

  if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  func_sym = resolve_member_func_by_arg_types(&vtop[-1].type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
    func_sym = resolve_member_func(&vtop[-1].type, method_tok);
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");
  func_type = func_sym->type.ref;
  sa = func_type->next;
  if (!sa || !sa->next || sa->next->next)
    cprime_error("index operator overload must have exactly one explicit parameter");
  vswap();
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  vswap();
  save_lvalues();
  gfunc_param_typed(func_type, sa->next);
  vswap();
  gfunc_param_typed(func_type, sa);
  vswap();
  vpushsym(&func_sym->type, func_sym);
  vrott(3);
  finish_cpp_member_func_call(func_type, 2);
  return 1;
}

static int try_call_cpp_assignment_operator(void)
{
  int method_tok = tok_alloc_const("operator=");
  Sym *func_sym;
  Sym *func_type;
  Sym *sa, *this_arg, *value_arg;

  if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  func_sym = resolve_member_func_by_arg_types(&vtop[-1].type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
    func_sym = resolve_member_func(&vtop[-1].type, method_tok);
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");
  func_type = func_sym->type.ref;
  sa = func_type->next;
  if (!sa || !sa->next || sa->next->next)
    cprime_error("operator overload must have exactly one explicit parameter");
  this_arg = sa;
  value_arg = sa->next;
  vswap();
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  vswap();
  gfunc_param_typed(func_type, value_arg);
  vswap();
  gfunc_param_typed(func_type, this_arg);
  vswap();
  vpushsym(&func_sym->type, func_sym);
  vrott(3);
  finish_cpp_member_func_call(func_type, 2);
  return 1;
}

static int try_call_cpp_compound_assign_operator(int op)
{
  int method_tok = get_cpp_binary_operator_method_tok(op);
  Sym *func_sym;
  Sym *func_type;
  Sym *sa;

  if (!method_tok)
    return 0;
  if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  func_sym = resolve_member_func_by_arg_types(&vtop[-1].type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
    func_sym = resolve_member_func(&vtop[-1].type, method_tok);
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");
  func_type = func_sym->type.ref;
  sa = func_type->next;
  if (!sa || !sa->next || sa->next->next)
    cprime_error("operator overload must have exactly one explicit parameter");
  vswap();
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  vswap();
  save_lvalues();
  gfunc_param_typed(func_type, sa->next);
  vswap();
  gfunc_param_typed(func_type, sa);
  vswap();
  vpushsym(&func_sym->type, func_sym);
  vrott(3);
  finish_cpp_member_func_call(func_type, 2);
  return 1;
}

static CType make_lifecycle_func_type(CType *struct_type)
{
  CType void_type, func_type, this_type;
  Sym *fref, *param;
  Sym *saved_ls;

  void_type.t = VT_VOID;
  void_type.ref = NULL;
  func_type.t = VT_FUNC;
  /* Function-type construction symbols must not leak onto a live local
     scope: an enclosing scope pop would unlink the 'this'-named parameter
     symbols and clobber the token table entry for the real 'this'.  Push
     them on the global stack (as at file scope) instead. */
  saved_ls = local_stack;
  local_stack = NULL;
  func_type.ref = fref = sym_push(SYM_FIELD, &void_type, 0, 0);
  fref->f.func_call = FUNC_CDECL;
  fref->f.func_type = FUNC_NEW;

  this_type = *struct_type;
  mk_pointer(&this_type);
  param = sym_push(SYM_FIELD, &this_type, VT_LOCAL | VT_LVAL, 0);
  param->v = tok_alloc_const("this");
  fref->next = param;
  local_stack = saved_ls;
  return func_type;
}

static Sym *declare_lifecycle_func(CType *struct_type, int method_tok)
{
  int struct_tok, mangled_tok;
  CType func_type;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("constructors/destructors require a named struct/class");
  if (method_tok == TOK_CONSTRUCTOR1)
    struct_type->ref->a.lifecycle_ctor = 1;
  else if (method_tok == TOK_DESTRUCTOR1)
    struct_type->ref->a.lifecycle_dtor = 1;
  mangled_tok = make_lifecycle_func_tok_for_params(struct_tok, method_tok, NULL, 1);
  func_type = make_lifecycle_func_type(struct_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok, &func_type);
  return external_global_sym(mangled_tok, &func_type);
}

static Sym *resolve_lifecycle_func(CType *type, int method_tok)
{
  int struct_tok, mangled_tok;
  Sym *s;

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return NULL;
  if (method_tok == TOK_CONSTRUCTOR1 && !type->ref->a.lifecycle_ctor)
    return NULL;
  if (method_tok == TOK_DESTRUCTOR1 && !type->ref->a.lifecycle_dtor)
    return NULL;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  instantiate_template_member_for_call(type, method_tok, NULL,
                                       method_tok == TOK_CONSTRUCTOR1 ? 0 : -1);
  mangled_tok = make_lifecycle_func_tok_for_params(struct_tok, method_tok, NULL, 1);
  s = sym_find(mangled_tok);
  if (!s)
    s = sym_find2(global_stack, mangled_tok);
  if (!s)
    return NULL;
  if ((s->type.t & VT_BTYPE) != VT_FUNC)
    cprime_error("lifecycle target '%s' is not declared as function",
              get_tok_str(mangled_tok, NULL));
  return s;
}

static int lifecycle_has_only_this_param(Sym *func_sym)
{
  Sym *param;

  if (!func_sym || (func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    return 0;
  param = func_sym->type.ref->next;
  return param && !param->next;
}

static Sym *resolve_autodtor_func(CType *type)
{
  Sym *s = resolve_lifecycle_func(type, TOK_DESTRUCTOR1);
  if (!lifecycle_has_only_this_param(s))
    return NULL;
  return s;
}

static Sym *resolve_autoctor_func(CType *type)
{
  Sym *s = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1, 0);
  if (!s)
    s = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
  if (s && !lifecycle_has_only_this_param(s)
      && member_func_min_arg_count(&s->type) > 0)
    return NULL;
  return s;
}

static void push_void_value(void)
{
  vpushi(0);
  vtop->type.t = VT_VOID;
}

static int is_pseudo_destructor_type_tok(int t)
{
  return t >= TOK_UIDENT || t == TOK_VOID || t == TOK_CHAR || t == TOK_SHORT
         || t == TOK_INT || t == TOK_LONG || t == TOK_FLOAT
         || t == TOK_DOUBLE || t == TOK_BOOL;
}

static void call_explicit_destructor(CType *type, int name_tok)
{
  int struct_tok;
  Sym *func_sym;

  if ((type->t & VT_BTYPE) != VT_STRUCT)
  {
    vpop();
    push_void_value();
    return;
  }

  struct_tok = get_struct_type_name_tok(type);
  if (struct_tok && name_tok != struct_tok)
  {
    char expected[512];
    snprintf(expected, sizeof(expected), "%s__",
             get_tok_str(name_tok, NULL));
    if (strncmp(get_tok_str(struct_tok, NULL), expected, strlen(expected)))
      cprime_error("destructor name must match class name");
  }

  func_sym = resolve_lifecycle_func(type, TOK_DESTRUCTOR1);
  if (!func_sym)
  {
    vpop();
    push_void_value();
    return;
  }
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("destructor target is not declared as function");
  if (!lifecycle_has_only_this_param(func_sym))
    cprime_error("explicit destructor call requires destructor with no parameters");

  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&func_sym->type, func_sym);
  vswap();
  finish_cpp_member_func_call(func_sym->type.ref, 1);
}

static void call_lifecycle_constructor_raw(CType *type, int r, int addr, Sym *sym,
                                           Sym *ctor_func)
{
  Sym *func_type, *sa;
  TokenString *call_args[32];
  CType call_arg_types[32];
  int nb_args, call_arg_count, ai;

  skip('(');
  skip_line_markers();
  if (tok == '{')
    call_arg_count = count_saved_braced_ctor_args(call_args, 32);
  else
    call_arg_count = count_saved_call_args(call_args, 32);
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  {
    Sym *overload = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                     call_arg_types,
                                                     call_arg_count);
    if (!overload)
      overload = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                        call_arg_types,
                                                        call_arg_count);
    if (!overload && call_arg_count == 1
        && is_same_template_family_conversion_ctor(type, &call_arg_types[0]))
    {
      next();
      return;
    }
    if (!overload)
      overload = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                  call_arg_count);
    if (overload)
      ctor_func = overload;
  }
  if (!ctor_func || (ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");


  save_lvalues();
  vpushsym(&ctor_func->type, ctor_func);
  vset(type, r, addr);
  if (sym)
    vtop->sym = sym;
  mk_pointer(&vtop->type);
  gaddrof();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    if (saved_arg_is_braced(call_args[ai]) && sa
        && (((sa->type.t & VT_BTYPE) == VT_STRUCT)
            || (is_reference_type(&sa->type)
                && ((pointed_type(&sa->type)->t & VT_BTYPE) == VT_STRUCT))))
    {
      CType temp_type;
      int arg_size, arg_align, arg_r2, arg_addr;
      temp_type = is_reference_type(&sa->type) ? *pointed_type(&sa->type)
                                               : sa->type;
      arg_size = type_size(&temp_type, &arg_align);
      arg_addr = get_temp_local_var(arg_size, arg_align, &arg_r2);
      vset(&temp_type, VT_LOCAL | VT_LVAL, arg_addr);
      vtop->r2 = arg_r2;
    }
    else
    {
      begin_macro(call_args[ai], 1);
      next();
      expr_eq();
      end_macro();
    }
    gfunc_param_typed(func_type, sa);
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
  drop_leaked_call_target(ctor_func);
}

static void call_lifecycle_constructor_noargs(CType *type, int r, int addr, Sym *sym,
                                              Sym *ctor_func)
{
  Sym *func_type, *sa;
  int nb_args, saved_tok;
  CValue saved_tokc;

  if (!ctor_func || (ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");

  save_lvalues();
  vpushsym(&ctor_func->type, ctor_func);
  vset(type, r, addr);
  if (sym)
    vtop->sym = sym;
  mk_pointer(&vtop->type);
  gaddrof();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;
  saved_tok = tok;
  saved_tokc = tokc;

  while (sa)
  {
    emit_default_arg(func_type, sa);
    nb_args++;
    sa = sa->next;
  }
  tok = saved_tok;
  tokc = saved_tokc;

  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
}

static void call_lifecycle_constructor_members(CType *type, int r, int addr)
{
  Sym *field, *ctor_func;
  int i, align, elem_size;
  int struct_tok;
  CType *elem_type;

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      call_lifecycle_constructor_members(elem_type, r, addr + i * elem_size);
      ctor_func = resolve_lifecycle_func(elem_type, TOK_CONSTRUCTOR1);
      if (ctor_func)
        call_lifecycle_constructor_noargs(elem_type, r, addr + i * elem_size,
                                          NULL, ctor_func);
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return;
  struct_tok = get_struct_type_name_tok(type);
  if (struct_tok && struct_has_member_init_list(struct_tok))
    return;

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_BTYPE) != VT_STRUCT
        && !((field->type.t & VT_ARRAY)
             && (field->type.t & VT_BTYPE) == VT_PTR))
      continue;

    call_lifecycle_constructor_members(&field->type, r, addr + field->c);
    ctor_func = resolve_lifecycle_func(&field->type, TOK_CONSTRUCTOR1);
    if (ctor_func)
      call_lifecycle_constructor_noargs(&field->type, r, addr + field->c, NULL, ctor_func);
  }
}

static void push_field_lvalue_from_base_ptr(SValue *base_ptr, CType *field_type,
                                            int offset)
{
  vpushv(base_ptr);
  if (vtop->r & VT_LVAL)
    gv(RC_INT);
  vtop->type = char_pointer_type;
  vpushi(offset);
  gen_op('+');
  vtop->type = *field_type;
  if (!(vtop->type.t & VT_ARRAY) && (vtop->type.t & VT_BTYPE) != VT_FUNC)
    vtop->r |= VT_LVAL;
  maybe_indir_reference();
}

static void spill_pointer_svalue_to_local(SValue *ptr)
{
  CType ptr_type = ptr->type;
  SValue local_slot;
  int addr;

  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  addr = loc;
  vset(&ptr_type, VT_LOCAL | VT_LVAL, addr);
  local_slot = *vtop;
  vpushv(ptr);
  vstore();
  *ptr = local_slot;
  vtop--;
}

static void stabilize_pointer_svalue_pair(SValue *dst_ptr, SValue *src_ptr)
{
  vpushv(dst_ptr);
  vpushv(src_ptr);
  save_regs(0);
  *dst_ptr = vtop[-1];
  *src_ptr = vtop[0];
  vtop -= 2;
}

static void call_lifecycle_constructor_noargs_expr(CType *type, Sym *ctor_func)
{
  Sym *func_type, *sa;

  if (!ctor_func)
    return;
  if ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");

  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&ctor_func->type, ctor_func);
  vswap();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  if (sa)
    cprime_error("too few arguments to constructor");
  gfunc_call(1);
  drop_leaked_call_target(ctor_func);
}

static void call_lifecycle_constructor_members_base_ptr(CType *type,
                                                        SValue *base_ptr,
                                                        int base_offset)
{
  Sym *field, *ctor_func;
  int i, align, elem_size;
  int struct_tok;
  CType *elem_type;

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      int elem_offset = base_offset + i * elem_size;
      call_lifecycle_constructor_members_base_ptr(elem_type, base_ptr, elem_offset);
      ctor_func = resolve_lifecycle_func(elem_type, TOK_CONSTRUCTOR1);
      if (ctor_func)
      {
        push_field_lvalue_from_base_ptr(base_ptr, elem_type, elem_offset);
        call_lifecycle_constructor_noargs_expr(elem_type, ctor_func);
      }
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return;
  struct_tok = get_struct_type_name_tok(type);
  if (struct_tok && struct_has_member_init_list(struct_tok))
    return;

  for (field = type->ref->next; field; field = field->next)
  {
    int field_offset;
    if ((field->type.t & VT_BTYPE) != VT_STRUCT
        && !((field->type.t & VT_ARRAY)
             && (field->type.t & VT_BTYPE) == VT_PTR))
      continue;

    field_offset = base_offset + field->c;
    call_lifecycle_constructor_members_base_ptr(&field->type, base_ptr, field_offset);
    ctor_func = resolve_lifecycle_func(&field->type, TOK_CONSTRUCTOR1);
    if (ctor_func)
    {
      push_field_lvalue_from_base_ptr(base_ptr, &field->type, field_offset);
      call_lifecycle_constructor_noargs_expr(&field->type, ctor_func);
    }
  }
}

static int struct_needs_memberwise_assignment(CType *type)
{
  Sym *field;
  CType *elem_type;
  int method_tok, align, elem_size;

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    return struct_needs_memberwise_assignment(elem_type);
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return 0;

  method_tok = tok_alloc_const("operator=");
  if (resolve_member_func(type, method_tok))
    return 1;

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_ARRAY) && (field->type.t & VT_BTYPE) == VT_PTR
        && field->type.ref && field->type.ref->c > 0)
    {
      elem_type = pointed_type(&field->type);
      elem_size = type_size(elem_type, &align);
      if (elem_size >= 0 && struct_needs_memberwise_assignment(elem_type))
        return 1;
      continue;
    }
    if (struct_needs_memberwise_assignment(&field->type))
      return 1;
  }
  return 0;
}

static void assign_one_lvalue_pair(CType *type, SValue *dst_ptr, SValue *src_ptr,
                                   int offset)
{
  SValue *saved_vtop = vtop;
  push_field_lvalue_from_base_ptr(dst_ptr, type, offset);
  push_field_lvalue_from_base_ptr(src_ptr, type, offset);
  if (!try_call_cpp_assignment_operator())
    vstore();
  vtop = saved_vtop;
}

static void assign_struct_memberwise_from_base_ptr(CType *type, SValue *dst_ptr,
                                                   SValue *src_ptr,
                                                   int base_offset)
{
  Sym *field;
  CType *elem_type;
  int i, align, elem_size;
  SValue stable_dst_ptr, stable_src_ptr;

  if (((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
       && type->ref && type->ref->c > 0)
      || ((type->t & VT_BTYPE) == VT_STRUCT && type->ref))
  {
    stable_dst_ptr = *dst_ptr;
    stable_src_ptr = *src_ptr;
    stabilize_pointer_svalue_pair(&stable_dst_ptr, &stable_src_ptr);
    dst_ptr = &stable_dst_ptr;
    src_ptr = &stable_src_ptr;
  }

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      int elem_offset = base_offset + i * elem_size;
      assign_one_lvalue_pair(elem_type, dst_ptr, src_ptr, elem_offset);
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
  {
    assign_one_lvalue_pair(type, dst_ptr, src_ptr, base_offset);
    return;
  }

  for (field = type->ref->next; field; field = field->next)
  {
    int field_offset = base_offset + field->c;
    assign_one_lvalue_pair(&field->type, dst_ptr, src_ptr, field_offset);
  }
}

static int struct_needs_memberwise_copy(CType *type)
{
  Sym *field;
  CType *elem_type;
  int align, elem_size;

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    return struct_needs_memberwise_copy(elem_type);
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return 0;

  if (resolve_copy_constructor_func(type, type))
    return 1;

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_ARRAY) && (field->type.t & VT_BTYPE) == VT_PTR
        && field->type.ref && field->type.ref->c > 0)
    {
      elem_type = pointed_type(&field->type);
      elem_size = type_size(elem_type, &align);
      if (elem_size >= 0 && struct_needs_memberwise_copy(elem_type))
        return 1;
      continue;
    }
    if (struct_needs_memberwise_copy(&field->type))
      return 1;
  }
  return 0;
}

static void copy_construct_one_lvalue_pair(CType *type, SValue *dst_ptr,
                                           SValue *src_ptr, int offset)
{
  SValue *saved_vtop = vtop;
  Sym *ctor_func, *ctor_type, *sa;

  ctor_func = resolve_copy_constructor_func(type, type);
  if (!ctor_func)
  {
    assign_one_lvalue_pair(type, dst_ptr, src_ptr, offset);
    return;
  }
  if ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("copy constructor target is not declared as function");

  call_lifecycle_constructor_members_base_ptr(type, dst_ptr, offset);
  push_field_lvalue_from_base_ptr(dst_ptr, type, offset);
  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&ctor_func->type, ctor_func);
  vswap();

  ctor_type = ctor_func->type.ref;
  sa = ctor_type->next;
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;

  push_field_lvalue_from_base_ptr(src_ptr, type, offset);
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;
  if (sa)
    cprime_error("too few arguments to copy constructor");

  vcheck_cmp();
  gfunc_call(2);
  vtop = saved_vtop;
}

static void copy_construct_struct_memberwise_from_base_ptr(CType *type,
                                                           SValue *dst_ptr,
                                                           SValue *src_ptr,
                                                           int base_offset)
{
  Sym *field;
  CType *elem_type;
  int i, align, elem_size;
  SValue stable_dst_ptr, stable_src_ptr;

  if (((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
       && type->ref && type->ref->c > 0)
      || ((type->t & VT_BTYPE) == VT_STRUCT && type->ref))
  {
    stable_dst_ptr = *dst_ptr;
    stable_src_ptr = *src_ptr;
    stabilize_pointer_svalue_pair(&stable_dst_ptr, &stable_src_ptr);
    dst_ptr = &stable_dst_ptr;
    src_ptr = &stable_src_ptr;
  }

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      int elem_offset = base_offset + i * elem_size;
      if ((elem_type->t & VT_BTYPE) == VT_STRUCT
          && !resolve_copy_constructor_func(elem_type, elem_type))
        copy_construct_struct_memberwise_from_base_ptr(elem_type, dst_ptr,
                                                       src_ptr, elem_offset);
      else
        copy_construct_one_lvalue_pair(elem_type, dst_ptr, src_ptr,
                                       elem_offset);
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
  {
    copy_construct_one_lvalue_pair(type, dst_ptr, src_ptr, base_offset);
    return;
  }

  for (field = type->ref->next; field; field = field->next)
  {
    int field_offset = base_offset + field->c;
    if ((field->type.t & VT_BTYPE) == VT_STRUCT
        && !resolve_copy_constructor_func(&field->type, &field->type))
      copy_construct_struct_memberwise_from_base_ptr(&field->type, dst_ptr,
                                                     src_ptr, field_offset);
    else
      copy_construct_one_lvalue_pair(&field->type, dst_ptr, src_ptr,
                                     field_offset);
  }
}

static void register_struct_cleanups(CType *type, Sym *sym)
{
  Sym *field, *cleanup_func, *cls, *field_sym;
  int i, align, elem_size;
  CType *elem_type;

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return;

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_ARRAY) && (field->type.t & VT_BTYPE) == VT_PTR
        && field->type.ref && field->type.ref->c > 0)
    {
      elem_type = pointed_type(&field->type);
      elem_size = type_size(elem_type, &align);
      if (elem_size < 0)
        continue;
      for (i = field->type.ref->c - 1; i >= 0; --i)
      {
        CType elem_storage_type = *elem_type;

        if ((elem_type->t & VT_BTYPE) == VT_STRUCT)
          register_struct_cleanups(elem_type, sym);

        cleanup_func = resolve_autodtor_func(elem_type);
        if (!cleanup_func)
          continue;

        field_sym = sym_copy(sym, &local_stack);
        if ((field_sym->v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
          sym_link(field_sym, 0);
        field_sym->v = SYM_FIELD;
        field_sym->type = elem_storage_type;
        field_sym->c = sym->c + field->c + i * elem_size;

        cls = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
        cls->cleanup_sym = field_sym;
        cls->cleanup_func = cleanup_func;
        cls->next = cur_scope->cl.s;
        cur_scope->cl.s = cls;
      }
      continue;
    }

    if ((field->type.t & VT_BTYPE) == VT_STRUCT)
      register_struct_cleanups(&field->type, sym);

    cleanup_func = resolve_autodtor_func(&field->type);
    if (!cleanup_func)
      continue;

    field_sym = sym_copy(sym, &local_stack);
    if ((field_sym->v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
      sym_link(field_sym, 0);
    field_sym->v = SYM_FIELD;
    field_sym->type = field->type;
    field_sym->c = sym->c + field->c;

    cls = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
    cls->cleanup_sym = field_sym;
    cls->cleanup_func = cleanup_func;
    cls->next = cur_scope->cl.s;
    cur_scope->cl.s = cls;
  }
}

static void call_lifecycle_constructor(CType *type, int r, int addr, Sym *sym,
                                       Sym *ctor_func)
{
  call_lifecycle_constructor_members(type, r, addr);
  call_lifecycle_constructor_raw(type, r, addr, sym, ctor_func);
}

static void call_lifecycle_constructor_saved_args(CType *type, int r, int addr,
                                                  Sym *sym, Sym *ctor_func,
                                                  TokenString **call_args,
                                                  int call_arg_count)
{
  Sym *func_type, *sa;
  CType call_arg_types[32];
  int nb_args, ai, saved_delim;

  saved_delim = tok;
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  if (tok == TOK_EOF)
  {
    next();
    unget_tok(saved_delim);
  }
  {
    Sym *overload = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                     call_arg_types,
                                                     call_arg_count);
    int same_type_copy_init = 0;
    if (call_arg_count == 1)
    {
      CType arg_value_type = call_arg_types[0];
      arg_value_type.t &= ~VT_RVALUE_REFERENCE;
      same_type_copy_init = is_compatible_unqualified_types(type,
                                                            &arg_value_type);
    }
    if (same_type_copy_init)
    {
      init_params p = {0};
      TokenString macro = *call_args[0];
      begin_macro(&macro, 0);
      next();
      expr_eq();
      end_macro();
      init_putv(&p, type, addr);
      if (tok == TOK_EOF)
      {
        next();
        unget_tok(saved_delim);
      }
      return;
    }
    if (call_arg_count == 1 && saved_arg_is_braced(call_args[0]))
    {
      Sym *initializer_list_ctor = resolve_initializer_list_constructor(type);
      if (initializer_list_ctor)
        overload = initializer_list_ctor;
    }
    if (!overload)
      overload = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                        call_arg_types,
                                                        call_arg_count);
    if (overload)
      ctor_func = overload;
    else if (call_arg_count == 1
             && is_same_template_family_conversion_ctor(type, &call_arg_types[0]))
    {
      call_lifecycle_constructor_members(type, r, addr);
      return;
    }
    else
    {
      overload = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                  call_arg_count);
      if (overload)
        ctor_func = overload;
      else
        cprime_error("no matching constructor for copy-initialization");
    }
  }
  if (!ctor_func || (ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");


  call_lifecycle_constructor_members(type, r, addr);
  save_lvalues();
  vpushsym(&ctor_func->type, ctor_func);
  vset(type, r, addr);
  if (sym)
    vtop->sym = sym;
  mk_pointer(&vtop->type);
  gaddrof();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    if (saved_arg_is_braced(call_args[ai]) && sa
        && (((sa->type.t & VT_BTYPE) == VT_STRUCT)
            || (is_reference_type(&sa->type)
                && ((pointed_type(&sa->type)->t & VT_BTYPE) == VT_STRUCT))))
    {
      CType temp_type;
      AttributeDef temp_ad;
      TokenString macro;
      temp_type = is_reference_type(&sa->type) ? *pointed_type(&sa->type)
                                               : sa->type;
      memset(&temp_ad, 0, sizeof temp_ad);
      macro = *call_args[ai];
      begin_macro(&macro, 0);
      next();
      decl_initializer_alloc(&temp_type, &temp_ad, VT_LOCAL | VT_LVAL,
                             1, 0, NULL, 0, VT_LOCAL);
      end_macro();
    }
    else
    {
      TokenString macro = *call_args[ai];
      begin_macro(&macro, 0);
      next();
      expr_eq();
      end_macro();
    }
    gfunc_param_typed(func_type, sa);
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
  if (tok == TOK_EOF)
  {
    next();
    unget_tok(saved_delim);
  }
  vcheck_cmp();
  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
}

static int try_parse_cpp_functional_constructor(int type_tok)
{
  Sym *struct_sym, *ctor_func;
  TokenString *call_args[32];
  CType call_arg_types[32];
  CType type;
  int size, align, addr, r2, call_arg_count;

  struct_sym = struct_find(type_tok);
  if (struct_sym && (struct_sym->type.t & VT_BTYPE) == VT_STRUCT)
  {
    type.t = struct_sym->type.t;
    type.ref = struct_sym;
  }
  else
  {
    Sym *alias = sym_find(type_tok);
    if (!alias)
      alias = sym_find2(global_stack, type_tok);
    if (!alias || !(alias->type.t & VT_TYPEDEF)
        || (alias->type.t & VT_BTYPE) != VT_STRUCT)
      return 0;
    type = alias->type;
    type.t &= ~VT_TYPEDEF;
  }

  if (tok == '{')
  {
    AttributeDef ad;

    memset(&ad, 0, sizeof ad);
    decl_initializer_alloc(&type, &ad, VT_LOCAL | VT_LVAL, 1, 0, NULL, 0,
                           VT_LOCAL);
    return 1;
  }

  if (tok != '(')
    return 0;

  next();
  skip_line_markers();
  if (tok == '{')
    call_arg_count = count_saved_braced_ctor_args(call_args, 32);
  else
    call_arg_count = count_saved_call_args(call_args, 32);
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  ctor_func = resolve_member_func_by_arg_types(&type, TOK_CONSTRUCTOR1,
                                               call_arg_types,
                                               call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_field_func_by_arg_types(&type, TOK_CONSTRUCTOR1,
                                                       call_arg_types,
                                                       call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_func_by_arg_count(&type, TOK_CONSTRUCTOR1,
                                                 call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_lifecycle_func(&type, TOK_CONSTRUCTOR1);
  if (!ctor_func)
  {
    if (call_arg_count != 0)
      cprime_error("no matching constructor for functional-style construction");
    size = type_size(&type, &align);
    addr = get_temp_local_var(size, align, &r2);
    call_lifecycle_constructor_members(&type, VT_LOCAL | VT_LVAL, addr);
    if (tok == TOK_EOF)
      next();
    else
      skip(')');
    vset(&type, VT_LOCAL | VT_LVAL, addr);
    vtop->r2 = r2;
    return 1;
  }

  size = type_size(&type, &align);
  addr = get_temp_local_var(size, align, &r2);
  call_lifecycle_constructor_saved_args(&type, VT_LOCAL | VT_LVAL, addr,
                                        NULL, ctor_func, call_args,
                                        call_arg_count);
  if (tok == TOK_EOF)
    next();
  else
    skip(')');

  vset(&type, VT_LOCAL | VT_LVAL, addr);
  vtop->r2 = r2;
  return 1;
}

static void call_placement_constructor(CType *type, TokenString *placement,
                                       TokenString **call_args,
                                       int call_arg_count)
{
  Sym *ctor_func, *func_type, *sa;
  CType call_arg_types[32], ptr_type;
  int nb_args, ai, saved_tok;
  CValue saved_tokc;

  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  ctor_func = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                               call_arg_types,
                                               call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                       call_arg_types,
                                                       call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
  if (ctor_func
      && ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref))
    cprime_error("placement constructor target is not a function");

  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(placement, 1);
  next();
  expr_eq();
  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;

  ptr_type = *type;
  mk_pointer(&ptr_type);
  gen_cast(&ptr_type);

  if (!ctor_func)
  {
    SValue base_ptr = *vtop;
    call_lifecycle_constructor_members_base_ptr(type, &base_ptr, 0);
    return;
  }

  save_lvalues();
  vdup();
  vpushsym(&ctor_func->type, ctor_func);
  vswap();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    saved_tok = tok;
    saved_tokc = tokc;
    begin_macro(call_args[ai], 1);
    next();
    expr_eq();
    end_macro();
    tok = saved_tok;
    tokc = saved_tokc;
    gfunc_param_typed(func_type, sa);
    nb_args++;
    if (sa)
      sa = sa->next;
  }
  if (sa)
    cprime_error("too few arguments to constructor");

  vcheck_cmp();
  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
  if ((vtop->type.t & VT_BTYPE) == VT_VOID)
    vpop();
}

static int try_parse_cpp_placement_new_after_name(void)
{
  CType type;
  AttributeDef ad;
  TokenString *placement, *call_args[32];
  int call_arg_count = 0, saved_tok;
  CValue saved_tokc;

  if (tok != '(')
    return 0;

  next();
  skip_or_save_block(&placement);
  skip(')');

  memset(&ad, 0, sizeof(ad));
  if (!parse_btype(&type, &ad, 0))
    cprime_error("placement new requires a type");

  if (tok == '(')
  {
    next();
    call_arg_count = count_saved_call_args(call_args, 32);
    skip(')');
  }
  if ((type.t & VT_BTYPE) != VT_STRUCT)
  {
    CType ptr_type = type;
    int i;

    if (call_arg_count > 1)
      cprime_error("scalar placement new accepts at most one argument");
    saved_tok = tok;
    saved_tokc = tokc;
    begin_macro(placement, 1);
    next();
    expr_eq();
    end_macro();
    tok = saved_tok;
    tokc = saved_tokc;
    mk_pointer(&ptr_type);
    gen_cast(&ptr_type);
    vdup();
    indir();
    if (call_arg_count)
    {
      begin_macro(call_args[0], 1);
      next();
      expr_eq();
      end_macro();
      tok = saved_tok;
      tokc = saved_tokc;
    }
    else
      vpushi(0);
    vstore();
    vpop();
    for (i = 0; i < call_arg_count; ++i)
      tok_str_free(call_args[i]);
    tok_str_free(placement);
    return 1;
  }
  else if (!type.ref->a.lifecycle_ctor)
  {
    int saved_tok = tok;
    CValue saved_tokc = tokc;
    begin_macro(placement, 1);
    next();
    expr_eq();
    end_macro();
    tok = saved_tok;
    tokc = saved_tokc;
    mk_pointer(&type);
    gen_cast(&type);
    return 1;
  }

  saved_tok = tok;
  saved_tokc = tokc;
  call_placement_constructor(&type, placement, call_args, call_arg_count);
  tok = saved_tok;
  tokc = saved_tokc;
  return 1;
}

static Sym *resolve_member_func(CType *type, int method_tok)
{
  int struct_tok, owner_tok, mangled_tok;
  Sym *s, *field;
  CType struct_type, lowered_type;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  owner_tok = struct_tok;
  if (!is_lifecycle_member_tok(method_tok))
    instantiate_template_member_for_call(type, method_tok, NULL, -1);
  mangled_tok = make_member_func_tok(struct_tok, method_tok);
  s = sym_find(mangled_tok);
  if (!s)
    s = sym_find2(global_stack, mangled_tok);
  if (s && (s->type.t & VT_BTYPE) != VT_FUNC
      && !((s->type.t & VT_BTYPE) == VT_PTR
           && ((pointed_type(&s->type)->t & VT_BTYPE) == VT_FUNC)))
    s = NULL;
  if (!s)
  {
    /* Fallback: recover from field-only member declarations by lowering here. */
    field = find_field_try_with_owner(type, method_tok, &struct_tok,
                                      &owner_tok);
    if (!field)
      field = find_field_try_with_owner(type, method_tok | SYM_FIELD,
                                        &struct_tok, &owner_tok);
    if (field && (field->type.t & VT_BTYPE) == VT_FUNC)
    {
      if (!make_class_type_from_tok(&struct_type, owner_tok))
        struct_type = *type;
      mangled_tok = make_member_func_tok(owner_tok, method_tok);
      lowered_type = make_lowered_member_func_type(&struct_type, &field->type);
      s = external_global_sym(mangled_tok, &lowered_type);
    }
  }
  else
  {
    /* Prefer non-void lowered signature from class member declaration. */
    field = find_field_try_with_owner(type, method_tok, &struct_tok,
                                      &owner_tok);
    if (!field)
      field = find_field_try_with_owner(type, method_tok | SYM_FIELD,
                                        &struct_tok, &owner_tok);
    if (field && (field->type.t & VT_BTYPE) == VT_FUNC
        && s->type.ref
        && (s->type.ref->type.t & VT_BTYPE) == VT_VOID
        && (field->type.ref->type.t & VT_BTYPE) != VT_VOID)
    {
      if (!make_class_type_from_tok(&struct_type, owner_tok))
        struct_type = *type;
      mangled_tok = make_member_func_tok(owner_tok, method_tok);
      lowered_type = make_lowered_member_func_type(&struct_type, &field->type);
      s = external_global_sym(mangled_tok, &lowered_type);
    }
  }
  if (!s)
    return NULL;
  if ((s->type.t & VT_BTYPE) != VT_FUNC)
    cprime_error("member function target '%s' is not declared as function",
              get_tok_str(mangled_tok, NULL));
  return s;
}

static int member_receiver_is_const(CType *type)
{
  return (type->t & VT_CONSTANT) != 0;
}

static int member_overload_receiver_ok(MemberFuncOverload *o, int receiver_const)
{
  return o->is_const || !receiver_const;
}

static void member_overload_note_match(Sym **match, Sym **const_match,
                                       Sym *s, int is_const)
{
  Sym **slot = is_const ? const_match : match;
  if (!*slot)
    *slot = s;
}

static void member_overload_note_ranked_match(Sym **match, Sym **const_match,
                                              int *best_omitted_defaults,
                                              Sym *s, int is_const,
                                              int omitted_defaults)
{
  Sym **slot;

  if (omitted_defaults > *best_omitted_defaults)
    return;
  if (omitted_defaults < *best_omitted_defaults)
  {
    *match = NULL;
    *const_match = NULL;
    *best_omitted_defaults = omitted_defaults;
  }
  slot = is_const ? const_match : match;
  if (!*slot)
    *slot = s;
}

static Sym *member_overload_pick_match(Sym *match, Sym *const_match,
                                       int receiver_const)
{
  if (receiver_const)
    return const_match;
  return match ? match : const_match;
}

static Sym *resolve_member_func_by_arg_count(CType *type, int method_tok,
                                             int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = member_receiver_is_const(type);
  int best_omitted_defaults = 0x7fffffff;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  instantiate_template_member_for_call(type, method_tok, NULL,
                                       explicit_arg_count);
  for (o = member_func_overloads; o;)
  {
    MemberFuncOverload *next_o = o->next;
    Sym *s;
    if (o->struct_tok != struct_tok || o->method_tok != method_tok
        || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      goto next_overload;
    if (!member_overload_receiver_ok(o, receiver_const))
      goto next_overload;
    s = sym_find2(global_stack, o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (!s)
      goto next_overload;
    use_overload_func_type(s, &o->func_type);
    if (method_tok == TOK_CONSTRUCTOR1
        && o->explicit_arg_count - explicit_arg_count == best_omitted_defaults)
    {
      Sym **slot = o->is_const ? &const_match : &match;
      *slot = s;
    }
    else
      member_overload_note_ranked_match(&match, &const_match,
                                        &best_omitted_defaults, s,
                                        o->is_const,
                                        o->explicit_arg_count - explicit_arg_count);
next_overload:
    o = next_o;
  }
  match = member_overload_pick_match(match, const_match, receiver_const);
  if (match)
    return match;
  {
    ClassBaseInfo *info;
    for (info = class_base_infos; info; info = info->next)
    {
      CType base_type;
      Sym *base_match;

      if (info->class_tok != struct_tok)
        continue;
      if (!make_class_type_from_tok(&base_type, info->base_tok))
        continue;
      base_type.t |= type->t & VT_CONSTANT;
      base_match = resolve_member_func_by_arg_count(&base_type, method_tok,
                                                    explicit_arg_count);
      if (base_match)
        return base_match;
    }
  }
  return explicit_arg_count < 0 ? resolve_member_func(type, method_tok) : NULL;
}

static int member_overload_exists_for_call(CType *type, int method_tok,
                                           int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return 0;
  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
      return 1;
  return 0;
}

static int member_overload_count_for_call(CType *type, int method_tok,
                                          int explicit_arg_count)
{
  int struct_tok, count = 0;
  MemberFuncOverload *o;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return 0;
  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
      ++count;
  return count;
}

static int type_is_std_initializer_list(CType *type)
{
  int struct_tok;

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return 0;
  struct_tok = get_struct_type_name_tok(type);
  return struct_tok >= TOK_UIDENT
         && strstr(get_tok_str(struct_tok, NULL), "initializer_list") != NULL;
}

static int class_has_single_arg_constructor_for(CType *class_type,
                                                CType *arg_type)
{
  CType arg_value_type;
  Sym *class_sym, *field, *arg;
  int class_tok;

  if ((class_type->t & VT_BTYPE) != VT_STRUCT
      || (arg_type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_compatible_unqualified_types(class_type, &arg_value_type))
    return 0;
  class_tok = get_struct_type_name_tok(class_type);
  if (!class_tok)
    return 0;
  class_sym = struct_find(class_tok);
  if (!class_sym)
    return 0;
  for (field = class_sym->next; field; field = field->next)
  {
    if (((field->v & ~SYM_FIELD) != TOK_CONSTRUCTOR1
         && (field->v & ~SYM_FIELD) != class_tok)
        || (field->type.t & VT_BTYPE) != VT_FUNC
        || !field->type.ref)
      continue;
    arg = field->type.ref->next;
    if (!arg || arg->next)
      continue;
    if (call_arg_match_rank(&arg->type, &arg_value_type) >= 0)
      return 1;
  }
  return 0;
}

static int is_same_template_family_conversion_ctor(CType *class_type,
                                                   CType *arg_type)
{
  CType arg_value_type;
  int class_tok, arg_tok, i, j;

  if (!class_type || !arg_type
      || (class_type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  if ((arg_value_type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  class_tok = get_struct_type_name_tok(class_type);
  arg_tok = get_struct_type_name_tok(&arg_value_type);
  if (!class_tok || !arg_tok || class_tok == arg_tok)
    return 0;
  if (!class_or_inst_has_member_template_name(class_tok, TOK_CONSTRUCTOR1))
    return 0;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    int class_seen = 0, arg_seen = 0;
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
    {
      if (td->inst_name_toks[j] == class_tok)
        class_seen = 1;
      if (td->inst_name_toks[j] == arg_tok)
        arg_seen = 1;
    }
    if (class_seen && arg_seen)
      return 1;
  }
  return 0;
}

static int member_field_func_arg_match_rank(Sym *field, CType *arg_types,
                                            int explicit_arg_count)
{
  int i, rank, total = 0;
  Sym *arg;

  if (!field || (field->type.t & VT_BTYPE) != VT_FUNC || !field->type.ref)
    return -1;
  arg = field->type.ref->next;
  for (i = 0; i < explicit_arg_count; ++i)
  {
    if (!arg)
      return -1;
    rank = call_arg_match_rank(&arg->type, &arg_types[i]);
    if (rank < 0)
      return -1;
    total += rank;
    arg = arg->next;
  }
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      return -1;
  return total;
}

static Sym *resolve_member_field_func_by_arg_types(CType *type, int method_tok,
                                                   CType *arg_types,
                                                   int explicit_arg_count)
{
  int struct_tok, best_rank = 0x7fffffff;
  Sym *class_sym, *field, *best = NULL;
  CType lowered_type, struct_type;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  class_sym = struct_find(struct_tok);
  if (!class_sym)
    return NULL;
  for (field = class_sym->next; field; field = field->next)
  {
    int rank;
    if ((field->v & ~SYM_FIELD) != method_tok
        && !(method_tok == TOK_CONSTRUCTOR1
             && (field->v & ~SYM_FIELD) == struct_tok)
        || (field->type.t & VT_BTYPE) != VT_FUNC)
      continue;
    rank = member_field_func_arg_match_rank(field, arg_types,
                                            explicit_arg_count);
    if (rank < 0 || rank > best_rank)
      continue;
    if (rank == best_rank && best && best != field)
      continue;
    best_rank = rank;
    best = field;
  }
  if (!best)
    return NULL;
  struct_type = *type;
  lowered_type = make_lowered_member_func_type(&struct_type, &best->type);
  return external_global_sym(make_member_func_tok_for_type(struct_tok,
                                                           method_tok,
                                                           &best->type),
                             &lowered_type);
}

static int member_func_matches_arg_types(Sym *s, CType *arg_types,
                                         int explicit_arg_count)
{
  int i;
  Sym *arg;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
    return 0;
  arg = s->type.ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  for (i = 0; i < explicit_arg_count; ++i)
  {
    if (!arg)
      return 0;
    if (!call_arg_matches_param_type(&arg->type, &arg_types[i]))
      return 0;
    arg = arg->next;
  }
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      return 0;
  return 1;
}

static int member_func_arg_match_rank(Sym *s, CType *arg_types,
                                      int explicit_arg_count)
{
  int i, rank, total = 0;
  Sym *arg;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
    return -1;
  arg = s->type.ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  for (i = 0; i < explicit_arg_count; ++i)
  {
    if (!arg)
      return -1;
    rank = call_arg_match_rank(&arg->type, &arg_types[i]);
    if (rank < 0)
      return -1;
    total += rank;
    arg = arg->next;
  }
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      return -1;
  return total;
}

static int same_lowered_member_func_signature(CType *type1, CType *type2)
{
  Sym *arg1, *arg2;

  if (!type1 || !type1->ref || !type2 || !type2->ref
      || (type1->t & VT_BTYPE) != VT_FUNC
      || (type2->t & VT_BTYPE) != VT_FUNC)
    return 0;
  arg1 = type1->ref->next;
  arg2 = type2->ref->next;
  for (;;)
  {
    if (!arg1 || !arg2)
      return arg1 == arg2;
    if ((arg1->type.t & VT_RVALUE_REFERENCE)
        != (arg2->type.t & VT_RVALUE_REFERENCE))
      return 0;
    if (!is_compatible_types(&arg1->type, &arg2->type))
      return 0;
    arg1 = arg1->next;
    arg2 = arg2->next;
  }
}

static Sym *resolve_member_func_by_arg_types(CType *type, int method_tok,
                                             CType *arg_types,
                                             int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = member_receiver_is_const(type);
  int best_omitted_defaults = 0x7fffffff;
  int best_arg_rank = 0x7fffffff;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  instantiate_template_member_for_call(type, method_tok, arg_types,
                                       explicit_arg_count);
  if (method_tok == TOK_CONSTRUCTOR1 && nb_pending_member_funcs)
    compile_pending_member_funcs(0);
  for (o = member_func_overloads; o;)
  {
    MemberFuncOverload *next_o = o->next;
    Sym *s;
    if (o->struct_tok != struct_tok || o->method_tok != method_tok
        || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      goto next_overload;
    if (!member_overload_receiver_ok(o, receiver_const))
      goto next_overload;
    s = sym_find2(global_stack, o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (s)
      use_overload_func_type(s, &o->func_type);
    {
      int arg_rank = member_func_arg_match_rank(s, arg_types,
                                                explicit_arg_count);
      if (arg_rank < 0)
        goto next_overload;
      if (arg_rank > best_arg_rank)
        goto next_overload;
      if (arg_rank < best_arg_rank)
      {
        match = NULL;
        const_match = NULL;
        best_omitted_defaults = 0x7fffffff;
        best_arg_rank = arg_rank;
      }
    }
    if (!member_func_matches_arg_types(s, arg_types, explicit_arg_count))
      goto next_overload;
    {
      Sym **slot = o->is_const ? &const_match : &match;
      if (*slot && *slot != s
          && same_lowered_member_func_signature(&(*slot)->type, &s->type))
        goto next_overload;
    }
    member_overload_note_ranked_match(&match, &const_match,
                                      &best_omitted_defaults, s,
                                      o->is_const,
                                      o->explicit_arg_count - explicit_arg_count);
next_overload:
    o = next_o;
  }
  match = member_overload_pick_match(match, const_match, receiver_const);
  if (match)
    return match;
  {
    ClassBaseInfo *info;
    for (info = class_base_infos; info; info = info->next)
    {
      CType base_type;
      Sym *base_match;

      if (info->class_tok != struct_tok)
        continue;
      if (!make_class_type_from_tok(&base_type, info->base_tok))
        continue;
      base_type.t |= type->t & VT_CONSTANT;
      base_match = resolve_member_func_by_arg_types(&base_type, method_tok,
                                                    arg_types,
                                                    explicit_arg_count);
      if (base_match)
        return base_match;
    }
  }
  return NULL;
}

static int member_func_has_param_signature(Sym *s, CType *func_type)
{
  Sym *member_arg, *def_arg;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref
      || !func_type || (func_type->t & VT_BTYPE) != VT_FUNC || !func_type->ref)
    return 0;
  member_arg = s->type.ref->next;
  if (member_arg)
    member_arg = member_arg->next; /* skip implicit this */
  def_arg = func_type->ref->next;
  for (;;)
  {
    if (!member_arg || !def_arg)
      return member_arg == def_arg;
    if (!is_compatible_types(&member_arg->type, &def_arg->type))
      return 0;
    member_arg = member_arg->next;
    def_arg = def_arg->next;
  }
}

static Sym *resolve_member_func_by_param_signature(CType *type, int method_tok,
                                                   CType *func_type)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = member_receiver_is_const(type);

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  for (o = member_func_overloads; o; o = o->next)
  {
    Sym *s;
    if (o->struct_tok != struct_tok || o->method_tok != method_tok)
      continue;
    if (!member_overload_receiver_ok(o, receiver_const))
      continue;
    s = sym_find(o->mangled_tok);
    if (!s)
      s = sym_find2(global_stack, o->mangled_tok);
    if (s)
      use_overload_func_type(s, &o->func_type);
    if (!member_func_has_param_signature(s, func_type))
      continue;
    member_overload_note_match(&match, &const_match, s, o->is_const);
  }
  return member_overload_pick_match(match, const_match, receiver_const);
}

static int add_ctype_tokens(TokenString *str, CType *type)
{
  int t = type->t;
  int bt = t &VT_BTYPE;
  CType pt;

  if (bt == VT_PTR)
  {
    pt = *pointed_type(type);
    if (!add_ctype_tokens(str, &pt))
      return 0;
    if (t & VT_RVALUE_REFERENCE)
      tok_str_add(str, TOK_LAND);
    else
      tok_str_add(str, (t & VT_REFERENCE) ? '&' : '*');
    if (t & VT_CONSTANT)
      tok_str_add(str, TOK_CONST1);
    if (t & VT_VOLATILE)
      tok_str_add(str, TOK_VOLATILE1);
    return 1;
  }

  if (t & VT_CONSTANT)
    tok_str_add(str, TOK_CONST1);
  if (t & VT_VOLATILE)
    tok_str_add(str, TOK_VOLATILE1);

  if (IS_ENUM(t))
  {
    int enum_tok;
    if (!type->ref)
      return 0;
    enum_tok = type->ref->v & ~SYM_STRUCT;
    if (enum_tok < TOK_UIDENT)
      return 0;
    tok_str_add(str, TOK_ENUM);
    tok_str_add(str, enum_tok);
    return 1;
  }

  if (bt == VT_LLONG)
  {
    if ((t & VT_UNSIGNED) && bt != VT_FLOAT && bt != VT_DOUBLE)
      tok_str_add(str, TOK_UNSIGNED);
    tok_str_add(str, TOK_LONG);
    tok_str_add(str, TOK_LONG);
    tok_str_add(str, TOK_INT);
    return 1;
  }

  if ((t & VT_DEFSIGN) && bt == VT_BYTE && !(t & VT_UNSIGNED))
    tok_str_add(str, TOK_SIGNED1);
  if ((t & VT_UNSIGNED) && bt != VT_FLOAT && bt != VT_DOUBLE
      && !(t & VT_WCHAR_T))
    tok_str_add(str, TOK_UNSIGNED);
  if (t & VT_LONG)
    tok_str_add(str, TOK_LONG);
  if (bt == VT_VOID)
    tok_str_add(str, TOK_VOID);
  else if (bt == VT_BYTE)
    tok_str_add(str, TOK_CHAR);
  else if (bt == VT_SHORT)
  {
    if (t & VT_WCHAR_T)
      tok_str_add(str, tok_alloc_const("wchar_t"));
    else
      tok_str_add(str, TOK_SHORT);
  }
  else if (bt == VT_INT || bt == VT_LLONG)
    tok_str_add(str, TOK_INT);
  else if (bt == VT_FLOAT)
    tok_str_add(str, TOK_FLOAT);
  else if (bt == VT_DOUBLE)
    tok_str_add(str, TOK_DOUBLE);
  else if (bt == VT_BOOL)
    tok_str_add(str, TOK_BOOL);
  else if (bt == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
    if (!struct_tok)
      return 0;
    tok_str_add(str, TOK_STRUCT);
    tok_str_add(str, struct_tok);
  }
  else
    return 0;
  return 1;
}

static CType make_lowered_member_func_type(CType *struct_type, CType *func_type)
{
  CType lowered_type, return_type, this_type, arg_type;
  Sym *fref, *param, *arg, *last;
  Sym *saved_ls;

  return_type = func_type->ref->type;
  lowered_type.t = VT_FUNC;
  /* Same as make_lifecycle_func_type: keep these type symbols off a live
     local scope so scope pops cannot unlink the 'this' parameter symbols. */
  saved_ls = local_stack;
  local_stack = NULL;
  lowered_type.ref = fref = sym_push(SYM_FIELD, &return_type, 0, 0);
  fref->f = func_type->ref->f;

  this_type = *struct_type;
  if (func_type->t & VT_CONSTANT)
    this_type.t |= VT_CONSTANT;
  mk_pointer(&this_type);
  param = sym_push(SYM_FIELD, &this_type, VT_LOCAL | VT_LVAL, 0);
  param->v = tok_alloc_const("this");
  fref->next = param;
  last = param;

  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    arg_type = arg->type;
    param = sym_push(SYM_FIELD, &arg_type, arg->r, arg->c);
    param->v = arg->v;
    param->default_arg = arg->default_arg;
    last->next = param;
    last = param;
  }
  local_stack = saved_ls;

  return lowered_type;
}

static void add_pending_member_func(CType *struct_type, int method_tok,
                                    CType *func_type, TokenString *body)
{
  int struct_tok, mangled_tok, i;
  Sym *arg;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline member functions require a named struct/class");
  mangled_tok = make_member_func_tok_for_type(struct_tok, method_tok, func_type);
  {
    CType lowered_type = make_lowered_member_func_type(struct_type, func_type);
    external_global_sym(mangled_tok, &lowered_type);
    note_member_func_overload(struct_tok, method_tok, mangled_tok, &lowered_type);
  }

  str = tok_str_alloc();
  tok_str_add(str, TOK_STATIC);
  tok_str_add(str, TOK_INLINE1);
  if (!add_ctype_tokens(str, &func_type->ref->type))
    cprime_error("unsupported member function return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  if (func_type->t & VT_CONSTANT)
    tok_str_add(str, TOK_CONST1);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    tok_str_add(str, ',');
    if (!add_ctype_tokens(str, &arg->type))
      cprime_error("unsupported member function parameter type");
    tok_str_add(str, arg->v & ~SYM_FIELD);
  }
  tok_str_add(str, ')');
  for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
  {
    if (body->str[i] >= TOK_UIDENT && i + 1 < body->len
        && body->str[i + 1] == '('
        && (i == 0 || (body->str[i - 1] != '.'
                       && body->str[i - 1] != TOK_ARROW))
        && body->str[i] != struct_tok
        && (class_has_static_member_func(struct_tok, body->str[i])
            || type_has_member_func_name(struct_type, body->str[i])))
    {
      if (class_has_static_member_func(struct_tok, body->str[i]))
      {
        /* Static member calls cannot go through this->; qualify them as
           Class::Member. */
        tok_str_add(str, struct_tok);
        tok_str_add(str, ':');
        tok_str_add(str, ':');
      }
      else
      {
        tok_str_add(str, tok_alloc_const("this"));
        tok_str_add(str, TOK_ARROW);
      }
    }
    tok_str_add(str, body->str[i]);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->struct_tok = struct_tok;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static Sym *declare_member_func(CType *struct_type, int method_tok,
                                CType *func_type)
{
  int struct_tok, mangled_tok;
  CType lowered_type;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("member functions require a named struct/class");
  mangled_tok = make_member_func_tok_for_type(struct_tok, method_tok, func_type);
  lowered_type = make_lowered_member_func_type(struct_type, func_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok, &lowered_type);
  return external_global_sym(mangled_tok, &lowered_type);
}

static Sym *declare_static_member_func(CType *struct_type, int method_tok,
                                       CType *func_type)
{
  int struct_tok, base_tok, mangled_tok;
  CType static_type;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("static member functions require a named struct/class");
  base_tok = make_static_member_tok(struct_tok, method_tok);
  static_type = *func_type;
  static_type.t &= ~VT_STATIC;
  mangled_tok = make_free_func_tok_for_type(base_tok, &static_type);
  note_free_func_overload(base_tok, mangled_tok, &static_type);
  if (!sym_find(base_tok) && !sym_find2(global_stack, base_tok))
    external_global_sym(base_tok, &static_type);
  return external_global_sym(mangled_tok, &static_type);
}

static void add_pending_static_member_func(CType *struct_type, int method_tok,
                                            CType *func_type, TokenString *body,
                                            int is_auto_return)
{
  int struct_tok, base_tok, mangled_tok, i;
  Sym *arg;
  CType static_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline static member functions require a named struct/class");

  if (is_auto_return && func_type && func_type->ref && body)
    for (i = 0; i + 1 < body->len; ++i)
      if (body->str[i] == TOK_RETURN)
      {
        int ri = i + 1;
        Sym *return_func;
        while (ri < body->len && body->str[ri] == TOK_LINENUM)
          ri += 2;
        return_func = ri < body->len ? sym_find(body->str[ri]) : NULL;
        if (return_func && (return_func->type.t & VT_BTYPE) == VT_FUNC
            && return_func->type.ref)
          func_type->ref->type = return_func->type.ref->type;
        break;
      }

  declare_static_member_func(struct_type, method_tok, func_type);

  base_tok = make_static_member_tok(struct_tok, method_tok);
  static_type = *func_type;
  static_type.t &= ~VT_STATIC;
  mangled_tok = make_free_func_tok_for_type(base_tok, &static_type);

  str = tok_str_alloc();
  tok_str_add(str, TOK_STATIC);
  tok_str_add(str, TOK_INLINE1);
  if (!add_ctype_tokens(str, &func_type->ref->type))
    cprime_error("unsupported static member function return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    if (arg != func_type->ref->next)
      tok_str_add(str, ',');
    if (!add_ctype_tokens(str, &arg->type))
      cprime_error("unsupported static member function parameter type");
    tok_str_add(str, arg->v & ~SYM_FIELD);
  }
  tok_str_add(str, ')');
  for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
  {
    int bt = body->str[i];
    if (bt >= TOK_UIDENT
        && (i == 0 || (body->str[i - 1] != '.'
                       && body->str[i - 1] != TOK_ARROW
                       && body->str[i - 1] != ':'))
        && (i + 1 < body->len
            && (body->str[i + 1] == '('
                || body->str[i + 1] == TOK_LT
                || body->str[i + 1] == '<'))
        && class_has_static_member_func(struct_tok, bt))
    {
      /* Static bodies have no this pointer: qualify nested static member
         and member-template calls as Class::Member so the call resolves
         through the static overload table. */
      tok_str_add(str, struct_tok);
      tok_str_add(str, ':');
      tok_str_add(str, ':');
      tok_str_add(str, bt);
      continue;
    }
    tok_str_add(str, bt);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void add_lifecycle_param_tokens(TokenString *str, TokenString *params,
                                       int struct_tok)
{
  int i;
  const char *struct_name = get_tok_str(struct_tok, NULL);

  if (!params)
    return;
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    int lt = i + 1;
    while (lt + 1 < params->len && params->str[lt] == TOK_LINENUM)
      lt += 2;
    if (params->str[i] >= TOK_IDENT && lt < params->len
        && params->str[lt] == '<')
    {
      const char *param_name = get_tok_str(params->str[i], NULL);
      int is_self = !strcmp(param_name, struct_name)
                    || (!strncmp(param_name, struct_name, strlen(struct_name))
                        && param_name[strlen(struct_name)] == '_'
                        && param_name[strlen(struct_name) + 1] == '_')
                    || (!strncmp(struct_name, param_name, strlen(param_name))
                        && struct_name[strlen(param_name)] == '_'
                        && struct_name[strlen(param_name) + 1] == '_');
      if (is_self)
      {
        int level = 1;
        tok_str_add(str, params->str[i]);
        i = lt + 1;
        while (i < params->len && params->str[i] != TOK_EOF && level > 0)
        {
          if (params->str[i] == '<')
            level++;
          else if (params->str[i] == TOK_GT || params->str[i] == TOK_SAR)
            level--;
          i++;
        }
        i--;
        continue;
      }
    }
    tok_str_add(str, params->str[i]);
  }
}

static void add_pending_lifecycle_func(CType *struct_type, int method_tok,
                                       TokenString *params,
                                       TokenString *body, int internal)
{
  int struct_tok, mangled_tok, i;
  int empty_body;
  CType ret_type, func_type, lowered_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline constructors/destructors require a named struct/class");
  ret_type.t = VT_VOID;
  ret_type.ref = NULL;
  func_type = make_func_type_from_saved_params(&ret_type, params);
  lowered_type = make_lowered_member_func_type(struct_type, &func_type);
  mangled_tok = make_lifecycle_func_tok_for_type(struct_tok, method_tok,
                                                &lowered_type, 1);
  if (pending_member_func_has_body_tok(mangled_tok))
    return;
  note_raw_lifecycle_overload(struct_tok, method_tok, mangled_tok, params);
  external_global_sym(mangled_tok, &lowered_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok,
                            &lowered_type);
  empty_body = token_string_contains_tok(body, tok_alloc_const("begin"));

  str = tok_str_alloc();
  if (internal)
  {
    tok_str_add(str, TOK_STATIC);
    tok_str_add(str, TOK_INLINE1);
  }
  tok_str_add(str, TOK_VOID);
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  if (params)
  {
    if (params->len > 1)
      tok_str_add(str, ',');
    add_lifecycle_param_tokens(str, params, struct_tok);
  }
  tok_str_add(str, ')');
  if (empty_body)
  {
    tok_str_add(str, '{');
    tok_str_add(str, '}');
  }
  else
  {
    for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
      tok_str_add(str, body->str[i]);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->struct_tok = struct_tok;
  pm->is_lifecycle_member = 1;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void add_pending_lifecycle_decl(CType *struct_type, int method_tok,
                                       TokenString *params)
{
  int struct_tok, mangled_tok;
  CType ret_type, func_type, lowered_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("constructors/destructors require a named struct/class");
  ret_type.t = VT_VOID;
  ret_type.ref = NULL;
  func_type = make_func_type_from_saved_params(&ret_type, params);
  lowered_type = make_lowered_member_func_type(struct_type, &func_type);
  mangled_tok = make_lifecycle_func_tok_for_type(struct_tok, method_tok,
                                                &lowered_type, 0);
  if (pending_member_func_has_tok(mangled_tok))
    return;
  if (sym_find(mangled_tok) || sym_find2(global_stack, mangled_tok))
    return;
  note_raw_lifecycle_overload(struct_tok, method_tok, mangled_tok, params);
  external_global_sym(mangled_tok, &lowered_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok,
                            &lowered_type);

  str = tok_str_alloc();
  tok_str_add(str, TOK_VOID);
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  if (params)
  {
    if (params->len > 1)
      tok_str_add(str, ',');
    add_lifecycle_param_tokens(str, params, struct_tok);
  }
  tok_str_add(str, ')');
  tok_str_add(str, ';');
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->struct_tok = struct_tok;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static int can_lower_global_dynamic_init(CType *type, int l, int has_init)
{
  int bt;

  if (l != VT_CONST || !has_init)
    return 0;
  if (type->t & (VT_CONSTANT | VT_ARRAY))
    return 0;
  bt = type->t & VT_BTYPE;
  if (bt == VT_STRUCT)
    return get_struct_type_name_tok(type) && type->ref
           && type->ref->a.lifecycle_ctor;
  return bt != VT_FUNC && bt != VT_VOID;
}

static Sym *resolve_initializer_list_constructor(CType *type)
{
  int struct_tok = get_struct_type_name_tok(type);
  MemberFuncOverload *o;
  TemplateDef *td;
  int i, j;

  if (!struct_tok)
    return NULL;
  /* Class-template lifecycle declarations may still be queued until their
     first call.  Force the one-argument constructor set to be materialized
     before inspecting its parameter types. */
  (void)resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1, 1);
  /* Out-of-class class-template constructors with a std::initializer_list
     parameter are captured as template members and only become concrete
     overloads once instantiated.  A braced initializer always targets the
     initializer-list constructor, so materialize it for this class before
     scanning the overload table. */
  td = find_class_template_def_for_class_tok(struct_tok);
  if (td)
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == struct_tok)
        for (i = 0; i < nb_template_member_defs; ++i)
        {
          TemplateMemberDef *md = template_member_defs[i];
          if (md->class_tok == td->name_tok
              && template_member_def_method_tok(md) == TOK_CONSTRUCTOR1
              && token_string_has_initializer_list(md->def_str))
            instantiate_template_member_if_needed(
                md, td->inst_type_toks[j * td->nb_type_params],
                struct_tok, 0);
        }
  for (o = member_func_overloads; o; o = o->next)
  {
    Sym *param, *s;
    CType *param_type;

    if (o->struct_tok != struct_tok || o->method_tok != TOK_CONSTRUCTOR1
        || o->explicit_arg_count != 1)
      continue;
    s = sym_find2(global_stack, o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (!s)
      continue;
    use_overload_func_type(s, &o->func_type);
    param = s->type.ref ? s->type.ref->next : NULL;
    if (param)
      param = param->next; /* skip the lowered this parameter */
    if (!param)
      continue;
    param_type = &param->type;
    if (is_reference_type(param_type))
      param_type = pointed_type(param_type);
    if (!type_is_std_initializer_list(param_type))
      continue;
    return s;
  }
  return NULL;
}

static void add_pending_global_dynamic_init(int var_tok, CType *type,
                                            TokenString *init)
{
  char name[64];
  int func_tok, i;
  PendingMemberFunc *pm;
  TokenString *str;

  snprintf(name, sizeof(name), "__cpc_global_dynamic_init_%d",
           ++nb_pending_global_inits);
  func_tok = tok_alloc_const(name);

  str = tok_str_alloc();
  tok_str_add(str, TOK_STATIC);
  tok_str_add(str, TOK_VOID);
  tok_str_add(str, func_tok);
  tok_str_add(str, '(');
  tok_str_add(str, ')');
  tok_str_add(str, TOK_ATTRIBUTE1);
  tok_str_add(str, '(');
  tok_str_add(str, '(');
  tok_str_add(str, TOK_CONSTRUCTOR1);
  tok_str_add(str, ')');
  tok_str_add(str, ')');
  tok_str_add(str, '{');
  if ((type->t & VT_BTYPE) == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
    if (!struct_tok)
      cprime_error("global class initializer requires a named class");
    tok_str_add(str, tok_alloc_const("new"));
    tok_str_add(str, '(');
    tok_str_add(str, '&');
    tok_str_add(str, var_tok);
    tok_str_add(str, ')');
    tok_str_add(str, struct_tok);
    tok_str_add(str, '(');
  }
  else
  {
    tok_str_add(str, var_tok);
    tok_str_add(str, '=');
  }
  for (i = 0; i < init->len && init->str[i] != TOK_EOF; ++i)
    tok_str_add(str, init->str[i]);
  if ((type->t & VT_BTYPE) == VT_STRUCT)
    tok_str_add(str, ')');
  tok_str_add(str, ';');
  tok_str_add(str, '}');
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void rewrite_pending_member_field_access(PendingMemberFunc *pm)
{
  int i, in_body = 0, this_tok = tok_alloc_const("this");
  Sym *class_sym;
  CType class_type;
  TokenString *dst;

  if (!pm || !pm->struct_tok || !pm->str || pm->is_static_member)
    return;
  class_sym = struct_find(pm->struct_tok);
  if (!class_sym)
    return;
  class_type = class_sym->type;
  class_type.ref = class_sym;
  dst = tok_str_alloc();
  for (i = 0; i < pm->str->len && pm->str->str[i] != TOK_EOF; ++i)
  {
    int t = pm->str->str[i];
    if (!in_body)
    {
      tok_str_add(dst, t);
      if (t == '{')
        in_body = 1;
      continue;
    }
    if (t == this_tok && i + 3 < pm->str->len
        && pm->str->str[i + 1] == TOK_ARROW
        && pm->str->str[i + 2] >= TOK_UIDENT
        && pm->str->str[i + 3] == '=')
    {
      int field_tok = pm->str->str[i + 2], dummy_ofs, field_struct_tok, j;
      int rhs_i = i + 4;
      Sym *field = find_field_try(&class_type, field_tok, &dummy_ofs);
      field_struct_tok = field ? get_struct_type_name_tok(&field->type) : 0;
      while (rhs_i + 1 < pm->str->len
             && pm->str->str[rhs_i] == TOK_LINENUM)
        rhs_i += 2;
      if (rhs_i + 1 < pm->str->len
          && pm->str->str[rhs_i] >= TOK_UIDENT
          && pm->str->str[rhs_i + 1] == TOK_ARROW)
        goto keep_field_assignment;
      if (field && (field->type.t & VT_BTYPE) == VT_STRUCT && field_struct_tok)
      {
        tok_str_add(dst, tok_alloc_const("new"));
        tok_str_add(dst, '(');
        tok_str_add(dst, '&');
        tok_str_add(dst, '(');
        tok_str_add(dst, this_tok);
        tok_str_add(dst, TOK_ARROW);
        tok_str_add(dst, field_tok);
        tok_str_add(dst, ')');
        tok_str_add(dst, ')');
        tok_str_add(dst, field_struct_tok);
        tok_str_add(dst, '(');
        for (j = i + 4; j < pm->str->len && pm->str->str[j] != ';'; ++j)
          tok_str_add(dst, pm->str->str[j]);
        tok_str_add(dst, ')');
        if (j < pm->str->len)
          tok_str_add(dst, ';');
        i = j;
        continue;
      }
    }
keep_field_assignment:
    if (t >= TOK_IDENT
        && t != this_tok
        && (i == 0 || (pm->str->str[i - 1] != '.'
                       && pm->str->str[i - 1] != TOK_ARROW
                       && pm->str->str[i - 1] != ':'))
        && (i + 1 >= pm->str->len || pm->str->str[i + 1] != '('))
    {
      int dummy_ofs;
      if (find_field_try(&class_type, t, &dummy_ofs))
      {
        tok_str_add(dst, this_tok);
        tok_str_add(dst, TOK_ARROW);
      }
    }
    tok_str_add(dst, t);
  }
  tok_str_add(dst, TOK_EOF);
  tok_str_free(pm->str);
  pm->str = dst;
}

static void compile_pending_member_funcs(int start)
{
  int i, saved_tok, out, len_tok, array_tok;
  int saved_local_scope;
  int saved_defer_pending_member_funcs;
  CValue saved_tokc;
  Sym *saved_local_stack;

  /* Re-entrancy guard: compiling a replayed member body can itself resolve
     member calls that queue and flush pending members.  A nested flush would
     compact the shared queue while the outer pass is mid-iteration and can
     drop entries the outer pass still owes (e.g. out-of-class template
     operator bodies).  The outer pass iterates with a live bound, so it picks
     up every entry appended during the pass; nested flushes must simply stay
     out of the way. */
  if (compiling_pending_member_funcs)
    return;
  compiling_pending_member_funcs++;

  saved_tok = tok;
  saved_tokc = tokc;
  saved_local_stack = local_stack;
  saved_local_scope = local_scope;
  saved_defer_pending_member_funcs = defer_pending_member_funcs;
  len_tok = tok_alloc_const("_len");
  array_tok = tok_alloc_const("_array");
  out = start;
  for (i = start; i < nb_pending_member_funcs; ++i)
  {
    PendingMemberFunc *pm = pending_member_funcs[i];
    if (!pm)
      continue;
    int guard_non_lifecycle =
      pm->is_template_member
      && !pm->is_lifecycle_member;
    int compile_in_global_scope = 0;
    if (compile_lifecycle_member_funcs_only && !pm->is_lifecycle_member)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    if ((local_stack || local_scope) && !compile_in_global_scope)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    if (pm->struct_tok)
    {
      Sym *class_sym = struct_find(pm->struct_tok);
      if (!class_sym || class_sym->c < 0)
      {
        pending_member_funcs[out++] = pm;
        continue;
      }
    }
    if (pm->str->len > 0
        && pm->str->str[0] == TOK_AUTO)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    if (token_string_contains_tok(pm->str, len_tok)
        || token_string_contains_tok(pm->str, array_tok))
    {
      tok_str_free(pm->str);
      cprime_free(pm);
      pending_member_funcs[i] = NULL;
      continue;
    }
    rewrite_pending_member_field_access(pm);
    if (guard_non_lifecycle)
      compiling_non_lifecycle_template_member_body++;
    begin_macro(pm->str, 1);
    next();
    if (compile_in_global_scope)
    {
      local_stack = NULL;
      local_scope = 0;
    }
    defer_pending_member_funcs = 1;
    decl(VT_CONST);
    defer_pending_member_funcs = saved_defer_pending_member_funcs;
    /* Each replayed member body is an independent function definition.
       decl() leaves the enclosing local scope live, so restore the saved
       scope before the next queued member is inspected; otherwise a member
       appended mid-flush (e.g. a constructor needed by a static member's
       body) is wrongly deferred forever. */
    local_stack = saved_local_stack;
    local_scope = saved_local_scope;
    end_macro();
    if (guard_non_lifecycle)
      compiling_non_lifecycle_template_member_body--;
    /* The compiled body's token string is freed by end_macro(); clear the
       queue slot so nested scans (pending_member_func_has_body_tok) cannot
       read the dangling token string before the queue is compacted. */
    pending_member_funcs[i] = NULL;
  }
  nb_pending_member_funcs = out;
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
  tok = saved_tok;
  tokc = saved_tokc;
  compiling_pending_member_funcs--;
}

static void compile_pending_lifecycle_member_funcs(int start)
{
  int saved_compile_lifecycle_member_funcs_only =
    compile_lifecycle_member_funcs_only;
  compile_lifecycle_member_funcs_only = 1;
  compile_pending_member_funcs(start);
  compile_lifecycle_member_funcs_only =
    saved_compile_lifecycle_member_funcs_only;
}

static void parse_namespace_decl(void)
{
  int ns_tok;

  next();
  if (tok < TOK_UIDENT)
    cprime_error("namespace name");
  ns_tok = tok;
  note_namespace_tok(ns_tok);
  next();
  if (nb_namespace_stack >= (int)(sizeof(namespace_stack) / sizeof(namespace_stack[0])))
    cprime_error("namespace nesting too deep");
  skip('{');
  namespace_stack[nb_namespace_stack++] = ns_tok;
  while (tok != TOK_EOF && tok != '}')
    decl(VT_CONST);
  skip('}');
  --nb_namespace_stack;
  if (tok == ';')
    next();
}

static TemplateDef *find_template_def(int name_tok)
{
  int i;
  for (i = 0; i < nb_template_defs; ++i)
  {
    if ((template_defs[i]->lookup_tok ? template_defs[i]->lookup_tok
                                      : template_defs[i]->name_tok) == name_tok)
      return template_defs[i];
  }
  return NULL;
}

static TemplateDef *find_class_template_def(int name_tok)
{
  int i;

  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class)
      continue;
    if ((td->lookup_tok ? td->lookup_tok : td->name_tok) == name_tok)
      return td;
  }
  return NULL;
}

static int template_def_first_param_matches_call(TemplateDef *td,
                                                 CType *arg_types,
                                                 int arg_count)
{
  int i, n, name_index = -1, paren_index = -1, depth = 0;
  CType arg_value_type;

  if (!td || !td->def_str || arg_count <= 0 || !arg_types)
    return 1;
  n = td->def_str->len;
  for (i = 0; i + 1 < n; ++i)
    if (td->def_str->str[i] == td->name_tok
        && td->def_str->str[i + 1] == '(')
    {
      name_index = i;
      paren_index = i + 1;
      break;
    }
  if (name_index < 0)
    return 1;

  arg_value_type = arg_types[0];
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);

  for (i = paren_index + 1; i + 1 < n; ++i)
  {
    int t = td->def_str->str[i];
    if (t == TOK_LINENUM)
      continue;
    if (t == '(' || t == '[')
      ++depth;
    else if ((t == ')' || t == ']') && depth > 0)
      --depth;
    else if (t == TOK_LT || t == '<')
      ++depth;
    else if ((t == TOK_GT || t == '>' || t == TOK_SAR) && depth > 0)
      --depth;
    else if (t == ',' && depth == 0)
      break;
    else if (t == ')' && depth == 0)
      break;
    if (t >= TOK_UIDENT)
    {
      TemplateDef *class_td = find_class_template_def(t);
      int next = i + 1;
      while (next + 1 < n && td->def_str->str[next] == TOK_LINENUM)
        next += 2;
      if (class_td && class_td->is_class
          && next < n
          && (td->def_str->str[next] == TOK_LT
              || td->def_str->str[next] == '<'))
      {
        int arg_tok = 0, j;
        if ((arg_value_type.t & VT_BTYPE) == VT_STRUCT)
          arg_tok = get_struct_type_name_tok(&arg_value_type);
        if (!arg_tok)
          return 0;
        for (j = 0; j < class_td->nb_inst; ++j)
          if (class_td->inst_name_toks[j] == arg_tok)
            return 1;
        return 0;
      }
    }
  }
  return 1;
}

static TemplateDef *find_function_template_for_call(int name_tok,
                                                    CType *arg_types,
                                                    int arg_count)
{
  int i;
  TemplateDef *variadic = NULL;
  TemplateDef *best_defaulted = NULL;
  int best_min = 0;

  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (td->is_class)
      continue;
    if ((td->lookup_tok ? td->lookup_tok : td->name_tok) != name_tok)
      continue;
    if (!td->func_is_variadic && td->func_min_args == arg_count)
    {
      if (template_def_first_param_matches_call(td, arg_types, arg_count))
        return td;
      continue;
    }
    if (!td->func_is_variadic && arg_count >= td->func_min_args
        && arg_count <= td->func_max_args
        && template_def_first_param_matches_call(td, arg_types, arg_count)
        && (!best_defaulted || td->func_min_args > best_min))
    {
      best_defaulted = td;
      best_min = td->func_min_args;
    }
    if (td->func_is_variadic && arg_count >= td->func_min_args
        && template_def_first_param_matches_call(td, arg_types, arg_count))
      variadic = td;
  }
  if (best_defaulted)
    return best_defaulted;
  return variadic;
}

static TemplateDef *find_function_template_def(int name_tok)
{
  int i;

  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (td->is_class)
      continue;
    if ((td->lookup_tok ? td->lookup_tok : td->name_tok) == name_tok)
      return td;
  }
  return NULL;
}

static int token_string_template_duplicate(TemplateDef *td, int name_tok,
                                           int is_class, int func_min_args,
                                           int func_max_args,
                                           int func_is_variadic,
                                           unsigned func_sig_hash,
                                           int is_partial_specialization)
{
  int lookup_tok = nb_namespace_stack ? make_current_namespace_tok(name_tok)
                                      : name_tok;

  if ((td->lookup_tok ? td->lookup_tok : td->name_tok) != lookup_tok)
    return 0;
  if (td->is_class || is_class)
  {
    if (td->is_class == is_class
        && (td->is_partial_specialization || is_partial_specialization))
      return 0;
    return td->is_class == is_class;
  }
  return td->func_min_args == func_min_args
         && td->func_max_args == func_max_args
         && td->func_is_variadic == func_is_variadic
         && td->func_sig_hash == func_sig_hash;
}

static void analyze_template_function_signature(TokenString *str, int name_tok,
                                                int *min_args,
                                                int *max_args,
                                                int *is_variadic,
                                                unsigned *sig_hash)
{
  int i, n = str->len, paren = 0, angle = 0;
  int in_param = 0, param_has_pack = 0, param_has_default = 0;

  *min_args = 0;
  *max_args = 0;
  *is_variadic = 0;
  *sig_hash = 2166136261u;
  for (i = 0; i + 1 < n; ++i)
  {
    if (str->str[i] == name_tok && str->str[i + 1] == '(')
    {
      i += 2;
      break;
    }
  }
  if (i >= n)
    return;
  for (; i < n; ++i)
  {
    int t = str->str[i];
    if (t == TOK_EOF)
      break;
    *sig_hash ^= (unsigned)t;
    *sig_hash *= 16777619u;
    if (t == TOK_LT)
      angle++;
    else if ((t == TOK_GT || t == TOK_SAR) && angle > 0)
      angle--;
    else if (angle == 0)
    {
      if (t == '(')
        paren++;
      else if (t == ')' && paren == 0)
      {
        if (in_param && !param_has_pack)
        {
          (*min_args)++;
          (*max_args)++;
          if (param_has_default)
            (*min_args)--;
        }
        break;
      }
      else if (t == ')' && paren > 0)
        paren--;
      else if (t == ',' && paren == 0)
      {
        if (in_param && !param_has_pack)
        {
          (*min_args)++;
          (*max_args)++;
          if (param_has_default)
            (*min_args)--;
        }
        in_param = 0;
        param_has_pack = 0;
        param_has_default = 0;
        continue;
      }
      else if (t == '=')
        param_has_default = 1;
      else if (t == TOK_DOTS)
      {
        *is_variadic = 1;
        param_has_pack = 1;
      }
    }
    if (t != TOK_LINENUM)
      in_param = 1;
  }
}

static int find_template_name_in_str(TokenString *str)
{
  int i, n = str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int t = str->str[i];
    if (t >= TOK_UIDENT && str->str[i + 1] == '(')
      return t;
  }
  return 0;
}

static int is_class_template_partial_specialization(TokenString *str,
                                                    int name_tok)
{
  int i, n = str->len;

  for (i = 0; i + 1 < n; ++i)
    if (str->str[i] == name_tok && str->str[i + 1] == TOK_LT)
      return 1;
  return 0;
}

static int find_template_member_class_in_str(TokenString *str, int type_param_tok)
{
  int i, n = str->len, brace = 0;

  for (i = 0; i + 6 < n; ++i)
  {
    if (str->str[i] == '{')
    {
      if (brace == 0)
        break;
      ++brace;
    }
    else if (str->str[i] == '}' && brace > 0)
      --brace;
    if (str->str[i] >= TOK_UIDENT
        && (str->str[i + 1] == TOK_LT || str->str[i + 1] == '<'))
    {
      int j, angle = 1, has_param = 0;
      for (j = i + 2; j < n && angle > 0; ++j)
      {
        int t = str->str[j];
        if (t == type_param_tok)
          has_param = 1;
        if (t == TOK_LT || t == '<')
          ++angle;
        else if (t == TOK_GT || t == '>')
          --angle;
        else if (t == TOK_SAR)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
      }
      if (has_param && angle == 0 && j + 2 < n
          && str->str[j] == ':' && str->str[j + 1] == ':'
          && (str->str[j + 2] >= TOK_UIDENT
              || str->str[j + 2] == TOK_OPERATOR))
        return str->str[i];
      if (has_param && angle == 0 && j + 3 < n
          && str->str[j] == ':' && str->str[j + 1] == ':'
          && str->str[j + 2] == '~' && str->str[j + 3] >= TOK_UIDENT)
        return str->str[i];
    }
  }
  return 0;
}

static int token_string_starts_template(TokenString *str);

static int template_class_needs_member_layout(TemplateDef *td)
{
  int i;

  if (!td->is_class)
    return 0;
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (md->class_tok == td->name_tok
        && !(md->def_str->len > 0 && is_template_keyword_tok(md->def_str->str[0])))
      return 1;
  }
  return 0;
}

static int template_member_lookup_inst(TemplateMemberDef *md, int type_tok,
                                       int pack_tok)
{
  int i;
  int is_variadic = token_string_has_variadic_pack(md->def_str);

  for (i = 0; i < md->nb_inst; ++i)
    if (md->inst_type_toks[i] == type_tok
        && (!is_variadic
            || (md->inst_pack_toks && md->inst_pack_toks[i] == pack_tok)))
      return 1;
  return 0;
}

static int template_member_cached_ret(TemplateMemberDef *md, int class_tok,
                                      CType *out)
{
  int i;

  if (!md || !out)
    return 0;
  for (i = 0; i < md->nb_inst_ret_types; ++i)
    if (md->inst_class_toks[i] == class_tok)
    {
      *out = md->inst_ret_types[i];
      return 1;
    }
  return 0;
}

static void template_member_note_cached_ret(TemplateMemberDef *md,
                                            int class_tok, CType *ret)
{
  int i;

  if (!md || !ret)
    return;
  for (i = 0; i < md->nb_inst_ret_types; ++i)
    if (md->inst_class_toks[i] == class_tok)
    {
      md->inst_ret_types[i] = *ret;
      return;
    }
  if (md->nb_inst_ret_types >= md->al_inst_ret_types)
  {
    md->al_inst_ret_types = md->al_inst_ret_types
                            ? md->al_inst_ret_types * 2 : 4;
    md->inst_class_toks =
      cprime_realloc(md->inst_class_toks,
                     md->al_inst_ret_types * sizeof(int));
    md->inst_ret_types =
      cprime_realloc(md->inst_ret_types,
                     md->al_inst_ret_types * sizeof(CType));
  }
  md->inst_class_toks[md->nb_inst_ret_types] = class_tok;
  md->inst_ret_types[md->nb_inst_ret_types++] = *ret;
}

static int is_currently_defining_class(int type_tok)
{
  int i;

  for (i = 0; i < nb_defining_class_stack; ++i)
    if (defining_class_stack[i] == type_tok)
      return 1;
  return 0;
}

static void template_member_note_inst(TemplateMemberDef *md, int type_tok,
                                      int pack_tok)
{
  if (md->nb_inst >= md->al_inst)
  {
    md->al_inst = md->al_inst ? md->al_inst * 2 : 4;
    md->inst_type_toks = cprime_realloc(md->inst_type_toks,
                                     md->al_inst *sizeof(int));
    md->inst_pack_toks = cprime_realloc(md->inst_pack_toks,
                                     md->al_inst *sizeof(int));
  }
  md->inst_type_toks[md->nb_inst++] = type_tok;
  md->inst_pack_toks[md->nb_inst - 1] = pack_tok;
}

static void tok_str_add_template_subst(TokenString *str, int type_param_tok,
                                       int type_tok)
{
  if (tok == type_param_tok)
    tok_str_add(str, type_tok);
  else
    tok_str_add_tok(str);
}

static void tok_str_add_cint(TokenString *str, int value);

static void tok_str_add_template_subst_plain(TokenString *str,
    int type_param_tok, int type_tok)
{
  tok_str_add(str, tok == type_param_tok ? type_tok : tok);
}

static int template_member_class_arg_subst_tok(TemplateMemberDef *md,
    int class_mangled_tok, int param_tok)
{
  TemplateDef *td;
  int i, j;

  if (!md || class_mangled_tok < TOK_UIDENT || param_tok < TOK_UIDENT)
    return 0;
  td = find_template_def(md->class_tok);
  if (!td || !td->is_class)
    return 0;
  for (i = 0; i < td->nb_type_params; ++i)
    if (td->type_param_toks[i] == param_tok)
      break;
  if (i == td->nb_type_params)
    return 0;
  for (j = 0; j < td->nb_inst; ++j)
    if (td->inst_name_toks[j] == class_mangled_tok)
      return td->inst_type_toks[j * td->nb_type_params + i];
  return 0;
}

static void tok_str_add_template_member_subst(TokenString *str,
    TemplateMemberDef *md, int class_mangled_tok, int type_param_tok,
    int type_tok, int member_type_param_tok, int member_type_arg_tok,
    int value_param_tok)
{
  int class_arg_tok = template_member_class_arg_subst_tok(md,
                                                          class_mangled_tok,
                                                          tok);
  if (class_arg_tok)
    tok_str_add(str, class_arg_tok);
  else if (tok == type_param_tok)
    tok_str_add(str, type_tok);
  else if (member_type_param_tok && tok == member_type_param_tok)
    tok_str_add(str, member_type_arg_tok ? member_type_arg_tok : type_tok);
  else if (value_param_tok && tok == value_param_tok)
    tok_str_add_cint(str, 1);
  else
    tok_str_add_tok(str);
}

static void note_template_alias_inst(int class_tok, int alias_tok,
                                     int scoped_tok)
{
  int i;

  if (class_tok < TOK_UIDENT || alias_tok < TOK_UIDENT)
    return;
  for (i = 0; i < nb_template_alias_insts; ++i)
    if (template_alias_insts[i].class_tok == class_tok
        && template_alias_insts[i].alias_tok == alias_tok)
      return;
  if (nb_template_alias_insts >= al_template_alias_insts)
  {
    al_template_alias_insts = al_template_alias_insts
                              ? al_template_alias_insts * 2
                              : 32;
    template_alias_insts = cprime_realloc(template_alias_insts,
        al_template_alias_insts * sizeof(*template_alias_insts));
  }
  template_alias_insts[nb_template_alias_insts].class_tok = class_tok;
  template_alias_insts[nb_template_alias_insts].alias_tok = alias_tok;
  template_alias_insts[nb_template_alias_insts].scoped_tok = scoped_tok;
  nb_template_alias_insts++;
}

static int template_member_scoped_alias_tok(int class_tok, int alias_tok)
{
  int i;

  if (class_tok < TOK_UIDENT || alias_tok < TOK_UIDENT)
    return 0;
  for (i = nb_template_alias_insts - 1; i >= 0; --i)
    if (template_alias_insts[i].class_tok == class_tok
        && template_alias_insts[i].alias_tok == alias_tok)
      return template_alias_insts[i].scoped_tok;
  return 0;
}

static void tok_str_add_template_member_subst_scoped(TokenString *str,
    TemplateMemberDef *md, int class_mangled_tok, int type_param_tok,
    int type_tok, int member_type_param_tok, int member_type_arg_tok,
    int value_param_tok)
{
  int alias_tok, class_arg_tok;

  class_arg_tok = template_member_class_arg_subst_tok(md, class_mangled_tok,
                                                      tok);
  if (class_arg_tok)
  {
    tok_str_add(str, class_arg_tok);
    return;
  }
  if (tok == type_param_tok)
  {
    tok_str_add(str, type_tok);
    return;
  }
  if (member_type_param_tok && tok == member_type_param_tok)
  {
    tok_str_add(str, member_type_arg_tok ? member_type_arg_tok : type_tok);
    return;
  }
  if (value_param_tok && tok == value_param_tok)
  {
    tok_str_add_cint(str, 1);
    return;
  }
  alias_tok = tok >= TOK_UIDENT
              ? template_member_scoped_alias_tok(class_mangled_tok, tok)
              : 0;
  if (alias_tok)
    tok_str_add(str, alias_tok);
  else
    tok_str_add_tok(str);
}

static void tok_str_append(TokenString *dst, TokenString *src)
{
  int i;

  for (i = 0; i < src->len; ++i)
    tok_str_add(dst, src->str[i]);
}

static void tok_str_append_without_eof(TokenString *dst, TokenString *src)
{
  int i, n = src->len;

  if (n > 0 && src->str[n - 1] == TOK_EOF)
    --n;
  for (i = 0; i < n; ++i)
    tok_str_add(dst, src->str[i]);
}

static TokenString *strip_static_call_synthetic_this(TokenString *src)
{
  int i, changed = 0;
  TokenString *dst = tok_str_alloc();

  for (i = 0; i < src->len; ++i)
  {
    int t = src->str[i];
    if (i + 6 < src->len
        && t >= TOK_UIDENT
        && has_free_func_overload(t)
        && src->str[i + 1] == '('
        && src->str[i + 2] == TOK_STRUCT
        && src->str[i + 3] >= TOK_UIDENT
        && src->str[i + 4] == '*'
        && src->str[i + 5] >= TOK_UIDENT
        && !strcmp(get_tok_str(src->str[i + 5], NULL), "this")
        && src->str[i + 6] == ',')
    {
      tok_str_add(dst, t);
      tok_str_add(dst, '(');
      i += 6;
      changed = 1;
      continue;
    }
    tok_str_add(dst, t);
  }

  if (!changed)
  {
    tok_str_free(dst);
    return src;
  }
  return dst;
}

static TokenString *balance_generated_template_spec(TokenString *src)
{
  int i, brace = 0, changed = 0;
  TokenString *dst;

  for (i = 0; i < src->len; ++i)
  {
    if (src->str[i] == '{')
      ++brace;
    else if (src->str[i] == '}' && brace > 0)
      --brace;
  }
  if (brace <= 0)
    return src;

  dst = tok_str_alloc();
  for (i = 0; i < src->len; ++i)
  {
    if (src->str[i] == TOK_EOF)
    {
      while (brace-- > 0)
        tok_str_add(dst, '}');
      changed = 1;
    }
    tok_str_add(dst, src->str[i]);
  }
  if (!changed)
    while (brace-- > 0)
      tok_str_add(dst, '}');
  return dst;
}

static void tok_str_add_cint(TokenString *str, int value)
{
  CValue cv;
  cv.i = value;
  tok_str_add2(str, TOK_CINT, &cv);
}

static int is_template_keyword_tok(int t)
{
  return t >= TOK_UIDENT && !strcmp(get_tok_str(t, NULL), "template");
}

static int token_string_starts_template(TokenString *str)
{
  int i = 0;

  if (!str)
    return 0;
  while (i + 1 < str->len && str->str[i] == TOK_LINENUM)
    i += 2;
  return i < str->len && is_template_keyword_tok(str->str[i]);
}

static int token_string_has_variadic_pack(TokenString *str)
{
  int i;

  if (!str)
    return 0;
  for (i = 0; i < str->len && str->str[i] != TOK_EOF; ++i)
    if (str->str[i] == TOK_DOTS)
      return 1;
  return 0;
}

/* Only the body's pack expansions (TOK_DOTS after the first '{') are unsafe
   to replay with a single concrete element so far.  Signature `...` in
   `Args&&... args` is handled by the substitution pass. */
static int token_string_body_has_variadic_pack(TokenString *str)
{
  int i, brace = 0;

  if (!str)
    return 0;
  for (i = 0; i < str->len && str->str[i] != TOK_EOF; ++i)
  {
    if (str->str[i] == '{')
    {
      brace = 1;
      continue;
    }
    if (brace && str->str[i] == TOK_DOTS)
      return 1;
  }
  return 0;
}

static TokenString *skip_or_save_template_member_decl(int save)
{
  int angle = 0, paren = 0, brace = 0, prev_tok = 0;
  TokenString *str = save ? tok_str_alloc() : NULL;

  if (is_template_keyword_tok(tok))
  {
    if (str)
      tok_str_add_tok(str);
    next();
    if (tok == TOK_LT || tok == '<')
    {
      angle = 1;
      if (str)
        tok_str_add_tok(str);
      next();
      while (tok != TOK_EOF && angle > 0)
      {
        if (str)
          tok_str_add_tok(str);
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
        next();
      }
    }
  }
  angle = 0;
  for (;;)
  {
    if (tok == TOK_EOF)
      cprime_error("unexpected end of file in member template declaration (angle=%d, paren=%d, brace=%d, prev='%s')",
                angle, paren, brace, get_tok_str(prev_tok, NULL));
    if (str)
      tok_str_add_tok(str);
    if (brace > 0)
    {
      if (tok == '{')
        ++brace;
      else if (tok == '}')
      {
        --brace;
        if (brace == 0)
        {
          next();
          break;
        }
      }
      prev_tok = tok;
      next();
      continue;
    }
    if (prev_tok != TOK_OPERATOR && (tok == TOK_LT || tok == '<'))
      ++angle;
    else if (prev_tok != TOK_OPERATOR && (tok == TOK_GT || tok == '>') && angle > 0)
      --angle;
    else if (prev_tok != TOK_OPERATOR && tok == TOK_SAR && angle > 0)
    {
      --angle;
      if (angle > 0)
        --angle;
    }
    else if (angle == 0)
    {
      if (tok == '(')
        ++paren;
      else if (tok == ')' && paren > 0)
        --paren;
      else if (tok == '{')
        ++brace;
      else if (tok == '}' && brace > 0)
      {
        --brace;
        if (brace == 0)
        {
          next();
          break;
        }
      }
      else if (tok == ';' && paren == 0 && brace == 0)
      {
        next();
        break;
      }
    }
    prev_tok = tok;
    next();
  }
  if (str)
    tok_str_add(str, TOK_EOF);
  return str;
}

static void skip_template_member_decl(void)
{
  TokenString *str = skip_or_save_template_member_decl(0);
  if (str)
    tok_str_free(str);
}

static int is_lifecycle_member_tok(int t)
{
  return t == TOK_CONSTRUCTOR1 || t == TOK_CONSTRUCTOR2
         || t == TOK_DESTRUCTOR1 || t == TOK_DESTRUCTOR2;
}

static int skip_member_func_cv_qualifiers(void)
{
  int qualifiers = 0;
  for (;;)
  {
    if (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
      qualifiers |= VT_CONSTANT;
    else if (tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
             || tok == TOK_VOLATILE3)
      qualifiers |= VT_VOLATILE;
    else if (tok >= TOK_UIDENT
             && (!strcmp(get_tok_str(tok, NULL), "override")
                 || !strcmp(get_tok_str(tok, NULL), "final")))
    {
      next();
      continue;
    }
    else
      break;
    next();
  }
  return qualifiers;
}

static int skip_defaulted_or_deleted_member_suffix(void)
{
  int is_deleted;
  if (tok != '=')
    return 0;
  next();
  if (tok == TOK_CINT && tokc.i == 0)
  {
    next();
    return 3;
  }
  if (strcmp(get_tok_str(tok, NULL), "default")
      && strcmp(get_tok_str(tok, NULL), "delete"))
    cprime_error("expected defaulted or deleted member function");
  is_deleted = !strcmp(get_tok_str(tok, NULL), "delete");
  next();
  return is_deleted ? 2 : 1;
}

static int class_has_member_func_name(int class_tok, int member_tok)
{
  CType class_type;
  Sym *class_sym, *field;
  int dummy_ofs;

  class_sym = struct_find(class_tok);
  if (!class_sym)
    return 0;
  class_type.t = class_sym->type.t;
  class_type.ref = class_sym;
  field = find_field_try(&class_type, member_tok, &dummy_ofs);
  if (!field)
    field = find_field_try(&class_type, member_tok | SYM_FIELD, &dummy_ofs);
  return field && ((field->type.t & VT_BTYPE) == VT_FUNC);
}

static int type_has_member_func_name(CType *type, int member_tok)
{
  Sym *func_sym, *field;
  MemberFuncOverload *o;
  int dummy_ofs, struct_tok;

  if (!type || ((type->t & VT_BTYPE) != VT_STRUCT))
    return 0;
  struct_tok = get_struct_type_name_tok(type);
  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == member_tok)
      return 1;
  field = find_field_try(type, member_tok, &dummy_ofs);
  if (!field)
    field = find_field_try(type, member_tok | SYM_FIELD, &dummy_ofs);
  if (field && ((field->type.t & VT_BTYPE) == VT_FUNC))
    return 1;
  func_sym = resolve_member_func(type, member_tok);
  return func_sym && !IS_ASM_SYM(func_sym);
}

static int template_member_def_method_tok(TemplateMemberDef *md)
{
  int i, j, tok0, angle = 0, paren = 0;
  int result = 0;

  if (!md || !md->def_str)
    return 0;
  for (i = 0; i + 2 < md->def_str->len; ++i)
  {
    if (md->def_str->str[i] == ':' && md->def_str->str[i + 1] == ':')
    {
      tok0 = md->def_str->str[i + 2];
      if (tok0 == TOK_OPERATOR && i + 3 < md->def_str->len)
      {
        int operator_tok = md->def_str->str[i + 3];
        if (operator_tok == '[' && i + 4 < md->def_str->len
            && md->def_str->str[i + 4] == ']')
          return tok_alloc_const("operator[]");
        if (operator_tok == '(' && i + 4 < md->def_str->len
            && md->def_str->str[i + 4] == ')')
          return tok_alloc_const("operator()");
        return get_cpp_binary_operator_method_tok(operator_tok);
      }
      if (tok0 == '~' && i + 3 < md->def_str->len)
        return md->def_str->str[i + 3] == md->class_tok
               ? TOK_DESTRUCTOR1 : md->def_str->str[i + 3];
      if (tok0 == md->class_tok)
        return TOK_CONSTRUCTOR1;
      if (tok0 >= TOK_UIDENT)
        return tok0;
    }
  }
  /* Inline member templates are captured without a Class:: qualifier.
     Skip the template header, then find the method name in the
     declaration: an identifier/operator that introduces the parameter
     list at angle depth zero. */
  i = 0;
  while (i + 1 < md->def_str->len && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  if (i < md->def_str->len && is_template_keyword_tok(md->def_str->str[i]))
  {
    ++i;
    if (i < md->def_str->len
        && (md->def_str->str[i] == TOK_LT || md->def_str->str[i] == '<'))
    {
      angle = 1;
      ++i;
      while (i < md->def_str->len && md->def_str->str[i] != TOK_EOF
             && angle > 0)
      {
        int t = md->def_str->str[i];
        if (t == TOK_LT || t == '<')
          ++angle;
        else if ((t == TOK_GT || t == '>') && angle > 0)
          --angle;
        else if (t == TOK_SAR && angle > 0)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
        ++i;
      }
    }
    angle = 0;
    while (i < md->def_str->len && md->def_str->str[i] != TOK_EOF)
    {
      int t = md->def_str->str[i];
      if (t == TOK_LT || t == '<')
      {
        ++angle;
        ++i;
        continue;
      }
      else if ((t == TOK_GT || t == '>') && angle > 0)
      {
        --angle;
        ++i;
        continue;
      }
      if (angle == 0)
      {
        if (t == TOK_OPERATOR && i + 1 < md->def_str->len)
        {
          int op_tok = md->def_str->str[i + 1];
          if (op_tok == '[' && i + 2 < md->def_str->len
              && md->def_str->str[i + 2] == ']')
            return tok_alloc_const("operator[]");
          if (op_tok == '(' && i + 2 < md->def_str->len
              && md->def_str->str[i + 2] == ')')
            return tok_alloc_const("operator()");
          return get_cpp_binary_operator_method_tok(op_tok);
        }
        if (t >= TOK_UIDENT
            && t != md->type_param_tok
            && i + 1 < md->def_str->len
            && md->def_str->str[i + 1] == '(')
          return t;
      }
      ++i;
    }
  }
  return 0;
}

static int template_member_def_is_scoped(TemplateMemberDef *md)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 5 < n; ++i)
  {
    if (md->def_str->str[i] == md->class_tok
        && (md->def_str->str[i + 1] == TOK_LT
            || md->def_str->str[i + 1] == '<'))
    {
      int j, angle = 1, has_param = 0;
      for (j = i + 2; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (t == md->type_param_tok)
          has_param = 1;
        if (t == TOK_LT || t == '<')
          ++angle;
        else if (t == TOK_GT || t == '>')
          --angle;
        else if (t == TOK_SAR)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
      }
      if (has_param && angle == 0 && j + 1 < n
          && md->def_str->str[j] == ':' && md->def_str->str[j + 1] == ':')
        return 1;
    }
  }
  return 0;
}

static int template_member_def_has_type_template_param(TemplateMemberDef *md)
{
  int angle = 0;
  int i;

  if (!md || !md->def_str || !token_string_starts_template(md->def_str))
    return 0;
  for (i = 0; i < md->def_str->len; ++i)
  {
    int t = md->def_str->str[i];
    if (t == TOK_LT || t == '<')
    {
      ++angle;
      continue;
    }
    if ((t == TOK_GT || t == '>') && angle > 0)
    {
      --angle;
      if (!angle)
        return 0;
      continue;
    }
    if (angle == 1
        && (t == TOK_CLASS
            || (t >= TOK_UIDENT
                && !strcmp(get_tok_str(t, NULL), "typename"))))
      return 1;
  }
  return 0;
}

static int token_string_contains_tok(TokenString *str, int needle)
{
  int i;

  if (!str)
    return 0;
  for (i = 0; i < str->len; ++i)
    if (str->str[i] == needle)
      return 1;
  return 0;
}

static int token_string_has_initializer_list(TokenString *str)
{
  int i;

  if (!str)
    return 0;
  for (i = 0; i < str->len; ++i)
    if (str->str[i] >= TOK_UIDENT
        && strstr(get_tok_str(str->str[i], NULL), "initializer_list"))
      return 1;
  return 0;
}

static int ctype_is_initializer_list(CType *type)
{
  int struct_tok;

  if (!type)
    return 0;
  struct_tok = get_struct_type_name_tok(type);
  return struct_tok
         && strstr(get_tok_str(struct_tok, NULL), "initializer_list") != NULL;
}

static int token_string_has_empty_body(TokenString *str)
{
  int i;

  if (!str)
    return 0;
  for (i = 0; i < str->len; ++i)
  {
    if (str->str[i] == '{')
    {
      ++i;
      while (i + 1 < str->len && str->str[i] == TOK_LINENUM)
        i += 2;
      return i < str->len && str->str[i] == '}';
    }
  }
  return 0;
}

static int template_member_def_param_count(TemplateMemberDef *md,
                                           int method_tok)
{
  int i, n, name_tok;

  if (!md || !md->def_str)
    return -1;
  if (method_tok == TOK_DESTRUCTOR1)
    return 0;
  n = md->def_str->len;
  name_tok = method_tok == TOK_CONSTRUCTOR1 ? md->class_tok : method_tok;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0, count = 0, saw = 0;

    if (md->def_str->str[i] != name_tok)
      continue;
    j = i + 1;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (t == TOK_LT || t == '<')
          ++angle;
        else if (t == TOK_GT || t == '>')
          --angle;
        else if (t == TOK_SAR)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
      }
      while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
    }
    if (j >= n || md->def_str->str[j] != '(')
      continue;
    for (++j; j < n; ++j)
    {
      int t = md->def_str->str[j];
      if (t == TOK_EOF)
        return -1;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if (t == ')' && !paren && !angle && !bracket)
        return saw ? count + 1 : 0;
      else if (t == '[')
        ++bracket;
      else if (t == ']' && bracket > 0)
        --bracket;
      else if (t == TOK_LT || t == '<')
        ++angle;
      else if ((t == TOK_GT || t == '>') && angle > 0)
        --angle;
      else if (t == TOK_SAR && angle > 0)
      {
        --angle;
        if (angle > 0)
          --angle;
      }
      else if (t == ',' && !paren && !angle && !bracket)
      {
        ++count;
        saw = 0;
        continue;
      }
      if (t != TOK_LINENUM)
        saw = 1;
    }
  }
  return -1;
}

static int template_member_def_first_param_is_array_ref(TemplateMemberDef *md,
                                                        int method_tok)
{
  int i, n, name_tok;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  name_tok = method_tok == TOK_CONSTRUCTOR1 ? md->class_tok : method_tok;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0;
    int saw_ref = 0, saw_array = 0;

    if (md->def_str->str[i] != name_tok)
      continue;
    j = i + 1;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (t == TOK_LT || t == '<')
          ++angle;
        else if (t == TOK_GT || t == '>')
          --angle;
        else if (t == TOK_SAR)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
      }
      while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
    }
    if (j >= n || md->def_str->str[j] != '(')
      continue;
    for (++j; j < n; ++j)
    {
      int t = md->def_str->str[j];
      if (t == TOK_EOF)
        return 0;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if ((t == ')' && !paren) || (t == ',' && !paren))
        return saw_ref && saw_array;
      else if (t == '&')
        saw_ref = 1;
      else if (t == '[')
        saw_array = 1;
    }
  }
  return 0;
}

/* Classify the definition's first parameter type: 1 for an explicit
   class/struct type (e.g. const clList<i64> &), 0 for an explicit scalar
   type, -1 when the type is a generic template parameter or otherwise
   unknown.  Used to keep an unrelated arity match (Erase(c--) with i64)
   from instantiating a member whose signature takes a different class
   argument. */
static int template_member_def_first_param_class_kind(TemplateMemberDef *md,
                                                      int method_tok)
{
  int i, n, name_tok;

  if (!md || !md->def_str)
    return -1;
  n = md->def_str->len;
  name_tok = method_tok == TOK_CONSTRUCTOR1 ? md->class_tok : method_tok;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0;

    if (md->def_str->str[i] != name_tok)
      continue;
    j = i + 1;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (t == TOK_LT || t == '<')
          ++angle;
        else if (t == TOK_GT || t == '>')
          --angle;
        else if (t == TOK_SAR)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
      }
      while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
    }
    if (j >= n || md->def_str->str[j] != '(')
      continue;
    for (++j; j < n; ++j)
    {
      int t = md->def_str->str[j];
      if (t == TOK_EOF)
        return -1;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if ((t == ')' || t == ',') && !paren && !angle && !bracket)
        return -1;
      else if (t == '[')
        ++bracket;
      else if (t == ']' && bracket > 0)
        --bracket;
      else if (t == TOK_LT || t == '<')
        ++angle;
      else if ((t == TOK_GT || t == '>') && angle > 0)
        --angle;
      else if (t == TOK_SAR && angle > 0)
      {
        --angle;
        if (angle > 0)
          --angle;
      }
      else if (t == TOK_STRUCT
               || (t >= TOK_UIDENT
                   && (t == md->class_tok
                       || (md->class_tok >= TOK_UIDENT
                           && strstr(get_tok_str(t, NULL),
                                     get_tok_str(md->class_tok, NULL))))))
        return 1;
      else if (t == md->type_param_tok
               || (t >= TOK_UIDENT
                   && (!strcmp(get_tok_str(t, NULL), "i64")
                       || !strcmp(get_tok_str(t, NULL), "i32")
                       || !strcmp(get_tok_str(t, NULL), "ui64")
                       || !strcmp(get_tok_str(t, NULL), "ui32")
                       || !strcmp(get_tok_str(t, NULL), "ui8")
                       || !strcmp(get_tok_str(t, NULL), "char")
                       || !strcmp(get_tok_str(t, NULL), "short")
                       || !strcmp(get_tok_str(t, NULL), "int")
                       || !strcmp(get_tok_str(t, NULL), "long")
                       || !strcmp(get_tok_str(t, NULL), "float")
                       || !strcmp(get_tok_str(t, NULL), "double")
                       || !strcmp(get_tok_str(t, NULL), "bool")
                       || !strcmp(get_tok_str(t, NULL), "void")
                       || !strcmp(get_tok_str(t, NULL), "size_t"))))
        return 0;
    }
  }
  return -1;
}

/* For a variadic member-template definition (e.g. template<typename... Args>
   void List<T>::Resize(i64 newSize, Args&&... args)), find the first pack
   element's call argument type.  The pack parameter position is determined
   by the top-level parameter index of the first `...` token. */
static int template_member_def_pack_arg_tok(TemplateMemberDef *md,
                                            int method_tok,
                                            CType *arg_types,
                                            int explicit_arg_count)
{
  int i, n, name_tok, j, paren = 0, angle = 0, bracket = 0;
  int param_index = 0;

  if (!md || !md->def_str || !arg_types || explicit_arg_count <= 0)
    return 0;
  n = md->def_str->len;
  name_tok = method_tok == TOK_CONSTRUCTOR1 ? md->class_tok : method_tok;
  for (i = 0; i + 1 < n; ++i)
  {
    if (md->def_str->str[i] != name_tok)
      continue;
    j = i + 1;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (t == TOK_LT || t == '<')
          ++angle;
        else if (t == TOK_GT || t == '>')
          --angle;
        else if (t == TOK_SAR)
        {
          --angle;
          if (angle > 0)
            --angle;
        }
      }
      while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
    }
    if (j >= n || md->def_str->str[j] != '(')
      continue;
    for (++j; j < n; ++j)
    {
      int t = md->def_str->str[j];
      if (t == TOK_EOF)
        return 0;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if (t == ')' && !paren && !angle && !bracket)
        return 0;
      else if (t == '[')
        ++bracket;
      else if (t == ']' && bracket > 0)
        --bracket;
      else if (t == TOK_LT || t == '<')
        ++angle;
      else if ((t == TOK_GT || t == '>') && angle > 0)
        --angle;
      else if (t == TOK_SAR && angle > 0)
      {
        --angle;
        if (angle > 0)
          --angle;
      }
      else if (t == ',' && !paren && !angle && !bracket)
        ++param_index;
      else if (t == TOK_DOTS && !paren && !angle && !bracket
               && param_index < explicit_arg_count)
        return template_type_tok_from_ctype(&arg_types[param_index]);
    }
  }
  return 0;
}

static int token_string_has_array_brackets(TokenString *str)
{
  int i;

  if (!str)
    return 0;
  for (i = 0; i < str->len; ++i)
    if (str->str[i] == '[')
      return 1;
  return 0;
}

/* Array brackets in the parameter list (e.g. const T(&values)[N]) mark a
   signature whose non-type template argument is not yet known during eager
   replay.  Body-only indexing like m_pData[index] must not trigger the skip. */
static int token_string_signature_has_array_brackets(TokenString *str)
{
  int i, brace = 0;

  if (!str)
    return 0;
  for (i = 0; i < str->len; ++i)
  {
    if (str->str[i] == '{')
    {
      ++brace;
      if (brace == 1)
        return 0;
    }
    else if (str->str[i] == '}')
      --brace;
    else if (str->str[i] == '[' && brace == 0)
      return 1;
  }
  return 0;
}

static int same_template_family_compatible_elements(CType *type1, CType *type2)
{
  int tok1, tok2, i, j, k;
  if (!type1 || !type2
      || (type1->t & VT_BTYPE) != VT_STRUCT
      || (type2->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  tok1 = get_struct_type_name_tok(type1);
  tok2 = get_struct_type_name_tok(type2);
  if (!tok1 || !tok2 || tok1 == tok2)
    return 0;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    int inst1 = -1, inst2 = -1;
    if (!td->is_class || td->nb_type_params != 1)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
    {
      if (td->inst_name_toks[j] == tok1)
        inst1 = j;
      if (td->inst_name_toks[j] == tok2)
        inst2 = j;
    }
    if (inst1 >= 0 && inst2 >= 0)
    {
      CType elem1, elem2;
      k = td->nb_type_params;
      if (make_type_from_type_arg_tok(&elem1, td->inst_type_toks[inst1 * k])
          && make_type_from_type_arg_tok(&elem2, td->inst_type_toks[inst2 * k]))
        return is_compatible_unqualified_types(&elem1, &elem2);
    }
  }
  return 0;
}

static int make_member_template_func_tok_for_type(int struct_tok,
                                                   int method_tok,
                                                   CType *func_type)
{
  char name[512];
  Sym *arg;

  snprintf(name, sizeof(name), "%s_%s",
           get_tok_str(struct_tok, NULL), get_tok_str(method_tok, NULL));
  if (func_type && (func_type->t & VT_CONSTANT))
    pstrcat(name, sizeof(name), "_const");
  if (func_type && func_type->ref)
    for (arg = func_type->ref->next; arg; arg = arg->next)
      if (!append_type_mangle(name, sizeof(name), &arg->type))
        cprime_error("unsupported member-template parameter type");
  return tok_alloc_const(name);
}

static int token_string_is_auto_type(TokenString *str)
{
  int i, saw_auto = 0;
  if (!str)
    return 0;
  for (i = 0; i < str->len; ++i)
  {
    if (str->str[i] == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (str->str[i] == TOK_AUTO)
      saw_auto = 1;
    else
      return 0;
  }
  return saw_auto;
}

static int promoted_template_element_tok(int lhs_tok, int rhs_tok)
{
  if (!rhs_tok || lhs_tok == rhs_tok)
    return lhs_tok;
  if (lhs_tok == TOK_DOUBLE || rhs_tok == TOK_DOUBLE)
    return TOK_DOUBLE;
  if (lhs_tok == TOK_FLOAT || rhs_tok == TOK_FLOAT)
    return TOK_FLOAT;
  return lhs_tok;
}

static int template_member_operator_has_no_params(TemplateMemberDef *md,
                                                  int method_tok)
{
  int i, j;
  if (!md || !md->def_str)
    return 0;
  for (i = 0; i < md->def_str->len; ++i)
    if (md->def_str->str[i] == TOK_OPERATOR)
    {
      j = i + 1;
      while (j < md->def_str->len && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
      if (j >= md->def_str->len
          || get_cpp_binary_operator_method_tok(md->def_str->str[j]) != method_tok)
        continue;
      ++j;
      while (j < md->def_str->len && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
      if (j >= md->def_str->len || md->def_str->str[j++] != '(')
        continue;
      while (j < md->def_str->len && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
      return j < md->def_str->len && md->def_str->str[j] == ')';
    }
  return 0;
}

static int template_member_is_same_family_conversion_ctor(TemplateMemberDef *md)
{
  int i, j, in_params = 0;
  if (!md || !md->def_str)
    return 0;
  for (i = 0; i < md->def_str->len; ++i)
  {
    if (md->def_str->str[i] == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (!in_params && md->def_str->str[i] == md->class_tok)
    {
      j = i + 1;
      while (j < md->def_str->len && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
      if (j < md->def_str->len && md->def_str->str[j] == '(')
      {
        in_params = 1;
        i = j;
      }
      continue;
    }
    if (!in_params)
      continue;
    if (md->def_str->str[i] == ',' || md->def_str->str[i] == ')')
      return 0;
    if (md->def_str->str[i] == md->class_tok)
    {
      j = i + 1;
      while (j < md->def_str->len && md->def_str->str[j] == TOK_LINENUM)
        j += 2;
      return j < md->def_str->len && md->def_str->str[j] == TOK_LT;
    }
  }
  return 0;
}

static int class_template_inst_tok_for_member_arg(int class_tok,
                                                  int member_type_arg_tok)
{
  TemplateDef *td;
  TemplateArgList args;

  if (!member_type_arg_tok)
    return 0;
  td = find_class_template_def(class_tok);
  if (!td || !td->is_class || td->nb_type_params != 1)
    return 0;
  template_arg_list_one(&args, member_type_arg_tok);
  return instantiate_template_if_needed(td, &args);
}

/* For a member template whose declared return type is Class<OwnParam>
   (e.g. template<typename T> static clMatrix4x4<T> CreateMatrix(...)),
   return the concrete instantiated class token for this class argument.
   The qualifier spelling Class<T>:: is skipped; only a bare Class<T>
   followed by the method name counts as a return type. */
static int member_template_def_class_return_inst_tok(TemplateMemberDef *cm,
                                                     int type_tok)
{
  int i;

  if (!cm || !cm->def_str)
    return 0;
  for (i = 0; i + 4 < cm->def_str->len; ++i)
  {
    if (cm->def_str->str[i] == cm->class_tok
        && cm->def_str->str[i + 1] == TOK_LT
        && cm->def_str->str[i + 2] >= TOK_UIDENT
        && cm->def_str->str[i + 3] == TOK_GT)
    {
      int after = i + 4;
      while (after < cm->def_str->len
             && cm->def_str->str[after] == TOK_LINENUM)
        after += 2;
      if (after < cm->def_str->len && cm->def_str->str[after] == ':')
        continue;
      return class_template_inst_tok_for_member_arg(cm->class_tok,
                                                    type_tok);
    }
  }
  return 0;
}

static TemplateDef *find_class_template_def_for_class_tok(int class_tok)
{
  int i, j;
  TemplateDef *td = find_template_def(class_tok);

  if (td && td->is_class)
    return td;
  for (i = 0; i < nb_template_defs; ++i)
  {
    td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == class_tok)
        return td;
  }
  return NULL;
}

static int class_has_member_template_name(int class_tok, int member_tok)
{
  int i;

  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (md->class_tok == class_tok
        && template_member_def_method_tok(md) == member_tok)
    {
      return 1;
    }
  }
  return 0;
}

static int class_or_inst_has_member_template_name(int class_tok, int member_tok)
{
  int i, j;

  if (class_has_member_template_name(class_tok, member_tok))
    return 1;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
    {
      if (td->inst_name_toks[j] == class_tok
          && class_has_member_template_name(td->name_tok, member_tok))
        return 1;
    }
  }
  return 0;
}

static void skip_initializer_expression(void)
{
  int paren = 0, brace = 0, bracket = 0;

  while (tok != TOK_EOF)
  {
    if (!paren && !brace && !bracket && (tok == ',' || tok == ';'))
      break;
    if (tok == '(')
      ++paren;
    else if (tok == ')' && paren > 0)
      --paren;
    else if (tok == '{')
      ++brace;
    else if (tok == '}' && brace > 0)
      --brace;
    else if (tok == '[')
      ++bracket;
    else if (tok == ']' && bracket > 0)
      --bracket;
    next();
  }
}

/* Infer the scalar return type of an auto-return out-of-class template
   member from its first return expression.  The expression is replayed in a
   statement-expression probe where the class pointer is bound, under
   nocode_wanted, so unqualified member/field references resolve to concrete
   types. */
static int infer_template_member_auto_return_type_inner(TemplateMemberDef *md,
                                                        int class_mangled_tok,
                                                        CType *ret_type,
                                                        int depth)
{
  int i, start = -1, end = 0, paren = 0, brace = 0;
  Sym *class_sym;
  CType class_type;
  TokenString *expr, *probe;
  CType inferred;
  CValue cval;
  SValue *saved_vtop = vtop;
  int this_tok = tok_alloc_const("this");
  int probe_this_tok = tok_alloc_const("__cpc_auto_this");

  if (!md || !md->def_str || !ret_type || depth >= 16)
    return 0;
  class_sym = struct_find(class_mangled_tok);
  if (!class_sym)
    return 0;
  class_type = class_sym->type;
  class_type.ref = class_sym;
  if (template_member_cached_ret(md, class_mangled_tok, ret_type))
    return 1;

  for (i = 0; i + 1 < md->def_str->len; ++i)
    if (md->def_str->str[i] == TOK_RETURN)
    {
      start = i + 1;
      break;
    }
  if (start < 0)
    return 0;
  while (start < md->def_str->len
         && md->def_str->str[start] == TOK_LINENUM)
    start += 2;
  end = md->def_str->len;
  for (i = start; i < md->def_str->len; ++i)
  {
    int t = md->def_str->str[i];
    if (t == '(')
      ++paren;
    else if (t == ')' && paren > 0)
      --paren;
    else if (t == '{')
      ++brace;
    else if (t == '}' && brace > 0)
      --brace;
    else if (t == ';' && paren == 0 && brace == 0)
    {
      end = i;
      break;
    }
  }

  expr = tok_str_alloc();
  for (i = start; i < end; ++i)
  {
    int t = md->def_str->str[i];
    int ni2 = i + 1;
    while (ni2 < md->def_str->len && md->def_str->str[ni2] == TOK_LINENUM)
      ni2 += 2;
    if (t >= TOK_UIDENT && ni2 < md->def_str->len
        && md->def_str->str[ni2] == '(')
    {
      int k, call_level = 0;
      TemplateMemberDef *called_member = NULL;
      for (k = 0; k < nb_template_member_defs; ++k)
      {
        TemplateMemberDef *cm = template_member_defs[k];
        if (cm->class_tok == md->class_tok
            && template_member_def_method_tok(cm) == t)
        {
          called_member = cm;
          break;
        }
      }
      if (called_member)
      {
        CType called_ret;
        memset(&called_ret, 0, sizeof(called_ret));
        if (infer_template_member_auto_return_type_inner(
                called_member, class_mangled_tok, &called_ret, depth + 1)
            && (called_ret.t & VT_BTYPE) != VT_STRUCT
            && (called_ret.t & VT_BTYPE) != VT_VOID)
        {
          tok_str_add(expr, '(');
          add_ctype_tokens(expr, &called_ret);
          tok_str_add(expr, ')');
          tok_str_add_cint(expr, 0);
          for (i = ni2; i < end; ++i)
          {
            if (md->def_str->str[i] == '(')
              ++call_level;
            else if (md->def_str->str[i] == ')')
            {
              --call_level;
              if (call_level == 0)
                break;
            }
          }
          continue;
        }
      }
      else
      {
        /* A free-function wrapper whose first argument is a same-class
           member call (e.g. clSqrt(LengthSquared())) can be lowered to the
           member's scalar result without probing the outer function. */
        int ai = ni2 + 1;
        while (ai + 1 < md->def_str->len
               && md->def_str->str[ai] == TOK_LINENUM)
          ai += 2;
        if (ai + 1 < md->def_str->len
            && md->def_str->str[ai] >= TOK_UIDENT
            && md->def_str->str[ai + 1] == '(')
        {
          int inner_tok = md->def_str->str[ai];
          TemplateMemberDef *inner_member = NULL;
          for (k = 0; k < nb_template_member_defs; ++k)
          {
            TemplateMemberDef *cm = template_member_defs[k];
            if (cm->class_tok == md->class_tok
                && template_member_def_method_tok(cm) == inner_tok)
            {
              inner_member = cm;
              break;
            }
          }
          if (inner_member)
          {
            CType inner_ret;
            memset(&inner_ret, 0, sizeof(inner_ret));
            if (infer_template_member_auto_return_type_inner(
                    inner_member, class_mangled_tok, &inner_ret, depth + 1)
                && (inner_ret.t & VT_BTYPE) != VT_STRUCT
                && (inner_ret.t & VT_BTYPE) != VT_VOID)
            {
              int outer_level = 0;
              tok_str_add(expr, '(');
              add_ctype_tokens(expr, &inner_ret);
              tok_str_add(expr, ')');
              tok_str_add_cint(expr, 0);
              for (i = ni2; i < end; ++i)
              {
                if (md->def_str->str[i] == '(')
                  ++outer_level;
                else if (md->def_str->str[i] == ')')
                {
                  --outer_level;
                  if (outer_level == 0)
                    break;
                }
              }
              continue;
            }
          }
        }
      }
      /* Free-function calls (and member calls whose return type cannot be
         inferred yet) are kept so the probe resolves them if possible. */
    }
    if (t >= TOK_UIDENT && t != this_tok
        && (i == start || (md->def_str->str[i - 1] != '.'
                           && md->def_str->str[i - 1] != TOK_ARROW
                           && md->def_str->str[i - 1] != ':')))
    {
      int ni = i + 1, dummy_ofs;
      Sym *probe_sym;
      while (ni < md->def_str->len
             && md->def_str->str[ni] == TOK_LINENUM)
        ni += 2;
      if (find_field_try(&class_type, t, &dummy_ofs))
      {
        tok_str_add(expr, probe_this_tok);
        tok_str_add(expr, TOK_ARROW);
        tok_str_add(expr, t);
        continue;
      }
      probe_sym = sym_find(t);
      if (!probe_sym)
        probe_sym = sym_find2(global_stack, t);
      if (!probe_sym)
      {
        tok_str_free(expr);
        return 0;
      }
    }
    tok_str_add(expr, t);
  }
  tok_str_add(expr, TOK_EOF);

  probe = tok_str_alloc();
  tok_str_add(probe, '(');
  tok_str_add(probe, '{');
  tok_str_add(probe, TOK_STRUCT);
  tok_str_add(probe, class_mangled_tok);
  tok_str_add(probe, '*');
  tok_str_add(probe, probe_this_tok);
  tok_str_add(probe, '=');
  cval.i = 0;
  tok_str_add2(probe, TOK_CINT, &cval);
  tok_str_add(probe, ';');
  tok_str_append_without_eof(probe, expr);
  tok_str_add(probe, ';');
  tok_str_add(probe, '}');
  tok_str_add(probe, ')');
  tok_str_add(probe, TOK_EOF);

  memset(&inferred, 0, sizeof(inferred));
  infer_expr_type_from_tokens(probe, &inferred);
  while (vtop > saved_vtop)
    vpop();
  tok_str_free(expr);
  tok_str_free(probe);
  if ((inferred.t & VT_BTYPE) == VT_VOID
      || (inferred.t & VT_BTYPE) == VT_STRUCT)
    return 0;
  *ret_type = inferred;
  template_member_note_cached_ret(md, class_mangled_tok, ret_type);
  return 1;
}

static int infer_template_member_auto_return_type(TemplateMemberDef *md,
                                                  int class_mangled_tok,
                                                  CType *ret_type)
{
  return infer_template_member_auto_return_type_inner(md, class_mangled_tok,
                                                      ret_type, 0);
}

/* Copy a token string while dropping TOK_LINENUM/value pairs and
   substituting the class type parameter.  Line-info markers get inserted
   into captured strings by tok_str_add_tok(); leaving them inside a
   template-id such as Box < float > breaks the later re-parse. */
static void tok_str_append_clean(TokenString *dst, TokenString *src,
                                 int type_param_tok, int type_tok)
{
  int i;

  for (i = 0; i < src->len; ++i)
  {
    int t = src->str[i];
    if (t == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    tok_str_add(dst, t == type_param_tok ? type_tok : t);
  }
}

static int token_string_starts_like_type(TokenString *str)
{
  int i, t;

  for (i = 0; i < str->len; ++i)
  {
    t = str->str[i];
    if (t == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (t == TOK_STRUCT || t == TOK_CLASS || t == TOK_UNION
        || t == TOK_ENUM || t == TOK_VOID || t == TOK_CHAR
        || t == TOK_SHORT || t == TOK_INT || t == TOK_LONG
        || t == TOK_FLOAT || t == TOK_DOUBLE || t == TOK_BOOL
        || t == TOK_UNSIGNED || t == TOK_CONST1 || t == TOK_VOLATILE1
        || t >= TOK_UIDENT)
      return 1;
    return 0;
  }
  return 0;
}

static int parse_ctype_from_token_string(TokenString *str, CType *out)
{
  int saved_tok, v, ok = 0;
  CValue saved_tokc;
  AttributeDef ad;
  CType type;

  if (!str || !token_string_starts_like_type(str))
    return 0;
  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(str, 3);
  next();
  memset(&type, 0, sizeof type);
  memset(&ad, 0, sizeof ad);
  if (parse_btype(&type, &ad, 0))
  {
    v = 0;
    type_decl(&type, &ad, &v, TYPE_ABSTRACT);
    if (tok == TOK_EOF || tok == 0)
    {
      *out = type;
      ok = 1;
    }
  }
  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;
  return ok;
}

static void instantiate_template_member_if_needed(TemplateMemberDef *md,
                                                  int type_tok,
                                                  int class_mangled_tok,
                                                  int member_type_arg_tok)
{
  int saved_tok, method_tok, member_func_tok, paren, i, cv_qualifiers;
  int duplicate_member = 0;
  int member_decl_exists = 0;
  int qualifier_already_parsed = 0;
  int method_already_parsed = 0;
  CValue saved_tokc;
  CType struct_type;
  Sym *class_sym;
  CType replay_ret_type, replay_func_type;
  TokenString *ret_str, *sig, *params_str, *proto, *body_def;
  TokenString *typed_params = NULL;
  TokenString *init_prefix = NULL;
  TokenString *clean_ret = NULL;
  PendingMemberFunc *pm;
  int value_param_tok = 0;
  int member_type_param_tok = 0;
  int auto_return_struct_tok = 0;
  CType inferred_auto_ret;
  int has_inferred_auto_ret = 0;
  int is_static_member = 0;
  int orig_pack_tok = member_type_arg_tok;
  int pack_is_empty = member_type_arg_tok == -1;
  int pack_name_tok = 0;

  if (member_type_arg_tok == -1)
    member_type_arg_tok = 0;

  if (template_member_lookup_inst(md, type_tok, orig_pack_tok))
    return;
  method_tok = template_member_def_method_tok(md);
  if ((md->class_tok >= TOK_UIDENT
       && strstr(get_tok_str(md->class_tok, NULL), "initializer_list"))
      || (class_mangled_tok >= TOK_UIDENT
          && strstr(get_tok_str(class_mangled_tok, NULL), "initializer_list")))
  {
    template_member_note_inst(md, type_tok, orig_pack_tok);
    return;
  }
  if (method_tok != tok_alloc_const("operator[]")
      && token_string_signature_has_array_brackets(md->def_str)
      && !template_member_def_first_param_is_array_ref(md, method_tok))
  {
    template_member_note_inst(md, type_tok, orig_pack_tok);
    return;
  }

  if (token_string_starts_template(md->def_str)
      && token_string_body_has_variadic_pack(md->def_str)
      && !pack_is_empty
      && member_type_arg_tok == 0)
  {
    template_member_note_inst(md, type_tok, orig_pack_tok);
    return;
  }

  saved_tok = tok;
  saved_tokc = tokc;
  ret_str = tok_str_alloc();
  sig = tok_str_alloc();
  params_str = tok_str_alloc();

  begin_macro(md->def_str, 3);
  next();
  if (is_template_keyword_tok(tok))
  {
    int angle = 0;
    int saw_type_param = 0;
    int expect_type_param_name = 0;
    next();
    skip(TOK_LT);
    angle = 1;
    while (tok != TOK_EOF && angle > 0)
    {
      if (angle == 1
          && (tok == TOK_CLASS
              || (tok >= TOK_UIDENT
                  && !strcmp(get_tok_str(tok, NULL), "typename"))))
      {
        saw_type_param = 1;
        expect_type_param_name = 1;
      }
      else if (angle == 1 && expect_type_param_name && tok >= TOK_UIDENT)
      {
        member_type_param_tok = tok;
        expect_type_param_name = 0;
      }
      else if (angle == 1 && tok >= TOK_UIDENT && !saw_type_param)
        value_param_tok = tok;
      if (tok == TOK_LT)
        ++angle;
      else if (tok == TOK_GT)
        --angle;
      else if (tok == TOK_SAR)
      {
        --angle;
        if (angle > 0)
          --angle;
      }
    next();
  }
}

  for (;;)
  {
    if (tok == TOK_EOF)
      cprime_error("unsupported template member declaration");
    if (tok == md->class_tok)
    {
      TokenString *candidate = tok_str_alloc();
      next();
      if (tok == '(' && ret_str->len == 0)
      {
        tok_str_free(candidate);
        method_tok = TOK_CONSTRUCTOR1;
        method_already_parsed = 1;
        break;
      }
      if (tok == TOK_LT)
      {
        int level = 1, is_scoped;
        tok_str_add(candidate, md->class_tok);
        tok_str_add(candidate, TOK_LT);
        next();
        while (tok != TOK_EOF && level > 0)
        {
          if (tok == TOK_LT)
            ++level;
          else if (tok == TOK_GT)
            --level;
          else if (tok == TOK_SAR)
          {
            --level;
            if (level > 0)
              --level;
          }
          tok_str_add_template_member_subst(candidate, md,
                                            class_mangled_tok,
                                            md->type_param_tok, type_tok,
                                            member_type_param_tok,
                                            member_type_arg_tok,
                                            value_param_tok);
          if (level > 0)
            next();
        }
        if (tok == TOK_SAR)
        {
          tok = TOK_GT;
          unget_tok(TOK_GT);
        }
        next();
        is_scoped = tok == ':';
        if (is_scoped)
        {
          tok_str_free(candidate);
          qualifier_already_parsed = 1;
          break;
        }
        tok_str_append(ret_str, candidate);
        tok_str_free(candidate);
        continue;
      }
      tok_str_free(candidate);
      tok_str_add(ret_str, md->class_tok);
      continue;
    }
    if (tok == TOK_STATIC
        || (tok >= TOK_UIDENT
            && !strcmp(get_tok_str(tok, NULL), "static")))
    {
      next();
      continue;
    }
    if (tok >= TOK_UIDENT && tok != md->class_tok
        && ret_str->len > 0
        && tok != md->type_param_tok
        && tok != member_type_param_tok)
    {
      /* Inline member templates have no Class:: qualifier: after the return
         type, the identifier that introduces the parameter list is the
         method name.  Out-of-class plain members can have qualified return
         types (const clVector4<T>& ...), so only treat an identifier as the
         method name when it is directly followed by '(' ??? otherwise it is
         still part of the return type. */
      int method_candidate = tok;
      next();
      if (tok == '(')
      {
        method_tok = method_candidate;
        method_already_parsed = 1;
        break;
      }
      unget_tok(method_candidate);
    }
    tok_str_add_template_member_subst(ret_str, md, class_mangled_tok,
                                      md->type_param_tok, type_tok,
                                      member_type_param_tok,
                                      member_type_arg_tok, value_param_tok);
    next();
  }

  if (method_already_parsed)
  {
    /* An in-class member-template constructor has no return type or
       Class<T>:: qualifier.  TOK is already the opening parenthesis. */
  }
  else if (qualifier_already_parsed)
  {
    skip(':');
    skip(':');
  }
  else
  {
    next();
    if (tok != md->type_param_tok)
      cprime_error("unsupported template member type argument");
    next();
    skip(TOK_GT);
    skip(':');
    skip(':');
  }
  clean_ret = tok_str_alloc();
  tok_str_append_clean(clean_ret, ret_str, md->type_param_tok, type_tok);
  tok_str_add(clean_ret, TOK_EOF);

  if (method_already_parsed)
  {
    /* Method token was recognized while separating the return type. */
  }
  else if (tok == '~')
  {
    next();
    if (tok != md->class_tok)
      cprime_error("destructor name must match class name");
    method_tok = TOK_DESTRUCTOR1;
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
  }
  else
  {
    if (tok == TOK_OPERATOR)
      method_tok = parse_cpp_operator_method_tok();
    else
    {
      method_tok = tok;
      if (method_tok < TOK_UIDENT)
        expect("member function name");
      if (method_tok == md->class_tok)
        method_tok = TOK_CONSTRUCTOR1;
      next();
    }
  }
  skip('(');
  is_static_member = !is_lifecycle_member_tok(method_tok)
                     && class_has_static_member_func(class_mangled_tok,
                                                     method_tok);

  if (ret_str->len == 0)
  {
    if (!is_lifecycle_member_tok(method_tok))
      cprime_error("template member function requires a return type");
    tok_str_add(sig, TOK_VOID);
  }
  else if (token_string_is_auto_type(ret_str))
  {
    const char *method_name = get_tok_str(method_tok, NULL);
    int return_struct_tok = 0;
    for (i = 0; i + 1 < md->def_str->len; ++i)
      if (md->def_str->str[i] == TOK_RETURN
          && md->def_str->str[i + 1] == md->class_tok)
      {
        return_struct_tok = class_template_inst_tok_for_member_arg(md->class_tok,
                                                                  type_tok);
        break;
      }
    if (return_struct_tok)
    {
      auto_return_struct_tok = return_struct_tok;
      tok_str_add(sig, TOK_STRUCT);
      tok_str_add(sig, return_struct_tok);
    }
    else if (!strcmp(method_name, "operator+")
        || !strcmp(method_name, "operator-")
        || !strcmp(method_name, "operator*")
        || !strcmp(method_name, "operator/")
        || !strcmp(method_name, "operator>>")
        || !strcmp(method_name, "operator<<")
        || !strcmp(method_name, "operator"))
    {
      int promoted_tok = promoted_template_element_tok(type_tok,
                                                       member_type_arg_tok);
      auto_return_struct_tok =
        class_template_inst_tok_for_member_arg(md->class_tok, promoted_tok);
      if (!auto_return_struct_tok)
        auto_return_struct_tok = class_mangled_tok;
      tok_str_add(sig, TOK_STRUCT);
      tok_str_add(sig, auto_return_struct_tok);
    }
    else
    {
      /* The return expression may be a call to another member template of
         the same class with a concrete Class<Param> return type (e.g.
         clMatrix4x4<T>::Translated returns CreateMatrix(...)).  Deduce the
         auto return from that member's declared return type. */
      int return_call_tok = 0;
      int ri;
      for (i = 0; i + 1 < md->def_str->len; ++i)
        if (md->def_str->str[i] == TOK_RETURN)
        {
          ri = i + 1;
          while (ri < md->def_str->len
                 && md->def_str->str[ri] == TOK_LINENUM)
            ri += 2;
          if (ri < md->def_str->len && md->def_str->str[ri] >= TOK_UIDENT)
            return_call_tok = md->def_str->str[ri];
          break;
        }
      if (return_call_tok)
        for (i = 0; i < nb_template_member_defs; ++i)
        {
          TemplateMemberDef *cm = template_member_defs[i];
          if (cm->class_tok != md->class_tok
              || template_member_def_method_tok(cm) != return_call_tok)
            continue;
          auto_return_struct_tok =
            member_template_def_class_return_inst_tok(cm, type_tok);
          if (auto_return_struct_tok)
          {
            tok_str_add(sig, TOK_STRUCT);
            tok_str_add(sig, auto_return_struct_tok);
            break;
          }
        }
      if (!auto_return_struct_tok)
      {
        CType scalar_ret;
        int saved_infer_tok = tok;
        CValue saved_infer_tokc = tokc;
        if (infer_template_member_auto_return_type(md, class_mangled_tok,
                                                    &scalar_ret))
        {
          add_ctype_tokens(sig, &scalar_ret);
          inferred_auto_ret = scalar_ret;
          has_inferred_auto_ret = 1;
        }
        else
          tok_str_add(sig, TOK_AUTO);
        tok = saved_infer_tok;
        tokc = saved_infer_tokc;
      }
    }
  }
  else
  {
    tok_str_append_without_eof(sig, clean_ret);
  }

  if (tok != ')')
  {
    tok_str_add(params_str, ',');
    paren = 0;
    for (;;)
    {
      if (tok == TOK_EOF)
        cprime_error("unexpected end of file in template member parameters");
      if (tok == ')' && paren == 0)
        break;
      if (tok == '(')
        ++paren;
      else if (tok == ')')
        --paren;
      if (tok == md->class_tok)
      {
        int selected_class_tok = class_mangled_tok;
        tok_str_add(params_str, class_mangled_tok);
        next();
        while (tok == TOK_LINENUM)
          next();
        if (tok == TOK_LT || tok == '<')
        {
          int angle = 1;
          int first_arg_tok = 0;
          next();
          while (tok != TOK_EOF && angle > 0)
          {
            if (angle == 1 && !first_arg_tok && tok >= TOK_UIDENT)
              first_arg_tok = tok;
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
          if (tok != TOK_GT)
            cprime_error("member-template parameter close expected before '%s'",
                         get_tok_str(tok, &tokc));
          next();
          if (member_type_param_tok && first_arg_tok == member_type_param_tok)
          {
            int inst_tok =
              class_template_inst_tok_for_member_arg(md->class_tok,
                                                     member_type_arg_tok);
            if (inst_tok)
              selected_class_tok = inst_tok;
          }
        }
        if (selected_class_tok != class_mangled_tok)
          params_str->str[params_str->len - 1] = selected_class_tok;
        continue;
      }
      if (tok == TOK_DOTS && member_type_arg_tok)
      {
        next();
        continue;
      }
      if (pack_is_empty && tok == TOK_DOTS && paren == 0)
      {
        /* The pack parameter (e.g. `, Args&&... args`) disappears when the
           pack has zero elements.  Trim the tokens already collected for it
           back to the preceding comma, then consume the parameter name and
           the separating comma. */
        int k = params_str->len;
        while (k > 0)
        {
          if (params_str->str[k - 1] == TOK_LINENUM && k >= 2)
            k -= 2;
          else if (params_str->str[k - 1] == ',')
          {
            --k;
            break;
          }
          else
            --k;
        }
        params_str->len = k;
        next();
        while (tok == TOK_LINENUM)
          next();
        if (tok >= TOK_UIDENT)
        {
          pack_name_tok = tok;
          next();
        }
        while (tok == TOK_LINENUM)
          next();
        if (tok == ',')
          next();
        continue;
      }
      tok_str_add_template_member_subst_scoped(params_str, md,
                                               class_mangled_tok,
                                               md->type_param_tok, type_tok,
                                               member_type_param_tok,
                                               member_type_arg_tok,
                                               value_param_tok);
      next();
    }
  }
  skip(')');
  cv_qualifiers = skip_member_func_cv_qualifiers();

  typed_params = tok_str_alloc();
  if (params_str->len > 0)
  {
    int start = params_str->str[0] == ',' ? 1 : 0;
    for (i = start; i < params_str->len; ++i)
      tok_str_add(typed_params, params_str->str[i]);
  }
  tok_str_add(typed_params, TOK_EOF);

  class_sym = NULL;
  if (method_tok == TOK_CONSTRUCTOR1 && tok == ':')
  {
    class_sym = struct_find(class_mangled_tok);
    if (!class_sym)
      cprime_error("template member replay requires instantiated class '%s'",
                   get_tok_str(class_mangled_tok, NULL));
    struct_type.t = class_sym->type.t;
    struct_type.ref = class_sym;
    init_prefix = parse_constructor_member_initializers(&struct_type);
  }
  if (init_prefix)
    note_struct_member_init_list(class_mangled_tok);

  if (is_lifecycle_member_tok(method_tok))
  {
    CType lowered_type;

    replay_ret_type.t = VT_VOID;
    replay_ret_type.ref = NULL;
    replay_func_type = make_func_type_from_saved_params(&replay_ret_type,
                                                        typed_params);
    class_sym = struct_find(class_mangled_tok);
    if (!class_sym)
      cprime_error("template member replay requires instantiated class '%s'",
                   get_tok_str(class_mangled_tok, NULL));
    struct_type.t = class_sym->type.t;
    struct_type.ref = class_sym;
    lowered_type = make_lowered_member_func_type(&struct_type,
                                                 &replay_func_type);
    member_func_tok = make_lifecycle_func_tok_for_type(class_mangled_tok,
                                                       method_tok,
                                                       &lowered_type,
                                                       0);
    note_raw_lifecycle_overload(class_mangled_tok, method_tok, member_func_tok,
                                params_str);
    external_global_sym(member_func_tok, &lowered_type);
    tok_str_add(sig, member_func_tok);
  }
  else
  {
    Sym *canonical_sym;
    replay_ret_type.t = VT_VOID;
    replay_ret_type.ref = NULL;
    if (auto_return_struct_tok
        && template_return_ctype_from_struct_tok(&replay_ret_type,
                                                 auto_return_struct_tok))
    {
      /* Auto-return member templates deduced through the return expression
         (return Class<T>, operators, or a same-class helper call) must
         carry the concrete struct return in the overload metadata. */
    }
    else if (clean_ret
        && parse_ctype_from_token_string(clean_ret, &replay_ret_type))
    {
      /* Explicit member-template return types are parsed from the clean
         replay tokens so the overload metadata (and therefore call-site
         typing) sees the real return type instead of void. */
    }
    replay_func_type = make_func_type_from_saved_params(&replay_ret_type,
                                                        typed_params);
    replay_func_type.t |= cv_qualifiers;
    class_sym = struct_find(class_mangled_tok);
    if (!class_sym)
      cprime_error("template member replay requires instantiated class '%s'",
                   get_tok_str(class_mangled_tok, NULL));
    struct_type.t = class_sym->type.t;
    struct_type.ref = class_sym;
    if (cv_qualifiers & VT_CONSTANT)
      struct_type.t |= VT_CONSTANT;
    if (is_static_member)
    {
      int static_base_tok = make_static_member_tok(class_mangled_tok,
                                                   method_tok);
      CType static_type = replay_func_type;
      static_type.t &= ~VT_STATIC;
      declare_static_member_func(&struct_type, method_tok,
                                 &static_type);
      member_func_tok = make_free_func_tok_for_type(static_base_tok,
                                                    &static_type);
    }
    else if (member_type_param_tok)
    {
      member_func_tok = make_member_template_func_tok_for_type(class_mangled_tok,
                                                               method_tok,
                                                               &replay_func_type);
    }
    else
    {
      canonical_sym = resolve_member_func_by_param_signature(&struct_type,
                                                             method_tok,
                                                             &replay_func_type);
      if (canonical_sym)
        member_func_tok = canonical_sym->v;
      else
        member_func_tok = make_member_func_tok_for_type(class_mangled_tok,
                                                        method_tok,
                                                        &replay_func_type);
    }
    if (auto_return_struct_tok || member_type_param_tok)
      last_instantiated_member_func_tok = member_func_tok;
    if (has_inferred_auto_ret
        && member_is_auto_return_tok(member_func_tok))
    {
      MemberFuncOverload *o;
      Sym *prev = sym_find(member_func_tok);
      if (!prev)
        prev = sym_find2(global_stack, member_func_tok);
      if (prev && prev->type.ref)
        prev->type.ref->type = inferred_auto_ret;
      for (o = member_func_overloads; o; o = o->next)
        if (o->mangled_tok == member_func_tok && o->func_type.ref)
          o->func_type.ref->type = inferred_auto_ret;
    }
    tok_str_add(sig, member_func_tok);
  }
  if (is_static_member)
  {
    int i2;
    TokenString *static_sig = tok_str_alloc();
    TokenString *static_params = tok_str_alloc();
    int param_start = (params_str->len > 0 && params_str->str[0] == ',')
                      ? 1 : 0;
    tok_str_add(static_sig, TOK_STATIC);
    tok_str_add(static_sig, TOK_INLINE1);
    for (i2 = 0; i2 < sig->len; ++i2)
      tok_str_add(static_sig, sig->str[i2]);
    tok_str_add(static_sig, '(');
    for (i2 = param_start; i2 < params_str->len
         && params_str->str[i2] != TOK_EOF; ++i2)
      tok_str_add(static_params, params_str->str[i2]);
    tok_str_append(static_sig, static_params);
    tok_str_add(static_sig, ')');
    tok_str_free(static_params);
    tok_str_free(sig);
    sig = static_sig;
  }
  else
  {
    tok_str_add(sig, '(');
    tok_str_add(sig, TOK_STRUCT);
    tok_str_add(sig, class_mangled_tok);
    if (cv_qualifiers & VT_CONSTANT)
      tok_str_add(sig, TOK_CONST1);
    tok_str_add(sig, '*');
    tok_str_add(sig, tok_alloc_const("this"));
    tok_str_append(sig, params_str);
    tok_str_add(sig, ')');
  }
  if (!is_lifecycle_member_tok(method_tok)
      && (sym_find(member_func_tok) || sym_find2(global_stack, member_func_tok)))
  {
    Sym *dup_sym = sym_find(member_func_tok);
    if (!dup_sym)
      dup_sym = sym_find2(global_stack, member_func_tok);
    member_decl_exists = 1;
    /* A member declaration (VT_EXTERN, no body yet) is completed by the
       out-of-class template definition.  Only an already-defined symbol is
       a genuine duplicate whose body must not be replayed. */
    if (dup_sym && !(dup_sym->type.t & VT_EXTERN))
      duplicate_member = 1;
  }
  if (auto_return_struct_tok)
    duplicate_member = 0;

  {
    int skip_proto = 0;
    if (method_tok == TOK_DESTRUCTOR1 || is_static_member)
    {
      skip_proto = 1;
    }
    if (!skip_proto && !member_decl_exists)
    {
      char proto_this_name[64];
      int proto_this_tok;

      snprintf(proto_this_name, sizeof proto_this_name,
               "__cpc_this_proto_%d", member_func_tok);
      proto_this_tok = tok_alloc_const(proto_this_name);
      proto = tok_str_alloc();
      for (i = 0; i < sig->len; ++i)
      {
        int ptok = sig->str[i];
        if (ptok == tok_alloc_const("this"))
          ptok = proto_this_tok;
        tok_str_add(proto, ptok);
      }
      tok_str_add(proto, ';');
      tok_str_add(proto, TOK_EOF);
      for (i = 0; i < nb_pending_template_specs; ++i)
      {
        if (!pending_template_specs[i])
          continue;
        if (token_string_contains_tok(pending_template_specs[i], member_func_tok))
          break;
      }
      if (i < nb_pending_template_specs)
        tok_str_free(proto);
      else
        dynarray_add(&pending_template_specs, &nb_pending_template_specs, proto);
    }
  }

  body_def = tok_str_alloc();
  tok_str_append(body_def, sig);

  while (tok != TOK_EOF)
  {
    int prev_body_tok = body_def->len > 0 ? body_def->str[body_def->len - 1] : 0;
    if (tok >= TOK_UIDENT
        && prev_body_tok != '.'
        && prev_body_tok != TOK_ARROW
        && (class_has_member_func_name(class_mangled_tok, tok)
            || class_has_member_func_name(md->class_tok, tok)
            || class_has_member_template_name(md->class_tok, tok)
            || (is_static_member
                && class_has_static_member_func(class_mangled_tok, tok))))
    {
      int member_tok = tok;
      TokenString *line_toks = tok_str_alloc();
      next();
      while (tok == TOK_LINENUM)
      {
        tok_str_add2(line_toks, tok, &tokc);
        next();
      }
      if (member_tok != md->class_tok
          && (is_static_member
              || class_has_static_member_func(class_mangled_tok, member_tok))
          && (tok == '(' || tok == TOK_LT || tok == '<'))
      {
        /* Static members have no this pointer and cannot be reached
           through this->: qualify nested static member calls (e.g.
           Translation() calling Identity(), or a member template like
           CreateMatrix<T>(...)) as Class::Member. */
        tok_str_add(body_def, class_mangled_tok);
        tok_str_add(body_def, ':');
        tok_str_add(body_def, ':');
        tok_str_add(body_def, member_tok);
        tok_str_append(body_def, line_toks);
        tok_str_free(line_toks);
        if (tok == TOK_LT || tok == '<')
        {
          int angle = 1;
          tok_str_add(body_def, tok);
          next();
          while (tok != TOK_EOF && angle > 0)
          {
            if (tok == TOK_LINENUM)
            {
              next();
              continue;
            }
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
            {
              tok_str_add_tok(body_def);
              next();
            }
          }
          if (tok != TOK_GT)
            cprime_error("member-template call close expected before '%s'",
                         get_tok_str(tok, &tokc));
          tok_str_add(body_def, TOK_GT);
          next();
          while (tok == TOK_LINENUM)
            next();
        }
        continue;
      }
      if (tok == '(')
      {
        tok_str_add(body_def, tok_alloc_const("this"));
        tok_str_add(body_def, TOK_ARROW);
        tok_str_add(body_def, member_tok);
        tok_str_append(body_def, line_toks);
        tok_str_free(line_toks);
        continue;
      }
      tok_str_add(body_def, member_tok);
      tok_str_append(body_def, line_toks);
      tok_str_free(line_toks);
      continue;
    }
    if (tok == md->class_tok)
    {
      tok_str_add(body_def, class_mangled_tok);
      next();
      while (tok == TOK_LINENUM)
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
        if (tok != TOK_GT)
          cprime_error("member-template body type close expected before '%s'",
                       get_tok_str(tok, &tokc));
        next();
      }
      continue;
    }
    if (tok == TOK_DOTS && member_type_arg_tok)
    {
      next();
      continue;
    }
    if (tok == TOK_DOTS && pack_is_empty)
    {
      /* Drop the pack-expansion expression before the `...`: either a bare
         pack name (`args...`) or a call/group whose closing paren precedes
         the dots (`clForward(Args, args)...`). */
      int bt = body_def->len;
      while (bt > 0 && body_def->str[bt - 1] == TOK_LINENUM && bt >= 2)
        bt -= 2;
      if (bt > 0 && body_def->str[bt - 1] == ')')
      {
        int depth = 0, k = bt - 1;
        for (; k >= 0; --k)
        {
          int tt = body_def->str[k];
          if (tt == TOK_LINENUM && k >= 1)
          {
            --k;
            continue;
          }
          if (tt == ')')
            ++depth;
          else if (tt == '(')
          {
            --depth;
            if (depth == 0)
              break;
          }
        }
        if (k >= 0)
        {
          /* Also skip the postfix-expression prefix of the expansion:
             template-argument groups, scope qualifiers, member access, and
             identifiers (e.g. `std::forward<char>(args)...`). */
          int j = k - 1, run = 1;
          while (j >= 0 && run)
          {
            int tt = body_def->str[j];
            if (tt == TOK_LINENUM && j >= 1)
            {
              j -= 2;
              continue;
            }
            if (tt == '>' || tt == TOK_GT || tt == TOK_SAR)
            {
              int angle = 1;
              --j;
              while (j >= 0 && angle > 0)
              {
                int a2 = body_def->str[j];
                if (a2 == TOK_LINENUM && j >= 1)
                {
                  j -= 2;
                  continue;
                }
                if (a2 == '<' || a2 == TOK_LT)
                  --angle;
                else if (a2 == '>' || a2 == TOK_GT)
                  ++angle;
                else if (a2 == TOK_SAR)
                  angle += 2;
                --j;
              }
              continue;
            }
            if (tt == TOK_ARROW || tt == '.')
            {
              --j;
              continue;
            }
            if (tt == ':' && j >= 1 && body_def->str[j - 1] == ':')
            {
              j -= 2;
              continue;
            }
            if (tt >= TOK_UIDENT)
            {
              --j;
              continue;
            }
            run = 0;
          }
          body_def->len = j + 1;
        }
      }
      else if (bt > 0 && pack_name_tok
               && body_def->str[bt - 1] == pack_name_tok)
        body_def->len = bt - 1;
      else
        body_def->len = bt;
      next();
      continue;
    }
    tok_str_add_template_member_subst(body_def, md, class_mangled_tok,
                                      md->type_param_tok, type_tok,
                                      member_type_param_tok,
                                      member_type_arg_tok,
                                      value_param_tok);
    if (init_prefix && tok == '{')
      tok_str_append(body_def, init_prefix);
    next();
  }
  tok_str_add(body_def, TOK_EOF);

  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;
  tok_str_free(ret_str);
  tok_str_free(clean_ret);
  tok_str_free(sig);
  tok_str_free(params_str);
  if (typed_params)
  tok_str_free(typed_params);
  if (init_prefix)
    tok_str_free(init_prefix);
  if ((duplicate_member && method_tok != tok_alloc_const("operator[]"))
      || pending_member_func_has_body_tok(member_func_tok))
    tok_str_free(body_def);
  else
  {
    pm = cprime_mallocz(sizeof(*pm));
    pm->str = body_def;
    pm->struct_tok = class_mangled_tok;
    pm->is_template_member = 1;
    pm->is_lifecycle_member = is_lifecycle_member_tok(method_tok);
    pm->is_static_member = is_static_member;
    dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
  }
  template_member_note_inst(md, type_tok, orig_pack_tok);
}

static void instantiate_template_members_for_class(TemplateDef *td, int type_tok,
    int class_mangled_tok)
{
  int i;
  Sym *arg_sym;

  if (!td->is_class)
    return;
  arg_sym = struct_find(type_tok);
  if ((arg_sym && arg_sym->c < 0) || is_currently_defining_class(type_tok))
    return;
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    int method_tok = template_member_def_method_tok(md);
    if (md->class_tok == td->name_tok
        && template_member_def_is_scoped(md)
        && is_lifecycle_member_tok(method_tok)
        && method_tok == TOK_DESTRUCTOR1)
      instantiate_template_member_if_needed(md, type_tok,
                                            class_mangled_tok, 0);
    }
}

static int first_template_arg_tok_from_inst_type(CType *type)
{
  CType value_type;
  int struct_tok, i, j;

  value_type = *type;
  value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&value_type))
    value_type = *pointed_type(&value_type);
  struct_tok = get_struct_type_name_tok(&value_type);
  if (!struct_tok)
    return 0;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class || td->nb_type_params <= 0)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == struct_tok)
        return td->inst_type_toks[j * td->nb_type_params];
  }
  return 0;
}

static void instantiate_static_template_member_for_call(int class_mangled_tok,
                                                        int member_tok)
{
  int i, j, k;

  if (!class_mangled_tok || !member_tok)
    return;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
    {
      if (td->inst_name_toks[j] != class_mangled_tok)
        continue;
      for (k = 0; k < nb_template_member_defs; ++k)
      {
        TemplateMemberDef *md = template_member_defs[k];
        if (md->class_tok == td->name_tok
            && template_member_def_method_tok(md) == member_tok
            && class_has_static_member_func(class_mangled_tok, member_tok))
          instantiate_template_member_if_needed(
              md, td->inst_type_toks[j * td->nb_type_params],
              class_mangled_tok, 0);
      }
      return;
    }
  }
}

static void instantiate_template_member_for_call(CType *type, int method_tok,
                                                 CType *arg_types,
                                                 int explicit_arg_count)
{
  int i, j, struct_tok, type_tok = 0;
  int saved_nb_pending_template_specs;
  int saved_nb_pending_member_funcs;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return;
  /* Allow nested member instantiation from template member bodies so
     helper members (TryReserve/Realloc/Move) are emitted too.  The
     pending-member machinery defers compilation to a safe point. */
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
    {
      if (td->inst_name_toks[j] == struct_tok)
      {
        type_tok = td->inst_type_toks[j * td->nb_type_params];
        break;
      }
    }
    if (!type_tok)
      continue;
    saved_nb_pending_template_specs = nb_pending_template_specs;
    saved_nb_pending_member_funcs = nb_pending_member_funcs;
    last_instantiated_member_func_tok = 0;
    {
      /* Only instantiate the overloads whose declared signatures actually
         accept the call's argument types.  Arity alone is not enough:
         e.g. Erase(c--) (i64) must not instantiate Erase(clList<i64>&). */
      int compatible_counts[16], nb_compatible = 0;
      MemberFuncOverload *o2;
      if (arg_types && explicit_arg_count >= 0)
        for (o2 = member_func_overloads; o2; o2 = o2->next)
        {
          Sym *os;
          if (o2->struct_tok != struct_tok || o2->method_tok != method_tok
              || explicit_arg_count < o2->min_arg_count
              || explicit_arg_count > o2->explicit_arg_count)
            continue;
          os = sym_find2(global_stack, o2->mangled_tok);
          if (!os)
            os = sym_find(o2->mangled_tok);
          if (!os)
            continue;
          if (member_func_matches_arg_types(os, arg_types,
                                            explicit_arg_count)
              && nb_compatible < 16)
            compatible_counts[nb_compatible++] = o2->explicit_arg_count;
        }
    for (j = 0; j < nb_template_member_defs; ++j)
    {
      TemplateMemberDef *md = template_member_defs[j];
      int member_type_arg_tok = 0;
      if (md->class_tok == td->name_tok
          && template_member_def_method_tok(md) == method_tok)
      {
        /* Inline member templates declared inside the class body have no
           Class:: qualifier and are not replayed as out-of-class
           definitions; they are emitted directly during class layout. */
        if (!template_member_def_is_scoped(md))
          continue;
        if (explicit_arg_count > 0
            && template_member_operator_has_no_params(md, method_tok))
          continue;
        if (explicit_arg_count >= 0)
        {
          int param_count = template_member_def_param_count(md, method_tok);
          if (param_count >= 0)
          {
            MemberFuncOverload *o;
            int overload_ok = 0;
            for (o = member_func_overloads; o; o = o->next)
            {
              if (o->struct_tok == struct_tok
                  && o->method_tok == method_tok
                  && o->explicit_arg_count == param_count
                  && explicit_arg_count >= o->min_arg_count
                  && explicit_arg_count <= o->explicit_arg_count)
              {
                overload_ok = 1;
                break;
              }
            }
            if (!overload_ok && param_count != explicit_arg_count
                && !token_string_has_variadic_pack(md->def_str))
              continue;
          }
          if (param_count < 0 && explicit_arg_count == 0)
            continue;
          if (param_count >= 0 && nb_compatible > 0)
          {
            int k, ok = 0;
            for (k = 0; k < nb_compatible; ++k)
              if (compatible_counts[k] == param_count)
              {
                ok = 1;
                break;
              }
            if (!ok)
              continue;
          }
        }
        /* A member whose signature uses std::initializer_list must only be
           instantiated for a call that actually passes an initializer_list;
           replaying its body for an unrelated arity match (e.g. PushFront(x)
           also matching PushFront(initializer_list)) can fail to compile. */
        if (token_string_has_initializer_list(md->def_str))
        {
          int k, has_il_arg = 0;
          if (arg_types && explicit_arg_count > 0)
            for (k = 0; k < explicit_arg_count; ++k)
              if (ctype_is_initializer_list(&arg_types[k]))
                has_il_arg = 1;
          if (!has_il_arg)
            continue;
        }
        if (arg_types && explicit_arg_count > 0
            && template_member_def_first_param_is_array_ref(md, method_tok)
            && !(arg_types[0].t & VT_ARRAY))
          continue;
        if (arg_types && explicit_arg_count > 0)
        {
          int first_kind =
            template_member_def_first_param_class_kind(md, method_tok);
          int arg_is_struct = (arg_types[0].t & VT_BTYPE) == VT_STRUCT;
          /* Only guard against instantiating an explicitly class-typed
             member for a scalar call argument (e.g. Erase(c--) must not
             instantiate Erase(clList<i64>&)).  Generic template parameters
             and scalar signatures stay untouched. */
          if (first_kind == 1 && !arg_is_struct)
            continue;
        }
        if (method_tok == TOK_CONSTRUCTOR1 && arg_types
            && explicit_arg_count == 1
            && template_member_is_same_family_conversion_ctor(md)
            && !is_same_template_family_conversion_ctor(type, &arg_types[0]))
          continue;
        if (arg_types && explicit_arg_count > 0)
        {
          member_type_arg_tok = first_template_arg_tok_from_inst_type(&arg_types[0]);
          if (!member_type_arg_tok)
            member_type_arg_tok = template_type_tok_from_ctype(&arg_types[0]);
          /* Inference resolves typedef spellings (f32 -> float), which would
             instantiate the member against a duplicate class token such as
             clMatrix4x4__float instead of the real clMatrix4x4__f32.  When the
             inferred scalar arg is compatible with the class's own type
             argument, keep the class token so the member's Class<U> return
             and constructor references match the instantiated class. */
          if (member_type_arg_tok && type_tok
              && member_type_arg_tok != type_tok)
          {
            CType inferred_arg_type, class_arg_type;
            if (make_type_from_type_arg_tok(&inferred_arg_type,
                                            member_type_arg_tok)
                && make_type_from_type_arg_tok(&class_arg_type, type_tok)
                && is_compatible_unqualified_types(&inferred_arg_type,
                                                  &class_arg_type))
              member_type_arg_tok = type_tok;
          }
          if (token_string_has_variadic_pack(md->def_str))
          {
            int pack_arg_tok =
              template_member_def_pack_arg_tok(md, method_tok, arg_types,
                                               explicit_arg_count);
            if (pack_arg_tok)
              member_type_arg_tok = pack_arg_tok;
            else
              member_type_arg_tok = -1; /* variadic pack with zero elements */
          }
        }
        instantiate_template_member_if_needed(md, type_tok, struct_tok,
                                              member_type_arg_tok);
      }
    }
    }
    if (nb_pending_template_specs != saved_nb_pending_template_specs)
      compile_pending_template_specs_without_member_flush();
    if (last_instantiated_member_func_tok)
    {
      Sym *inst_sym = sym_find(last_instantiated_member_func_tok);
      if (!inst_sym)
        inst_sym = sym_find2(global_stack, last_instantiated_member_func_tok);
      if (inst_sym)
        note_member_func_overload(struct_tok, method_tok,
                                  last_instantiated_member_func_tok,
                                  &inst_sym->type);
    }
    if (saved_nb_pending_member_funcs != nb_pending_member_funcs)
      compile_pending_member_funcs(saved_nb_pending_member_funcs);
    return;
  }
}

static void add_template_member_def(int class_tok, int type_param_tok,
                                     TokenString *str)
{
  int i;
  TemplateDef *td;
  TemplateMemberDef *md;

  md = cprime_mallocz(sizeof(*md));
  md->class_tok = class_tok;
  md->type_param_tok = type_param_tok;
  md->def_str = str;
  dynarray_add(&template_member_defs, &nb_template_member_defs, md);


  td = find_template_def(class_tok);
  if (!td || !td->is_class)
    return;
  for (i = 0; i < td->nb_inst; ++i)
  {
    int method_tok = template_member_def_method_tok(md);
    if (template_member_def_is_scoped(md)
        && is_lifecycle_member_tok(method_tok)
        && method_tok == TOK_DESTRUCTOR1)
      instantiate_template_member_if_needed(md,
                                            td->inst_type_toks[i * td->nb_type_params],
                                            td->inst_name_toks[i], 0);
  }
}

static int skip_template_parameter_default(void)
{
  int angle = 0, paren = 0, bracket = 0;

  if (tok != '=')
    return 0;
  next();
  while (tok != TOK_EOF)
  {
    if (tok == '(')
      ++paren;
    else if (tok == ')' && paren > 0)
      --paren;
    else if (tok == '[')
      ++bracket;
    else if (tok == ']' && bracket > 0)
      --bracket;
    else if (tok == TOK_LT || tok == '<')
      ++angle;
    else if ((tok == TOK_GT || tok == '>') && angle > 0)
      --angle;
    else if (tok == TOK_SAR && angle > 0)
    {
      --angle;
      if (angle > 0)
        --angle;
    }
    else if (!angle && !paren && !bracket && (tok == ',' || tok == TOK_GT || tok == '>'))
      break;
    next();
  }
  return 1;
}

static int parse_template_type_parameter_tok(int index, int *is_variadic,
                                             int *has_default)
{
  int param_tok;
  char name[64];

  *is_variadic = 0;
  *has_default = 0;
  next();
  if (tok == TOK_DOTS)
  {
    *is_variadic = 1;
    next();
  }
  if (tok >= TOK_UIDENT)
  {
    param_tok = tok;
    next();
  }
  else
  {
    snprintf(name, sizeof(name), "__cpc_template_param_%d", index);
    param_tok = tok_alloc_const(name);
  }
  *has_default = skip_template_parameter_default();
  return param_tok;
}

static int parse_template_non_type_parameter_tok(int *has_default)
{
  int param_tok = 0;
  int paren = 0, angle = 0;
  int macro_first_arg = 0, macro_type_default = 0;

  *has_default = 0;
  while (tok != TOK_EOF)
  {
    if (!paren && !angle && (tok == ',' || tok == TOK_GT || tok == '>'))
      break;
    if (tok >= TOK_UIDENT)
    {
      param_tok = tok;
      if (paren == 1 && macro_first_arg
          && !strcmp(get_tok_str(tok, NULL), "typename"))
        macro_type_default = 1;
    }
    else if (tok == TOK_CLASS && paren == 1 && macro_first_arg)
      macro_type_default = 1;
    if (tok == '=')
      *has_default = 1;
    else if (tok == '(')
    {
      ++paren;
      if (paren == 1)
        macro_first_arg = 1;
    }
    else if (tok == ')' && paren > 0)
    {
      --paren;
      if (!paren)
        macro_first_arg = 0;
    }
    else if (tok == ',' && paren == 1 && macro_first_arg)
      macro_first_arg = 0;
    else if (tok == TOK_LT || tok == '<')
      ++angle;
    else if ((tok == TOK_GT || tok == '>') && angle > 0)
      --angle;
    else if (tok == TOK_SAR && angle > 0)
    {
      --angle;
      if (angle > 0)
        --angle;
    }
    next();
  }
  if (!*has_default && macro_type_default)
    *has_default = 1;
  return param_tok;
}

static void parse_template_decl(void)
{
  int type_param_toks[16], nb_type_params = 0;
  int nb_required_type_params = 0;
  int variadic_param_index = -1;
  int name_tok = 0, member_class_tok, prev_ident = 0, prev_tok = 0;
  int brace = 0, saw_body = 0;
  int is_class = 0;
  const char *kw;
  TemplateDef *td;
  TokenString *str;

  next();
  skip(TOK_LT);
  if (tok == TOK_GT)
  {
    int class_tok, mangled_tok;
    TemplateArgList args;

    next();
    if (tok != TOK_CLASS && tok != TOK_STRUCT)
    {
      TokenString *prefix = tok_str_alloc();
      int name_index = -1;

      while (tok != TOK_EOF && tok != TOK_LT && tok != '<')
      {
        if (tok >= TOK_UIDENT)
        {
          name_tok = tok;
          name_index = prefix->len;
        }
        tok_str_add_tok(prefix);
        next();
      }
      if (!name_tok || name_index < 0)
        cprime_error("unsupported explicit template specialization");
      td = find_function_template_def(nb_namespace_stack
                                      ? make_current_namespace_tok(name_tok)
                                      : name_tok);
      if (!td)
        cprime_error("unknown function template specialization '%s'",
                     get_tok_str(name_tok, NULL));
      parse_template_type_args(&args);
      mangled_tok = make_template_inst_name_tok(td, &args);
      if (template_lookup_inst(td, &args))
        cprime_error("redefinition of template specialization '%s'",
                     get_tok_str(name_tok, NULL));
      prefix->str[name_index] = mangled_tok;
      str = prefix;
      for (;;)
      {
        if (tok == TOK_EOF)
          cprime_error("unexpected end of file in template specialization");
        tok_str_add_tok(str);
        if (tok == '{')
        {
          saw_body = 1;
          ++brace;
        }
        else if (tok == '}')
        {
          if (saw_body && 0 == --brace)
          {
            next();
            break;
          }
        }
        else if (!saw_body && brace == 0 && tok == ';')
        {
          next();
          break;
        }
        next();
      }
      if (tok == ';')
      {
        tok_str_add_tok(str);
        next();
      }
      tok_str_add(str, TOK_EOF);
      note_template_inst(td, &args, mangled_tok);
      {
        int saved_tok = tok;
        CValue saved_tokc = tokc;
        begin_macro(str, 1);
        next();
        decl(VT_CONST);
        end_macro();
        tok = saved_tok;
        tokc = saved_tokc;
      }
      return;
    }
    class_tok = tok;
    next();
    if (tok < TOK_UIDENT)
      expect("identifier");
    name_tok = tok;
    td = find_class_template_def(nb_namespace_stack ? make_current_namespace_tok(name_tok) : name_tok);
    if (!td || !td->is_class)
      cprime_error("unknown class template specialization '%s'",
                   get_tok_str(name_tok, NULL));
    next();
    if (tok != TOK_LT && tok != '<')
      cprime_error("expected template specialization argument list");
    parse_template_type_args(&args);
    mangled_tok = make_template_inst_name_tok(td, &args);

    if (template_lookup_inst(td, &args))
      cprime_error("redefinition of template specialization '%s'",
                   get_tok_str(name_tok, NULL));

    str = tok_str_alloc();
    tok_str_add(str, class_tok);
    tok_str_add(str, mangled_tok);
    for (;;)
    {
      if (tok == TOK_EOF)
        cprime_error("unexpected end of file in template specialization");
      tok_str_add_tok(str);
      if (tok == '{')
      {
        saw_body = 1;
        ++brace;
      }
      else if (tok == '}')
      {
        if (saw_body && 0 == --brace)
        {
          next();
          break;
        }
      }
      else if (!saw_body && brace == 0 && tok == ';')
      {
        next();
        break;
      }
      next();
    }
    if (tok == ';')
    {
      tok_str_add_tok(str);
      next();
    }
    tok_str_add(str, TOK_EOF);

    note_template_inst(td, &args, mangled_tok);
    if (macro_stack)
    {
      dynarray_add(&pending_template_specs, &nb_pending_template_specs, str);
      return;
    }
    {
      int saved_tok = tok;
      int saved_defer_pending_member_funcs = defer_pending_member_funcs;
      CValue saved_tokc = tokc;
      defer_pending_member_funcs = 1;
      begin_macro(str, 1);
      next();
      decl(VT_CONST);
      end_macro();
      defer_pending_member_funcs = saved_defer_pending_member_funcs;
      tok = saved_tok;
      tokc = saved_tokc;
    }
    instantiate_template_members_for_class(td, args.toks[0], mangled_tok);
    return;
  }
  if (tok == TOK_CLASS || (tok >= TOK_UIDENT
      && !strcmp(get_tok_str(tok, NULL), "typename")))
  {
    int is_variadic, has_default, param_index = nb_type_params;
    type_param_toks[nb_type_params++] =
      parse_template_type_parameter_tok(param_index, &is_variadic,
                                        &has_default);
    if (!has_default && !is_variadic)
      nb_required_type_params = nb_type_params;
    if (is_variadic)
      variadic_param_index = param_index;
  }
  else
  {
    int has_default, param_tok = parse_template_non_type_parameter_tok(&has_default);
    if (!param_tok)
      cprime_error("template parameter");
    type_param_toks[nb_type_params++] = param_tok;
    if (!has_default)
      nb_required_type_params = nb_type_params;
  }
  while (tok == ',')
  {
    next();
    if (nb_type_params >= (int)(sizeof(type_param_toks) / sizeof(type_param_toks[0])))
      cprime_error("too many template parameters");
    if (tok == TOK_CLASS || (tok >= TOK_UIDENT
        && !strcmp(get_tok_str(tok, NULL), "typename")))
    {
      int is_variadic, has_default, param_index = nb_type_params;
      type_param_toks[nb_type_params++] =
        parse_template_type_parameter_tok(param_index, &is_variadic,
                                          &has_default);
      if (!has_default && !is_variadic)
        nb_required_type_params = nb_type_params;
      if (is_variadic)
        variadic_param_index = param_index;
    }
    else
    {
      int has_default, param_tok = parse_template_non_type_parameter_tok(&has_default);
      if (!param_tok)
        cprime_error("template parameter");
      type_param_toks[nb_type_params++] = param_tok;
      if (!has_default)
        nb_required_type_params = nb_type_params;
    }
  }
  skip(TOK_GT);

  str = tok_str_alloc();
  for (;;)
  {
    if (tok == TOK_EOF)
      cprime_error("unexpected end of file in template declaration");
    tok_str_add_tok(str);
    if (!name_tok && (prev_tok == TOK_CLASS || prev_tok == TOK_STRUCT)
        && tok >= TOK_UIDENT)
    {
      name_tok = tok;
      is_class = 1;
    }
    if (!name_tok && prev_tok >= TOK_UIDENT
        && !strcmp(get_tok_str(prev_tok, NULL), "using")
        && tok >= TOK_UIDENT)
    {
      name_tok = tok;
      is_class = 0;
    }
    if (!saw_body && brace == 0 && tok == '(' && prev_ident)
      name_tok = prev_ident;
    if (!saw_body && brace == 0 && tok == ';')
    {
      next();
      break;
    }
    if (tok == '{')
    {
      saw_body = 1;
      ++brace;
    }
    else if (tok == '}')
    {
      if (saw_body && 0 == --brace)
      {
        next();
        break;
      }
    }
    prev_tok = tok;
    if (tok >= TOK_UIDENT)
      prev_ident = tok;
    next();
  }
  if (is_class && tok == ';')
  {
    tok_str_add_tok(str);
    next();
  }
  tok_str_add(str, TOK_EOF);

  if (!name_tok)
    name_tok = find_template_name_in_str(str);
  if (!name_tok)
    cprime_error("unsupported template declaration");

  if (!is_class && variadic_param_index < 0
      && nb_required_type_params < nb_type_params)
  {
    tok_str_free(str);
    return;
  }

  member_class_tok = find_template_member_class_in_str(str, type_param_toks[0]);
  if (member_class_tok)
  {
    add_template_member_def(member_class_tok, type_param_toks[0], str);
    return;
  }

  {
    int i, func_min_args = 0, func_max_args = 0, func_is_variadic = 0;
    int is_partial_specialization =
      is_class && is_class_template_partial_specialization(str, name_tok);
    unsigned func_sig_hash = 0;
    if (!is_class)
      analyze_template_function_signature(str, name_tok, &func_min_args,
                                          &func_max_args,
                                          &func_is_variadic, &func_sig_hash);
    for (i = 0; i < nb_template_defs; ++i)
    {
      TemplateDef *old = template_defs[i];
      int lookup_tok = nb_namespace_stack ? make_current_namespace_tok(name_tok)
                                          : name_tok;
      if (!is_class && !old->is_class
          && (old->lookup_tok ? old->lookup_tok : old->name_tok) == lookup_tok
          && saw_body && !old->has_body
          && old->func_min_args == func_min_args
          && old->func_is_variadic == func_is_variadic)
      {
        tok_str_free(old->def_str);
        old->def_str = str;
        old->has_body = 1;
        return;
      }
      if (token_string_template_duplicate(template_defs[i], name_tok, is_class,
                                          func_min_args, func_max_args,
                                          func_is_variadic,
                                          func_sig_hash,
                                          is_partial_specialization))
      {
        if (!is_class && saw_body && !old->has_body)
        {
          tok_str_free(old->def_str);
          old->def_str = str;
          old->has_body = 1;
          return;
        }
        if (is_class && saw_body && !old->has_body)
        {
          /* A forward-declared class template completed by its definition
             (e.g. `template<class K, class V> struct E;` followed by the
             full body) must merge instead of being rejected. */
          tok_str_free(old->def_str);
          old->def_str = str;
          old->has_body = 1;
          return;
        }
        if (!is_class && !saw_body)
        {
          tok_str_free(str);
          return;
        }
        if (is_class && !saw_body && !old->has_body)
        {
          tok_str_free(str);
          return;
        }
        cprime_error("redefinition of template '%s'", get_tok_str(name_tok, NULL));
      }
    }
  }

  td = cprime_mallocz(sizeof(*td));
  td->name_tok = name_tok;
  td->lookup_tok = nb_namespace_stack ? make_current_namespace_tok(name_tok) : name_tok;
  td->type_param_toks = cprime_malloc(nb_type_params * sizeof(int));
  memcpy(td->type_param_toks, type_param_toks, nb_type_params * sizeof(int));
  td->nb_type_params = nb_type_params;
  td->nb_required_type_params = nb_required_type_params;
  td->variadic_param_index = variadic_param_index;
  td->is_class = is_class;
  td->is_partial_specialization =
    is_class && is_class_template_partial_specialization(str, name_tok);
  td->has_body = saw_body;
  if (!is_class)
    analyze_template_function_signature(str, name_tok, &td->func_min_args,
                                        &td->func_max_args,
                                        &td->func_is_variadic,
                                        &td->func_sig_hash);
  td->def_str = str;
  dynarray_add(&template_defs, &nb_template_defs, td);

}

static int sanitize_type_suffix(char *dst, int cap, const char *src)
{
  int i = 0;
  while (*src && i + 1 < cap)
  {
    char c = *src++;
    if ((c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '_')
      dst[i++] = c;
    else
      dst[i++] = '_';
  }
  dst[i] = '\0';
  return i;
}

static void template_validate_arg_count(TemplateDef *td, TemplateArgList *args)
{
  if (td->variadic_param_index >= 0)
  {
    if (args->nb < td->nb_required_type_params)
      cprime_error("template '%s' expects at least %d type arguments, got %d",
                   get_tok_str(td->name_tok, NULL),
                   td->nb_required_type_params,
                   args->nb);
    return;
  }
  if (args->nb < td->nb_required_type_params || args->nb > td->nb_type_params)
    cprime_error("template '%s' expects %d type argument%s, got %d",
                 get_tok_str(td->name_tok, NULL),
                 td->nb_required_type_params,
                 td->nb_required_type_params == 1 ? "" : "s",
                 args->nb);
  while (args->nb < td->nb_type_params)
    args->toks[args->nb++] = 0;
}

static int make_template_inst_name_tok(TemplateDef *td, TemplateArgList *args)
{
  char name[512], typebuf[128];
  int i;

  template_validate_arg_count(td, args);
  snprintf(name, sizeof(name), "%s",
           get_tok_str(td->lookup_tok ? td->lookup_tok : td->name_tok, NULL));
  for (i = 0; i < args->nb; ++i)
  {
    if (!args->toks[i])
      continue;
    sanitize_type_suffix(typebuf, sizeof(typebuf),
                         get_tok_str(args->toks[i], NULL));
    pstrcat(name, sizeof(name), "__");
    pstrcat(name, sizeof(name), typebuf);
  }
  if (!td->is_class)
  {
    char sigbuf[64];
    snprintf(sigbuf, sizeof(sigbuf), "__a%d_h%08x",
             td->func_min_args, td->func_sig_hash);
    pstrcat(name, sizeof(name), sigbuf);
  }
  return tok_alloc_const(name);
}

static int template_type_arg_toks_match(int t1, int t2)
{
  CType c1, c2;

  if (t1 == t2)
    return 1;
  /* Typedef spellings (i32, f32) must resolve to the same instantiation as
     their underlying types (int, float), while distinct types (ui8 vs i8)
     must stay separate.  Compare the underlying CType. */
  if (make_type_from_type_arg_tok(&c1, t1)
      && make_type_from_type_arg_tok(&c2, t2))
  {
    if ((c1.t & VT_BTYPE) != (c2.t & VT_BTYPE)
        || (c1.t & VT_UNSIGNED) != (c2.t & VT_UNSIGNED)
        || (c1.t & VT_LONG) != (c2.t & VT_LONG))
      return 0;
    if ((c1.t & VT_BTYPE) == VT_STRUCT)
      return c1.ref == c2.ref;
    return 1;
  }
  return 0;
}

static int template_lookup_inst(TemplateDef *td, TemplateArgList *args)
{
  int i, j, same;

  template_validate_arg_count(td, args);
  if (td->nb_inst <= 0)
    return 0;
  if (!td->inst_type_toks || !td->inst_name_toks
      || td->al_inst <= 0 || td->nb_inst > td->al_inst)
    cprime_error("corrupt template instantiation table for '%s'",
                 get_tok_str(td->name_tok, NULL));
  for (i = 0; i < td->nb_inst; ++i)
  {
    same = 1;
    for (j = 0; j < td->nb_type_params; ++j)
    {
      if (!template_type_arg_toks_match(
              td->inst_type_toks[i * td->nb_type_params + j],
              args->toks[j]))
      {
        same = 0;
        break;
      }
    }
    if (same)
      return td->inst_name_toks[i];
  }
  return 0;
}

static int template_alias_lookup(int *from, int *to, int n, int tok)
{
  int i;

  for (i = 0; i < n; ++i)
    if (from[i] == tok)
      return to[i];
  return 0;
}

static int template_arg_for_param(TemplateDef *td, TemplateArgList *args, int t)
{
  int i;

  for (i = 0; i < td->nb_type_params; ++i)
    if (td->type_param_toks[i] == t)
      return args->toks[i];
  return 0;
}

static Sym *find_typedef_sym_in_stack(Sym *stack, int tok)
{
  Sym *s;
  for (s = stack; s; s = s->prev)
    if ((s->v & ~SYM_FIELD) == tok && (s->type.t & VT_TYPEDEF))
      return s;
  return NULL;
}

static int template_param_index(TemplateDef *td, int t)
{
  int i;

  if (!td)
    return -1;
  for (i = 0; i < td->nb_type_params; ++i)
    if (td->type_param_toks[i] == t)
      return i;
  return -1;
}

static int template_inst_arg_for_name(TemplateDef *td, int inst_tok,
                                      int param_index)
{
  int i;

  if (!td || param_index < 0 || param_index >= td->nb_type_params)
    return 0;
  for (i = 0; i < td->nb_inst; ++i)
    if (td->inst_name_toks[i] == inst_tok)
      return td->inst_type_toks[i * td->nb_type_params + param_index];
  return 0;
}

static int template_subst_token(TemplateDef *td, TemplateArgList *args,
                                int class_mangled_tok,
                                int *alias_from, int *alias_to,
                                int nb_aliases, int t)
{
  int alias_tok, arg_tok;

  alias_tok = template_alias_lookup(alias_from, alias_to, nb_aliases, t);
  if (alias_tok)
    return alias_tok;
  arg_tok = template_arg_for_param(td, args, t);
  if (arg_tok)
    return arg_tok;
  if (td->is_class && t == td->name_tok)
    return class_mangled_tok;
  return t;
}

static void template_add_subst_token(TokenString *spec, TemplateDef *td,
                                     TemplateArgList *args,
                                     int class_mangled_tok,
                                     int *alias_from, int *alias_to,
                                     int nb_aliases, int t)
{
  int subst_tok;

  subst_tok = template_subst_token(td, args, class_mangled_tok,
                                   alias_from, alias_to, nb_aliases, t);
  if (subst_tok != t)
    tok_str_add(spec, subst_tok);
  else
    tok_str_add_tok(spec);
}

static int template_typedef_is_type_pos(TokenString *spec)
{
  int prev = spec->len > 0 ? spec->str[spec->len - 1] : 0;

  /* A typedef token is a declarator name, not a type, when it directly
     follows a reference/pointer marker, a closing delimiter, a template
     close, an arrow/dot, or another identifier (the type itself).  Only
     rewrite it when a type is expected at a declaration boundary. */
  if (prev == '&' || prev == '*' || prev == ')' || prev == ']'
      || prev == TOK_GT || prev == '>'
      || prev == TOK_ARROW || prev == '.'
      || prev >= TOK_UIDENT)
    return 0;
  return 1;
}

static int template_type_tok_from_ctype(CType *type)
{
  int bt = type->t &VT_BTYPE;

  if (bt == VT_PTR)
  {
    CType *pt = pointed_type(type);
    if (pt)
      return template_type_tok_from_ctype(pt);
  }
  if (bt == VT_STRUCT)
    return get_struct_type_name_tok(type);
  if (bt == VT_DOUBLE)
    return TOK_DOUBLE;
  if (bt == VT_FLOAT)
    return TOK_FLOAT;
  if (bt == VT_BYTE)
    return TOK_CHAR;
  if (bt == VT_BOOL)
    return TOK_BOOL;
  if (bt == VT_LLONG)
    return TOK_LONG;
  if (bt == VT_INT)
    return TOK_INT;
  return 0;
}

static int infer_template_type_tok_from_first_arg(void)
{
  Sym *arg_sym;

  switch (tok)
  {
  case TOK_CINT:
  case TOK_CCHAR:
  case TOK_CUINT:
    return TOK_INT;
  case TOK_CLONG:
  case TOK_CULONG:
    return TOK_LONG;
  case TOK_CFLOAT:
    return TOK_FLOAT;
  default:
    if (tok >= TOK_UIDENT)
    {
      arg_sym = sym_find(tok);
      if (arg_sym)
        return template_type_tok_from_ctype(&arg_sym->type);
    }
    return 0;
  }
}

static int parse_one_template_type_arg(void)
{
  int type_tok;

  if (tok == TOK_CHAR || tok == TOK_INT || tok == TOK_LONG || tok == TOK_FLOAT
      || tok == TOK_DOUBLE || tok == TOK_BOOL || tok == TOK_BOOL2
      || tok == TOK_TRUE || tok == TOK_FALSE)
  {
    type_tok = tok;
    next();
  }
  else if (tok == TOK_ENUM || tok == TOK_STRUCT || tok == TOK_CLASS
           || tok == TOK_UNION)
  {
    CType type;
    int kind = tok == TOK_ENUM ? VT_ENUM
             : tok == TOK_UNION ? VT_UNION
             : VT_STRUCT;
    struct_decl(&type, kind, tok == TOK_CLASS);
    if (!type.ref)
      cprime_error("unsupported template type argument");
    type_tok = type.ref->v & ~SYM_STRUCT;
  }
  else if (tok == TOK_DECLTYPE)
  {
    CType type;
    AttributeDef ad;
    memset(&type, 0, sizeof(type));
    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      cprime_error("unsupported decltype template argument");
    type_tok = template_type_tok_from_ctype(&type);
    if (!type_tok)
      cprime_error("could not instantiate decltype template argument");
  }
  else if (tok >= TOK_UIDENT)
  {
    int name_tok = tok;
    TemplateDef *td;
    {
      int nested_tok = find_current_class_nested_type_tok(tok);
      if (nested_tok != tok)
      {
        next();
        return nested_tok;
      }
    }
    if (is_namespace_tok(tok))
    {
      int parts[16], nb_parts = 0;
      parts[nb_parts++] = tok;
      next();
      while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
      {
        next();
        if (tok != ':')
          cprime_error("qualified template type requires '::'");
        next();
        if (tok < TOK_UIDENT)
          cprime_error("qualified template type name expected");
        parts[nb_parts++] = tok;
        next();
      }
      name_tok = make_namespace_tok_from_parts(parts, nb_parts);
      td = find_class_template_def(name_tok);
      if (!td && tok == TOK_LT
          && (strstr(get_tok_str(name_tok, NULL), "remove_const")
              || strstr(get_tok_str(name_tok, NULL), "remove_reference")))
      {
        TemplateArgList alias_args;
        parse_template_type_args(&alias_args);
        if (alias_args.nb != 1)
          cprime_error("type-transform alias requires one argument");
        if (tok == ':')
        {
          next();
          skip(':');
          if (tok < TOK_UIDENT)
            cprime_error("type-transform member name expected");
          next();
        }
        {
          /* std::remove_const<T>::type / std::remove_reference<T>::type
             denote the type itself; return its canonical spelling so a
             typedef argument (f32) matches the plain-type specialization
             (clAdditiveIdentity<float>). */
          CType canonical_type;
          int alias_tok = alias_args.toks[0];
          if (make_type_from_type_arg_tok(&canonical_type, alias_tok))
          {
            int canonical_tok = template_type_tok_from_ctype(&canonical_type);
            if (canonical_tok)
              return canonical_tok;
          }
          return alias_tok;
        }
      }
      if (td && td->is_class && tok == TOK_LT)
      {
        int nested_mangled_tok;
        TemplateArgList nested_args;
        parse_template_type_args(&nested_args);
        nested_mangled_tok = instantiate_template_if_needed(td, &nested_args);
        compile_pending_template_specs();
        if (!struct_find(nested_mangled_tok))
          cprime_error("template class instantiation failed for '%s'",
                       get_tok_str(td->name_tok, NULL));
        return nested_mangled_tok;
      }
      return name_tok;
    }
    td = find_class_template_def(tok);
    if (td && td->is_class)
    {
      int nested_mangled_tok;
      TemplateArgList nested_args;
      next();
      if (tok != TOK_LT)
        cprime_error("unsupported template type argument");
      parse_template_type_args(&nested_args);
      nested_mangled_tok = instantiate_template_if_needed(td, &nested_args);
      compile_pending_template_specs();
      if (!struct_find(nested_mangled_tok))
        cprime_error("template class instantiation failed for '%s'",
                  get_tok_str(td->name_tok, NULL));
      type_tok = nested_mangled_tok;
    }
    else
    {
      Sym *alias_sym = sym_find(tok);
      Sym *global_alias = sym_find2(global_stack, tok);
      if (!alias_sym)
        alias_sym = sym_find2(global_stack, tok);
      /* Resolve local/class-scope aliases only.  Global typedef names such
         as ui8/i8 must stay distinct template arguments (their
         specializations differ even when the underlying type is char). */
      if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
          && alias_sym != global_alias)
      {
        CType alias_type = alias_sym->type;
        alias_type.t &= ~VT_TYPEDEF;
        type_tok = template_type_tok_from_ctype(&alias_type);
        if (type_tok)
        {
          next();
          return type_tok;
        }
      }
      type_tok = tok;
      next();
      if (tok == ':')
      {
        int parts[16], nb_parts = 0;
        parts[nb_parts++] = type_tok;
        while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
        {
          next();
          skip(':');
          if (tok < TOK_UIDENT)
            cprime_error("qualified template type name expected");
          parts[nb_parts++] = tok;
          next();
        }
        if (nb_parts == 2 && struct_find(parts[0]))
        {
          int qualified_tok = make_static_member_tok(parts[0], parts[1]);
          type_tok = struct_find(qualified_tok) ? qualified_tok : parts[1];
        }
        else
          type_tok = parts[nb_parts - 1];
      }
    }
  }
  else
  {
    cprime_error("unsupported template type argument near '%s'", get_tok_str(tok, &tokc));
  }
  return type_tok;
}

static void parse_template_type_args(TemplateArgList *args)
{
  int opening_source_line = file ? file->line_num : 0;
  args->nb = 0;
  skip(TOK_LT);
  for (;;)
  {
    if (args->nb >= (int)(sizeof(args->toks) / sizeof(args->toks[0])))
      cprime_error("too many template type arguments");
    args->toks[args->nb++] = parse_one_template_type_arg();
    if (tok == TOK_SAR)
    {
      tok = TOK_GT;
      unget_tok(TOK_GT);
    }
    if (tok != ',')
      break;
    next();
  }
  if (tok == TOK_SAR)
  {
    tok = TOK_GT;
    unget_tok(TOK_GT);
  }
  if (tok != TOK_GT)
    cprime_error("'>' expected after template argument '%s' opened near line %d (got '%s')",
                 args->nb > 0 ? get_tok_str(args->toks[args->nb - 1], NULL) : "-",
                 opening_source_line, get_tok_str(tok, &tokc));
  next();
}

static int parse_template_type_param_subst(TemplateDef *ctx_td,
                                           TemplateArgList *ctx_args)
{
  int i;

  if (!ctx_td || !ctx_args)
    return 0;
  for (i = 0; i < ctx_td->nb_type_params; ++i)
    if (tok == ctx_td->type_param_toks[i])
    {
      next();
      return ctx_args->toks[i];
    }
  return 0;
}

static int parse_one_template_type_arg_subst(TemplateDef *ctx_td,
                                             TemplateArgList *ctx_args)
{
  int type_tok;

  if (tok >= TOK_UIDENT
      && (!strcmp(get_tok_str(tok, NULL), "typename")
          || !strcmp(get_tok_str(tok, NULL), "class")))
    next();
  if (tok == TOK_BOOL || tok == TOK_BOOL2 || tok == TOK_TRUE || tok == TOK_FALSE)
  {
    type_tok = tok;
    next();
    return type_tok;
  }
  if (tok == TOK_CHAR || tok == TOK_INT || tok == TOK_LONG || tok == TOK_FLOAT
      || tok == TOK_DOUBLE)
  {
    type_tok = tok;
    next();
    if (tok == ':')
    {
      int parts[16], nb_parts = 0;
      parts[nb_parts++] = type_tok;
      while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
      {
        next();
        skip(':');
        if (tok < TOK_UIDENT)
          cprime_error("qualified template type name expected");
        parts[nb_parts++] = tok;
        next();
      }
      type_tok = parts[nb_parts - 1];
    }
    return type_tok;
  }
  type_tok = parse_template_type_param_subst(ctx_td, ctx_args);
  if (type_tok)
    return type_tok;
  if (tok == TOK_ENUM || tok == TOK_STRUCT || tok == TOK_CLASS
      || tok == TOK_UNION)
  {
    CType type;
    int kind = tok == TOK_ENUM ? VT_ENUM
             : tok == TOK_UNION ? VT_UNION
             : VT_STRUCT;
    struct_decl(&type, kind, tok == TOK_CLASS);
    if (!type.ref)
      cprime_error("unsupported template type argument");
    return type.ref->v & ~SYM_STRUCT;
  }
  if (tok == TOK_DECLTYPE)
  {
    CType type;
    AttributeDef ad;
    memset(&type, 0, sizeof(type));
    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      cprime_error("unsupported decltype template argument");
    type_tok = template_type_tok_from_ctype(&type);
    if (!type_tok)
      cprime_error("could not instantiate decltype template argument");
    return type_tok;
  }
  if (tok >= TOK_UIDENT)
  {
    int name_tok = tok;
    TemplateDef *td;
    if (is_namespace_tok(tok))
    {
      int parts[16], nb_parts = 0;
      parts[nb_parts++] = tok;
      next();
      while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
      {
        next();
        if (tok != ':')
          cprime_error("qualified template type requires '::'");
        next();
        if (tok < TOK_UIDENT)
          cprime_error("qualified template type name expected");
        parts[nb_parts++] = tok;
        next();
      }
      name_tok = make_namespace_tok_from_parts(parts, nb_parts);
      td = find_class_template_def(name_tok);
      if (tok == TOK_LT
          && (strstr(get_tok_str(name_tok, NULL), "is_floating_point")
              || strstr(get_tok_str(name_tok, NULL), "is_signed")
              || strstr(get_tok_str(name_tok, NULL), "is_integral")
              || strstr(get_tok_str(name_tok, NULL), "is_same")))
      {
        TemplateArgList trait_args;
        int value;
        parse_template_type_args_subst(&trait_args, ctx_td, ctx_args);
        value = standard_type_trait_value(name_tok, &trait_args);
        if (tok == ':')
        {
          next();
          skip(':');
          if (tok < TOK_UIDENT || strcmp(get_tok_str(tok, NULL), "value"))
            cprime_error("type-trait value member expected");
          next();
        }
        return value ? TOK_TRUE : TOK_FALSE;
      }
      if (!td && tok == TOK_LT
          && (strstr(get_tok_str(name_tok, NULL), "remove_const")
              || strstr(get_tok_str(name_tok, NULL), "remove_reference")))
      {
        TemplateArgList alias_args;
        CType canonical_type;
        parse_template_type_args_subst(&alias_args, ctx_td, ctx_args);
        if (alias_args.nb != 1)
          cprime_error("type-transform alias requires one argument");
        if (tok == ':')
        {
          next();
          skip(':');
          if (tok < TOK_UIDENT)
            cprime_error("type-transform member name expected");
          next();
        }
        /* std::remove_const<T>::type / std::remove_reference<T>::type
           denote the type itself; return its canonical spelling so a
           typedef argument (f32) matches the plain-type specialization
           (clAdditiveIdentity<float>). */
        if (make_type_from_type_arg_tok(&canonical_type, alias_args.toks[0]))
        {
          int canonical_tok =
            template_type_tok_from_ctype(&canonical_type);
          if (canonical_tok)
            return canonical_tok;
        }
        return alias_args.toks[0];
      }
      if (!td || !td->is_class || tok != TOK_LT)
        return name_tok;
    }
    else
      td = find_class_template_def(tok);
    if (td && td->is_class)
    {
      int nested_mangled_tok;
      TemplateArgList nested_args;

      if (tok != TOK_LT)
        next();
      if (tok != TOK_LT && tok != '<')
        cprime_error("unsupported template type argument");
      nested_args.nb = 0;
      skip(TOK_LT);
      for (;;)
      {
        if (nested_args.nb >= (int)(sizeof(nested_args.toks) / sizeof(nested_args.toks[0])))
          cprime_error("too many template type arguments");
        nested_args.toks[nested_args.nb++] =
          parse_one_template_type_arg_subst(ctx_td, ctx_args);
        if (tok == TOK_SAR)
        {
          tok = TOK_GT;
          unget_tok(TOK_GT);
        }
        if (tok != ',')
          break;
        next();
      }
      if (tok == TOK_SAR)
      {
        tok = TOK_GT;
        unget_tok(TOK_GT);
      }
      skip(TOK_GT);
      nested_mangled_tok = instantiate_template_if_needed(td, &nested_args);
      compile_pending_template_specs_without_member_flush();
      if (!struct_find(nested_mangled_tok))
        cprime_error("template class instantiation failed for '%s'",
                  get_tok_str(td->name_tok, NULL));
      if (tok == ':')
      {
        /* typename Nested<T>::Member resolves to the member's type
           (e.g. std::remove_reference<T>::type after substitution). */
        Sym *class_sym, *field;
        CType member_type;
        int dummy_ofs;
        next();
        if (tok != ':')
          cprime_error("':' expected in nested template type argument");
        next();
        if (tok < TOK_UIDENT)
          cprime_error("nested template type member expected");
        {
          int member_name_tok = tok;
        field = NULL;
        class_sym = struct_find(nested_mangled_tok);
        if (class_sym)
        {
          CType class_type;
          Sym *fs;
          class_type.t = class_sym->type.t;
          class_type.ref = class_sym;
          field = find_field_try(&class_type, member_name_tok, &dummy_ofs);
          if (!field)
            field = find_field_try(&class_type, member_name_tok | SYM_FIELD,
                                   &dummy_ofs);
        }
        next();
        if (!field || !(field->type.t & VT_TYPEDEF))
        {
          cprime_error("nested template type member must be a typedef");
        }
        }
        member_type = field->type;
        member_type.t &= ~VT_TYPEDEF;
        type_tok = template_type_tok_from_ctype(&member_type);
        if (!type_tok)
          type_tok = get_struct_type_name_tok(&member_type);
        return type_tok;
      }
      return nested_mangled_tok;
    }
    {
      Sym *arg_sym = sym_find(tok);
      if (!arg_sym)
        arg_sym = sym_find2(global_stack, tok);
      if (arg_sym && (arg_sym->type.t & VT_TYPEDEF))
      {
        int canonical_tok = template_type_tok_from_ctype(&arg_sym->type);
        if (canonical_tok)
        {
          type_tok = canonical_tok;
          next();
          return type_tok;
        }
      }
    }
    type_tok = tok;
    next();
    return type_tok;
  }
  cprime_error("unsupported template type argument near '%s'", get_tok_str(tok, &tokc));
  return 0;
}

static void parse_template_type_args_subst(TemplateArgList *out_args,
                                           TemplateDef *ctx_td,
                                           TemplateArgList *ctx_args)
{
  out_args->nb = 0;
  skip(TOK_LT);
  for (;;)
  {
    if (out_args->nb >= (int)(sizeof(out_args->toks) / sizeof(out_args->toks[0])))
      cprime_error("too many template type arguments");
    out_args->toks[out_args->nb++] =
      parse_one_template_type_arg_subst(ctx_td, ctx_args);
    if (tok == TOK_SAR)
    {
      tok = TOK_GT;
      unget_tok(TOK_GT);
    }
    if (tok != ',')
      break;
    next();
  }
  if (tok == TOK_SAR)
  {
    tok = TOK_GT;
    unget_tok(TOK_GT);
  }
  if (tok != TOK_GT)
    cprime_error("'>' expected after substituted template argument '%s' (got '%s')",
                 out_args->nb > 0
                   ? get_tok_str(out_args->toks[out_args->nb - 1], NULL) : "-",
                 get_tok_str(tok, &tokc));
  next();
}

static int parse_template_type_arg(void)
{
  TemplateArgList args;

  parse_template_type_args(&args);
  if (args.nb != 1)
    cprime_error("single template type argument expected");
  return args.toks[0];
}

static void template_arg_list_one(TemplateArgList *args, int type_tok)
{
  args->nb = 1;
  args->toks[0] = type_tok;
}

static void note_template_inst(TemplateDef *td, TemplateArgList *args,
                               int mangled_tok)
{
  int i;

  template_validate_arg_count(td, args);
  if (td->nb_inst >= td->al_inst)
  {
    td->al_inst = td->al_inst ? td->al_inst * 2 : 4;
    td->inst_type_toks = cprime_realloc(td->inst_type_toks,
        td->al_inst * td->nb_type_params * sizeof(int));
    td->inst_name_toks = cprime_realloc(td->inst_name_toks,
        td->al_inst * sizeof(int));
  }
  for (i = 0; i < td->nb_type_params; ++i)
    td->inst_type_toks[td->nb_inst * td->nb_type_params + i] = args->toks[i];
  td->inst_name_toks[td->nb_inst] = mangled_tok;
  td->nb_inst++;
}

static int pending_template_spec_index_for_tok(int mangled_tok)
{
  int i;

  for (i = 0; i < nb_pending_template_specs; ++i)
  {
    if (!pending_template_specs[i])
      continue;
    if (token_string_contains_tok(pending_template_specs[i], mangled_tok))
      return i;
  }
  return -1;
}

static TemplateDef *matching_partial_class_template(TemplateDef *primary,
                                                     TemplateArgList *args,
                                                     int *pattern_toks)
{
  int d;

  if (!primary || !primary->is_class || primary->is_partial_specialization)
    return NULL;
  for (d = 0; d < nb_template_defs; ++d)
  {
    TemplateDef *candidate = template_defs[d];
    int i, p = -1, nb = 0, matched = 1;
    if (!candidate->is_class || !candidate->is_partial_specialization
        || candidate->lookup_tok != primary->lookup_tok)
      continue;
    for (i = 0; i + 1 < candidate->def_str->len; ++i)
      if (candidate->def_str->str[i] == candidate->name_tok
          && candidate->def_str->str[i + 1] == TOK_LT)
      {
        p = i + 2;
        break;
      }
    if (p < 0)
      continue;
    while (p < candidate->def_str->len
           && candidate->def_str->str[p] != TOK_GT
           && candidate->def_str->str[p] != TOK_SAR)
    {
      int t = candidate->def_str->str[p++];
      int is_param = template_param_index(candidate, t) >= 0;
      if (t == ',')
        continue;
      if (nb >= 16 || nb >= args->nb)
      {
        matched = 0;
        break;
      }
      pattern_toks[nb] = t;
      if (!is_param && t != args->toks[nb])
        matched = 0;
      ++nb;
    }
    if (matched && nb == args->nb)
      return candidate;
  }
  return NULL;
}

static int instantiate_template_if_needed(TemplateDef *td, TemplateArgList *args)
{
  int mangled_tok;
  TokenString *spec;
  int saved_tok;
  CValue saved_tokc;
  int alias_from[64], alias_to[64], nb_aliases = 0;
  Sym *type_sym, *ident_sym;
  int type_tok;
  int function_decl_name_done = 0;

  if (!td)
    cprime_error("instantiate_template_if_needed called with NULL template");
  if (td->is_class && !td->is_partial_specialization)
  {
    int pattern_toks[16];
    TemplateDef *partial = matching_partial_class_template(td, args,
                                                           pattern_toks);
    if (partial)
    {
      TemplateDef selected = *partial;
      selected.type_param_toks = pattern_toks;
      selected.nb_type_params = args->nb;
      selected.nb_required_type_params = args->nb;
      selected.variadic_param_index = -1;
      selected.inst_type_toks = NULL;
      selected.inst_name_toks = NULL;
      selected.nb_inst = selected.al_inst = 0;
      return instantiate_template_if_needed(&selected, args);
    }
  }

  if (td->is_class)
  {
    int spec_index;
    int i, j;
    int existing_inst;

    /* An explicit specialization may have been registered with a typedef
       spelling of the argument (e.g. clAdditiveIdentity<i32> while the call
       spells clAdditiveIdentity<int>).  Match by underlying type before
       building a new instance name. */
    existing_inst = template_lookup_inst(td, args);
    if (existing_inst)
    {
      if (struct_find(existing_inst))
        return existing_inst;
      /* The matching explicit specialization may still be queued as a
         pending spec (e.g. a macro-expanded template<> declaration is
         deferred while the macro is active).  Compile pending specs so the
         struct materializes before the call proceeds. */
      if (pending_template_spec_index_for_tok(existing_inst) >= 0)
      {
        compile_pending_template_specs_without_member_flush();
        if (struct_find(existing_inst))
          return existing_inst;
      }
    }
    mangled_tok = make_template_inst_name_tok(td, args);
    if (struct_find(mangled_tok))
    {
      return mangled_tok;
    }
    spec_index = pending_template_spec_index_for_tok(mangled_tok);
    if (spec_index >= compiled_template_specs)
    {
      compile_pending_template_specs_without_member_flush();
      if (struct_find(mangled_tok))
        return mangled_tok;
    }
  }

  if (!td->is_class)
  {
    mangled_tok = template_lookup_inst(td, args);
    if (mangled_tok)
    {
      return mangled_tok;
    }
  }

  mangled_tok = make_template_inst_name_tok(td, args);
  if (!td->is_class)
  {
    int pending_index;
    if (sym_find(mangled_tok) || sym_find2(global_stack, mangled_tok))
      return mangled_tok;
    pending_index = pending_template_spec_index_for_tok(mangled_tok);
    if (pending_index >= 0)
      return mangled_tok;
  }
  note_template_inst(td, args, mangled_tok);
  spec = tok_str_alloc();
  type_tok = args->toks[0];
  type_sym = struct_find(type_tok);
  ident_sym = sym_find(type_tok);
  if (type_sym && !type_sym->a.is_class_tag
      && (!ident_sym || !(ident_sym->type.t & VT_TYPEDEF)))
  {
    int bt = type_sym->type.t & VT_BTYPE;
    tok_str_add(spec, TOK_TYPEDEF);
    if (bt == VT_STRUCT)
      tok_str_add(spec, TOK_STRUCT);
    else if (bt == VT_UNION)
      tok_str_add(spec, TOK_UNION);
    else if (IS_ENUM(type_sym->type.t))
      tok_str_add(spec, TOK_ENUM);
    else
      cprime_error("unsupported template type argument");
    tok_str_add(spec, type_tok);
    tok_str_add(spec, type_tok);
    tok_str_add(spec, ';');
  }
  if (!td->is_class)
  {
    tok_str_add(spec, TOK_STATIC);
    tok_str_add(spec, TOK_INLINE1);
  }
  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(td->def_str, 3);
  next();
  while (tok != TOK_EOF)
  {
    if (td->variadic_param_index >= 0 && tok == TOK_DOTS)
    {
      next();
      continue;
    }
    if (!td->is_class && tok == td->name_tok)
    {
      tok_str_add(spec, function_decl_name_done ? tok : mangled_tok);
      function_decl_name_done = 1;
      next();
      continue;
    }
    if (!td->is_class && tok == TOK_AUTO && args->nb > 0)
    {
      int ret_struct_tok = infer_template_return_struct_tok(td, args);
      if (ret_struct_tok)
      {
        tok_str_add(spec, ret_struct_tok);
        next();
        continue;
      }
      {
        CType scalar_ret;
        if (infer_template_return_scalar_ctype(td, args, &scalar_ret)
            && add_ctype_tokens(spec, &scalar_ret))
        {
          next();
          continue;
        }
      }
    }
    if (!td->is_class && tok >= TOK_UIDENT)
    {
      int class_template_tok = tok;
      TemplateDef *class_td = find_class_template_def(class_template_tok);
      if (class_td)
      {
        next();
        if (tok == TOK_LT || tok == '<')
        {
          TemplateArgList class_args;
          parse_template_type_args_subst(&class_args, td, args);
          if (tok == ':')
          {
            int class_inst_tok, member_tok;
            next();
            if (tok != ':')
              cprime_error("':' expected");
            next();
            if (tok < TOK_UIDENT)
              cprime_error("static member name");
            member_tok = tok;
            class_inst_tok = instantiate_template_if_needed(class_td,
                                                            &class_args);
            tok_str_add(spec, make_static_member_tok(class_inst_tok,
                                                     member_tok));
            next();
            continue;
          }
          tok_str_add(spec, instantiate_template_if_needed(class_td,
                                                           &class_args));
          continue;
        }
        tok_str_add(spec, class_template_tok);
        continue;
      }
    }
    if (td->is_class && tok >= TOK_UIDENT && is_namespace_tok(tok))
    {
      int parts[16], nb_parts = 0, qtok, handled = 0;
      TokenString *replay = tok_str_alloc();

      parts[nb_parts++] = tok;
      tok_str_add2(replay, tok, &tokc);
      next();
      while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
      {
        tok_str_add(replay, tok);
        next();
        if (tok != ':')
          break;
        tok_str_add(replay, tok);
        next();
        if (tok < TOK_UIDENT)
          break;
        parts[nb_parts++] = tok;
        tok_str_add2(replay, tok, &tokc);
        next();
      }
      qtok = make_namespace_tok_from_parts(parts, nb_parts);
      {
        TemplateDef *class_td = find_class_template_def(qtok);
        if (class_td && class_td->is_class && (tok == TOK_LT || tok == '<'))
        {
          int i, inst_tok;
          TemplateArgList class_args;

          next();
          class_args.nb = 0;
          for (;;)
          {
            if (class_args.nb >= (int)(sizeof(class_args.toks) / sizeof(class_args.toks[0])))
              cprime_error("too many template type arguments");
            for (i = 0; i < td->nb_type_params; ++i)
              if (tok == td->type_param_toks[i])
                break;
            if (i != td->nb_type_params)
            {
              class_args.toks[class_args.nb++] = args->toks[i];
              next();
            }
            else
              class_args.toks[class_args.nb++] = parse_one_template_type_arg();
            if (tok == TOK_SAR)
            {
              tok = TOK_GT;
              unget_tok(TOK_GT);
            }
            if (tok != ',')
              break;
            next();
          }
          skip(TOK_GT);
          inst_tok = instantiate_template_if_needed(class_td, &class_args);
          compile_pending_template_specs_without_member_flush();
          tok_str_add(spec, TOK_STRUCT);
          tok_str_add(spec, inst_tok);
          handled = 1;
        }
      }
      if (handled)
      {
        tok_str_free(replay);
        continue;
      }
      tok_str_append_without_eof(spec, replay);
      tok_str_free(replay);
      continue;
    }
    if (td->is_class && tok == TOK_OPERATOR)
    {
      next();
      if (tok == '[')
      {
        next();
        skip(']');
        tok_str_add(spec, tok_alloc_const("operator[]"));
        continue;
      }
      tok_str_add(spec, TOK_OPERATOR);
      continue;
    }
    if (td->is_class && is_template_keyword_tok(tok))
    {
      TokenString *member_str = skip_or_save_template_member_decl(1);
      if (member_str && td->nb_type_params > 0)
        add_template_member_def(td->name_tok, td->type_param_toks[0],
                                member_str);
      else if (member_str)
        tok_str_free(member_str);
      continue;
    }
    if (td->is_class && tok == td->name_tok)
    {
      next();
      while (tok == TOK_LINENUM)
        next();
      if (tok == TOK_LT || tok == '<')
      {
        TemplateArgList self_args;
        int i, same_self_args = 1, self_mangled_tok;
        next();
        self_args.nb = 0;
        for (;;)
        {
          if (self_args.nb >= (int)(sizeof(self_args.toks) / sizeof(self_args.toks[0])))
            cprime_error("too many self template arguments");
          for (i = 0; i < td->nb_type_params; ++i)
          {
            if (tok == td->type_param_toks[i])
              break;
          }
          if (i != td->nb_type_params)
          {
            self_args.toks[self_args.nb++] = args->toks[i];
            next();
          }
          else
            self_args.toks[self_args.nb++] = parse_one_template_type_arg();
          if (tok == TOK_SAR)
          {
            tok = TOK_GT;
            unget_tok(TOK_GT);
          }
          if (tok != ',')
            break;
          next();
        }
        if (self_args.nb != td->nb_type_params)
          cprime_error("unsupported self template reference");
        skip(TOK_GT);
        for (i = 0; i < self_args.nb; ++i)
          if (self_args.toks[i] != args->toks[i])
            same_self_args = 0;
        self_mangled_tok = same_self_args
                           ? mangled_tok
                           : make_template_inst_name_tok(td, &self_args);
        if (!same_self_args)
          tok_str_add(spec, TOK_STRUCT);
        tok_str_add(spec, self_mangled_tok);
      }
      else
      {
        tok_str_add(spec, mangled_tok);
      }
      continue;
    }
    if (td->is_class && tok == TOK_TYPEDEF)
    {
      int i, alias_tok = 0, new_alias_tok = 0;
      char alias_name[512];
      TokenString *typedef_str = tok_str_alloc();

      while (tok != TOK_EOF)
      {
        tok_str_add(typedef_str, tok);
        if (tok >= TOK_UIDENT)
          alias_tok = tok;
        if (tok == ';')
          break;
        next();
      }
      if (alias_tok)
      {
        snprintf(alias_name, sizeof(alias_name), "%s__%s",
                 get_tok_str(mangled_tok, NULL), get_tok_str(alias_tok, NULL));
        new_alias_tok = tok_alloc_const(alias_name);
        note_template_alias_inst(mangled_tok, alias_tok, new_alias_tok);
        if (nb_aliases < 64)
        {
          alias_from[nb_aliases] = alias_tok;
          alias_to[nb_aliases] = new_alias_tok;
          nb_aliases++;
        }
      }
      for (i = 0; i < typedef_str->len; ++i)
      {
        int t = typedef_str->str[i];
        if (t == alias_tok && new_alias_tok)
          tok_str_add(spec, new_alias_tok);
        else if (template_arg_for_param(td, args, t))
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, t);
        else
          tok_str_add(spec, template_subst_token(td, args, mangled_tok,
                                                 alias_from, alias_to,
                                                 nb_aliases, t));
      }
      tok_str_free(typedef_str);
    }
    else if (td->is_class && tok >= TOK_UIDENT
             && !strcmp(get_tok_str(tok, NULL), "using"))
    {
      int i, alias_tok = 0, new_alias_tok = 0;
      char alias_name[512];
      TokenString *using_str = tok_str_alloc();

      tok_str_add(using_str, tok);
      next();
      if (tok >= TOK_UIDENT)
      {
        alias_tok = tok;
        snprintf(alias_name, sizeof(alias_name), "%s__%s",
                 get_tok_str(mangled_tok, NULL), get_tok_str(alias_tok, NULL));
        new_alias_tok = tok_alloc_const(alias_name);
        note_template_alias_inst(mangled_tok, alias_tok, new_alias_tok);
        if (nb_aliases < 64)
        {
          alias_from[nb_aliases] = alias_tok;
          alias_to[nb_aliases] = new_alias_tok;
          nb_aliases++;
        }
      }
      while (tok != TOK_EOF)
      {
        tok_str_add(using_str, tok);
        if (tok == ';')
          break;
        next();
      }
      for (i = 0; i < using_str->len; ++i)
      {
        int t = using_str->str[i];
        if (t == alias_tok && new_alias_tok)
          tok_str_add(spec, new_alias_tok);
        else if (template_arg_for_param(td, args, t))
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, t);
        else
          tok_str_add(spec, template_subst_token(td, args, mangled_tok,
                                                 alias_from, alias_to,
                                                 nb_aliases, t));
      }
      tok_str_free(using_str);
    }
    else if (td->is_class && tok >= TOK_UIDENT)
    {
      TemplateDef *class_td = find_class_template_def(tok);
      Sym *alias_sym;

      if (class_td && class_td->is_class)
      {
        int class_template_tok = tok;
        next();
        if (tok == TOK_LT || tok == '<')
        {
          TemplateArgList class_args;
          parse_template_type_args_subst(&class_args, td, args);
          tok_str_add(spec, TOK_STRUCT);
          tok_str_add(spec, instantiate_template_if_needed(class_td,
                                                           &class_args));
          continue;
        }
        tok_str_add(spec, class_template_tok);
        continue;
      }

      alias_sym = find_typedef_sym_in_stack(global_stack, tok);
      if (!alias_sym)
      {
        Sym *visible_sym = sym_find(tok);
        if (visible_sym && (visible_sym->type.t & VT_TYPEDEF))
          alias_sym = visible_sym;
      }
      if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
          && ((alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
      {
        int struct_tok = get_struct_type_name_tok(&alias_sym->type);
        if (struct_tok && template_typedef_is_type_pos(spec))
        {
          tok_str_add(spec, TOK_STRUCT);
          tok_str_add(spec, struct_tok);
        }
        else
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, tok);
      }
      else
        template_add_subst_token(spec, td, args, mangled_tok,
                                 alias_from, alias_to, nb_aliases, tok);
    }
    else
    {
      template_add_subst_token(spec, td, args, mangled_tok,
                               alias_from, alias_to, nb_aliases, tok);
    }
    next();
  }
  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;
  tok_str_add(spec, TOK_EOF);

  dynarray_add(&pending_template_specs, &nb_pending_template_specs, spec);
  if (template_class_needs_member_layout(td))
  {
    compile_pending_template_specs();
  }
  return mangled_tok;
}

static CType make_template_func_type(int type_tok, int typed_first_param)
{
  CType ret_type, func_type, param_type;
  Sym *fref, *param;

  if (type_tok == TOK_LONG)
    ret_type.t = (LONG_SIZE == 8 ? VT_LLONG : VT_INT) | VT_LONG;
  else if (type_tok == TOK_FLOAT)
    ret_type.t = VT_FLOAT;
  else if (type_tok == TOK_DOUBLE)
    ret_type.t = VT_DOUBLE;
  else
    ret_type.t = VT_INT;
  ret_type.ref = NULL;

  func_type.t = VT_FUNC;
  fref = sym_push(SYM_FIELD, &ret_type, 0, 0);
  fref->f.func_call = FUNC_CDECL;
  if (typed_first_param)
  {
    fref->f.func_type = FUNC_ELLIPSIS;
    param_type = ret_type;
    convert_parameter_type(&param_type);
    param = sym_push(SYM_FIELD, &param_type, VT_LOCAL | VT_LVAL, 0);
    fref->next = param;
  }
  else
    fref->f.func_type = FUNC_NEW;
  func_type.ref = fref;
  return func_type;
}

static CType make_template_func_type_from_return(CType *ret_type_in,
                                                  int typed_first_param)
{
  CType func_type, param_type;
  Sym *fref, *param;

  func_type.t = VT_FUNC;
  fref = sym_push(SYM_FIELD, ret_type_in, 0, 0);
  fref->f.func_call = FUNC_CDECL;
  if (typed_first_param)
  {
    fref->f.func_type = FUNC_ELLIPSIS;
    param_type = *ret_type_in;
    convert_parameter_type(&param_type);
    param = sym_push(SYM_FIELD, &param_type, VT_LOCAL | VT_LVAL, 0);
    fref->next = param;
  }
  else
    fref->f.func_type = FUNC_NEW;
  func_type.ref = fref;
  return func_type;
}

static CType make_template_func_type_with_return(CType *func_type_in,
                                                   CType *ret_type_in)
{
  CType func_type = *func_type_in;
  Sym *fref;

  if ((func_type.t & VT_BTYPE) != VT_FUNC || !func_type.ref)
    return make_template_func_type_from_return(ret_type_in, 0);
  fref = sym_push(SYM_FIELD, ret_type_in, 0, 0);
  fref->f = func_type.ref->f;
  fref->next = func_type.ref->next;
  func_type.ref = fref;
  return func_type;
}

static int infer_template_return_scalar_ctype(TemplateDef *td,
                                              TemplateArgList *args,
                                              CType *ret_type)
{
  int i;
  if (!td || !td->def_str || !args || !ret_type)
    return 0;
  for (i = 0; i + 7 < td->def_str->len; ++i)
    if (td->def_str->str[i] == TOK_RETURN)
    {
      int ci = i + 1;
      TemplateDef *class_td;
      TemplateArgList class_args;
      int param_index, class_tok, member_tok;
      Sym *member_sym;
      while (ci < td->def_str->len && td->def_str->str[ci] == TOK_LINENUM)
        ci += 2;
      if (ci + 6 >= td->def_str->len)
        continue;
      class_td = find_class_template_def(td->def_str->str[ci]);
      if (!class_td || td->def_str->str[ci + 1] != TOK_LT
          || td->def_str->str[ci + 3] != TOK_GT
          || td->def_str->str[ci + 4] != ':'
          || td->def_str->str[ci + 5] != ':')
        continue;
      param_index = template_param_index(td, td->def_str->str[ci + 2]);
      if (param_index < 0 || param_index >= args->nb)
        continue;
      template_arg_list_one(&class_args, args->toks[param_index]);
      class_tok = instantiate_template_if_needed(class_td, &class_args);
      compile_pending_template_specs_without_member_flush();
      member_tok = make_static_member_tok(class_tok, td->def_str->str[ci + 6]);
      member_sym = sym_find(member_tok);
      if (!member_sym)
        member_sym = sym_find2(global_stack, member_tok);
      if (member_sym && (member_sym->type.t & VT_BTYPE) == VT_FUNC
          && member_sym->type.ref
          && (member_sym->type.ref->type.t & VT_BTYPE) != VT_STRUCT)
      {
        *ret_type = member_sym->type.ref->type;
        return 1;
      }
    }
  return 0;
}

static int infer_template_return_struct_tok(TemplateDef *td,
                                            TemplateArgList *args)
{
  static int inference_depth;
  int i, name_index = -1;

  if (!td || !td->def_str || !args)
    return 0;
  for (i = 0; i < td->def_str->len; ++i)
    if (td->def_str->str[i] == td->name_tok)
    {
      name_index = i;
      break;
    }
  if (name_index <= 0)
    return 0;
  for (i = 0; i + 3 < name_index; ++i)
  {
    TemplateDef *class_td = find_class_template_def(td->def_str->str[i]);
    int param_idx, inst_tok;
    TemplateArgList class_args;

    if (!class_td || !class_td->is_class
        || (td->def_str->str[i + 1] != TOK_LT
            && td->def_str->str[i + 1] != '<'))
      continue;
    param_idx = template_param_index(td, td->def_str->str[i + 2]);
    if (param_idx < 0 || param_idx >= args->nb)
      continue;
    class_args.nb = 1;
    class_args.toks[0] = args->toks[param_idx];
    inst_tok = instantiate_template_if_needed(class_td, &class_args);
    compile_pending_template_specs_without_member_flush();
    return inst_tok;
  }
  /* An auto-return function template can spell its concrete result in the
     return expression rather than in the declarator, for example:

       template<class T> auto make(T x) { return Vec<T>(x); }

     Preserve that class result at call sites.  Restrict the body scan to the
     token immediately following return so parameter/local class spellings do
     not get mistaken for the result type. */
  for (i = name_index + 1; i + 5 < td->def_str->len; ++i)
  {
    TemplateDef *class_td;
    int param_idx, inst_tok;
    TemplateArgList class_args;

    if (td->def_str->str[i] != TOK_RETURN)
      continue;
    class_td = find_class_template_def(td->def_str->str[i + 1]);
    if (!class_td || !class_td->is_class
        || (td->def_str->str[i + 2] != TOK_LT
            && td->def_str->str[i + 2] != '<')
        || (td->def_str->str[i + 4] != TOK_GT
            && td->def_str->str[i + 4] != '>')
        || td->def_str->str[i + 5] != '(')
      continue;
    param_idx = template_param_index(td, td->def_str->str[i + 3]);
    if (param_idx < 0 || param_idx >= args->nb)
      continue;
    class_args.nb = 1;
    class_args.toks[0] = args->toks[param_idx];
    inst_tok = instantiate_template_if_needed(class_td, &class_args);
    compile_pending_template_specs_without_member_flush();
    return inst_tok;
  }
  /* Follow a simple auto-return template call chain.  The nested call's
     inferred template arguments are commonly the corresponding arguments of
     the enclosing template (factory/wrapper functions).  Its own return scan
     will only use parameters that exist in the nested definition. */
  if (inference_depth < 16)
    for (i = name_index + 1; i + 2 < td->def_str->len; ++i)
      if (td->def_str->str[i] == TOK_RETURN)
      {
        TemplateDef *return_td =
          find_function_template_def(td->def_str->str[i + 1]);
        int struct_tok;
        if (!return_td || td->def_str->str[i + 2] != '(')
          continue;
        ++inference_depth;
        struct_tok = infer_template_return_struct_tok(return_td, args);
        --inference_depth;
        if (struct_tok)
          return struct_tok;
      }
  return 0;
}

static int template_return_ctype_from_struct_tok(CType *ret_type, int struct_tok)
{
  Sym *s;

  if (!struct_tok)
    return 0;
  s = struct_find(struct_tok);
  if (!s)
    return 0;
  *ret_type = s->type;
  return 1;
}

static void compile_pending_template_specs(void)
{
  int i, saved_tok, saved_local_scope, saved_defer_pending_member_funcs;
  int saved_nb_pending_member_funcs;
  int saved_ind, saved_func_var, saved_func_vc;
  int saved_func_ind, saved_nocode_wanted;
  CValue saved_tokc;
  Sym *saved_local_stack, *saved_global_stack = NULL, *param_frame_tail = NULL;
  CType saved_func_vt;
  const char *saved_funcname;
  Section *saved_cur_text_section;

  saved_tok = tok;
  saved_tokc = tokc;
  saved_local_stack = local_stack;
  saved_local_scope = local_scope;
  saved_defer_pending_member_funcs = defer_pending_member_funcs;
  saved_nb_pending_member_funcs = nb_pending_member_funcs;
  saved_ind = ind;
  saved_func_vt = func_vt;
  saved_func_var = func_var;
  saved_func_vc = func_vc;
  saved_func_ind = func_ind;
  saved_funcname = funcname;
  saved_nocode_wanted = nocode_wanted;
  saved_cur_text_section = cur_text_section;
  if (!saved_local_stack && saved_local_scope && global_stack)
  {
    Sym *s;
    saved_global_stack = global_stack;
    for (s = global_stack; s; s = s->prev)
    {
      if (s->v == SYM_FIELD && s->sym_scope == 0)
      {
        param_frame_tail = s;
        global_stack = s->prev;
        break;
      }
    }
  }
  if (saved_local_stack || saved_local_scope)
  {
    local_stack = NULL;
    local_scope = 0;
    defer_pending_member_funcs = 1;
  }
  while (compiled_template_specs < nb_pending_template_specs)
  {
    i = compiled_template_specs++;
    TokenString *ts = pending_template_specs[i];
    {
      TokenString *normalized = balance_generated_template_spec(ts);
      if (normalized != ts)
      {
        tok_str_free(ts);
        pending_template_specs[i] = ts = normalized;
      }
      normalized = strip_static_call_synthetic_this(ts);
      if (normalized != ts)
      {
        tok_str_free(ts);
        pending_template_specs[i] = ts = normalized;
      }
    }
    defer_pending_member_funcs = 1;
    begin_macro(ts, 1);
    next();
    decl(VT_CONST);
    end_macro();
    /* end_macro() owns the replay buffer and frees it.  Clear the slot so
       later scans of pending_template_specs cannot dereference the dangling
       TokenString (it may already have been reused by a newer allocation). */
    pending_template_specs[i] = NULL;
    defer_pending_member_funcs = saved_defer_pending_member_funcs;
  }
  if (!suppress_template_member_flush
      && (saved_local_stack || saved_local_scope)
      && saved_nb_pending_member_funcs != nb_pending_member_funcs)
    compile_pending_member_funcs(saved_nb_pending_member_funcs);
  if (!suppress_template_member_flush
      && !saved_local_stack && !saved_local_scope
      && saved_nb_pending_member_funcs != nb_pending_member_funcs)
    compile_pending_lifecycle_member_funcs(saved_nb_pending_member_funcs);
  if (param_frame_tail)
  {
    param_frame_tail->prev = global_stack;
    global_stack = saved_global_stack;
  }
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
  ind = saved_ind;
  func_vt = saved_func_vt;
  func_var = saved_func_var;
  func_vc = saved_func_vc;
  func_ind = saved_func_ind;
  funcname = saved_funcname;
  nocode_wanted = saved_nocode_wanted;
  cur_text_section = saved_cur_text_section;
  tok = saved_tok;
  tokc = saved_tokc;
}

static void restore_cpp_lifecycle_probe(TokenString *replay)
{
  if (tok != TOK_EOF)
    tok_str_add2(replay, tok, &tokc);
  tok_str_add(replay, 0);
  begin_macro(replay, 1);
  next();
}

static void compile_pending_template_specs_without_member_flush(void)
{
  suppress_template_member_flush++;
  compile_pending_template_specs();
  suppress_template_member_flush--;
}

static int struct_has_member_init_list(int struct_tok)
{
  int i;
  for (i = 0; i < nb_member_init_list_struct_toks; ++i)
    if (member_init_list_struct_toks[i] == struct_tok)
      return 1;
  return 0;
}

static void infer_template_args_from_call(TemplateDef *td, CType *arg_types,
                                          int arg_count, TemplateArgList *args)
{
  int i, j, arg_index, type_tok;
  int pack_start = td->variadic_param_index >= 0
                   ? td->variadic_param_index
                   : -1;

  args->nb = td->nb_required_type_params;
  if (pack_start >= 0 && pack_start < arg_count
      && args->nb + (arg_count - pack_start) <= 16)
    args->nb += arg_count - pack_start;
  for (i = 0; i < args->nb; ++i)
    args->toks[i] = 0;
  if (td->def_str)
  {
    int name_index = -1, paren_index = -1, depth = 0, param_start = -1;

    for (i = 0; i < td->def_str->len; ++i)
    {
      if (td->def_str->str[i] == td->name_tok)
        name_index = i;
      if (name_index >= 0 && td->def_str->str[i] == '(')
      {
        paren_index = i;
        break;
      }
    }
    if (paren_index >= 0)
    {
      arg_index = 0;
      param_start = paren_index + 1;
      for (i = param_start; i < td->def_str->len && arg_index < arg_count; ++i)
      {
        int t = td->def_str->str[i];
        if (t == TOK_LT || t == '<' || t == '(' || t == '[')
          ++depth;
        else if ((t == TOK_GT || t == '>' || t == ')' || t == ']') && depth > 0)
          --depth;
        if ((t == ',' && depth == 0) || (t == ')' && depth == 0))
        {
          int end = i;
          type_tok = template_type_tok_from_ctype(&arg_types[arg_index]);
          for (j = param_start; j + 3 < end; ++j)
          {
            TemplateDef *class_td;
            int param_idx, inst_arg;

            class_td = find_class_template_def(td->def_str->str[j]);
            if (!class_td || !class_td->is_class
                || (td->def_str->str[j + 1] != TOK_LT
                    && td->def_str->str[j + 1] != '<'))
              continue;
            param_idx = template_param_index(td, td->def_str->str[j + 2]);
            if (param_idx < 0 || !type_tok)
              continue;
            inst_arg = template_inst_arg_for_name(class_td, type_tok, 0);
            if (inst_arg)
              args->toks[param_idx] = inst_arg;
          }
          if (t == ')')
            break;
          param_start = i + 1;
          arg_index++;
        }
      }
    }
  }
  /* Fill inferred pack element types from the remaining call arguments so
     variadic forwarding bodies (e.g. clConstruct's clForward(Args, args)...)
     receive a concrete type for each pack element. */
  if (pack_start >= 0)
    for (i = pack_start; i < arg_count && i < 16; ++i)
    {
      type_tok = template_type_tok_from_ctype(&arg_types[i]);
      if (type_tok && i < args->nb)
        args->toks[i] = type_tok;
    }
  for (i = 0; i < args->nb; ++i)
  {
    if (args->toks[i])
      continue;
    if (td->variadic_param_index == i)
      arg_index = td->func_min_args < arg_count ? td->func_min_args : arg_count - 1;
    else
      arg_index = i < arg_count ? i : arg_count - 1;
    if (arg_index < 0)
      cprime_error("unable to infer template type argument");
    type_tok = template_type_tok_from_ctype(&arg_types[arg_index]);
    if (!type_tok)
      cprime_error("unable to infer template type argument");
    args->toks[i] = type_tok;
  }
}

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
        && defaulted_member_method_toks[i] == method_tok)
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

static void queue_defaulted_assignment_body(CType *struct_type, int struct_tok,
                                            CType *func_type, int param_tok,
                                            int mangled_tok)
{
  Sym *member;
  PendingMemberFunc *pm;
  TokenString *str = tok_str_alloc();

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
    int name_tok;
    if ((member->type.t & VT_BTYPE) == VT_FUNC
        || (member->type.t & VT_STATIC))
      continue;
    name_tok = member->v & ~SYM_FIELD;
    if (!name_tok)
      continue;
    tok_str_add(str, tok_alloc_const("this"));
    tok_str_add(str, TOK_ARROW);
    tok_str_add(str, name_tok);
    tok_str_add(str, '=');
    tok_str_add(str, param_tok);
    tok_str_add(str, '.');
    tok_str_add(str, name_tok);
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

static TokenString *parse_constructor_member_initializers(CType *struct_type)
{
  TokenString *prefix, *args;
  int this_tok = tok_alloc_const("this");

  if (getenv("CPC_DBG_FIELD"))
    fprintf(stderr, "DBG member-init class=%s\n",
            get_struct_type_name_tok(struct_type) >= TOK_UIDENT
            ? get_tok_str(get_struct_type_name_tok(struct_type), NULL) : "?");
  if (tok != ':')
    return NULL;
  next();
  prefix = tok_str_alloc();

  for (;;)
  {
    int field_tok, dummy_ofs, skip_initializer_emit = 0;
    Sym *field;
    CType field_type;
    int field_struct_tok, level, saw_arg, arg_count;

    if (tok < TOK_UIDENT)
      cprime_error("member initializer name");
    field_tok = tok;
    field = find_field_try(struct_type, field_tok, &dummy_ofs);
    if (field)
      field_type = field->type;
    else
    {
      field_type.t = VT_VOID;
      field_type.ref = NULL;
      if (field_tok == get_struct_type_name_tok(struct_type)
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
    if (skip_initializer_emit)
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
      begin_macro(parse_args, 1);
      next();
      call_arg_count = count_saved_call_args(call_args, 32);
      infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
      /* end_macro() owns and frees parse_args. */
      end_macro();
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
      tok_str_add(prefix, ctor_func
                          ? ctor_func->v & ~SYM_FIELD
                          : make_member_func_tok(field_struct_tok,
                                                 TOK_CONSTRUCTOR1));
      tok_str_add(prefix, '(');
      tok_str_add(prefix, '&');
      tok_str_add(prefix, '(');
      tok_str_add(prefix, this_tok);
      tok_str_add(prefix, TOK_ARROW);
      tok_str_add(prefix, field_tok);
      tok_str_add(prefix, ')');
      if (saw_arg)
      {
        tok_str_add(prefix, ',');
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
  class_tok = tok;
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

  if (tok == '~')
  {
    next();
    if (tok != class_tok)
      cprime_error("destructor name must match class name");
    method_tok = TOK_DESTRUCTOR1;
    next();
  }
  else if (tok == class_tok)
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
    cprime_error("unsupported scoped member definition");

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
  if (tok == ';')
  {
    next();
    return 1;
  }
  if (method_tok == TOK_CONSTRUCTOR1)
    init_prefix = parse_constructor_member_initializers(&struct_type);
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
  add_pending_lifecycle_func(&struct_type, method_tok, params, body, 0);
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
  for (v = 0; v < params->len; ++v)
    tok_str_add(parse_params, params->str[v]);
  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(parse_params, 1);
  next();
  while (tok != TOK_EOF)
  {
    TokenString *default_arg = NULL;
    memset(&ad, 0, sizeof ad);
    if (!parse_btype(&param_type, &ad, 0))
      expect("parameter type");
    v = 0;
    type_decl(&param_type, &ad, &v, TYPE_DIRECT | TYPE_PARAM);
    if (tok == '=')
    {
      next();
      skip_or_save_param_default(&default_arg);
      expand_saved_single_object_macro(&default_arg);
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

static void skip_or_save_param_default(TokenString **str)
{
  int level = 0;

  if (str)
    *str = tok_str_alloc();
  while (tok != TOK_EOF)
  {
    if (level == 0 && tok == ',')
      break;
    if (str)
      tok_str_add_tok(*str);
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

static int parse_cpp_scoped_member_def_body(CType *ret_type, int class_tok)
{
  int method_tok, saved_tok, paren, i, mangled_tok, cv_qualifiers;
  int has_params = 0;
  int is_static_member_def = 0;
  CValue saved_tokc;
  Sym *class_sym, *decl_field, *canonical_sym;
  CType *effective_ret_type;
  CType class_type, lowered_type, static_func_type, canonical_type;
  int dummy_ofs;
  TokenString *params, *body = NULL, *str;

  if (tok == TOK_OPERATOR)
    method_tok = parse_cpp_operator_method_tok();
  else
  {
    if (tok < TOK_UIDENT)
      cprime_error("member function name");
    method_tok = tok;
    next();
  }
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
  static_func_type = make_func_type_from_saved_params(ret_type, params);
  static_func_type.t |= cv_qualifiers;
  if (cv_qualifiers & VT_CONSTANT)
    class_type.t |= VT_CONSTANT;
  canonical_sym = resolve_member_func_by_param_signature(&class_type, method_tok,
                                                        &static_func_type);
  if (!canonical_sym)
    canonical_sym = resolve_member_func_by_arg_count(&class_type, method_tok,
                                                     count_param_tokens(params));
  if (canonical_sym)
    mangled_tok = canonical_sym->v;
  if (!canonical_sym)
  {
    int static_base_tok = make_static_member_tok(class_tok, method_tok);
    mangled_tok = make_free_func_tok_for_type(static_base_tok, &static_func_type);
    canonical_sym = sym_find(mangled_tok);
    if (!canonical_sym)
      canonical_sym = sym_find2(global_stack, mangled_tok);
    if (canonical_sym)
      is_static_member_def = 1;
  }
  if (!canonical_sym)
    canonical_sym = sym_find2(global_stack, mangled_tok);

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
    tok_str_add(str, tok_alloc_const("this"));
  }
  if (has_params)
  {
    if (!is_static_member_def)
      tok_str_add(str, ',');
    for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
      tok_str_add(str, params->str[i]);
  }
  tok_str_add(str, ')');
  if (body)
  {
    for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
    {
      if (body->str[i] >= TOK_UIDENT
          && (i == 0 || (body->str[i - 1] != '.'
                         && body->str[i - 1] != TOK_ARROW
                         && body->str[i - 1] != ':')))
      {
        Sym *static_member =
          find_static_member_by_class_try(class_tok, body->str[i], NULL);
        if (static_member && static_member->r == VT_CONST
            && IS_ENUM_VAL(static_member->type.t))
        {
          tok_str_add(str, make_static_member_tok(class_tok, body->str[i]));
          continue;
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
      if (body->str[i] >= TOK_UIDENT && i + 1 < body->len
          && body->str[i + 1] == '('
          && (i == 0 || (body->str[i - 1] != '.'
                         && body->str[i - 1] != TOK_ARROW)))
      {
        int call_arg_count = count_call_arg_tokens_in_str(body, i + 1);
        if (call_arg_count >= 0
            && resolve_member_func_by_arg_count(&class_type, body->str[i],
                                                call_arg_count))
        {
          tok_str_add(str, tok_alloc_const("this"));
          tok_str_add(str, TOK_ARROW);
        }
        else if (call_arg_count >= 0
                 && class_has_static_member_func(class_tok, body->str[i]))
        {
          /* Unqualified static member calls in an out-of-class body must be
             qualified as Class::Member so they resolve to the static member
             instead of an implicit global function. */
          tok_str_add(str, class_tok);
          tok_str_add(str, ':');
          tok_str_add(str, ':');
        }
      }
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
    canonical_sym = sym_find2(global_stack, mangled_tok);
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
  class_tok = tok;
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
        decl_sym = sym_find2(global_stack, member_tok);
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
  next();
  if (tok != ':')
    cprime_error("':' expected");
  next();
  return parse_cpp_scoped_member_def_body(ret_type, class_tok);
}

static int try_rewrite_cpp_scoped_static_data_after_declarator(CType *type,
                                                              int *pv)
{
  int class_tok, member_tok;
  Sym *class_sym, *decl_sym;
  TokenString *replay;

  class_tok = *pv;
  if (tok != ':' || class_tok < TOK_UIDENT)
    return 0;
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
    decl_sym = sym_find2(global_stack, member_tok);
  if (!decl_sym)
    cprime_error("static data member definition requires class member declaration '%s'",
              get_tok_str(member_tok, NULL));
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
      expect("struct or union");
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
    if (getenv("CPC_DBG_FIELD"))
      fprintf(stderr, "DBG field not found: %s in class %s\n",
              get_tok_str(v, NULL),
              type->ref ? get_tok_str(type->ref->v & ~SYM_STRUCT, NULL) : "?");
    cprime_error("field not found: %s", get_tok_str(v, NULL));
  }
  return s;
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
    s = sym_find2(global_stack, static_tok);
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
  FreeFuncOverload *o;
  TemplateDef *td;
  int i;
  int static_tok = make_static_member_tok(class_tok, member_tok);

  /* Static member functions are registered as free-function overloads by
     declare_static_member_func; instance members are not.  Symbol lookup is
     not usable here because instance members share the same plain mangled
     token (Class_Member). */
  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == static_tok)
      return 1;
  /* Static member templates declared inline in the class body (e.g.
     template<typename U> static M<U> Make(...)) are captured in
     template_member_defs but never reach declare_static_member_func, so
     also recognize them through the template class's member table. */
  td = find_class_template_def_for_class_tok(class_tok);
  if (td)
    for (i = 0; i < nb_template_member_defs; ++i)
    {
      TemplateMemberDef *md = template_member_defs[i];
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
    else if ((s->type.t & VT_BTYPE) == VT_STRUCT)
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
    if (nb_namespace_stack)
    {
      if (tok != '{' && tok != ';')
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

  if (is_class_tag && tok == ':')
  {
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
      note_class_base(s->v & ~SYM_STRUCT, base_tok);
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
      cprime_error("struct/union/enum already defined");
    s->c = -2;
    if (u == VT_STRUCT && nb_defining_class_stack
        < (int)(sizeof(defining_class_stack) / sizeof(defining_class_stack[0])))
      defining_class_stack[nb_defining_class_stack++] = s->v & ~SYM_STRUCT;
    // Cannot Be Empty
    // Non Empty Enums Are Not Allowed
    ps = &s->next;
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
        TokenString *body = NULL, *lifecycle_params = NULL;
        TokenString *lifecycle_init_prefix = NULL;

        while (tok == TOK_LINENUM)
          next();

        if (is_cpp_translation_unit() && tok >= TOK_UIDENT
            && !strcmp(get_tok_str(tok, NULL), "virtual"))
          next();

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
          if (member_str && td && td->nb_type_params > 0)
            add_template_member_def(td->name_tok, td->type_param_toks[0],
                                    member_str);
          else if (member_str)
            tok_str_free(member_str);
          continue;
        }

        if (tok == tok_explicit)
        {
          lifecycle_explicit = 1;
          next();
          if (tok != get_struct_type_name_tok(type)
              && (tok < TOK_UIDENT
                  || make_current_namespace_tok(tok)
                     != get_struct_type_name_tok(type))
              && tok != '~')
            cprime_error("explicit is only supported on constructors");
        }

        if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "friend"))
        {
          skip_template_member_decl();
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

        if (tok == get_struct_type_name_tok(type)
            || (tok >= TOK_UIDENT
                && tok != get_struct_type_name_tok(type)
                && make_current_namespace_tok(tok)
                   == get_struct_type_name_tok(type))
            || tok == '~')
        {
          int class_tok = get_struct_type_name_tok(type);
          if (tok == '~')
          {
            if (lifecycle_explicit)
              cprime_error("explicit is only supported on constructors");
            next();
            if (tok != class_tok
                && (tok < TOK_UIDENT
                    || make_current_namespace_tok(tok) != class_tok))
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
              tok_str_add2(lifecycle_params, tok, &tokc);
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
              lifecycle_init_prefix = parse_constructor_member_initializers(type);
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
                                         body, 1);
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
              add_pending_lifecycle_func(type, lifecycle_tok, lifecycle_params, body, 1);
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
            if (tok == get_struct_type_name_tok(type))
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

              if (!class_tok)
                cprime_error("static data members require a named class");
              if (tok == '=')
              {
                next();
                skip_initializer_expression();
              }
              static_tok = make_static_member_tok(class_tok, v);
              static_type.t = (static_type.t & ~VT_STATIC) | VT_EXTERN;
              if (!(static_type.t & VT_ARRAY))
                r |= VT_LVAL;
              external_sym(static_tok, &static_type, r, &ad1);
              if (tok == ';' || tok == TOK_EOF)
                break;
              skip(',');
              continue;
            }
            if (tok == '=')
            {
              next();
              skip_initializer_expression();
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
            *ps = ss;
            ps = &ss->next;
          }
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
      check_fields(type, 1);
      check_fields(type, 0);
      struct_layout(type, &ad);
      emit_defaulted_member_bodies(type);
      if (saved_nb_pending_member_funcs != nb_pending_member_funcs
          && !defer_pending_member_funcs)
        compile_pending_member_funcs(saved_nb_pending_member_funcs);
    }
    if (u == VT_STRUCT && nb_defining_class_stack > 0
        && defining_class_stack[nb_defining_class_stack - 1]
           == (s->v & ~SYM_STRUCT))
      --nb_defining_class_stack;
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
              s = sym_find2(global_stack, qtok);
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
          n = tok;
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
            unget_tok(n);
            goto the_end;
          }
          if (tok == ':')
          {
            int class_tok = n;
            TokenString *replay = tok_str_alloc();
            tok_str_add(replay, n);
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
        Sym *global_typedef = sym_find2(global_stack, n);
        if ((!global_typedef || !(global_typedef->type.t & VT_TYPEDEF))
            && n != tok)
          global_typedef = sym_find2(global_stack, tok);
        if (global_typedef && (global_typedef->type.t & VT_TYPEDEF))
          s = global_typedef;
      }
      if (!s || !(s->type.t & VT_TYPEDEF))
        goto the_end;

      n = tok, next();
      if (tok == ':' && ignore_label)
      {
        // ignore if it's a label
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
            skip_or_save_block(&default_arg);
            expand_saved_single_object_macro(&default_arg);
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
        gexpr();
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
        && ((type->t & VT_BTYPE) == VT_STRUCT
            || last_btype_was_typedef))
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
    type = arg->type;
    decay_reference_type(&type);
    if (is_reference_type(&vtop->type))
    {
      decay_reference_type(&vtop->type);
      type.t &= ~VT_CONSTANT;
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
      vstore();
      vpop();
      vset(&storage_type, VT_LOCAL | VT_LVAL, addr);
      vtop->r2 = r2;
    }
    else if (!(vtop->type.t & VT_ARRAY))
      test_lvalue();
    type.t &= ~VT_CONSTANT;
    mk_pointer(&vtop->type);
    gaddrof();
    gen_assign_cast(&type);
  }
  else
  {
    type = arg->type;
    type.t &= ~VT_CONSTANT; // need to do that to avoid false warning
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
  case TOK_LONG:
    type->t = VT_LLONG;
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
        s = sym_find2(global_stack, name_tok);
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
    vstore();
  }
}

ST_FUNC void unary(void)
{
  int n, t, align, size, r;
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
  switch (tok)
  {
  case TOK_EXTENSION:
    next();
    goto tok_next;
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
      gen_cast(&cast_type);
      break;
    }
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
      TokenString *replay = tok_str_alloc();

      if (!type_sym)
      {
        Sym *alias_sym = sym_find(first_tok);
        if (!alias_sym)
          alias_sym = sym_find2(global_stack, first_tok);
        if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
            && ((alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
          type_sym = alias_sym->type.ref;
      }
      tok_str_add2(replay, first_tok, &first_tokc);
      next();
      if (type_sym && tok == '(')
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
    indir();
    break;
  case '&':
    next();
    unary();
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
      CType template_call_type;
      if (tok < TOK_UIDENT)
        cprime_error("expression expected before '%s'", get_tok_str(tok, &tokc));
      t = tok;
      next();
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
        TemplateDef *td = find_class_template_def(find_current_namespace_tok(t));
        if (td && td->is_class && (tok == TOK_LT || tok == '<'))
        {
          TemplateArgList args;
          parse_template_type_args(&args);
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
              /* Not a type trait: rewind the scoped name so the general
                 Class<T>::member path below parses the static member. */
              unget_tok(':');
              unget_tok(':');
            }
          }
          t = instantiate_template_if_needed(td, &args);
          compile_pending_template_specs_without_member_flush();
        }
      }
      if (tok == ':')
      {
        int class_tok = t, parts[16], nb_parts = 0;
        Sym *class_alias_sym;
        class_alias_sym = sym_find(t);
        if (!class_alias_sym)
          class_alias_sym = sym_find2(global_stack, t);
        if (class_alias_sym
            && (class_alias_sym->type.t & VT_TYPEDEF)
            && ((class_alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
        {
          int alias_struct_tok = get_struct_type_name_tok(&class_alias_sym->type);
          if (alias_struct_tok)
            class_tok = alias_struct_tok;
        }
        if (struct_find(class_tok))
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
              t = make_static_member_tok(class_tok, static_member_tok);
              instantiate_static_template_member_for_call(class_tok,
                                                          static_member_tok);
              next();
            }
          }
        }
        else
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
        if (td && td->is_class && (tok == TOK_LT || tok == '<'))
        {
          TemplateArgList args;
          parse_template_type_args(&args);
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
          }
          t = instantiate_template_if_needed(td, &args);
          compile_pending_template_specs_without_member_flush();
        }
      }
      if (tok == ':' && struct_find(t))
      {
        int class_tok = t;
        Sym *class_alias_sym;
        class_alias_sym = sym_find(t);
        if (!class_alias_sym)
          class_alias_sym = sym_find2(global_stack, t);
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
        int brace = 1;
        Sym *class_sym = struct_find(t);
        type.t = class_sym->type.t;
        type.ref = class_sym;
        if (type_is_std_initializer_list(&type))
        {
          AttributeDef ad;

          memset(&ad, 0, sizeof ad);
          decl_initializer_alloc(&type, &ad, VT_LOCAL | VT_LVAL, 1, 0, NULL,
                                 0, VT_LOCAL);
          break;
        }
        next();
        while (tok != TOK_EOF && brace > 0)
        {
          if (tok == '{')
            ++brace;
          else if (tok == '}')
            --brace;
          next();
        }
        vpush(&type);
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
          explicit_call_has_typed_arg = td->func_min_args > 0;
          if (template_return_ctype_from_struct_tok(&inferred_return_type,
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
          cprime_error("static assertion failed");
        vpushi(0);
        break;
      }
      /* A class name can also have function-template metadata recorded under
         the same token for its templated constructors.  In an expression such
         as Class(arg), the class functional construction takes precedence. */
      if (tok == '(' && struct_find(t)
          && try_parse_cpp_functional_constructor(t))
        break;
      {
        TemplateDef *td = find_function_template_def(t);
        if (td && tok == '(')
        {
          int type_tok = 0, inferred_call;
          CType inferred_return_type;
          int has_inferred_return_type = 0;
          next();
          inferred_call = 0;
          if (tok == TOK_CHAR || tok == TOK_INT || tok == TOK_LONG
              || tok == TOK_FLOAT || tok == TOK_DOUBLE)
          {
            type_tok = tok;
            next();
            skip(')');
            if (tok != '(')
              cprime_error("explicit template instantiation must be followed by call");
          }
          else
          {
            TokenString *call_args[32];
            CType call_arg_types[32];
            int call_arg_count;
            TemplateArgList inferred_args;
            unget_tok('(');
            call_arg_count = probe_template_call_args(call_args, call_arg_types, 32);
            td = find_function_template_for_call(t, call_arg_types,
                                                 call_arg_count);
            if (!td)
              cprime_error("no matching function template '%s'",
                           get_tok_str(t, NULL));
            infer_template_args_from_call(td, call_arg_types, call_arg_count,
                                          &inferred_args);
            type_tok = inferred_args.toks[0];
            if (template_return_ctype_from_struct_tok(&inferred_return_type,
                  infer_template_return_struct_tok(td, &inferred_args)))
              has_inferred_return_type = 1;
            t = instantiate_template_if_needed(td, &inferred_args);
            compile_pending_template_specs_without_member_flush();
            inferred_call = 1;
          }
          if (tok != '(')
            cprime_error("template instantiation must be followed by call");
          if (!inferred_call)
          {
            TemplateArgList args;
            template_arg_list_one(&args, type_tok);
            if (template_return_ctype_from_struct_tok(&inferred_return_type,
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
      if (!n)
      {
        int lookup_tok = t;
        s = find_namespace_or_plain_symbol(&t);
        if (!s && nb_defining_class_stack > 0)
        {
          int class_tok = defining_class_stack[nb_defining_class_stack - 1];
          int static_tok = make_static_member_tok(class_tok, lookup_tok);
          s = sym_find(static_tok);
          if (!s)
            s = sym_find2(global_stack, static_tok);
          if (s)
            t = static_tok;
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
        CType cast_type;
        cast_type.t = (s->type.t & ~VT_TYPEDEF);
        cast_type.ref = s->type.ref;

        next();
        if (tok == ')')
        {
          next();
          vpushi(0);
        }
        else
        {
          expr_eq();
          skip(')');
        }

        gen_cast(&cast_type);
        break;
      }
      if (try_parse_cpp_functional_constructor(t))
        break;
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
        if (tok == '(')
        {
          Sym *this_sym = sym_find(tok_alloc_const("this"));
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
          Sym *this_sym = sym_find(tok_alloc_const("this"));
          if (this_sym && ((this_sym->type.t & VT_BTYPE) == VT_PTR))
          {
            CType *this_target = pointed_type(&this_sym->type);
            if ((this_target->t & VT_BTYPE) == VT_STRUCT)
            {
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
          cprime_error("'%s' undeclared", name);
        }
        /* for simple function calls, we tolerate undeclared
           external reference to int() function */
        cprime_warning_c(warn_implicit_function_declaration)(
          "implicit declaration of function '%s'", name);
        s = external_global_sym(t, &func_old_type);
      }

      r = s->r;
      /* A symbol that has a register is a local register variable,
         which starts out as VT_LOCAL value.  */
      if ((r & VT_VALMASK) < VT_CONST
          && (r & VT_VALMASK) != VT_LLOCAL)
        r = (r & ~VT_VALMASK) | VT_LOCAL;

      vset(&s->type, r, s->c);
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
      Sym *field, *func_sym, *func_type, *sa;
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
      v = tok;
      if (v < TOK_UIDENT)
        cprime_error("field name expected after member access (got '%s')",
                     get_tok_str(tok, &tokc));
      next();
      field = NULL;
      if ((vtop->type.t & VT_BTYPE) == VT_STRUCT)
        field = find_field_try(&vtop->type, v, &cumofs);
      if (tok == '(' && (vtop->type.t & VT_BTYPE) == VT_STRUCT
          && (!field || ((field->type.t & VT_BTYPE) == VT_FUNC)))
      {
        const char *name = get_tok_str(v, NULL);
        TokenString *call_args[32];
        CType call_arg_types[32];
        int call_arg_count, ai;

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
        func_sym = resolve_member_func_by_arg_types(&vtop->type, v,
                                                    call_arg_types,
                                                    call_arg_count);
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

        ft = func_sym->type;
        if ((ft.t & VT_BTYPE) == VT_PTR)
          ft = *pointed_type(&ft);
        if ((ft.t & VT_BTYPE) != VT_FUNC)
          expect("function pointer");
        func_type = ft.ref;
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
          begin_macro(call_args[ai], 1);
          next();
          expr_eq();
          end_macro();
          gfunc_param_typed(func_type, sa);
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
              vstore();
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
        if (static_member && static_member->r == VT_CONST
            && IS_ENUM_VAL(static_member->type.t))
        {
          CValue cval;
          vpop();
          cval.i = static_member->enum_val;
          vsetc(&static_member->type, VT_CONST, &cval);
          vtop->sym = static_member;
          maybe_indir_reference();
          continue;
        }
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

      // Function Call
      saved_call_arg_count = -1;
      overload_name_tok = 0;
      if (vtop->sym && has_free_func_overload(vtop->sym->v))
      {
        Sym *func_sym;

        overload_name_tok = vtop->sym->v;
        next();
        saved_call_arg_count = count_saved_call_args(call_args, 32);
        infer_saved_arg_types(call_args, call_arg_types, saved_call_arg_count);
        func_sym = resolve_free_func_by_arg_types(overload_name_tok,
                                                  call_arg_types,
                                                  saved_call_arg_count);
        if (!func_sym)
          func_sym = resolve_free_func_by_arg_count(overload_name_tok,
                                                    saved_call_arg_count);
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
        if (overload_type)
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
          expect("function pointer");
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
          begin_macro(call_args[ai], 1);
          next();
          expr_eq();
          end_macro();
          gfunc_param_typed(s, sa);
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
            vstore();
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

static void expr_eq(void)
{
  int t;

  expr_cond();
  if ((t = tok) == '=' || TOK_ASSIGN(t))
  {
    test_lvalue();
    next();
    if (t == '=')
    {
      expr_eq();
      if (try_call_cpp_assignment_operator())
        return;
    }
    else
    {
      vdup();
      expr_eq();
      if (try_call_cpp_compound_assign_operator(t))
      {
        vswap();
        vpop();
        return;
      }
      gen_op(TOK_ASSIGN_OP(t));
    }
    vstore();
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

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref || !type->ref->a.lifecycle_ctor)
    return NULL;

  arg_type = *source_type;
  return resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1, &arg_type, 1);
}

static int try_gfunc_return_copy_construct(CType *func_type)
{
  SValue source;
  CType ptr_type;
  Sym *ctor_func, *ctor_type, *sa;
  int ret_align, ret_nregs, regsize, size, align, addr, r2;

  if ((func_type->t & VT_BTYPE) != VT_STRUCT)
    return 0;

  ctor_func = resolve_copy_constructor_func(func_type, &vtop->type);
  if (!ctor_func)
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

  save_lvalues();

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
    call_lifecycle_constructor_members(func_type, VT_LOCAL | VT_LVAL, addr);
    vset(func_type, VT_LOCAL | VT_LVAL, addr);
    vtop->r2 = r2;
    mk_pointer(&vtop->type);
    gaddrof();
  }

  if (ctor_func)
  {
    vpushsym(&ctor_func->type, ctor_func);
    vswap();
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
    vpushv(&source);
    mk_pointer(&vtop->type);
    gaddrof();
    src_ptr = *vtop;
    vtop--;
    copy_construct_struct_memberwise_from_base_ptr(func_type, &dst_ptr, &src_ptr,
                                                   0);
  }

  if (ret_nregs != 0)
  {
    vset(func_type, VT_LOCAL | VT_LVAL, addr);
    vtop->r2 = r2;
    gfunc_return(func_type);
  }

  return 1;
}

static void gfunc_return(CType *func_type)
{
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
      vstore();
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
        vset(&type, VT_LOCAL | VT_LVAL, addr);
        vswap();
        vstore();
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
    if (sv->sym && (sv->r & VT_LVAL))
    {
      int align, size = type_size(&sv->type, &align);
      int r2, l = get_temp_local_var(size, align, &r2);
      vset(&sv->type, VT_LOCAL | VT_LVAL, l), vtop->r2 = r2;
      vpushv(sv), *sv = vtop[-1], vstore(), --vtop;
    }
    --sv;
  }
}

static void try_call_scope_cleanup(Sym *stop)
{
  Sym *cls = cur_scope->cl.s;
  for (; cls != stop; cls = cls->next)
  {
    Sym *fs = cls->cleanup_func;
    Sym *vs = cls->cleanup_sym;
    if (!fs || !vs)
      continue;
    if (vs && (vs->type.t & VT_BTYPE) == VT_STRUCT)
    {
      int struct_tok = get_struct_type_name_tok(&vs->type);
      if (struct_tok
          && class_or_inst_has_member_template_name(struct_tok,
                                                    TOK_DESTRUCTOR1))
        instantiate_template_member_for_call(&vs->type, TOK_DESTRUCTOR1,
                                             NULL, 0);
    }
    save_lvalues();
    vpushsym(&fs->type, fs);
    vset(&vs->type, vs->r, vs->c);
    vtop->sym = vs;
    mk_pointer(&vtop->type);
    gaddrof();
    gfunc_call(1);
    drop_leaked_call_target(fs);
    if (vtop >= vstack && (vtop->type.t & VT_BTYPE) == VT_VOID)
      vpop();
  }
}


static void try_call_cleanup_goto(Sym *cleanupstate)
{
  Sym *oc, *cc;
  int ocd, ccd;

  if (!cur_scope->cl.s)
    return;

  // search NCA of both cleanup chains given parents and initial depth
  ocd = cleanupstate ? cleanupstate->v & ~SYM_FIELD : 0;
  for (ccd = cur_scope->cl.n, oc = cleanupstate; ocd > ccd; --ocd, oc = oc->next)
    ;
  for (cc = cur_scope->cl.s; ccd > ocd; --ccd, cc = cc->next)
    ;
  for (; cc != oc; cc = cc->next, oc = oc->next, --ccd)
    ;

  try_call_scope_cleanup(cc);
}

// Call 'Func' For Each __Attribute__((Cleanup(Func)))
static void block_cleanup(struct scope *o)
{
  int jmp = 0;
  Sym *g, **pg;
  for (pg = &pending_gotos; (g = *pg) && g->c > o->cl.n;)
  {
    if (g->cleanup_label->r & LABEL_FORWARD)
    {
      Sym *pcl = g->next;
      if (!jmp)
        jmp = gjmp(0);
      gsym(pcl->jnext);
      try_call_scope_cleanup(o->cl.s);
      pcl->jnext = gjmp(0);
      if (!o->cl.n)
        goto remove_pending;
      g->c = o->cl.n;
      pg = &g->prev;
    }
    else
    {
remove_pending:
      *pg = g->prev;
      sym_free(g);
    }
  }
  gsym(jmp);
  try_call_scope_cleanup(o->cl.s);
}

// -------------------------------------------------------------------------
// VLA

static void vla_restore(int loc)
{
  if (loc)
    gen_vla_sp_restore(loc);
}

static void vla_leave(struct scope *o)
{
  struct scope *c = cur_scope, *v = NULL;
  for (; c != o && c; c = c->prev)
    if (c->vla.num)
      v = c;
  if (v)
    vla_restore(v->vla.locorig);
}

// -------------------------------------------------------------------------
// Local Scopes

static void new_scope(struct scope *o)
{
  // Copy And Link Previous Scope
  *o = *cur_scope;
  o->prev = cur_scope;
  cur_scope = o;
  cur_scope->vla.num = 0;

  // Record Local Declaration Stack Position
  o->lstk = local_stack;
  o->llstk = local_label_stack;
  ++local_scope;
}

static void prev_scope(struct scope *o, int is_expr)
{
  vla_leave(o->prev);

  if (o->cl.s != o->prev->cl.s)
    block_cleanup(o->prev);

  if (debug_modes)
    cprime_debug_end_scope(o->lstk, !is_expr);

  // Pop Locally Defined Labels
  label_pop(&local_label_stack, o->llstk, is_expr);

  /* In the is_expr case (a statement expression is finished here),
     vtop might refer to symbols on the local_stack.  Either via the
     type or via vtop->sym.  We can't pop those nor any that in turn
     might be referred to.  To make it easier we don't roll back
     any symbols in that case; some upper level call to block() will
     do that.  We do have to remove such symbols from the lookup
     tables, though.  sym_pop will do that.  */

  // Pop Locally Defined Symbols
  sym_pop(&local_stack, o->lstk, is_expr);
  cur_scope = o->prev;
  --local_scope;
}

// Leave A Scope Via Break/Continue(/Goto)
static void leave_scope(struct scope *o)
{
  if (!o)
    return;
  try_call_scope_cleanup(o->cl.s);
  vla_leave(o);
}

/* short versiona for scopes with 'if/do/while/switch' which can
   declare only types (of struct/union/enum) */
static void new_scope_s(struct scope *o)
{
  o->lstk = local_stack;
  ++local_scope;
}

static void prev_scope_s(struct scope *o)
{
  sym_pop(&local_stack, o->lstk, 0);
  --local_scope;
}

// -------------------------------------------------------------------------
// call block from 'for do while' loops

static void lblock(int *bsym, int *csym)
{
  struct scope *lo = loop_scope, *co = cur_scope;
  int *b = co->bsym, *c = co->csym;
  if (csym)
  {
    co->csym = csym;
    loop_scope = co;
  }
  co->bsym = bsym;
  block(0);
  co->bsym = b;
  if (csym)
  {
    co->csym = c;
    loop_scope = lo;
  }
}

static void append_range_for_body(TokenString *dst, TokenString *body,
                                  int var_tok, int ptr_tok)
{
  const int *p = body->str;
  const int *end = body->str + body->len;
  CValue cv;
  int t;

  while (p < end)
  {
    TOK_GET(&t, &p, &cv);
    if (t == TOK_EOF || t == 0)
      break;
    if ((t & ~SYM_FIELD) == (var_tok & ~SYM_FIELD))
    {
      tok_str_add(dst, '(');
      tok_str_add(dst, '*');
      tok_str_add(dst, ptr_tok);
      tok_str_add(dst, ')');
    }
    else
    {
      tok_str_add2(dst, t, &cv);
    }
  }
}

static int try_parse_range_for(void)
{
  char tmp_name[64];
  int var_tok, range_tok, index_tok, ptr_tok, body_first_tok;
  int is_array_range, array_count;
  int has_range_elem_type = 0;
  Sym *range_sym;
  CType range_elem_type;
  TokenString *body, *lowered, *var_decl;

  var_decl = tok_str_alloc();
  while (tok == TOK_LINENUM)
    next();
  if (tok == TOK_AUTO)
  {
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    if (tok == '&' || tok == TOK_LAND)
    {
      tok_str_add(var_decl, '&');
      next();
      while (tok == TOK_LINENUM)
        next();
    }
    if (tok < TOK_UIDENT)
      cprime_error("range-for variable");
    var_tok = tok;
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
  }
  else if (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
  {
    CType range_type;
    AttributeDef range_ad;

    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    memset(&range_type, 0, sizeof range_type);
    memset(&range_ad, 0, sizeof range_ad);
    if (!parse_btype(&range_type, &range_ad, 0))
      cprime_error("range-for variable type");
    if (!add_ctype_tokens(var_decl, &range_type))
      cprime_error("range-for variable type");
    while (tok == TOK_LINENUM)
      next();
    if (tok != '&')
      cprime_error("range-for reference");
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    if (tok < TOK_UIDENT)
      cprime_error("range-for variable");
    var_tok = tok;
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
  }
  else if (tok >= TOK_UIDENT && struct_find(tok))
  {
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    if (tok != '&')
      cprime_error("range-for reference");
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    if (tok < TOK_UIDENT)
      cprime_error("range-for variable");
    var_tok = tok;
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
  }
  else
  {
    tok_str_free(var_decl);
    return 0;
  }
  skip(':');
  while (tok == TOK_LINENUM)
    next();
  if (tok < TOK_UIDENT)
    cprime_error("range-for range");
  range_tok = tok;
  range_sym = sym_find(range_tok);
  is_array_range = range_sym && (range_sym->type.t & VT_ARRAY);
  array_count = is_array_range ? range_sym->type.ref->c : 0;
  if (!is_array_range && range_sym
      && (range_sym->type.t & VT_BTYPE) == VT_STRUCT)
  {
    CType index_type = int_type;
    Sym *index_func =
      resolve_member_func_by_arg_types(&range_sym->type,
                                       tok_alloc_const("operator[]"),
                                       &index_type, 1);
    if (index_func && (index_func->type.t & VT_BTYPE) == VT_FUNC
        && index_func->type.ref)
    {
      range_elem_type = index_func->type.ref->type;
      if (is_reference_type(&range_elem_type))
      {
        decay_reference_type(&range_elem_type);
        range_elem_type = *pointed_type(&range_elem_type);
      }
      has_range_elem_type = 1;
    }
  }
  next();
  skip(')');

  body_first_tok = tok;
  skip_or_save_block(&body);
  if (body_first_tok != '{')
    skip(';');

  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_i_%d",
           range_for_temp_counter++);
  index_tok = tok_alloc_const(tmp_name);
  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_p_%d",
           range_for_temp_counter++);
  ptr_tok = tok_alloc_const(tmp_name);

  lowered = tok_str_alloc();
  tok_str_add(lowered, '{');
  tok_str_add(lowered, TOK_INT);
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, ';');
  tok_str_add(lowered, TOK_FOR);
  tok_str_add(lowered, '(');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, '=');
  tok_str_add_cint(lowered, 0);
  tok_str_add(lowered, ';');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, TOK_NE);
  if (is_array_range)
    tok_str_add_cint(lowered, array_count);
  else
  {
    tok_str_add(lowered, range_tok);
    tok_str_add(lowered, '.');
    tok_str_add(lowered, tok_alloc_const("Size"));
    tok_str_add(lowered, '(');
    tok_str_add(lowered, ')');
  }
  tok_str_add(lowered, ';');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, '=');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, '+');
  tok_str_add_cint(lowered, 1);
  tok_str_add(lowered, ')');
  tok_str_add(lowered, '{');
  if (is_array_range)
  {
    tok_str_append_without_eof(lowered, var_decl);
    tok_str_add(lowered, '=');
    tok_str_add(lowered, range_tok);
    tok_str_add(lowered, '[');
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, ']');
    tok_str_add(lowered, ';');
    tok_str_append_without_eof(lowered, body);
  }
  else
  {
    if (has_range_elem_type)
    {
      if (!add_ctype_tokens(lowered, &range_elem_type))
        tok_str_add(lowered, TOK_AUTO);
    }
    else
      tok_str_add(lowered, TOK_AUTO);
    tok_str_add(lowered, '*');
    tok_str_add(lowered, ptr_tok);
    tok_str_add(lowered, '=');
    tok_str_add(lowered, '&');
    tok_str_add(lowered, range_tok);
    tok_str_add(lowered, '[');
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, ']');
    tok_str_add(lowered, ';');
    append_range_for_body(lowered, body, var_tok, ptr_tok);
  }
  if (body_first_tok != '{')
    tok_str_add(lowered, ';');
  tok_str_add(lowered, '}');
  tok_str_add(lowered, '}');
  tok_str_add2(lowered, tok, &tokc);
  tok_str_add(lowered, 0);

  begin_macro(lowered, 1);
  next();
  block(STMT_COMPOUND);
  if (tok == 0)
    next();
  return 1;
}

// c2y if/switch declaration
static void gexpr_decl(void)
{
  if (tok >= TOK_UIDENT)
  {
    int first_tok = tok;
    CValue first_tokc = tokc;
    Sym *type_sym = struct_find(first_tok);
    TokenString *replay = tok_str_alloc();

    if (!type_sym)
    {
      Sym *alias_sym = sym_find(first_tok);
      if (!alias_sym)
        alias_sym = sym_find2(global_stack, first_tok);
      if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
          && ((alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
        type_sym = alias_sym->type.ref;
    }
    tok_str_add2(replay, first_tok, &first_tokc);
    next();
    if (type_sym && tok == '(')
    {
      restore_cpp_lifecycle_probe(replay);
      gexpr();
      return;
    }
    restore_cpp_lifecycle_probe(replay);
  }
  if ((tok >= TOK_UIDENT && is_namespace_tok(tok))
      || (tok >= TOK_UIDENT
          && find_class_template_def(find_current_namespace_tok(tok))))
  {
    gexpr();
    return;
  }
  int v = decl(VT_JMP);
  if (v > 1 && tok != ';')
  {
    Sym *s = sym_find(v);
    vset(&s->type, s->r, (s->r &VT_SYM) ? 0 : s->c);
    vtop->sym = s;
  }
  else
  {
    if (v)
      skip(';');
    gexpr();
  }
}

static void block(int flags)
{
  int a, b, c, d, e, t;
  struct scope o;
  Sym *s;
  CType ref_ret_type;

again:
  t = tok;
  /* If the token carries a value, next() might destroy it. Only with
     invalid code such as f(){"123"4;} */
  if (TOK_HAS_VALUE(t))
    goto expr;
  next();

  if (debug_modes)
    cprime_tcov_check_line (cprime_state, 0), cprime_tcov_block_begin (cprime_state);

  if (t >= TOK_UIDENT && !strcmp(get_tok_str(t, NULL), "using"))
  {
    int alias_tok, dummy_v = 0;
    CType alias_type;
    AttributeDef alias_ad;
    Sym *alias_sym;

    if (tok < TOK_UIDENT)
      expect("using alias name");
    alias_tok = tok;
    next();
    skip('=');
    memset(&alias_ad, 0, sizeof alias_ad);
    if (!parse_btype(&alias_type, &alias_ad, 0))
      expect("using alias type");
    type_decl(&alias_type, &alias_ad, &dummy_v, TYPE_ABSTRACT);
    alias_type.t |= VT_TYPEDEF;
    alias_sym = sym_push(alias_tok, &alias_type, 0, 0);
    alias_sym->a = alias_ad.a;
    if ((alias_type.t & VT_BTYPE) == VT_FUNC)
      merge_funcattr(&alias_sym->type.ref->f, &alias_ad.f);
    skip(';');
  }
  else if (t == TOK_IF)
  {
    new_scope_s(&o);
    skip('(');
    gexpr_decl();
    a = gvtst(1, 0);
    skip(')');
    block(0);
    if (tok == TOK_ELSE)
    {
      d = gjmp(0);
      gsym(a);
      next();
      block(0);
      gsym(d); // patch else jmp
    }
    else
      gsym(a);
    prev_scope_s(&o);

  }
  else if (t == TOK_WHILE)
  {
    new_scope_s(&o);
    d = gind();
    skip('(');
    gexpr();
    a = gvtst(1, 0);
    skip(')');
    b = 0;
    lblock(&a, &b);
    gjmp_addr(d);
    gsym_addr(b, d);
    gsym(a);
    prev_scope_s(&o);

  }
  else if (t == '{')
  {
    if (debug_modes)
      cprime_debug_stabn(cprime_state, 0xc0, ind - func_ind);
    new_scope(&o);

    // Handle Local Labels Declarations
    while (tok == TOK_LABEL)
    {
      do
      {
        next();
        if (tok < TOK_UIDENT)
          expect("label identifier");
        label_push(&local_label_stack, tok, LABEL_DECLARED);
        next();
      }
      while (tok == ',');
      skip(';');
    }

    while (tok != '}')
    {
      decl(VT_LOCAL);
      if (tok != '}')
        block(flags | STMT_COMPOUND);
    }

    prev_scope(&o, flags &STMT_EXPR);
    if (debug_modes)
      cprime_debug_stabn(cprime_state, 0xe0, ind - func_ind);
    if (local_scope)
      next();
    else if (!nocode_wanted)
      check_func_return();

  }
  else if (t == TOK_RETURN)
  {
    b = (func_vt.t &VT_BTYPE) != VT_VOID;
    a = 0;
    e = 0;
    if (tok != ';')
    {
      if (tok == '{' && (func_vt.t & VT_BTYPE) == VT_STRUCT)
      {
        int depth = 1;
        int struct_tok = get_struct_type_name_tok(&func_vt);
        int saved_tok;
        CValue saved_tokc;
        TokenString *return_expr;

        if (!struct_tok)
          cprime_error("braced return requires a named class or struct");
        return_expr = tok_str_alloc();
        tok_str_add(return_expr, struct_tok);
        tok_str_add(return_expr, '(');
        next();
        while (tok != TOK_EOF && depth > 0)
        {
          if (tok == '{')
            ++depth;
          else if (tok == '}' && --depth == 0)
          {
            if (return_expr->len > 0
                && return_expr->str[return_expr->len - 1] == ',')
              --return_expr->len;
            next();
            break;
          }
          tok_str_add_tok(return_expr);
          next();
        }
        tok_str_add(return_expr, ')');
        tok_str_add(return_expr, TOK_EOF);
        saved_tok = tok;
        saved_tokc = tokc;
        begin_macro(return_expr, 1);
        next();
        gexpr();
        /* end_macro() owns and frees return_expr. */
        end_macro();
        tok = saved_tok;
        tokc = saved_tokc;
      }
      else
        gexpr();
      if (b)
      {
        if (is_reference_type(&func_vt))
        {
          CType ret_ptr = func_vt;
          decay_reference_type(&ret_ptr);
          test_lvalue();
          mk_pointer(&vtop->type);
          gaddrof();
          gen_assign_cast(&ret_ptr);
          ref_ret_type = ret_ptr;
          e = 1;
        }
        else
        {
          int converting_ctor = 0;
          if ((func_vt.t & VT_BTYPE) == VT_STRUCT
              && !is_compatible_types(&func_vt, &vtop->type))
          {
            CType arg_type = vtop->type;
            if ((arg_type.t & VT_ARRAY) && arg_type.ref)
            {
              arg_type = *pointed_type(&arg_type);
              mk_pointer(&arg_type);
            }
            if (resolve_member_func_by_arg_types(&func_vt, TOK_CONSTRUCTOR1,
                                                 &arg_type, 1))
              converting_ctor = 1;
          }
          if (!converting_ctor)
            gen_assign_cast(&func_vt);
        }
      }
      else
      {
        if (vtop->type.t != VT_VOID)
          cprime_warning("void function returns a value");
        vtop--;
      }
    }
    else if (b)
    {
      cprime_warning("'return' with no value");
      b = 0;
    }
    if (b && (func_vt.t & VT_BTYPE) == VT_STRUCT)
    {
      a = try_gfunc_return_copy_construct(&func_vt);
      if (!a)
      {
        gfunc_return(&func_vt);
        a = 1;
      }
    }
    leave_scope(root_scope);
    if (b && !a)
      gfunc_return(e ? &ref_ret_type : &func_vt);
    skip(';');
    // Jump Unless Last Stmt In Top-Level Block
    if (tok != '}' || local_scope != 1)
      rsym = gjmp(rsym);
    if (debug_modes)
      cprime_tcov_block_end (cprime_state, -1);
    CODE_OFF();

  }
  else if (t == TOK_BREAK)
  {
    // Compute Jump
    if (!cur_scope->bsym)
      cprime_error("cannot break");
    if (cur_switch && cur_scope->bsym == cur_switch->bsym)
      leave_scope(cur_switch->scope);
    else
      leave_scope(loop_scope);
    *cur_scope->bsym = gjmp(*cur_scope->bsym);
    skip(';');

  }
  else if (t == TOK_CONTINUE)
  {
    // Compute Jump
    if (!cur_scope->csym)
      cprime_error("cannot continue");
    leave_scope(loop_scope);
    *cur_scope->csym = gjmp(*cur_scope->csym);
    skip(';');

  }
  else if (t == TOK_FOR)
  {
    new_scope(&o);

    skip('(');
    if (try_parse_range_for())
    {
      prev_scope(&o, 0);
      return;
    }
    if (tok != ';')
    {
      // C99 For-Loop Init Decl?
      if (!decl(VT_JMP))
      {
        // No, Regular For-Loop Init Expr
        gexpr();
        vpop();
      }
    }
    skip(';');
    a = b = 0;
    c = d = gind();
    if (tok != ';')
    {
      gexpr();
      a = gvtst(1, 0);
    }
    skip(';');
    if (tok != ')')
    {
      e = gjmp(0);
      d = gind();
      gexpr();
      vpop();
      gjmp_addr(c);
      gsym(e);
    }
    skip(')');
    lblock(&a, &b);
    gjmp_addr(d);
    gsym_addr(b, d);
    gsym(a);
    prev_scope(&o, 0);

  }
  else if (t == TOK_DO)
  {
    new_scope_s(&o);
    a = b = 0;
    d = gind();
    lblock(&a, &b);
    gsym(b);
    skip(TOK_WHILE);
    skip('(');
    gexpr();
    c = gvtst(0, 0);
    skip(')');
    skip(';');
    gsym_addr(c, d);
    gsym(a);
    prev_scope_s(&o);

  }
  else if (t == TOK_SWITCH)
  {
    struct switch_t *sw;

    sw = cprime_mallocz(sizeof *sw);
    sw->bsym = &a;
    sw->scope = cur_scope;
    sw->prev = cur_switch;
    sw->nocode_wanted = nocode_wanted;
    cur_switch = sw;

    new_scope_s(&o);
    skip('(');
    gexpr_decl();
    if (!is_integer_btype(vtop->type.t & VT_BTYPE))
      cprime_error("switch value not an integer");
    skip(')');
    sw->sv = *vtop--; // Save Switch Value
    a = 0;
    b = gjmp(0); // Jump To First Case
    lblock(&a, NULL);
    a = gjmp(a); // Add Implicit Break
    // Case Lookup
    gsym(b);
    prev_scope_s(&o);
    if (sw->nocode_wanted)
      goto skip_switch;
    case_sort(sw);
    sw->bsym = NULL; // Marker For 32Bit:Gen_Opl()
    vpushv(&sw->sv);
    gv(RC_INT);
    d = gcase(sw->p, sw->n, 0);
    vpop();
    if (sw->def_sym)
      gsym_addr(d, sw->def_sym);
    else
      gsym(d);
skip_switch:
    // Break Label
    gsym(a);
    end_switch();

  }
  else if (t == TOK_CASE)
  {
    struct case_t *cr;
    if (!cur_switch)
      expect("switch");
    cr = cprime_malloc(sizeof(struct case_t));
    dynarray_add(&cur_switch->p, &cur_switch->n, cr);
    t = cur_switch->sv.type.t;
    cr->v1 = cr->v2 = value64(expr_const64(), t);
    if (tok == TOK_DOTS && non_iso)
    {
      next();
      cr->v2 = value64(expr_const64(), t);
      if (case_cmp(cr->v2, cr->v1) < 0)
        cprime_warning("empty case range");
    }
    // Case And Default Are Unreachable From A Switch Under Nocode_Wanted
    if (!cur_switch->nocode_wanted)
      cr->ind = gind();
    cr->line = file->line_num;
    skip(':');
    goto block_after_label;

  }
  else if (t == TOK_DEFAULT)
  {
    if (!cur_switch)
      expect("switch");
    if (cur_switch->def_sym)
      cprime_error("too many 'default'");
    cur_switch->def_sym = cur_switch->nocode_wanted ? -1 : gind();
    skip(':');
    goto block_after_label;

  }
  else if (t == TOK_GOTO)
  {
    vla_restore(cur_scope->vla.locorig);
    if (tok == '*' && non_iso)
    {
      // Computed Goto
      next();
      gexpr();
      if ((vtop->type.t & VT_BTYPE) != VT_PTR)
        expect("pointer");
      ggoto();

    }
    else if (tok >= TOK_UIDENT)
    {
      s = label_find(tok);
      // put forward definition if needed
      if (!s)
        s = label_push(&global_label_stack, tok, LABEL_FORWARD);
      else if (s->r == LABEL_DECLARED)
        s->r = LABEL_FORWARD;

      if (s->r & LABEL_FORWARD)
      {
        // Start New Goto Chain For Cleanups, Linked Via Label->Next
        if (cur_scope->cl.s && !nocode_wanted)
        {
          sym_push2(&pending_gotos, SYM_FIELD, 0, cur_scope->cl.n);
          pending_gotos->cleanup_label = s;
          s = sym_push2(&s->next, SYM_FIELD, 0, 0);
          pending_gotos->next = s;
        }
        s->jnext = gjmp(s->jnext);
      }
      else
      {
        try_call_cleanup_goto(s->cleanupstate);
        gjmp_addr(s->jind);
      }
      next();

    }
    else
      expect("label identifier");
    skip(';');

  }
  else if (t == TOK_ASM1 || t == TOK_ASM2 || t == TOK_ASM3)
    asm_instr();

  else
  {
    if (tok == ':' && t >= TOK_UIDENT)
    {
      TokenString *scope_probe = tok_str_alloc();
      tok_str_add(scope_probe, tok);
      next();
      if (tok == ':')
      {
        tok_str_add2(scope_probe, tok, &tokc);
        tok_str_add(scope_probe, 0);
        begin_macro(scope_probe, 1);
        tok = t;
        goto expr;
      }
      tok_str_free(scope_probe);
      // Label Case
      s = label_find(t);
      if (s)
      {
        if (s->r == LABEL_DEFINED)
          cprime_error("duplicate label '%s'", get_tok_str(s->v, NULL));
        s->r = LABEL_DEFINED;
        if (s->next)
        {
          Sym *pcl; // Pending Cleanup Goto
          for (pcl = s->next; pcl; pcl = pcl->prev)
            gsym(pcl->jnext);
          sym_pop(&s->next, NULL, 0);
        }
        else
          gsym(s->jnext);
      }
      else
        s = label_push(&global_label_stack, t, LABEL_DEFINED);
      s->jind = gind();
      s->cleanupstate = cur_scope->cl.s;

block_after_label:
      // Accept attributes after labels (e.g. 'unused')
      parse_attribute(NULL);

      if (debug_modes)
        cprime_tcov_reset_ind(cprime_state);
      vla_restore(cur_scope->vla.loc);

      if (tok != '}')
      {
        if (0 == (flags & STMT_COMPOUND))
          goto again;
        // C23: insert implicit null-statement whithin compound statement
      }
      else
      {
        // We Accept This, But It Is A Mistake
        cprime_warning_c(warn_all)("deprecated use of label at end of compound statement");
      }
    }
    else
    {
      // Expression Case
      if (t != ';')
      {
        unget_tok(t);
expr:
        if (flags & STMT_EXPR)
        {
          vpop();
          gexpr();
        }
        else
        {
          gexpr();
          vpop();
        }
        skip(';');
      }
    }
  }

  if (debug_modes)
    cprime_tcov_check_line (cprime_state, 0), cprime_tcov_block_end (cprime_state, 0);
}

/* This skips over a stream of tokens containing balanced {} and ()
   pairs, stopping at outer ',' ';' and '}' (or matching '}' if we started
   with a '{').  If STR then allocates and stores the skipped tokens
   in *STR.  This doesn't check if () and {} are nested correctly,
   i.e. "({)}" is accepted.  */
static void skip_or_save_block(TokenString **str)
{
  int braces = tok == '{';
  int level = 0;
  if (str)
    *str = tok_str_alloc();

  while (1)
  {
    int t = tok;
    if (level == 0
        && (t == ','
            || t == ';'
            || t == '}'
            || t == ')'
            || t == ']'))
      break;
    if (t == TOK_EOF)
    {
      if (str || level > 0)
        cprime_error("unexpected end of file");
      else
        break;
    }
    if (str)
      tok_str_add_tok(*str);
    next();
    if (t == '{' || t == '(' || t == '[')
      level++;
    else if (t == '}' || t == ')' || t == ']')
    {
      level--;
      if (level == 0 && braces && t == '}')
        break;
    }
  }
  if (str)
    tok_str_add(*str, TOK_EOF);
}

#define EXPR_CONST 1
#define EXPR_ANY   2

static void parse_init_elem(int expr_type)
{
  int saved_global_expr;
  switch (expr_type)
  {
  case EXPR_CONST:
    // Compound Literals Must Be Allocated Globally In This Case
    saved_global_expr = global_expr;
    global_expr = 1;
    expr_const1();
    global_expr = saved_global_expr;
    /* NOTE: symbols are accepted, as well as lvalue for anon symbols
    (compound literals).  */
    if (((vtop->r & (VT_VALMASK | VT_LVAL)) != VT_CONST
         && ((vtop->r & (VT_SYM | VT_LVAL)) != (VT_SYM | VT_LVAL)
             || vtop->sym->v < SYM_FIRST_ANOM))
#ifdef CPRIME_TARGET_PE
        || ((vtop->r & VT_SYM) && vtop->sym->a.dllimport)
#endif
       )
      cprime_error("initializer element is not constant");
    break;
  case EXPR_ANY:
    expr_eq();
    break;
  }
}

#if 1
static void init_assert(init_params *p, int offset)
{
  if (p->sec ? !NODATA_WANTED && offset > p->sec->data_offset
      : !nocode_wanted && offset > p->local_offset)
    cprime_internal_error("initializer overflow");
}
#else
#define init_assert(sec, offset)
#endif

// Put Zeros For Variable Based Init
static void init_putz(init_params *p, unsigned long c, int size)
{
  init_assert(p, c + size);
  if (p->sec)
  {
    // nothing to do because globals are already set to zero
  }
  else
  {
    vpush_helper_func(TOK_memset);
    vseti(VT_LOCAL, c);
    vpushi(0);
    vpushs(size);
#if defined CPRIME_TARGET_ARM && defined CPRIME_ARM_EABI
    vswap();  // Using __Aeabi_Memset(Void*, Size_T, Int)
#endif
    gfunc_call(3);
  }
}

#define DIF_FIRST     1
#define DIF_SIZE_ONLY 2
#define DIF_HAVE_ELEM 4
#define DIF_CLEAR     8

/* delete relocations for specified range c ... c + size. Unfortunatly
   in very special cases, relocations may occur unordered */
static void decl_design_delrels(Section *sec, int c, int size)
{
  ObjW_Rel *rel, *rel2, *rel_end;
  if (!sec || !sec->reloc)
    return;
  rel = rel2 = (ObjW_Rel *)sec->reloc->data;
  rel_end = (ObjW_Rel *)(sec->reloc->data + sec->reloc->data_offset);
  while (rel < rel_end)
  {
    if (rel->r_offset >= c && rel->r_offset < c + size)
      sec->reloc->data_offset -= sizeof * rel;
    else
    {
      if (rel2 != rel)
        memcpy(rel2, rel, sizeof *rel);
      ++rel2;
    }
    ++rel;
  }
}

static void decl_design_flex(init_params *p, Sym *ref, int index)
{
  if (ref == p->flex_array_ref)
  {
    if (index >= ref->c)
      ref->c = index + 1;
  }
  else if (ref->c < 0 && index >= 0)
    cprime_error("flexible array has zero size in this context");
}

/* t is the array or struct type. c is the array or struct
   address. cur_field is the pointer to the current
   field, for arrays the 'c' member contains the current start
   index.  'flags' is as in decl_initializer.
   'al' contains the already initialized length of the
   current container (starting at c).  This returns the new length of that.  */
static int decl_designator(init_params *p, CType *type, unsigned long c,
                           Sym **cur_field, int flags, int al)
{
  Sym *s, *f;
  int index, index_last, align, l, nb_elems, elem_size;
  unsigned long corig = c;

  elem_size = 0;
  nb_elems = 1;

  if (flags & DIF_HAVE_ELEM)
    goto no_designator;

  if (non_iso && tok >= TOK_UIDENT)
  {
    l = tok, next();
    if (tok == ':')
      goto struct_field;
    unget_tok(l);
  }

  // NOTE: we only support ranges for last designator
  while (nb_elems == 1 && (tok == '[' || tok == '.'))
  {
    if (tok == '[')
    {
      if (!(type->t & VT_ARRAY))
        expect("array type");
      next();
      index = index_last = expr_const();
      if (tok == TOK_DOTS && non_iso)
      {
        next();
        index_last = expr_const();
      }
      skip(']');
      s = type->ref;
      decl_design_flex(p, s, index_last);
      if (index < 0 || index_last >= s->c || index_last < index)
        cprime_error("index exceeds array bounds or range is empty");
      if (cur_field)
        (*cur_field)->c = index_last;
      type = pointed_type(type);
      elem_size = type_size(type, &align);
      c += index * elem_size;
      nb_elems = index_last - index + 1;
    }
    else
    {
      int cumofs;
      next();
      l = tok;
struct_field:
      next();
      f = find_field(type, l, &cumofs);
      if (cur_field)
        *cur_field = f;
      type = &f->type;
      c += cumofs;
    }
    cur_field = NULL;
  }
  if (!cur_field)
  {
    if (tok == '=')
      next();
    else if (!non_iso)
      expect("=");
  }
  else
  {
no_designator:
    if (type->t & VT_ARRAY)
    {
      index = (*cur_field)->c;
      s = type->ref;
      decl_design_flex(p, s, index);
      if (index >= s->c)
        cprime_error("too many initializers");
      type = pointed_type(type);
      elem_size = type_size(type, &align);
      c += index * elem_size;
    }
    else
    {
      f = *cur_field;
      // Skip bitfield padding. Also with size 32 and 64.
      while (f && (f->v & SYM_FIRST_ANOM) &&
             is_integer_btype(f->type.t & VT_BTYPE))
        *cur_field = f = f->next;
      if (!f)
        cprime_error("too many initializers");
      type = &f->type;
      c += f->c;
    }
  }

  if (!elem_size) // For Structs
    elem_size = type_size(type, &align);

  /* Using designators the same element can be initialized more
     than once.  In that case we need to delete possibly already
     existing relocations. */
  if (!(flags & DIF_SIZE_ONLY) && c - corig < al)
  {
    decl_design_delrels(p->sec, c, elem_size *nb_elems);
    flags &= ~DIF_CLEAR; // Mark Stack Dirty Too
  }

  decl_initializer(p, type, c, flags & ~DIF_FIRST);

  if (!(flags & DIF_SIZE_ONLY) && nb_elems > 1)
  {
    Sym aref = {0};
    CType t1;
    int i;
    if (p->sec || (type->t & VT_ARRAY))
    {
      // Make Init_Putv/Vstore Believe It Were A Struct
      aref.c = elem_size;
      t1.t = VT_STRUCT, t1.ref = &aref;
      type = &t1;
    }
    if (p->sec)
      vpush_ref(type, p->sec, c, elem_size);
    else
      vset(type, VT_LOCAL | VT_LVAL, c);
    for (i = 1; i < nb_elems; i++)
    {
      vdup();
      init_putv(p, type, c + elem_size *i);
    }
    vpop();
  }

  c += nb_elems * elem_size;
  if (c - corig > al)
    al = c - corig;
  return al;
}

// Store A Value Or An Expression Directly In Global Data Or In Local Array
static void init_putv(init_params *p, CType *type, unsigned long c)
{
  int bt;
  void *ptr;
  CType dtype;
  int size, align;
  Section *sec = p->sec;
  uint64_t val;

  dtype = *type;
  dtype.t &= ~VT_CONSTANT; // need to do that to avoid false warning
  if (is_reference_type(&dtype))
  {
    decay_reference_type(&dtype);
    if (is_reference_type(&vtop->type))
      decay_reference_type(&vtop->type);
    else
    {
      test_lvalue();
      mk_pointer(&vtop->type);
      gaddrof();
    }
  }

  size = type_size(type, &align);
  if (type->t & VT_BITFIELD)
    size = (BIT_POS(type->t) + BIT_SIZE(type->t) + 7) / 8;
  init_assert(p, c + size);

  if (sec)
  {
    // XXX: not portable
    // XXX: generate error if incorrect relocation
    gen_assign_cast(&dtype);
    bt = dtype.t &VT_BTYPE;

    if ((vtop->r & VT_SYM)
        && bt != VT_PTR
        && (bt != (PTR_SIZE == 8 ? VT_LLONG : VT_INT)
            || (type->t & VT_BITFIELD))
        && !((vtop->r & VT_CONST) && vtop->sym->v >= SYM_FIRST_ANOM)
       )
      cprime_error("initializer element is not computable at load time");

    if (NODATA_WANTED)
    {
      vtop--;
      return;
    }

    ptr = sec->data + c;
    val = vtop->c.i;

    // XXX: make code faster ?
    if ((vtop->r & (VT_SYM | VT_CONST)) == (VT_SYM | VT_CONST) &&
        vtop->sym->v >= SYM_FIRST_ANOM &&
        /* XXX This rejects compound literals like
           '(void *){ptr}'.  The problem is that '&sym' is
           represented the same way, which would be ruled out
           by the SYM_FIRST_ANOM check above, but also '"string"'
           in 'char *p = "string"' is represented the same
           with the type being VT_PTR and the symbol being an
           anonymous one.  That is, there's no difference in vtop
           between '(void *){x}' and '&(void *){x}'.  Ignore
           pointer typed entities here.  Hopefully no real code
           will ever use compound literals with scalar type.  */
        (vtop->type.t & VT_BTYPE) != VT_PTR)
    {
      // These come from compound literals, memcpy stuff over.
      Section *ssec;
      ObjSym *esym;
      ObjW_Rel *rel;
      esym = elfsym(vtop->sym);
      ssec = cprime_state->sections[esym->st_shndx];
      memmove (ptr, ssec->data + esym->st_value + (int)vtop->c.i, size);
      if (ssec->reloc)
      {
        /* We need to copy over all memory contents, and that
           includes relocations.  Use the fact that relocs are
           created it order, so look from the end of relocs
           until we hit one before the copied region.  */
        unsigned long relofs = ssec->reloc->data_offset;
        while (relofs >= sizeof(*rel))
        {
          relofs -= sizeof(*rel);
          rel = (ObjW_Rel *)(ssec->reloc->data + relofs);
          if (rel->r_offset >= esym->st_value + size)
            continue;
          if (rel->r_offset < esym->st_value)
            break;
          put_elf_reloca(symtab_section, sec,
                         c + rel->r_offset - esym->st_value,
                         Obj64_R_TYPE(rel->r_info),
                         Obj64_R_SYM(rel->r_info),
#if PTR_SIZE == 8
                         rel->r_addend
#else
                         0
#endif
                        );
        }
      }
    }
    else
    {
      if (type->t & VT_BITFIELD)
      {
        int bit_pos, bit_size, bits, n;
        unsigned char *p, v, m;
        bit_pos = BIT_POS(vtop->type.t);
        bit_size = BIT_SIZE(vtop->type.t);
        p = (unsigned char *)ptr + (bit_pos >> 3);
        bit_pos &= 7, bits = 0;
        while (bit_size)
        {
          n = 8 - bit_pos;
          if (n > bit_size)
            n = bit_size;
          v = val >> bits << bit_pos;
          m = ((1 << n) - 1) << bit_pos;
          *p = (*p & ~m) | (v &m);
          bits += n, bit_size -= n, bit_pos = 0, ++p;
        }
      }
      else
        switch (bt)
        {
        case VT_BOOL:
          *(char *)ptr = val != 0;
          break;
        case VT_BYTE:
          *(char *)ptr = val;
          break;
        case VT_SHORT:
          write16le(ptr, val);
          break;
        case VT_FLOAT:
          write32le(ptr, val);
          break;
        case VT_DOUBLE:
          write64le(ptr, val);
          break;
        case VT_LDOUBLE:
#if defined CPRIME_IS_NATIVE_387
          /* Host and target platform may be different but both have x87.
             On windows, cpc does not use VT_LDOUBLE, except when it is a
             cross compiler.  In this case a mingw gcc as host compiler
             comes here with 10-byte long doubles, while msvc or cpc won't.
             cpc itself can still translate by asm.
             In any case we avoid possibly random bytes 11 and 12.
          */
          if (sizeof (long double) >= 10)
            memcpy(ptr, &vtop->c.ld, 10);
#ifdef __TINYC__
          else if (sizeof (long double) == sizeof (double))
            __asm__("fldl %1\nfstpt %0\n" : "=m" (*ptr) : "m" (vtop->c.ld));
#endif
          else
#endif
            /* For other platforms it should work natively, but may not work
               for cross compilers */
            if (sizeof(long double) == LDOUBLE_SIZE)
              memcpy(ptr, &vtop->c.ld, LDOUBLE_SIZE);
            else if (sizeof(double) == LDOUBLE_SIZE)
              *(double * )ptr = (double)vtop->c.ld;
            else if (0 == memcmp(ptr, &vtop->c.ld, LDOUBLE_SIZE))
              ; // nothing to do for 0.0
#ifndef CPRIME_CROSS_TEST
            else
              cprime_error("can't cross compile long double constants");
#endif
          break;

#if PTR_SIZE == 8
        // Intptr_T May Need A Reloc Too, See Tcctest.C:Relocation_Test()
        case VT_LLONG:
        case VT_PTR:
          if (vtop->r & VT_SYM)
            greloca(sec, vtop->sym, c, R_DATA_PTR, val);
          else
            write64le(ptr, val);
          break;
        case VT_INT:
          write32le(ptr, val);
          break;
#else
        case VT_LLONG:
          write64le(ptr, val);
          break;
        case VT_PTR:
        case VT_INT:
          if (vtop->r & VT_SYM)
            greloc(sec, vtop->sym, c, R_DATA_PTR);
          write32le(ptr, val);
          break;
#endif
        default:
          //cprime_internal_error("unexpected type");
          break;
        }
    }
    vtop--;
  }
  else
  {
    if ((dtype.t & VT_BTYPE) == VT_STRUCT
        && struct_needs_memberwise_copy(&dtype))
    {
      SValue dst_ptr, src_ptr, source;
      source = *vtop;
      vtop--;
      vset(&dtype, VT_LOCAL | VT_LVAL, c);
      mk_pointer(&vtop->type);
      gaddrof();
      dst_ptr = *vtop;
      vtop--;
      vpushv(&source);
      mk_pointer(&vtop->type);
      gaddrof();
      src_ptr = *vtop;
      vtop--;
      copy_construct_struct_memberwise_from_base_ptr(&dtype, &dst_ptr,
                                                     &src_ptr, 0);
    }
    else
    {
      vset(&dtype, VT_LOCAL | VT_LVAL, c);
      vswap();
      vstore();
      vpop();
    }
  }
}

/* 't' contains the type and storage info. 'c' is the offset of the
   object in section 'sec'. If 'sec' is NULL, it means stack based
   allocation. 'flags & DIF_FIRST' is true if array '{' must be read (multi
   dimension implicit array init handling). 'flags & DIF_SIZE_ONLY' is true if
   size only evaluation is wanted (only for arrays). */
static void decl_initializer(init_params *p, CType *type, unsigned long c, int flags)
{
  int len, n, no_oblock, i;
  int size1, align1;
  Sym *s, *f;
  Sym indexsym;
  CType *t1;

  // Generate Line Number Info
  if (debug_modes && !(flags & DIF_SIZE_ONLY) && !p->sec)
    cprime_debug_line(cprime_state), cprime_tcov_check_line (cprime_state, 1);

  if (!(flags & DIF_HAVE_ELEM) && tok != '{' &&
      /* In case of strings we have special handling for arrays, so
         don't consume them as initializer value (which would commit them
         to some anonymous symbol).  */
      tok != TOK_LSTR && tok != TOK_STR &&
      (!(flags & DIF_SIZE_ONLY)
       /* a struct may be initialized from a struct of same type, as in
               struct {int x,y;} a = {1,2}, b = {3,4}, c[] = {a,b};
          In that case we need to parse the element in order to check
          it for compatibility below */
       || (type->t & VT_BTYPE) == VT_STRUCT)
     )
  {
    int ncw_prev = nocode_wanted;
    if ((flags & DIF_SIZE_ONLY) && !p->sec)
      ++nocode_wanted;
    parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
    nocode_wanted = ncw_prev;
    flags |= DIF_HAVE_ELEM;
  }

  if (type->t & VT_ARRAY)
  {
    no_oblock = 1;
    if (((flags & DIF_FIRST) && tok != TOK_LSTR && tok != TOK_STR) ||
        tok == '{')
    {
      skip('{');
      no_oblock = 0;
    }

    s = type->ref;
    n = s->c;
    t1 = pointed_type(type);
    size1 = type_size(t1, &align1);

    /* only parse strings here if correct type (otherwise: handle
       them as ((w)char *) expressions */
    if ((tok == TOK_LSTR &&
#ifdef CPRIME_TARGET_PE
         (t1->t & VT_BTYPE) == VT_SHORT && (t1->t & VT_UNSIGNED)
#else
         (t1->t & VT_BTYPE) == VT_INT
#endif
        ) || (tok == TOK_STR && (t1->t & VT_BTYPE) == VT_BYTE))
    {
      len = 0;
      cstr_reset(&initstr);
      if (size1 != (tok == TOK_STR ? 1 : sizeof(nwchar_t)))
        cprime_error("unhandled string literal merging");
      while (tok == TOK_STR || tok == TOK_LSTR)
      {
        if (initstr.size)
          initstr.size -= size1;
        if (tok == TOK_STR)
          len += tokc.str.size;
        else
          len += tokc.str.size / sizeof(nwchar_t);
        len--;
        cstr_cat(&initstr, tokc.str.data, tokc.str.size);
        next();
      }
      if (tok != ')' && tok != '}' && tok != ',' && tok != ';'
          && tok != TOK_EOF)
      {
        // Not a lone literal but part of a bigger expression.
        unget_tok(size1 == 1 ? TOK_STR : TOK_LSTR);
        tokc.str.size = initstr.size;
        tokc.str.data = initstr.data;
        goto do_init_array;
      }

      decl_design_flex(p, s, len);
      if (!(flags & DIF_SIZE_ONLY))
      {
        int nb = n, ch;
        if (len < nb)
          nb = len;
        if (len > nb)
          cprime_warning("initializer-string for array is too long");
        /* in order to go faster for common case (char
           string in global variable, we handle it
           specifically */
        if (p->sec && size1 == 1)
        {
          init_assert(p, c + nb);
          if (!NODATA_WANTED)
            memcpy(p->sec->data + c, initstr.data, nb);
        }
        else
        {
          for (i = 0; i < n; i++)
          {
            if (i >= nb)
            {
              /* only add trailing zero if enough storage (no
                 warning in this case since it is standard) */
              if (flags & DIF_CLEAR)
                break;
              if (n - i >= 4)
              {
                init_putz(p, c + i *size1, (n - i) * size1);
                break;
              }
              ch = 0;
            }
            else if (size1 == 1)
              ch = ((unsigned char *)initstr.data)[i];
            else
              ch = ((nwchar_t *)initstr.data)[i];
            vpushi(ch);
            init_putv(p, t1, c + i *size1);
          }
        }
      }
    }
    else
    {

do_init_array:
      indexsym.c = 0;
      f = &indexsym;

do_init_list:
      // Zero Memory Once In Advance
      if (!(flags & (DIF_CLEAR | DIF_SIZE_ONLY)))
      {
        init_putz(p, c, n *size1);
        flags |= DIF_CLEAR;
      }

      len = 0;
      /* language extension: if the initializer is empty for a flex array,
         it's size is zero.  We won't enter the loop, so set the size
         now.  */
      decl_design_flex(p, s, len - 1);
      while (tok != '}' || (flags & DIF_HAVE_ELEM))
      {
        len = decl_designator(p, type, c, &f, flags, len);
        flags &= ~DIF_HAVE_ELEM;
        if (type->t & VT_ARRAY)
        {
          ++indexsym.c;
          /* special test for multi dimensional arrays (may not
             be strictly correct if designators are used at the
             same time) */
          if (no_oblock && len >= n * size1)
            break;
        }
        else
        {
          if (s->type.t == VT_UNION)
            f = NULL;
          else
            f = f->next;
          if (no_oblock && f == NULL)
            break;
        }

        if (tok == '}')
          break;
        skip(',');
      }
    }
    if (!no_oblock)
      skip('}');

  }
  else if ((flags & DIF_HAVE_ELEM)
           /* Use i_c_parameter_t, to strip toplevel qualifiers.
              The source type might have VT_CONSTANT set, which is
              of course assignable to non-const elements.  */
           && is_compatible_unqualified_types(type, &vtop->type))
  {
    /* Fall through to the scalar initializer path. */
    if ((flags & DIF_SIZE_ONLY))
    {
      if (flags & DIF_HAVE_ELEM)
        vpop();
      else
        skip_or_save_block(NULL);
    }
    else
    {
      if (!(flags & DIF_HAVE_ELEM))
      {
        /* This should happen only when we haven't parsed
           the init element above for fear of committing a
           string constant to memory too early.  */
        if (tok != TOK_STR && tok != TOK_LSTR)
          expect("string constant");
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      }
      if (!p->sec && (flags & DIF_CLEAR) // Container Was Already Zero'D
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
          && vtop->c.i == 0
          && btype_size(type->t & VT_BTYPE) // Not For Fp Constants
         )
        vpop();
      else
        init_putv(p, type, c);
    }
  }

  else if ((type->t & VT_BTYPE) == VT_STRUCT
           && type_is_std_initializer_list(type)
           && tok == '{')
  {
    Sym *field;
    TokenString *elements[32];
    CType *element_type;
    int element_size, element_align, element_addr = 0, element_r2 = 0;
    int i;

    len = count_saved_braced_ctor_args(elements, 32);
    if (!(flags & DIF_SIZE_ONLY))
    {
      field = type->ref->next;
      element_type = field ? pointed_type(&field->type) : NULL;
      if (!field || !element_type)
        cprime_error("initializer_list layout missing array field");
      element_size = type_size(element_type, &element_align);
      if (element_size < 0)
        cprime_error("initializer_list element has incomplete type");
      if (len)
      {
        init_params element_init = {0};
        element_addr = get_temp_local_var(element_size * len, element_align,
                                          &element_r2);
        element_init.local_offset = element_addr + element_size * len;
        for (i = 0; i < len; ++i)
        {
          TokenString macro = *elements[i];
          begin_macro(&macro, 0);
          next();
          expr_eq();
          end_macro();
          init_putv(&element_init, element_type,
                    element_addr + i * element_size);
        }
        vset(element_type, VT_LOCAL | VT_LVAL, element_addr);
        vtop->r2 = element_r2;
        mk_pointer(&vtop->type);
        gaddrof();
      }
      else
        vpushi(0);
      init_putv(p, &field->type, c + field->c);
      field = field->next;
      if (!field)
        cprime_error("initializer_list layout missing length field");
      vpushi(len);
      init_putv(p, &field->type, c + field->c);
    }
    for (i = 0; i < len; ++i)
      tok_str_free(elements[i]);
  }
  else if ((type->t & VT_BTYPE) == VT_STRUCT)
  {
    no_oblock = 1;
    if ((flags & DIF_FIRST) || tok == '{')
    {
      skip('{');
      no_oblock = 0;
    }
    s = type->ref;
    f = s->next;
    n = s->c;
    size1 = 1;
    goto do_init_list;

  }
  else if (tok == '{')
  {
    if (flags & DIF_HAVE_ELEM)
      skip(';');
    next();
    decl_initializer(p, type, c, flags & ~DIF_HAVE_ELEM);
    skip('}');

  }
  else
  {
    /* If we supported only ISO C we wouldn't have to accept calling
       this on anything than an array if DIF_SIZE_ONLY (and even then
       only on the outermost level, so no recursion would be needed),
       because initializing a flex array member isn't supported.
       But the language supports it, so we need to recurse even into
       subfields of structs and arrays when DIF_SIZE_ONLY is set.  */
    if ((flags & DIF_SIZE_ONLY))
    {
      // Just Skip Expression
      if (flags & DIF_HAVE_ELEM)
        vpop();
      else
        skip_or_save_block(NULL);
    }
    else
    {
      if (!(flags & DIF_HAVE_ELEM))
      {
        /* This should happen only when we haven't parsed
           the init element above for fear of committing a
           string constant to memory too early.  */
        if (tok != TOK_STR && tok != TOK_LSTR)
          expect("string constant");
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      }
      if (!p->sec && (flags & DIF_CLEAR) // Container Was Already Zero'D
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
          && vtop->c.i == 0
          && btype_size(type->t & VT_BTYPE) // Not For Fp Constants
         )
        vpop();
      else
        init_putv(p, type, c);
    }
  }
}

/* parse an initializer for type 't' if 'has_init' is non zero, and
   allocate space in local or global data space ('r' is either
   VT_LOCAL or VT_CONST). If 'v' is non zero, then an associated
   variable 'v' of scope 'scope' is declared before initializers
   are parsed. If 'v' is zero, then a reference to the new object
   is put in the value stack. If 'has_init' is 2, a special parsing
   is done to handle string constants. */
static void decl_initializer_alloc(CType *type, AttributeDef *ad, int r,
                                   int has_init, int has_ctor_init,
                                   TokenString *copy_ctor_init, int v, int decl_scope)
{
  int size, align, addr;
  TokenString *init_str = NULL;
  Sym *ctor_func = NULL;
  int has_expr_struct_init;

  Section *sec;
  Sym *flexible_array;
  Sym *sym = NULL;
  int saved_nocode_wanted = nocode_wanted;
#ifdef CONFIG_CPRIME_BCHECK
  int bcheck = cprime_state->do_bounds_check && !NODATA_WANTED;
#endif
  init_params p = {0};
  has_expr_struct_init = has_init
                         && (type->t & VT_BTYPE) == VT_STRUCT
                         && (tok != '{' || type_is_std_initializer_list(type));

  if (decl_scope == VT_CONST)
  {
    // see if a global symbol was already defined
    sym = sym_find(v);
    if (sym)
    {
      patch_storage(sym, ad, type);
      // We Accept Several Definitions Of The Same Global Variable.
      if (!has_init && sym->c && elfsym(sym)->st_shndx != SHN_UNDEF)
        return;
      type = &sym->type;
    }
  }

  // Always allocate static or global variables
  if (v && (r & VT_VALMASK) == VT_CONST)
    nocode_wanted |= DATA_ONLY_WANTED;

  flexible_array = NULL;
  size = type_size(type, &align);

  /* exactly one flexible array may be initialized, either the
     toplevel array or the last member of the toplevel struct */

  if (size < 0)
  {
    // Error Out Except For Top-Level Incomplete Arrays
    // (Arrays Of Incomplete Types Are Handled In Array Parsing)
    if (!(type->t & VT_ARRAY))
      cprime_error("initialization of incomplete type");
    /* If the base type itself was an array type of unspecified
       size (like in 'typedef int arr[]; arr x = {1};') then
       we will overwrite the unknown size by the real one for
       this decl.  We need to unshare the ref symbol holding
       that size.  */
    if (IS_BT_ARRAY(type->t))
      type->ref = sym_push(SYM_FIELD, &type->ref->type, 0, type->ref->c);
    p.flex_array_ref = type->ref;

  }
  else if (has_init && (type->t & VT_BTYPE) == VT_STRUCT)
  {
    Sym *field = type->ref->next;
    if (field)
    {
      while (field->next)
        field = field->next;
      if (field->type.t & VT_ARRAY && field->type.ref->c < 0)
      {
        flexible_array = field;
        p.flex_array_ref = field->type.ref;
        size = -1;
      }
    }
  }

  if (size < 0)
  {
    // If unknown size, do a dry-run 1st pass
    if (!has_init)
      goto err_size;
    if (has_init == 2)
    {
      // Only Get Strings
      init_str = tok_str_alloc();
      while (tok == TOK_STR || tok == TOK_LSTR)
      {
        tok_str_add_tok(init_str);
        next();
      }
      tok_str_add(init_str, TOK_EOF);
    }
    else
      skip_or_save_block(&init_str);
    unget_tok(0);

    // Compute Size
    begin_macro(init_str, 1);
    next();
    decl_initializer(&p, type, 0, DIF_FIRST | DIF_SIZE_ONLY);
    // Prepare Second Initializer Parsing
    macro_ptr = init_str->str;
    next();

    // if still unknown size, error
    size = type_size(type, &align);
    if (size < 0)
err_size:
      cprime_error("unknown type size");

    /* If there's a flex member and it was used in the initializer
       adjust size.  */
    if (flexible_array && flexible_array->type.ref->c > 0)
      size += flexible_array->type.ref->c
              * pointed_size(&flexible_array->type);
  }

  // take into account specified alignment if bigger
  if (ad->a.aligned)
  {
    int speca = 1 << (ad->a.aligned - 1);
    if (speca > align)
      align = speca;
  }
  else if (ad->a.packed)
    align = 1;

  if (!v && NODATA_WANTED)
    size = 0, align = 1;

  if ((r & VT_VALMASK) == VT_LOCAL)
  {
    sec = NULL;
#ifdef CONFIG_CPRIME_BCHECK
    if (bcheck && v)
    {
      // Add Padding Between Stack Variables For Bound Checking
      loc -= align;
    }
#endif
    loc = (loc - size) & -align;
    addr = loc;
    p.local_offset = addr + size;
#ifdef CONFIG_CPRIME_BCHECK
    if (bcheck && v)
    {
      // Add Padding Between Stack Variables For Bound Checking
      loc -= align;
    }
#endif
    if (v)
    {
      // Local Variable
#ifdef CONFIG_CPRIME_ASM
      if (ad->asm_label)
      {
        int reg = asm_parse_regvar(ad->asm_label);
        if (reg >= 0)
          r = (r & ~VT_VALMASK) | reg;
      }
#endif
      sym = sym_push(v, type, r, addr);
      if (!ad->cleanup_func)
        ad->cleanup_func = resolve_autodtor_func(type);
      if (ad->cleanup_func)
      {
        Sym *cls = sym_push2(&all_cleanups,
                             SYM_FIELD | ++cur_scope->cl.n, 0, 0);
        cls->cleanup_sym = sym;
        cls->cleanup_func = ad->cleanup_func;
        cls->next = cur_scope->cl.s;
        cur_scope->cl.s = cls;
      }
      else if ((type->t & VT_BTYPE) == VT_STRUCT)
      {
        register_struct_cleanups(type, sym);
      }

      sym->a = ad->a;
    }
    else
    {
      // Push Local Reference
      vset(type, r, addr);
    }
  }
  else
  {
    // Allocate Symbol In Corresponding Section
    sec = ad->section;
    if (!sec)
    {
      CType *tp = type;
      while ((tp->t & (VT_BTYPE | VT_ARRAY)) == (VT_PTR | VT_ARRAY))
        tp = &tp->ref->type;
      if (tp->t & VT_CONSTANT)
        sec = rodata_section;
      else if (has_init)
      {
        sec = data_section;
        /*if (g_debug & 4)
            cprime_warning("rw data: %s", get_tok_str(v, 0));*/
      }
      else if (cprime_state->nocommon)
        sec = bss_section;
    }

    if (sec)
    {
      addr = section_add(sec, size, align);
#ifdef CONFIG_CPRIME_BCHECK
      // add padding if bound check
      if (bcheck)
        section_add(sec, 1, 1);
#endif
    }
    else
    {
      addr = align; // SHN_COMMON is special, symbol value is align
      sec = common_section;
    }

    if (v)
    {
      if (!sym)
      {
        sym = sym_push(v, type, r | VT_SYM, 0);
        patch_storage(sym, ad, NULL);
      }
      // Update Symbol Definition
      put_extern_sym(sym, sec, addr, size);
    }
    else
    {
      // Push Global Reference
      vpush_ref(type, sec, addr, size);
      sym = vtop->sym;
      vtop->r |= r;
    }

#ifdef CONFIG_CPRIME_BCHECK
    /* handles bounds now because the symbol must be defined
       before for the relocation */
    if (bcheck)
    {
      addr_t *bounds_ptr;

      greloca(bounds_section, sym, bounds_section->data_offset, R_DATA_PTR, 0);
      // Then Add Global Bound Info
      bounds_ptr = section_ptr_add(bounds_section, 2 * sizeof(addr_t));
      bounds_ptr[0] = 0; // Relocated
      bounds_ptr[1] = size;
    }
#endif
  }

  if (type->t & VT_VLA)
  {
    int a;

    if (has_init)
      cprime_error("variable length array cannot be initialized");

    if (NODATA_WANTED)
      goto no_alloc;

    // save before-VLA stack pointer if needed
    if (cur_scope->vla.num == 0)
    {
      if (cur_scope->prev && cur_scope->prev->vla.num)
        cur_scope->vla.locorig = cur_scope->prev->vla.loc;
      else
      {
        gen_vla_sp_save(loc -= PTR_SIZE);
        cur_scope->vla.locorig = loc;
      }
    }

    vpush_type_size(type, &a);
    gen_vla_alloc(type, a);
#if defined CPRIME_TARGET_PE && defined CPRIME_TARGET_X86_64
    /* on _WIN64, because of the function args scratch area, the
       result of alloca differs from RSP and is returned in RAX.  */
    gen_vla_result(addr), addr = (loc -= PTR_SIZE);
#endif
    gen_vla_sp_save(addr);
    cur_scope->vla.loc = addr;
    cur_scope->vla.num++;
  }
  else if (has_init)
  {
    p.sec = sec;
    decl_initializer(&p, type, addr, DIF_FIRST);
    // Patch Flexible Array Member Size Back To -1,
    // For Possible Subsequent Similar Declarations
    if (flexible_array)
      flexible_array->type.ref->c = -1;
  }

  if ((r & VT_VALMASK) == VT_LOCAL && sym && !NODATA_WANTED && !(type->t & VT_VLA))
  {
    if (has_ctor_init)
      ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
    else
      ctor_func = resolve_autoctor_func(type);
    if (copy_ctor_init)
    {
      TokenString *call_args[1];
      call_args[0] = copy_ctor_init;
      if (!ctor_func)
        ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
      call_lifecycle_constructor_saved_args(&sym->type, sym->r, sym->c,
                                            sym, ctor_func, call_args, 1);
    }
    else if (has_ctor_init)
      call_lifecycle_constructor(&sym->type, sym->r, sym->c, sym, ctor_func);
    else if (!copy_ctor_init)
    {
      if (!has_expr_struct_init)
        call_lifecycle_constructor_members(type, sym->r, sym->c);
      if (ctor_func && !has_expr_struct_init)
        call_lifecycle_constructor_noargs(&sym->type, sym->r, sym->c,
                                          sym, ctor_func);
    }
  }

no_alloc:
  // restore parse state if needed
  if (init_str)
  {
    end_macro();
    next();
  }

  nocode_wanted = saved_nocode_wanted;
}

// Generate Vla Code Saved In Post_Type()
static void func_vla_arg_code(Sym *arg)
{
  int align;
  TokenString *vla_array_tok = NULL;

  if (arg->type.ref)
    func_vla_arg_code(arg->type.ref);

  if ((arg->type.t & VT_VLA) && arg->type.ref->vla_array_str)
  {
    loc -= type_size(&int_type, &align);
    loc &= -align;
    arg->type.ref->c = loc;

    unget_tok(0);
    vla_array_tok = tok_str_alloc();
    vla_array_tok->str = arg->type.ref->vla_array_str;
    begin_macro(vla_array_tok, 1);
    next();
    gexpr();
    end_macro();
    next();
    vpush_type_size(&arg->type.ref->type, &align);
    gen_op('*');
    vset(&int_type, VT_LOCAL | VT_LVAL, arg->type.ref->c);
    vswap();
    vstore();
    vpop();
  }
}

static void func_vla_arg(Sym *sym)
{
  Sym *arg;

  for (arg = sym->type.ref->next; arg; arg = arg->next)
    if ((arg->type.t & VT_BTYPE) == VT_PTR && (arg->type.ref->type.t & VT_VLA))
      func_vla_arg_code(arg->type.ref);
}

// Set The Local Stack Address For Function Parameter From Gfunc_Prolog()
ST_FUNC Sym *gfunc_set_param(Sym *s, int c, int byref)
{
  s = sym_find(s->v);
  if (!s) // Unnamed Parameters, Not Enabled
    return NULL;
  s->c = c;
  if (byref)
    s->r = VT_LLOCAL | VT_LVAL; // otherwise VT_LOCAL
  return s;
}

// Push Parameters (And Their Types), Last First
static void sym_push_params(Sym *ref)
{
  Sym *s = ref;
  while (s->next)
    s = s->next;
  while (s != ref)
  {
    if (s->v & ~SYM_FIELD)
      sym_copy(s, &local_stack);
    s = s->prev;
  }
}

/* parse a function defined by symbol 'sym' and generate its code in
   'cur_text_section' */
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
        /* the function was used or forced (and then not internal):
           generate its code and convert it to a normal function */
        fn->sym = NULL;
        cprimepp_putfile(fn->filename);
        begin_macro(fn->func_str, 1);
        next();
        cur_text_section = text_section;
        gen_function(sym);
        end_macro();

        inline_generated = 1;
      }
    }
    /* Inline bodies emitted above can instantiate further template members
       (e.g. a static member body that constructs a class), queueing pending
       definitions after the end-of-TU flush already ran.  Flush before the
       next emission pass so bodies queued by an emitted inline function are
       themselves emitted (and can queue further members). */
    if (nb_pending_member_funcs)
      compile_pending_member_funcs(0);
    /* Replayed member bodies also queue free-function/class template
       specializations; compile those before the next pass so bodies called
       from a member (e.g. clCopyAssign from a constructor) are emitted. */
    if (compiled_template_specs < nb_pending_template_specs)
      compile_pending_template_specs();
  }
  while ((inline_generated || nb_pending_member_funcs
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
    if (fn->sym)
      tok_str_free(fn->func_str);
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
        cprime_error("static assertion failed");
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
          && try_rewrite_cpp_scoped_static_data_after_declarator(&type, &v))
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
            previous = sym_find2(global_stack, v);
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
          dynarray_add(&cprime_state->inline_fns,
                       &cprime_state->nb_inline_fns, fn);
          skip_or_save_block(&fn->func_str);
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
          else if (tok == '(' && l != VT_CONST
                   && btype_was_typedef)
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
              begin_macro(auto_init_str, 1);
              auto_macro_stack = macro_stack;
              next();
              decl_initializer_alloc(&type, &ad, r, 1, 0, NULL, v, l);
              if (macro_stack == auto_macro_stack)
              {
                end_macro();
                /* end_macro() owns the replay buffer and frees it; keep the
                   pointer NULL so the unconditional cleanup below does not
                   free the same TokenString twice. */
                auto_init_str = NULL;
              }
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









