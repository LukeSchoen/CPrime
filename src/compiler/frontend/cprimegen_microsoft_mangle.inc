/* Microsoft C++ external names. Source lookup tokens stay independent from
   these names: scopes and member properties come from declaration metadata. */
typedef struct CppMsMangleContext {
  CString *out;
  int names[10], nb_names;
  int types[10], nb_types;
  int depth;
} CppMsMangleContext;

enum {
  CPP_MS_TYPE_DROP, CPP_MS_TYPE_POINTEE, CPP_MS_TYPE_RESULT, CPP_MS_TYPE_ESCAPE
};

static void cpp_ms_mangle_type(CppMsMangleContext *ctx, CType *type, int mode);
static void cpp_ms_mangle_function_type(CppMsMangleContext *ctx, CType *type,
                                        int structor, int exception_spec);
static void cpp_ms_mangle_qualified_name(CppMsMangleContext *ctx, int name_tok,
                                         int owner_tok, int namespace_tok);
static int template_arg_const_int64(int arg_tok, int64_t *out);

static int cpp_ms_cv(int type)
{
  return ((type & VT_CONSTANT) ? 1 : 0) | ((type & VT_VOLATILE) ? 2 : 0);
}

static void cpp_ms_number(CppMsMangleContext *ctx, int64_t value)
{
  uint64_t magnitude;
  char digits[16];
  int n = 0;
  if (value < 0) {
    cstr_ccat(ctx->out, '?');
    magnitude = (uint64_t)(-(value + 1)) + 1;
  } else magnitude = value;
  if (magnitude >= 1 && magnitude <= 10) {
    cstr_ccat(ctx->out, '0' + magnitude - 1);
    return;
  }
  do {
    digits[n++] = 'A' + (magnitude & 15);
    magnitude >>= 4;
  } while (magnitude);
  while (n) cstr_ccat(ctx->out, digits[--n]);
  cstr_ccat(ctx->out, '@');
}

static void cpp_ms_source_name(CppMsMangleContext *ctx, int name_tok)
{
  int i;
  for (i = 0; i < ctx->nb_names; ++i)
    if (ctx->names[i] == name_tok) {
      cstr_ccat(ctx->out, '0' + i);
      return;
    }
  cstr_cat(ctx->out, get_tok_str(name_tok, NULL), -1);
  cstr_ccat(ctx->out, '@');
  if (ctx->nb_names < 10) ctx->names[ctx->nb_names++] = name_tok;
}

static int cpp_ms_unqualified_tok(int name_tok, int namespace_tok)
{
  const char *name = get_tok_str(name_tok, NULL);
  if (namespace_tok) {
    const char *scope = get_tok_str(namespace_tok, NULL);
    size_t length = strlen(scope);
    if (!strncmp(name, scope, length) && name[length] == '_')
      return tok_alloc_const(name + length + 1);
  }
  return name_tok;
}

static void cpp_ms_template_name(CppMsMangleContext *ctx, int source_tok,
                                 TemplateArgList *args, ParameterMask value_mask,
                                 int is_class, int trailing_value_pack)
{
  CppMsMangleContext nested;
  CString name;
  int i;
  memset(&nested, 0, sizeof(nested));
  cstr_new(&name);
  nested.out = &name;
  nested.depth = ctx->depth;
  cstr_cat(&name, "?$", 2);
  if (get_tok_str(source_tok, NULL)[0] == '?')
    cstr_cat(&name, get_tok_str(source_tok, NULL), -1);
  else cpp_ms_source_name(&nested, source_tok);
  if (!args->nb) cstr_cat(&name, "$$V", 3);
  for (i = 0; i < args->nb; ++i) {
    CType argument;
    int64_t value;
    int parameter_index = trailing_value_pack >= 0 && i >= trailing_value_pack ? trailing_value_pack : i;
    if (parameter_index < CPC_MAX_TEMPLATE_PARAMETERS
        && mask_test(&value_mask, parameter_index)) {
      Sym *address = global_symbol_find(args->toks[i]);
      if (address && address->cpp_member_address_tok) {
        CppMemberPointerType *member = find_cpp_member_pointer_type(&address->type);
        Sym *target = global_symbol_find(address->cpp_member_address_tok);
        if ((member->member_type.t & VT_BTYPE) == VT_FUNC) {
          int label = target->asm_label;
          if (!label) label = cpp_ms_linkage_tok(target->v, &target->type);
          cstr_cat(&name, "$1", 2);
          cstr_cat(&name, get_tok_str(label, NULL), -1);
        } else {
          CType owner;
          int offset = 0;
          make_class_type_from_tok(&owner, member->owner_tok);
          if (!find_field_try(&owner, address->cpp_member_address_tok, &offset))
            cprime_error("member template argument has no field declaration");
          cstr_cat(&name, "$0", 2);
          cpp_ms_number(&nested, offset);
        }
        continue;
      }
      if (address && (address->template_address_target
                      || (address->type.t & VT_BTYPE) == VT_FUNC)) {
        Sym *target = address->template_address_target ? address->template_address_target : address;
        int label = target->asm_label;
        cstr_cat(&name, "$1", 2);
        if ((target->type.t & VT_BTYPE) != VT_FUNC) {
          cstr_ccat(&name, '?');
          cpp_ms_mangle_qualified_name(&nested, target->v, 0, symbol_namespace_scope_tok(target->v));
          cstr_ccat(&name, '3');
          cpp_ms_mangle_type(&nested, &target->type, CPP_MS_TYPE_DROP);
          cstr_ccat(&name, 'A' + cpp_ms_cv(target->type.t));
          continue;
        }
        if (!label) label = cpp_ms_linkage_tok(target->v, &target->type);
        cstr_cat(&name, get_tok_str(label, NULL), -1);
        continue;
      }
      if (!template_arg_const_int64(args->toks[i], &value))
        cprime_error("unsupported non-type template argument '%s' in native linkage for '%s'",
                     get_tok_str(args->toks[i], NULL), get_tok_str(source_tok, NULL));
      cstr_cat(&name, "$0", 2);
      cpp_ms_number(&nested, value);
    } else {
      TemplateDef *template_argument = find_class_template_def(args->toks[i]);
      if (template_argument) {
        /* A class template argument has the tag's qualified name, without
           an instantiation argument list (Microsoft template-name ABI). */
        int first = tok_str_next_non_linenum_index(template_argument->def_str->str,
                                                    template_argument->def_str->len, 0);
        cstr_ccat(&name, template_argument->def_str->str[first] == TOK_CLASS ? 'V' : 'U');
        cpp_ms_mangle_qualified_name(&nested, template_argument->name_tok, 0,
                                      template_argument->namespace_tok);
        continue;
      }
      if (!make_type_from_type_arg_tok(&argument, args->toks[i]))
        cprime_error("unresolved type template argument in native linkage");
      cpp_ms_mangle_type(&nested, &argument, CPP_MS_TYPE_ESCAPE);
    }
  }
  if (is_class) {
    cstr_ccat(&name, 0);
    cpp_ms_source_name(ctx, tok_alloc_const(name.data));
  } else {
    cstr_cat(ctx->out, name.data, name.size);
    cstr_ccat(ctx->out, '@');
  }
  cstr_free(&name);
}

static int cpp_ms_instantiation_name(CppMsMangleContext *ctx, int name_tok)
{
  const CppRecordDeclInfo *record = lookup_cpp_record_decl(name_tok);
  TemplateInstOwner *entry = find_template_inst_owner_entry(name_tok);
  TemplateDef *definition;
  int i;
  if (record && record->template_definition) {
    definition = record->template_definition;
    cpp_ms_template_name(ctx,
        cpp_ms_unqualified_tok(record->source_name_tok, record->namespace_tok),
        (TemplateArgList *)&record->template_arguments,
        definition->value_param_mask, 1,
        definition->variadic_param_index == definition->nb_type_params - 1 ? definition->variadic_param_index : -1);
    return 1;
  }
  if (!entry) return 0;
  definition = entry->owner;
  for (i = 0; i < definition->nb_inst; ++i)
    if (definition->inst_name_toks[i] == name_tok) {
      cpp_ms_template_name(ctx,
          cpp_ms_unqualified_tok(definition->name_tok, definition->namespace_tok),
          &definition->inst_arg_lists[i], definition->value_param_mask,
          definition->is_class,
          definition->variadic_param_index == definition->nb_type_params - 1 ? definition->variadic_param_index : -1);
      return 1;
    }
  return 0;
}

static void cpp_ms_mangle_qualified_name(CppMsMangleContext *ctx, int name_tok,
                                         int owner_tok, int namespace_tok)
{
  int i;
  if (++ctx->depth > 128) cprime_error("native linkage type nesting too deep");
  if (name_tok && !cpp_ms_instantiation_name(ctx, name_tok)) {
    const CppRecordDeclInfo *record = lookup_cpp_record_decl(name_tok);
    cpp_ms_source_name(ctx, record ? record->source_name_tok
                         : cpp_ms_unqualified_tok(name_tok, namespace_tok));
  }
  while (owner_tok) {
    const CppRecordDeclInfo *record = lookup_cpp_record_decl(owner_tok);
    if (!cpp_ms_instantiation_name(ctx, owner_tok))
      cpp_ms_source_name(ctx, record ? record->source_name_tok
        : cpp_ms_unqualified_tok(owner_tok, symbol_namespace_scope_tok(owner_tok)));
    if (record) {
      namespace_tok = record->namespace_tok;
      owner_tok = record->owner_tok;
    } else {
      namespace_tok = symbol_namespace_scope_tok(owner_tok);
      owner_tok = 0;
    }
  }
  for (i = 0; i < nb_namespace_scopes; ++i)
    if (namespace_scopes[i].name_tok == namespace_tok) {
      int part;
      for (part = namespace_scopes[i].depth - 1; part >= 0; --part)
        cpp_ms_source_name(ctx, namespace_scopes[i].parts[part]);
      break;
    }
  cstr_ccat(ctx->out, '@');
  --ctx->depth;
}

static void cpp_ms_mangle_argument(CppMsMangleContext *ctx, CType *type)
{
  CString key;
  CType plain = *type;
  int key_tok, i, start;
  if (!is_reference_type(&plain)) plain.t &= ~(VT_CONSTANT | VT_VOLATILE);
  cstr_new(&key);
  if (!cpp_append_linkage_type(&key, &plain))
    cprime_error("unresolved parameter type in native linkage");
  cstr_ccat(&key, 0);
  key_tok = tok_alloc_const(key.data);
  cstr_free(&key);
  for (i = 0; i < ctx->nb_types; ++i)
    if (ctx->types[i] == key_tok) {
      cstr_ccat(ctx->out, '0' + i);
      return;
    }
  start = ctx->out->size;
  cpp_ms_mangle_type(ctx, &plain, CPP_MS_TYPE_DROP);
  if (ctx->out->size - start > 1 && ctx->nb_types < 10)
    ctx->types[ctx->nb_types++] = key_tok;
}

static void cpp_ms_mangle_function_type(CppMsMangleContext *ctx, CType *type,
                                        int structor, int exception_spec)
{
  Sym *argument;
  char convention = 'A';
  if (!type->ref) cprime_error("incomplete function type in native linkage");
#if !defined(CPRIME_TARGET_X86_64)
  if (type->ref->f.func_call == FUNC_THISCALL) convention = 'E';
  else if (type->ref->f.func_call == FUNC_STDCALL) convention = 'G';
  else if (type->ref->f.func_call == FUNC_FASTCALLW) convention = 'I';
#endif
  cstr_ccat(ctx->out, convention);
  if (structor) cstr_ccat(ctx->out, '@');
  else cpp_ms_mangle_type(ctx, &type->ref->type, CPP_MS_TYPE_RESULT);
  argument = type->ref->next;
  if (!argument && type->ref->f.func_type != FUNC_ELLIPSIS)
    cstr_ccat(ctx->out, 'X');
  else {
    for (; argument; argument = argument->next)
      cpp_ms_mangle_argument(ctx, &argument->type);
    cstr_ccat(ctx->out, type->ref->f.func_type == FUNC_ELLIPSIS ? 'Z' : '@');
  }
  if (exception_spec && type->ref->f.func_noexcept) cstr_cat(ctx->out, "_E", 2);
  else cstr_ccat(ctx->out, 'Z');
}

static void cpp_ms_mangle_type(CppMsMangleContext *ctx, CType *type, int mode)
{
  int bt = type->t & VT_BTYPE, cv = cpp_ms_cv(type->t);
  int is_tag = bt == VT_STRUCT || IS_ENUM(type->t) || IS_ENUM_VAL(type->t);
  const char *code = NULL;
  if (type->t & VT_ARRAY) {
    CType *element = type;
    CType base;
    int dimensions = 0;
    while (element->t & VT_ARRAY) {
      ++dimensions;
      element = pointed_type(element);
      cv |= cpp_ms_cv(element->t);
    }
    if (mode == CPP_MS_TYPE_POINTEE) cstr_ccat(ctx->out, 'A');
    else if (mode == CPP_MS_TYPE_RESULT || mode == CPP_MS_TYPE_ESCAPE)
      cstr_cat(ctx->out, "$$B", 3);
    cstr_ccat(ctx->out, 'Y');
    cpp_ms_number(ctx, dimensions);
    for (element = type; element->t & VT_ARRAY; element = pointed_type(element))
      cpp_ms_number(ctx, element->ref->c);
    base = *element;
    if (cv & 1) base.t |= VT_CONSTANT;
    if (cv & 2) base.t |= VT_VOLATILE;
    cpp_ms_mangle_type(ctx, &base, CPP_MS_TYPE_ESCAPE);
    return;
  }
  if (mode == CPP_MS_TYPE_POINTEE) {
    if (bt == VT_FUNC) {
      cstr_ccat(ctx->out, '6');
      cpp_ms_mangle_function_type(ctx, type, 0, 1);
      return;
    }
    cstr_ccat(ctx->out, 'A' + cv);
  } else if (mode == CPP_MS_TYPE_ESCAPE && bt != VT_PTR && cv) {
    cstr_cat(ctx->out, "$$C", 3);
    cstr_ccat(ctx->out, 'A' + cv);
  } else if (mode == CPP_MS_TYPE_RESULT && ((bt != VT_PTR && cv) || is_tag)) {
    cstr_ccat(ctx->out, '?');
    cstr_ccat(ctx->out, 'A' + cv);
  }
  if (bt == VT_PTR) {
    CType *pointee = pointed_type(type);
    if (type->t & VT_RVALUE_REFERENCE) cstr_cat(ctx->out, "$$Q", 3);
    else if (type->t & VT_REFERENCE) cstr_ccat(ctx->out, 'A');
    else cstr_ccat(ctx->out, 'P' + cv);
#if defined(CPRIME_TARGET_X86_64)
    if ((pointee->t & VT_BTYPE) != VT_FUNC) cstr_ccat(ctx->out, 'E');
#endif
    cpp_ms_mangle_type(ctx, pointee, CPP_MS_TYPE_POINTEE);
    return;
  }
  if (bt == VT_FUNC) {
    cstr_cat(ctx->out, "$$A6", 4);
    cpp_ms_mangle_function_type(ctx, type, 0, 1);
    return;
  }
  if (is_tag) {
    int tag = bt == VT_STRUCT ? get_struct_type_name_tok(type)
              : type->ref ? type->ref->v & ~SYM_STRUCT : 0;
    const CppRecordDeclInfo *record = lookup_cpp_record_decl(tag);
    if (!tag) {
      tag = cpp_unnamed_tag_typedef_tok(type);
      if (!tag) cprime_error("unnamed type has no native linkage name");
      record = lookup_cpp_record_decl(tag);
    }
    cstr_cat(ctx->out, bt != VT_STRUCT ? "W4" : IS_UNION(type->t) ? "T"
                      : type->ref && type->ref->a.is_class_tag ? "V" : "U", -1);
    if (record && record->owner_tok)
      cpp_ms_mangle_qualified_name(ctx, tag, record->owner_tok, record->namespace_tok);
    else cpp_ms_mangle_qualified_name(ctx, tag, 0,
             record ? record->namespace_tok : symbol_namespace_scope_tok(tag));
    return;
  }
  if (CHARACTER_TYPE_KIND(type->t))
    code = CHARACTER_TYPE_KIND(type->t) == 1 ? "_W"
           : CHARACTER_TYPE_KIND(type->t) == 2 ? "_S" : "_U";
  else if (bt == VT_NULLPTR) code = "$$T";
  else switch (bt) {
    case VT_VOID: code = "X"; break;
    case VT_BYTE: code = !(type->t & VT_DEFSIGN) ? "D"
                         : type->t & VT_UNSIGNED ? "E" : "C"; break;
    case VT_SHORT: code = type->t & VT_UNSIGNED ? "G" : "F"; break;
    case VT_INT: code = type->t & VT_LONG
                    ? (type->t & VT_UNSIGNED ? "K" : "J")
                    : (type->t & VT_UNSIGNED ? "I" : "H"); break;
    case VT_LLONG: code = type->t & VT_UNSIGNED ? "_K" : "_J"; break;
    case VT_FLOAT: code = "M"; break;
    case VT_DOUBLE: code = type->t & VT_LONG ? "O" : "N"; break;
    case VT_LDOUBLE: code = "O"; break;
    case VT_BOOL: code = "_N"; break;
    case VT_QLONG: code = type->t & VT_UNSIGNED ? "_M" : "_L"; break;
  }
  if (!code) cprime_error("unsupported type in native C++ linkage");
  cstr_cat(ctx->out, code, -1);
}

static const char *cpp_ms_operator_code(int name_tok)
{
  static const char *names[] = {
    "operator new", "operator delete", "operator=", "operator>>", "operator<<",
    "operator!", "operator==", "operator!=", "operator[]", "operator->",
    "operator*", "operator++", "operator--", "operator-", "operator+",
    "operator&", "operator->*", "operator/", "operator%", "operator<",
    "operator<=", "operator>", "operator>=", "operator,", "operator()",
    "operator~", "operator^", "operator|", "operator&&", "operator||",
    "operator*=", "operator+=", "operator-=", "operator/=", "operator%=",
    "operator>>=", "operator<<=", "operator&=", "operator|=", "operator^=",
    "operator new[]", "operator delete[]"
  };
  static const char *codes[] = {
    "?2", "?3", "?4", "?5", "?6", "?7", "?8", "?9", "?A", "?C",
    "?D", "?E", "?F", "?G", "?H", "?I", "?J", "?K", "?L", "?M",
    "?N", "?O", "?P", "?Q", "?R", "?S", "?T", "?U", "?V", "?W",
    "?X", "?Y", "?Z", "?_0", "?_1", "?_2", "?_3", "?_4", "?_5",
    "?_6", "?_U", "?_V"
  };
  const char *name = get_tok_str(name_tok, NULL);
  int i;
  for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
    if (!strcmp(name, names[i])) return codes[i];
  return NULL;
}

static int cpp_ms_linkage_tok(int name_tok, CType *function_type)
{
  const CppMemberDeclInfo *member = lookup_cpp_member_decl(name_tok);
  TemplateInstOwner *instantiation = find_template_inst_owner_entry(name_tok);
  CppMsMangleContext context;
  CString label;
  const char *operator_code;
  int namespace_tok, source_tok = name_tok, result;
  memset(&context, 0, sizeof(context));
  cstr_new(&label);
  context.out = &label;
  cstr_ccat(&label, '?');
  if (member) {
    CType semantic_type = member->function_type;
    operator_code = member->kind == 1 ? "?0" : member->kind == 2 ? "?1"
                    : member->kind == 3 ? "?B"
                    : cpp_ms_operator_code(member->source_name_tok);
    if (member->is_template) {
      int source = operator_code ? tok_alloc_const(operator_code) : member->source_name_tok;
      cpp_ms_template_name(&context, source,
          (TemplateArgList *)&member->template_arguments, member->template_value_mask, 0, -1);
    } else if (operator_code) cstr_cat(&label, operator_code, -1);
    cpp_ms_mangle_qualified_name(&context,
        operator_code || member->is_template ? 0 : member->source_name_tok,
        member->owner_tok, 0);
    cstr_ccat(&label, "QIA"[member->access]
      + (member->is_static ? 2 : member->is_virtual ? 4 : 0));
    if (!member->is_static) {
#if defined(CPRIME_TARGET_X86_64)
      cstr_ccat(&label, 'E');
#endif
      if (member->ref_qualifier)
        cstr_ccat(&label, member->ref_qualifier == 1 ? 'G' : 'H');
      cstr_ccat(&label, 'A' + cpp_ms_cv(member->cv_qualifiers));
    }
    cpp_ms_mangle_function_type(&context, &semantic_type,
                                member->kind == 1 || member->kind == 2, 0);
  } else {
    FreeFuncOverload *overload;
    for (overload = free_func_overloads; overload; overload = overload->next)
      if (overload->mangled_tok == name_tok) {
        source_tok = overload->name_tok;
        break;
      }
    namespace_tok = instantiation ? instantiation->owner->namespace_tok
                                  : symbol_namespace_scope_tok(source_tok);
    operator_code = cpp_ms_operator_code(cpp_ms_unqualified_tok(source_tok, namespace_tok));
    if (operator_code) cstr_cat(&label, operator_code, -1);
    cpp_ms_mangle_qualified_name(&context, operator_code ? 0
      : instantiation ? name_tok : source_tok, 0, namespace_tok);
    cstr_ccat(&label, 'Y');
    cpp_ms_mangle_function_type(&context, function_type, 0, 0);
  }
  cstr_ccat(&label, 0);
  result = tok_alloc_const(label.data);
  cstr_free(&label);
  return result;
}
