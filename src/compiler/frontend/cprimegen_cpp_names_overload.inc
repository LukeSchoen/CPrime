static int make_current_class_nested_tok(int name_tok)
{
  if (nb_defining_class_stack <= 0 || name_tok < TOK_UIDENT)
    return name_tok;
  return make_static_member_tok(defining_class_stack[nb_defining_class_stack - 1],
                                name_tok);
}

static int is_current_class_qualified_nested_tok(int name_tok)
{
  const char *owner_name, *name;
  size_t owner_len, name_len;
  int class_i;

  if (nb_defining_class_stack <= 0 || name_tok < TOK_UIDENT)
    return 0;
  name = get_tok_str(name_tok, NULL);
  name_len = strlen(name);
  for (class_i = nb_defining_class_stack - 1; class_i >= 0; --class_i)
  {
    owner_name = get_tok_str(defining_class_stack[class_i], NULL);
    owner_len = strlen(owner_name);
    if (name_len > owner_len && !strncmp(name, owner_name, owner_len)
        && name[owner_len] == '_')
      return 1;
  }
  return 0;
}

/* A member definition keeps the lexical scopes of its owning class, even
   when replayed after all enclosing class definitions have finished. */
static int find_class_scope_type_tok(int class_tok, int name_tok)
{
  while (class_tok)
  {
    const CppRecordDeclInfo *record;
    int nested_tok;
    Sym *alias;
    if (class_tok_matches_unqualified_name(class_tok, name_tok))
      return class_tok;
    nested_tok = make_static_member_tok(class_tok, name_tok);
    if (find_class_template_def(nested_tok))
      return nested_tok;
    if (struct_find(nested_tok))
      return nested_tok;
    alias = global_symbol_find(nested_tok);
    if (alias && (alias->type.t & VT_TYPEDEF))
      return nested_tok;
    {
      ClassBaseInfo *base;
      for (base = class_base_candidates(class_tok); base; base = base->bucket_next)
        if (base->class_tok == class_tok)
        {
          int inherited = find_class_scope_type_tok(base->base_tok, name_tok);
          if (inherited != name_tok) return inherited;
        }
    }
    record = lookup_cpp_record_decl(class_tok);
    class_tok = record ? record->lexical_owner_tok : 0;
  }
  return name_tok;
}

static int find_current_class_nested_type_tok(int name_tok)
{
  int class_i, found;
  if (name_tok < TOK_UIDENT)
    return name_tok;
  {
    Sym *local = sym_find(name_tok);
    if (local && local->sym_scope)
      return name_tok;
  }
  found = find_class_scope_type_tok(active_member_class_tok, name_tok);
  if (found != name_tok) return found;
  for (class_i = nb_defining_class_stack - 1; class_i >= 0; --class_i)
  {
    found = find_class_scope_type_tok(defining_class_stack[class_i], name_tok);
    if (found != name_tok) return found;
  }
  return name_tok;
}

static int make_namespace_tok_from_parts(const int *parts, int nb_parts)
{
  int i = 0, result;
  if (nb_parts <= 0) return 0;
  if (!strncmp(get_tok_str(parts[0], NULL), "__cpc_ns_", 9))
    result = parts[i++];
  else {
    if (!cpp_namespace_prefix_tok) cpp_namespace_prefix_tok = tok_alloc_const("__cpc_ns");
    result = cpp_namespace_prefix_tok;
  }
  for (; i < nb_parts; ++i) {
    result = make_member_func_tok(result, parts[i]);
    if (is_namespace_tok(result)) {
      Sym *alias = global_symbol_find(result);
      if (alias && alias->cpp_using_target) result = alias->cpp_using_target;
    }
  }
  return result;
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

static void note_current_namespace_scope(void)
{
  int name = make_namespace_tok_from_parts(namespace_stack, nb_namespace_stack);
  int i;
  NamespaceScope *scope;
  for (i = 0; i < nb_namespace_scopes; ++i)
    if (namespace_scopes[i].name_tok == name)
      return;
  namespace_scopes = cprime_realloc(namespace_scopes,
                       (nb_namespace_scopes + 1) * sizeof(*namespace_scopes));
  scope = &namespace_scopes[nb_namespace_scopes++];
  scope->name_tok = name;
  scope->depth = nb_namespace_stack;
  scope->parent_tok = nb_namespace_stack > 1
    ? make_namespace_tok_from_parts(namespace_stack, nb_namespace_stack - 1) : 0;
  scope->is_inline = 0;
  memcpy(scope->parts, namespace_stack, sizeof(namespace_stack));
}

static int namespace_is_inline_child(int child, int parent)
{
  int i;
  while (child != parent && child) {
    for (i = 0; i < nb_namespace_scopes; ++i)
      if (namespace_scopes[i].name_tok == child) break;
    if (i == nb_namespace_scopes || !namespace_scopes[i].is_inline) return 0;
    child = namespace_scopes[i].parent_tok;
  }
  return child == parent;
}

static int symbol_namespace_scope_tok(int symbol_tok)
{
  const CppRecordDeclInfo *record = lookup_cpp_record_decl(symbol_tok);
  if (record && record->namespace_tok) return record->namespace_tok;
  const char *name = get_tok_str(symbol_tok, NULL);
  size_t name_len = strlen(name), best_len = 0;
  NamespaceScope *best = NULL;
  int i;
  if (strncmp(name, "__cpc_ns_", 9))
    return 0;
  for (i = 0; i < nb_namespace_scopes; ++i)
  {
    NamespaceScope *scope = &namespace_scopes[i];
    const char *prefix = get_tok_str(scope->name_tok, NULL);
    size_t len = strlen(prefix);
    if (len > best_len && name_len > len && name[len] == '_'
        && !strncmp(name, prefix, len))
      best = scope, best_len = len;
  }
  return best ? best->name_tok : 0;
}

static int current_namespace_scope_tok(void)
{
  return make_namespace_tok_from_parts(namespace_stack, nb_namespace_stack);
}

static int symbol_has_internal_namespace_linkage(int symbol_tok)
{
  int namespace_tok, i, part;
  if (!unnamed_namespace_tok || symbol_tok < TOK_UIDENT)
    return 0;
  namespace_tok = symbol_namespace_scope_tok(symbol_tok);
  for (i = 0; i < nb_namespace_scopes; ++i)
    if (namespace_scopes[i].name_tok == namespace_tok)
      for (part = 0; part < namespace_scopes[i].depth; ++part)
        if (namespace_scopes[i].parts[part] == unnamed_namespace_tok)
          return 1;
  return 0;
}

static void enter_namespace_scope(int namespace_tok)
{
  int i;
  nb_namespace_stack = 0;
  for (i = 0; i < nb_namespace_scopes; ++i)
    if (namespace_scopes[i].name_tok == namespace_tok)
    {
      nb_namespace_stack = namespace_scopes[i].depth;
      memcpy(namespace_stack, namespace_scopes[i].parts, sizeof(namespace_stack));
      return;
    }
}

static void enter_symbol_namespace(int symbol_tok)
{
  int namespace_tok = symbol_namespace_scope_tok(symbol_tok);
  if (namespace_tok)
    enter_namespace_scope(namespace_tok);
}

static int namespace_member_exists(int t)
{
  int i;
  Sym *identifier;
  for (i = 0; i < nb_namespace_scopes; ++i)
    if (namespace_scopes[i].name_tok == t)
      return 1;
  identifier = sym_find(t);
  if (!identifier) identifier = global_symbol_find(t);
  return (identifier && !identifier->cpp_hidden_friend) || struct_find(t)
         || find_class_template_def(t) || find_function_template_def(t);
}

static int find_namespace_tok_from_parts(const int *parts, int nb_parts)
{
  int t = make_namespace_tok_from_parts(parts, nb_parts);
  int expanded[18], i, candidate;
  if (nb_parts == 1 && namespace_member_exists(parts[0])) t = parts[0];
  if (nb_inline_namespace_scopes && nb_parts) {
    int parent = nb_parts > 1 ? make_namespace_tok_from_parts(parts, nb_parts - 1) : 0;
    int found = namespace_member_exists(t) ? t : 0;
    for (i = 0; i < nb_namespace_scopes; ++i) {
        NamespaceScope *scope = &namespace_scopes[i];
        if (scope->name_tok == parent || !namespace_is_inline_child(scope->name_tok, parent)) continue;
        candidate = make_static_member_tok(scope->name_tok, parts[nb_parts - 1]);
        if (namespace_member_exists(candidate)) {
          if (found && found != candidate) {
            Sym *a = global_symbol_find(found), *b = global_symbol_find(candidate);
            int functions = (find_function_template_def(found)
                || (a && (a->type.t & VT_BTYPE) == VT_FUNC))
              && (find_function_template_def(candidate)
                || (b && (b->type.t & VT_BTYPE) == VT_FUNC));
            if (!functions && !(a && b && a->cpp_using_target
                && a->cpp_using_target == b->cpp_using_target))
              cprime_error("ambiguous inline namespace member '%s'", get_tok_str(parts[nb_parts - 1], NULL));
          } else found = candidate;
        }
      }
    if (found) return found;
  }
  if (!unnamed_namespace_tok || namespace_member_exists(t) || nb_parts < 1)
    return t;
  /* An unnamed namespace supplies an implicit using-directive in its parent,
     including for qualified lookup into that parent. */
  for (i = 0; i < nb_parts - 1; ++i)
    expanded[i] = parts[i];
  expanded[nb_parts - 1] = unnamed_namespace_tok;
  expanded[nb_parts] = parts[nb_parts - 1];
  candidate = make_namespace_tok_from_parts(expanded, nb_parts + 1);
  return namespace_member_exists(candidate) ? candidate : t;
}

static int find_current_namespace_tok(int name_tok)
{
  int t, i, depth, parts[17];
  Sym *local;

  if (name_tok < TOK_UIDENT)
    return name_tok;
  local = sym_find(name_tok);
  if (local && local->cpp_using_target)
    return local->cpp_using_target;
  if (local && local->sym_scope)
    return name_tok;
  for (i = 0; i < nb_namespace_stack; ++i)
    parts[i] = namespace_stack[i];
  for (depth = nb_namespace_stack; depth >= 0; --depth)
  {
    parts[depth] = name_tok;
    t = depth || nb_inline_namespace_scopes
      ? find_namespace_tok_from_parts(parts, depth + 1) : name_tok;
    if (namespace_member_exists(t))
      return t;
    if (!depth && unnamed_namespace_tok)
    {
      int anonymous_parts[2] = { unnamed_namespace_tok, name_tok };
      t = make_namespace_tok_from_parts(anonymous_parts, 2);
      if (namespace_member_exists(t))
        return t;
    }
  }
  for (i = nb_using_namespace_toks - 1; i >= 0; --i)
  {
    t = make_static_member_tok(using_namespace_toks[i], name_tok);
    if (sym_find(t) || global_symbol_find(t) || struct_find(t)
        || find_class_template_def(t) || find_function_template_def(t))
      return t;
  }
  return name_tok;
}

static int find_namespace_qualifier_tok(int name_tok)
{
  int parts[17], depth, i, candidate;
  for (i = 0; i < nb_namespace_stack; ++i) parts[i] = namespace_stack[i];
  for (depth = nb_namespace_stack; depth >= 0; --depth) {
    parts[depth] = name_tok;
    candidate = make_namespace_tok_from_parts(parts, depth + 1);
    if (is_namespace_tok(candidate)) {
      Sym *alias = global_symbol_find(candidate);
      if (alias && alias->cpp_using_target) return alias->cpp_using_target;
    }
    for (i = 0; i < nb_namespace_scopes; ++i)
      if (namespace_scopes[i].name_tok == candidate) return candidate;
  }
  return name_tok;
}

static Sym *find_namespace_or_plain_symbol(int *ptok)
{
  int t = *ptok;
  Sym *s;

  s = sym_find(t);
  if (active_member_class_tok && (!s || sym_scope_ex(s) == 0))
  {
    int owner_tok = 0;
    Sym *member = find_static_member_by_class_try(active_member_class_tok,
                                                  t, &owner_tok);
    /* Out-of-class definitions still perform lookup in their class scope.
       A local declaration hides a member; a namespace declaration does not. */
    if (member && !symbol_is_nonstatic_member_of_class(member, owner_tok))
    {
      s = member;
      t = member->v & ~SYM_FIELD;
    }
  }
  if (!s)
  {
    t = find_current_namespace_tok(t);
    if (t != *ptok)
      s = sym_find(t);
  }
  if (!s)
    s = global_symbol_find(t);
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

static int cpp_unnamed_tag_typedef_tok(CType *type);

static int append_type_mangle_string(CString *name, CType *type)
{
  int bt = type->t & VT_BTYPE;
  CType pt;
  char part[128];

  if (type->t & VT_CONSTANT) cstr_cat(name, "_const", -1);
  if (type->t & VT_VOLATILE) cstr_cat(name, "_volatile", -1);

  if (bt == VT_PTR)
  {
    pt = *pointed_type(type);
    if (!append_type_mangle_string(name, &pt))
      return 0;
    if (type->t & VT_ARRAY) {
      /* Array references with different bounds are different template
         arguments. Their spelling must remain stable across deductions. */
      snprintf(part, sizeof(part), "_array%lld", (long long)type->ref->c);
      cstr_cat(name, part, -1);
    } else if (type->t & VT_RVALUE_REFERENCE)
      cstr_cat(name, "_rref", -1);
    else
      cstr_cat(name, (type->t & VT_REFERENCE) ? "_ref" : "_ptr", -1);
    return 1;
  }

  if (bt == VT_FUNC)
  {
    Sym *arg;
    cstr_cat(name, "_func", -1);
    if (!type->ref || !append_type_mangle_string(name,
                                          &type->ref->type))
      return 0;
    for (arg = type->ref->next; arg; arg = arg->next)
      if (!append_type_mangle_string(name, &arg->type))
        return 0;
    if (type->ref->f.func_type == FUNC_ELLIPSIS)
      cstr_cat(name, "_vararg", -1);
    return 1;
  }

  if (IS_ENUM(type->t) || IS_ENUM_VAL(type->t))
  {
    if (!type->ref) return 0;
    cstr_cat(name, "_enum_", -1);
    cstr_cat(name, get_tok_str(type->ref->v & ~SYM_STRUCT, NULL), -1);
    return 1;
  }
  if (CHARACTER_TYPE_KIND(type->t))
    pstrcpy(part, sizeof(part), CHARACTER_TYPE_NAME(type->t));
  else if (bt == VT_VOID)
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
  else if (bt == VT_NULLPTR)
    pstrcpy(part, sizeof(part), "nullptr_t");
  else if (bt == VT_LLONG)
    pstrcpy(part, sizeof(part), (type->t & VT_UNSIGNED) ? "ullong" : "llong");
  else if (bt == VT_FLOAT)
    pstrcpy(part, sizeof(part), "float");
  else if (bt == VT_DOUBLE)
    pstrcpy(part, sizeof(part), (type->t & VT_LONG) ? "ldouble" : "double");
  else if (bt == VT_LDOUBLE)
    pstrcpy(part, sizeof(part), "ldouble");
  else if (bt == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
    if (!struct_tok) struct_tok = cpp_unnamed_tag_typedef_tok(type);
    if (!struct_tok)
      return 0;
    cstr_cat(name, "_struct_", -1);
    cstr_cat(name, get_tok_str(struct_tok, NULL), -1);
    return 1;
  }
  else
    return 0;

  cstr_cat(name, "_", -1);
  cstr_cat(name, part, -1);
  return 1;
}

/* Bounded callers can choose a different stable identifier on overflow;
   never accept a truncated type spelling as an overload identity. */
static int append_type_mangle(char *name, int name_size, CType *type)
{
  CString suffix;
  int ok, len = strlen(name);
  cstr_new(&suffix);
  ok = append_type_mangle_string(&suffix, type);
  if (ok && len + suffix.size < name_size)
  {
    memcpy(name + len, suffix.data, suffix.size);
    name[len + suffix.size] = 0;
  }
  else
    ok = 0;
  cstr_free(&suffix);
  return ok;
}

static int make_parameter_mangled_tok(int base_tok, Sym *first_arg)
{
  CString name;
  Sym *arg;
  int result;
  cstr_new(&name);
  cstr_cat(&name, get_tok_str(base_tok, NULL), -1);
  if (!first_arg)
    cstr_cat(&name, "_void", -1);
  for (arg = first_arg; arg; arg = arg->next)
  {
    CType parameter = arg->type;
    if (!is_reference_type(&parameter))
      parameter.t &= ~(VT_CONSTANT | VT_VOLATILE);
    if (!append_type_mangle_string(&name, &parameter))
      cprime_error("unsupported parameter type %x for overloaded function '%s' (parameter '%s')", parameter.t, get_tok_str(base_tok, NULL), get_tok_str(arg->v & ~SYM_FIELD, NULL));
  }
  cstr_ccat(&name, 0);
  result = tok_alloc_const(name.data);
  cstr_free(&name);
  return result;
}

static int make_member_func_tok_for_type(int struct_tok, int method_tok,
                                          CType *func_type)
{
  int base_tok;

  base_tok = make_member_func_tok_with_cv(struct_tok, method_tok,
                                         func_type ? func_type->t : 0);
  if (!func_type || !func_type->ref || !func_type->ref->next)
  {
    MemberFuncOverload *o;
    for (o = member_func_candidates(struct_tok, method_tok);
         o; o = o->bucket_next)
      if (o->struct_tok == struct_tok && o->method_tok == method_tok
          && !o->is_using_declaration
          && o->explicit_arg_count == 0
          && o->func_type.ref->f.func_ref_qualifier ==
             ((func_type->t & VT_FUNC_RREF) ? 2 : (func_type->t & VT_FUNC_LREF) ? 1 : 0)
          && o->is_const == !!(func_type && (func_type->t & VT_CONSTANT)))
        return o->mangled_tok;
    if (sym_find(base_tok) || global_symbol_find(base_tok))
    {
      return make_parameter_mangled_tok(base_tok, NULL);
    }
    return base_tok;
  }
  if (!sym_find(base_tok) && !global_symbol_find(base_tok))
    return base_tok;
  return make_parameter_mangled_tok(base_tok, func_type->ref->next);
}

static int make_lifecycle_func_tok_for_type(int struct_tok, int method_tok,
                                            CType *lowered_type,
                                            int prefer_existing)
{
  int base_tok = make_member_func_tok(struct_tok, method_tok);
  Sym *arg;
  int same_count = 0, other_count = 0, same_mangled_tok = 0;
  int explicit_arg_count = member_func_explicit_arg_count(lowered_type);
  MemberFuncOverload *o;

  for (o = member_func_candidates(struct_tok, method_tok); o; o = o->bucket_next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && o->explicit_arg_count == explicit_arg_count)
    {
      /* An out-of-class definition must attach to its declaration even
         when several constructors have the same arity. Parameter types,
         including reference and cv levels, determine that identity. */
      if (o->func_type.ref && lowered_type && lowered_type->ref
          && same_lowered_member_func_signature(&o->func_type, lowered_type))
        return o->mangled_tok;
      same_count++;
      if (!same_mangled_tok)
        same_mangled_tok = o->mangled_tok;
    }
    else if (o->struct_tok == struct_tok && o->method_tok == method_tok)
      other_count++;
  if (same_count == 1 && (prefer_existing || explicit_arg_count == 0))
    return same_mangled_tok;
  if (!lowered_type || !lowered_type->ref || !lowered_type->ref->next)
    return base_tok;
  if (!same_count && !other_count && !sym_find(base_tok)
      && !global_symbol_find(base_tok))
    return base_tok;
  arg = lowered_type->ref->next;
  if (arg)
    arg = arg->next;
  return arg ? make_parameter_mangled_tok(base_tok, arg) : base_tok;
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

  note_cpp_member_decl(struct_tok, method_tok, mangled_tok, lowered_type, 0, 1);
  note_member_function_scope(mangled_tok, struct_tok);

  if (lowered_type && lowered_type->ref)
  {
    this_arg = lowered_type->ref->next;
    if (this_arg && (this_arg->type.t & VT_BTYPE) == VT_PTR)
      is_const = pointed_type(&this_arg->type)->t & VT_CONSTANT;
  }

  for (o = member_mangled_buckets[(unsigned)mangled_tok & (MEMBER_CANDIDATE_BUCKETS - 1)];
       o; o = o->mangled_next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && o->mangled_tok == mangled_tok)
    {
      /* Raw lifecycle registration establishes the mangled name before the
         lowered function type exists. Complete that entry once parameter
         defaults and the implicit-this type are available. */
      if (lowered_type && lowered_type->ref)
      {
        int new_explicit_arg_count =
          member_func_explicit_arg_count(lowered_type);
        int new_min_arg_count = member_func_min_arg_count(lowered_type);
        int preserve_decl_defaults = o->func_type.ref
          && o->explicit_arg_count == new_explicit_arg_count
          && o->min_arg_count < new_min_arg_count;
        o->explicit_arg_count = new_explicit_arg_count;
        if (!preserve_decl_defaults)
        {
          o->min_arg_count = new_min_arg_count;
          o->func_type = *lowered_type;
        }
        o->is_const = is_const != 0;
      }
      index_conversion_overload(o);
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
  index_member_func_overload(o);
  index_conversion_overload(o);
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

static void note_member_function_scope(int function_tok, int class_tok)
{
  unsigned bucket = (unsigned)function_tok & (MEMBER_SCOPE_BUCKETS - 1);
  MemberFunctionScope *scope;
  for (scope = member_function_scopes[bucket]; scope; scope = scope->next)
    if (scope->function_tok == function_tok)
    {
      scope->class_tok = class_tok;
      return;
    }
  scope = cprime_mallocz(sizeof(*scope));
  scope->function_tok = function_tok;
  scope->class_tok = class_tok;
  scope->next = member_function_scopes[bucket];
  member_function_scopes[bucket] = scope;
}

static void note_member_function_template(int function_tok, int class_tok,
                                           TemplateMemberDef *definition)
{
  MemberFunctionScope *scope;
  note_member_function_scope(function_tok, class_tok);
  for (scope = member_function_scopes[(unsigned)function_tok & (MEMBER_SCOPE_BUCKETS - 1)];
       scope; scope = scope->next)
    if (scope->function_tok == function_tok)
    {
      scope->is_function_template = 1;
      if (!scope->template_definition
          || token_string_has_top_level_body(definition->def_str)
          || !token_string_has_top_level_body(scope->template_definition->def_str))
        scope->template_definition = definition;
      return;
    }
}

static int member_function_is_template(int function_tok)
{
  MemberFunctionScope *scope;
  for (scope = member_function_scopes[(unsigned)function_tok & (MEMBER_SCOPE_BUCKETS - 1)];
       scope; scope = scope->next)
    if (scope->function_tok == function_tok)
      return scope->is_function_template;
  return 0;
}

static int member_function_class_tok(int function_tok)
{
  MemberFunctionScope *scope;
  for (scope = member_function_scopes[(unsigned)function_tok & (MEMBER_SCOPE_BUCKETS - 1)];
       scope; scope = scope->next)
    if (scope->function_tok == function_tok)
      return scope->class_tok;
  return 0;
}

static void note_free_func_overload(int name_tok, int mangled_tok,
                                    CType *func_type)
{
  FreeFuncOverload *o;


  for (o = free_func_candidates(name_tok); o; o = o->name_next)
  {
    if (o->name_tok == name_tok && o->mangled_tok == mangled_tok)
    {
      if (func_type && func_type->ref)
      {
        /* The mangled token is the signature identity.  A dependent replay
           can rediscover that token while its active arguments are in the
           opposite order; never replace the canonical signature with that
           incompatible transient type. */
        if (!o->func_type.ref
            || same_func_param_signature(&o->func_type, func_type))
        {
          o->explicit_arg_count = func_explicit_arg_count(func_type);
          o->min_arg_count = func_min_arg_count(func_type);
          o->func_type = *func_type;
        }
      }
      return;
    }
  }
  o = cprime_mallocz(sizeof(*o));
  o->name_tok = name_tok;
  o->registration_order = ++free_func_registration_order;
  o->mangled_tok = mangled_tok;
  o->explicit_arg_count = func_explicit_arg_count(func_type);
  o->min_arg_count = func_min_arg_count(func_type);
  if (func_type)
    o->func_type = *func_type;
  o->next = free_func_overloads;
  free_func_overloads = o;
  {
    unsigned bucket = (unsigned)name_tok & (MEMBER_CANDIDATE_BUCKETS - 1);
    o->name_next = free_name_buckets[bucket];
    free_name_buckets[bucket] = o;
    bucket = (unsigned)mangled_tok & (MEMBER_CANDIDATE_BUCKETS - 1);
    o->mangled_next = free_mangled_buckets[bucket];
    free_mangled_buckets[bucket] = o;
  }
}

static void mark_unmangled_free_func_overload_provisional(int name_tok)
{
  FreeFuncOverload *o;
  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == name_tok && o->mangled_tok == name_tok)
      o->is_provisional_auto_member = 1;
}

static int is_provisional_free_func_tok(int tok)
{
  FreeFuncOverload *o;
  for (o = free_func_candidates(tok); o; o = o->name_next)
    if (o->name_tok == tok && o->mangled_tok == tok
        && o->is_provisional_auto_member)
      return 1;
  return 0;
}

static int is_mangled_free_func_tok(int tok)
{
  FreeFuncOverload *o;
  for (o = free_mangled_buckets[(unsigned)tok & (MEMBER_CANDIDATE_BUCKETS - 1)];
       o; o = o->mangled_next)
    if (o->mangled_tok == tok && o->name_tok != tok) {
      Sym *binding = find_global_symbol(o->name_tok);
      if (!binding || !binding->cpp_using_target) return 1;
    }
  return 0;
}

static int has_free_func_overload(int name_tok)
{
  FreeFuncOverload *o;
  for (o = free_func_candidates(name_tok); o; o = o->name_next)
    if (o->name_tok == name_tok) return 1;
  return 0;
}

static int has_unresolved_free_func_overloads(int name_tok)
{
  FreeFuncOverload *overload;
  int first = 0;
  for (overload = free_func_candidates(name_tok); overload; overload = overload->name_next)
    if (overload->name_tok == name_tok) {
      if (first && first != overload->mangled_tok) return 1;
      first = overload->mangled_tok;
    }
  return 0;
}

static int is_member_func_mangled_tok(int tok)
{
  MemberFuncOverload *o;
  for (o = member_mangled_buckets[(unsigned)tok & (MEMBER_CANDIDATE_BUCKETS - 1)];
       o; o = o->mangled_next)
    if (o->mangled_tok == tok) return 1;
  return 0;
}

static int free_func_accepts_argument_count(FreeFuncOverload *overload, int count)
{
  return count >= overload->min_arg_count
      && (count <= overload->explicit_arg_count
          || (overload->func_type.ref
              && overload->func_type.ref->f.func_type == FUNC_ELLIPSIS));
}

static int free_func_overload_exists_for_arg_count(int name_tok,
                                                   int explicit_arg_count)
{
  FreeFuncOverload *o;

  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == name_tok
        && free_func_accepts_argument_count(o, explicit_arg_count))
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

  if (cpp_translation_unit >= 0)
    return cpp_translation_unit;

  if (cprime_state && cprime_state->include_stack[0])
    root_file = cprime_state->include_stack[0];
  if (!root_file)
    root_file = file;
  filename = root_file ? root_file->filename : NULL;
  return cpp_translation_unit = has_suffix(filename, ".cpp")
         || has_suffix(filename, ".cxx")
         || has_suffix(filename, ".cc")
         || has_suffix(filename, ".C");
}

static int make_free_func_tok_for_type_ex(int name_tok, CType *func_type,
                                          int allow_member_base)
{
  Sym *base;

  if (!func_type || (func_type->t & VT_BTYPE) != VT_FUNC || !func_type->ref)
    return name_tok;
  if (!is_cpp_translation_unit())
    return name_tok;
  {
    const char *fn = get_tok_str(name_tok, NULL);
    if (fn[0] == '_' && fn[1] == '_' && strncmp(fn, "__cpc_ns_", 9))
      return name_tok;
  }
  if ((!allow_member_base && is_member_func_mangled_tok(name_tok))
      || is_provisional_free_func_tok(name_tok)
      || is_mangled_free_func_tok(name_tok))
    return name_tok;

  base = find_global_symbol(name_tok);
  if (!base && !has_free_func_overload(name_tok))
    return name_tok;
  if (base && (base->type.t & VT_BTYPE) == VT_FUNC
      && same_func_param_signature(&base->type, func_type))
    return name_tok;
  /* Static class-template members publish their concrete overload before
     installing an unmangled lookup alias.  That alias is not a second
     overload and its provisional dependent type must not participate in
     resolution.  Ordinary first overloads reach here before any overload is
     registered and still need to be recorded. */
  if (base && (base->type.t & VT_BTYPE) == VT_FUNC
      && !(allow_member_base && is_member_func_mangled_tok(name_tok)))
    note_free_func_overload(name_tok, name_tok, &base->type);

  return make_parameter_mangled_tok(name_tok, func_type->ref->next);
}

static int make_free_func_tok_for_type(int name_tok, CType *func_type)
{
  return make_free_func_tok_for_type_ex(name_tok, func_type, 0);
}

static int make_static_member_func_tok_for_type(int name_tok, CType *func_type)
{
  return make_free_func_tok_for_type_ex(name_tok, func_type, 1);
}

/* Source lookup tokens and object-file linkage are separate identities.
   The overload registry may keep the first source declaration under its
   lookup token; every emitted C++ free function nevertheless gets the same
   signature-based label in every translation unit. */
static int cpp_unnamed_tag_typedef_tok(CType *type)
{
  Sym *alias;
  int name_tok = 0;
  if (!type->ref) return 0;
  if (type->ref->linkage_typedef_tok) return type->ref->linkage_typedef_tok;
  /* Retained symbols are newest first. Only cache a successful lookup:
     an unnamed tag can be inspected before its first typedef is declared. */
  for (alias = global_stack; alias; alias = alias->prev)
    if ((alias->type.t & VT_TYPEDEF) && alias->type.ref == type->ref
        && (alias->type.t & VT_BTYPE) == (type->t & VT_BTYPE)
        && alias->v >= TOK_UIDENT && alias->v < SYM_FIRST_ANOM)
      name_tok = alias->v;
  type->ref->linkage_typedef_tok = name_tok;
  return name_tok;
}

static int cpp_append_linkage_type(CString *out, CType *type)
{
  CType plain = *type;
  int bt = type->t & VT_BTYPE;
  int name_tok = 0;
  char number[40];
  CppMemberPointerType *member = find_cpp_member_pointer_type(type);
  if (type->t & VT_CONSTANT) cstr_cat(out, "K", 1);
  if (type->t & VT_VOLATILE) cstr_cat(out, "V", 1);
  plain.t &= ~(VT_CONSTANT | VT_VOLATILE);
  if (member) {
    const char *owner = get_tok_str(member->owner_tok, NULL);
    snprintf(number, sizeof(number), "M%u_", (unsigned)strlen(owner));
    cstr_cat(out, number, -1);
    cstr_cat(out, owner, -1);
    return cpp_append_linkage_type(out, &member->member_type);
  }
  if (bt == VT_PTR) {
    if (type->t & VT_ARRAY) {
      snprintf(number, sizeof(number), "A%d_", type->ref->c);
      cstr_cat(out, number, -1);
    } else
      cstr_cat(out, (type->t & VT_RVALUE_REFERENCE) ? "O"
                        : (type->t & VT_REFERENCE) ? "R" : "P", 1);
    return type->ref && cpp_append_linkage_type(out, pointed_type(type));
  }
  if (bt == VT_FUNC) {
    Sym *arg;
    if (!type->ref) return 0;
    cstr_cat(out, "F", 1);
    snprintf(number, sizeof(number), "%u_", type->ref->f.func_call);
    cstr_cat(out, number, -1);
    if (type->ref->f.func_noexcept) cstr_cat(out, "N", 1);
    if (!cpp_append_linkage_type(out, &type->ref->type)) return 0;
    cstr_cat(out, "B", 1);
    for (arg = type->ref->next; arg; arg = arg->next) {
      CType parameter = arg->type;
      if (!is_reference_type(&parameter))
        parameter.t &= ~(VT_CONSTANT | VT_VOLATILE);
      if (!cpp_append_linkage_type(out, &parameter)) return 0;
    }
    cstr_cat(out, type->ref->f.func_type == FUNC_ELLIPSIS ? "ZE" : "E", -1);
    return 1;
  }
  if (IS_ENUM(type->t) || IS_ENUM_VAL(type->t)) {
    if (type->ref) name_tok = type->ref->v & ~SYM_STRUCT;
    cstr_cat(out, "E", 1);
  } else if (bt == VT_STRUCT) {
    name_tok = get_struct_type_name_tok(type);
    if (!name_tok) name_tok = cpp_unnamed_tag_typedef_tok(type);
    cstr_cat(out, "S", 1);
  } else {
    cstr_cat(out, "T", 1);
    if (!append_type_mangle_string(out, &plain)) return 0;
    cstr_cat(out, ";", 1);
    return 1;
  }
  if (!name_tok) return 0;
  {
    const char *name = get_tok_str(name_tok, NULL);
    snprintf(number, sizeof(number), "%u_", (unsigned)strlen(name));
    cstr_cat(out, number, -1);
    cstr_cat(out, name, -1);
  }
  return 1;
}

#include "cprimegen_microsoft_mangle.inc"

static int cpp_free_function_linkage_label(int name_tok, CType *func_type)
{
  CString label, parameters;
  char number[32];
  Sym *arg;
  const char *name = get_tok_str(name_tok, NULL);
  int result;

  cstr_new(&label);
  cstr_new(&parameters);
  cstr_cat(&label, "__cpc_cpp_", -1);
  snprintf(number, sizeof(number), "%u_", (unsigned)strlen(name));
  cstr_cat(&label, number, -1);
  cstr_cat(&label, name, -1);
  for (arg = func_type->ref->next; arg; arg = arg->next) {
    CType parameter = arg->type;
    parameters.size = 0;
    if (!is_reference_type(&parameter))
      parameter.t &= ~(VT_CONSTANT | VT_VOLATILE);
    if (!cpp_append_linkage_type(&parameters, &parameter))
      cprime_error("unsupported C++ function parameter linkage for '%s'", name);
    snprintf(number, sizeof(number), "_%u_", (unsigned)parameters.size);
    cstr_cat(&label, number, -1);
    cstr_cat(&label, parameters.data, parameters.size);
  }
  cstr_cat(&label, func_type->ref->f.func_type == FUNC_ELLIPSIS
                    ? "_variadic" : "_end", -1);
  cstr_ccat(&label, 0);
  result = tok_alloc_const(label.data);
  cstr_free(&parameters);
  cstr_free(&label);
  return result;
}

static void cpp_prepare_free_function_linkage(int name_tok, CType *func_type,
                                             AttributeDef *ad)
{
  Sym *prior = find_global_symbol(name_tok);
  FreeFuncOverload *overload, *by_name, *by_symbol;
  int source_name = name_tok;
  struct FuncAttr *attr = &func_type->ref->f;

  if (!is_cpp_translation_unit() || is_member_func_mangled_tok(name_tok)
      || lookup_cpp_member_decl(name_tok)
      || is_provisional_free_func_tok(name_tok)
      /* Template instantiations already carry their declaration identity and
         concrete template arguments in their existing linkage token. */
      || find_template_inst_owner_entry(name_tok))
    return;
  if (prior && (prior->type.t & VT_BTYPE) == VT_FUNC
      && prior->type.ref->f.func_language_linkage == 1
      && attr->func_language_linkage == 1
      && !same_func_param_signature(&prior->type, func_type))
    cprime_error("conflicting C function declaration for '%s'",
                 get_tok_str(name_tok, NULL));
  /* Merge the two relevant indexes in registration order. A using declaration
     can map a symbol back to another source name partway through the walk;
     preserve that transition without visiting unrelated declarations. */
  by_name = free_func_candidates(source_name);
  by_symbol = free_mangled_buckets[(unsigned)name_tok & (MEMBER_CANDIDATE_BUCKETS - 1)];
  for (;;) {
    while (by_name && by_name->name_tok != source_name)
      by_name = by_name->name_next;
    while (by_symbol && by_symbol->mangled_tok != name_tok)
      by_symbol = by_symbol->mangled_next;
    if (!by_name && !by_symbol) break;
    overload = !by_symbol || (by_name
                 && by_name->registration_order > by_symbol->registration_order)
                 ? by_name : by_symbol;
    if (by_name == overload) by_name = by_name->name_next;
    if (by_symbol == overload) by_symbol = by_symbol->mangled_next;
    if (profile_scans_enabled) ++profile_linkage_scans;
    {
      Sym *binding = find_global_symbol(overload->name_tok);
      /* Imports add lookup candidates without changing the declaration's
         owning namespace or its object-file identity. */
      if (binding && binding->cpp_using_target) continue;
    }
    if (overload->mangled_tok == name_tok && source_name != overload->name_tok) {
      source_name = overload->name_tok;
      by_name = free_func_candidates(source_name);
      while (by_name && by_name->registration_order >= overload->registration_order)
        by_name = by_name->name_next;
    }
    if (overload->name_tok == source_name
        && same_func_param_signature(&overload->func_type, func_type)) {
      Sym *candidate = find_global_symbol(overload->mangled_tok);
      if (candidate)
        prior = candidate;
    }
  }
  if (prior && (prior->type.t & VT_BTYPE) == VT_FUNC
      && same_func_param_signature(&prior->type, func_type)
      && prior->type.ref->f.func_language_linkage) {
    int previous_linkage = prior->type.ref->f.func_language_linkage;
    if (attr->func_linkage_explicit
        && previous_linkage != attr->func_language_linkage)
      cprime_error("conflicting language linkage for '%s'",
                   get_tok_str(source_name, NULL));
    attr->func_language_linkage = previous_linkage;
    ad->f.func_language_linkage = previous_linkage;
  }
  if (ad->asm_label)
    return;
  if (attr->func_language_linkage == 1) {
    int namespace_tok = symbol_namespace_scope_tok(source_name);
    const char *name = get_tok_str(source_name, NULL);
    if (namespace_tok)
      name += strlen(get_tok_str(namespace_tok, NULL)) + 1;
    ad->asm_label = tok_alloc_const(name);
  } else if (attr->func_language_linkage == 2
             && source_name != tok_alloc_const("main")) {
#ifdef CPRIME_TARGET_PE
    ad->asm_label = cpp_ms_linkage_tok(source_name, func_type);
#else
    ad->asm_label = cpp_free_function_linkage_label(source_name, func_type);
#endif
  }
}

static int prepare_free_func_declarator(int name_tok, CType *func_type,
                                      AttributeDef *ad)
{
  int mangled_tok;
  Sym *base;

  cpp_prepare_free_function_linkage(name_tok, func_type, ad);

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
  else if (!strncmp(get_tok_str(name_tok, NULL), "operator", 8))
    note_free_func_overload(name_tok, name_tok, func_type);
  return mangled_tok;
}

static int default_arg_is_empty_braces(TokenString *str)
{
  int i, syntax[3], n = 0;

  if (!str)
    return 0;
  for (i = 0; i < str->len && str->str[i] != TOK_EOF; ++i)
  {
    int t = str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      continue;
    }
    if (n >= 3)
      return 0;
    syntax[n++] = t;
  }
  return n == 2 && syntax[0] == '{' && syntax[1] == '}';
}

static void emit_default_arg(Sym *func_type, Sym *arg)
{
  int entry_tok = tok;
  CValue entry_tokc = tokc;
  TokenString macro;
  TokenString *value_init = NULL;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;

  if (!arg || !arg->default_arg)
  {
    cprime_error("too few arguments to function");
  }
  if (default_arg_is_empty_braces(arg->default_arg))
  {
    CType value_type = arg->type;
    int struct_tok;
    int saved_tok = tok;
    CValue saved_tokc = tokc;
    if (is_reference_type(&value_type))
      value_type = *pointed_type(&value_type);
    struct_tok = get_struct_type_name_tok(&value_type);
    if (struct_tok)
    {
      value_init = tok_str_alloc();
      tok_str_add(value_init, struct_tok);
      tok_str_add(value_init, '(');
      tok_str_add(value_init, ')');
      tok_str_add(value_init, TOK_EOF);
      begin_macro(value_init, 1);
      next();
      expr_eq();
    }
    else
    {
      CValue zero;
      memset(&zero, 0, sizeof zero);
      vsetc(&value_type, VT_CONST, &zero);
    }
    while (macro_stack && macro_stack != saved_macro_stack)
      end_macro();
    macro_ptr = saved_macro_ptr;
    tok = saved_tok;
    tokc = saved_tokc;
    gfunc_param_typed(func_type, arg);
    return;
  }
  macro = *arg->default_arg;
  begin_macro(&macro, 0);
  next();
  expr_eq();
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = entry_tok;
  tokc = entry_tokc;
  gfunc_param_typed_with_conversions(func_type, arg);
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
    if (tok == '}')
      break;
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
      prepare_saved_cpp_lambdas(args[n]);
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

/* Since C++11 only an integer literal zero (possibly parenthesized),
   rather than an arbitrary zero-valued integral expression, is a null
   pointer constant. Character zero and explicit casts retain their type. */
static int saved_arg_is_null_literal(TokenString *arg)
{
  int i, depth=0, seen=0;
  if (!arg || !arg->str) return 0;
  for (i=0;i<arg->len;++i) {
    int token=arg->str[i];
    if(token==TOK_LINENUM){++i;continue;}
    if(token==TOK_EOF || !token) return seen && depth==0;
    if(token=='(' && !seen){++depth;continue;}
    if(token==')' && seen && depth){--depth;continue;}
    if(!seen && token==TOK_GNU_NULL){seen=1;continue;}
    if(!seen && (token==TOK_CINT || token==TOK_CUINT
                 || token==TOK_CLLONG || token==TOK_CULLONG)) {
      seen=1;i+=tok_str_value_extra_words(arg->str,arg->len,i);continue;
    }
    if(!seen && token>=TOK_UIDENT && !strcmp(get_tok_str(token,NULL),"nullptr")) {
      seen=1;continue;
    }
    return 0;
  }
  return seen && depth==0;
}

static int saved_arg_is_functional_temporary(TokenString *arg)
{
  int i = 0, depth = 0;

  if (!arg || !arg->str)
    return 0;
  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= arg->len || arg->str[i] < TOK_UIDENT)
    return 0;
  ++i;
  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= arg->len || arg->str[i] != '(')
    return 0;
  /* This shortcut describes the whole expression, not just its first
     primary expression.  A member call, subscript, or operator after the
     temporary can have a different type and value category. */
  for (; i < arg->len; ++i)
  {
    int token = arg->str[i];
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(arg->str, arg->len, i);
      continue;
    }
    if (token == '(')
      ++depth;
    else if (token == ')' && !--depth)
    {
      ++i;
      while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
        i += 2;
      return i == arg->len || arg->str[i] == TOK_EOF || !arg->str[i];
    }
  }
  return 0;
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

  if (!saved_arg_is_functional_temporary(arg))
    return 0;
  type_tok = arg->str[i++];
  while (i + 1 < arg->len && arg->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= arg->len || arg->str[i] != '(')
    return 0;

  s = sym_find(type_tok);
  if (!s)
    s = global_symbol_find(type_tok);
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

static void infer_saved_arg_type_and_category(TokenString *arg, CType *type,
                                               int *is_class_prvalue)
{
  SValue *saved_vtop = vtop;
  int saved_nocode_wanted = nocode_wanted;
  int saved_tok = tok;
  CValue saved_tokc = tokc;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;
  int is_null_pointer_constant;
  TokenString macro;
  SValue saved_values[VSTACK_SIZE];
  struct temp_local_variable saved_temps[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
  int value_count = (int)(vtop - vstack + 1);
  int saved_loc = loc, saved_temp_count = nb_temp_local_vars;

  if (is_class_prvalue)
    *is_class_prvalue = 0;

  if (saved_arg_is_braced(arg))
  {
    type->t = VT_BRACED_LIST;
    type->ref = sym_push2(&global_stack, SYM_FIELD, VT_VOID, 0);
    type->ref->default_arg = arg;
    return;
  }
  if (saved_arg_functional_temporary_type(arg, type))
  {
    if (is_class_prvalue)
      *is_class_prvalue = 1;
    return;
  }

  prepare_saved_cpp_lambdas(arg);
  /* A semantic call probe can spill values already on the evaluation stack.
     Restore their descriptors as well as the stack pointer: those spills
     emit no code and cannot become the enclosing expression's live values. */
  memcpy(saved_values, vstack, value_count * sizeof(SValue));
  memcpy(saved_temps, arr_temp_local_vars, sizeof(saved_temps));
  nocode_wanted++;
  /* end_macro() resets a non-owning macro buffer.  Replay a shallow view so
     overload inference does not consume the saved argument that emission
     still needs. */
  macro = *arg;
  begin_macro(&macro, 0);
  next();
  expr_eq();
  is_null_pointer_constant =
    (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
    && is_integer_btype(vtop->type.t & VT_BTYPE) && vtop->c.i == 0
    && saved_arg_is_null_literal(arg);
  *type = vtop->type;
  if (is_class_prvalue)
    *is_class_prvalue = (vtop->r & VT_CXX_PRVALUE) != 0;
  if (!is_reference_type(type)
      && ((!(vtop->r & VT_LVAL) && !(type->t & VT_ARRAY)
       && (type->t & VT_BTYPE) != VT_FUNC)
      || (((type->t & VT_BTYPE) == VT_STRUCT)
          && (vtop->r & VT_CXX_PRVALUE))))
    type->t |= VT_RVALUE_REFERENCE;
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  if (is_null_pointer_constant)
    type->t |= VT_NULLPTR_TYPE;
  vtop = saved_vtop;
  memcpy(vstack, saved_values, value_count * sizeof(SValue));
  memcpy(arr_temp_local_vars, saved_temps, sizeof(saved_temps));
  loc = saved_loc;
  nb_temp_local_vars = saved_temp_count;
  nocode_wanted = saved_nocode_wanted;
  tok = saved_tok;
  tokc = saved_tokc;
}

static void infer_saved_arg_type(TokenString *arg, CType *type)
{
  infer_saved_arg_type_and_category(arg, type, NULL);
}

static void infer_saved_arg_types(TokenString **args, CType *types, int nb_args)
{
  int i;

  for (i = 0; i < nb_args; ++i)
    infer_saved_arg_type(args[i], &types[i]);
}

static void parse_typed_function_argument(Sym *param)
{
  CType type;
  if (tok == '{' && param && is_cpp_translation_unit())
  {
    type = param->type;
    if (is_reference_type(&type)) type = *pointed_type(&type);
    if ((type.t & VT_BTYPE) == VT_STRUCT || (type.t & VT_ARRAY))
    {
      materialize_braced_temporary(&type);
      return;
    }
  }
  expr_eq();
}

static TokenString *saved_list_direct_reference_argument(CType *parameter,
                                                         TokenString *argument,
                                                         CType *source)
{
  TokenString **elements, *element = NULL;
  int count;
  if (!is_reference_type(parameter)) return NULL;
  count = split_saved_array_initializer(argument, &elements);
  if (count == 1 && !saved_arg_is_braced(elements[0])) {
    CType value;
    infer_saved_arg_type(elements[0], source);
    value = is_reference_type(source) ? *pointed_type(source) : *source;
    value.t &= ~VT_RVALUE_REFERENCE;
    if (is_compatible_unqualified_types(pointed_type(parameter), &value)) {
      element = elements[0];
      count = 0; /* Transfer the sole token string; free only its array. */
    }
  }
  free_saved_array_elements(elements, count);
  return element;
}

static int emit_saved_arg_for_param(TokenString *arg, Sym *param)
{
  int saved_tok = tok;
  CValue saved_tokc = tokc;
  if (saved_arg_is_braced(arg) && param
      && (((param->type.t & VT_BTYPE) == VT_STRUCT) || (param->type.t & VT_ARRAY)
          || (is_reference_type(&param->type)
              && (((pointed_type(&param->type)->t & VT_BTYPE) == VT_STRUCT)
                  || (pointed_type(&param->type)->t & VT_ARRAY)))))
  {
    CType temp_type;
    CType source;
    TokenString *direct = saved_list_direct_reference_argument(&param->type,
                                                               arg, &source);
    TokenString macro;
    TokenString *saved_macro_stack = macro_stack;
    const int *saved_macro_ptr = macro_ptr;

    temp_type = is_reference_type(&param->type) ? *pointed_type(&param->type)
                                               : param->type;
    macro = direct ? *direct : *arg;
    begin_macro(&macro, 0);
    next();
    if (direct)
      expr_eq();
    else
      materialize_braced_temporary_mode(&temp_type, 1);
    while (macro_stack && macro_stack != saved_macro_stack)
      end_macro();
    macro_ptr = saved_macro_ptr;
    tok = saved_tok;
    tokc = saved_tokc;
    if (direct) tok_str_free(direct);
    return 1;
  }

  {
  /* Saved arguments remain owned by their caller, including the braced
     path above. Replaying a clause must not destroy its token storage. */
  TokenString macro = *arg;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;
  begin_macro(&macro, 0);
  next();
  expr_eq();
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  }
  return 0;
}

static void gfunc_param_typed_with_conversions(Sym *func_type, Sym *param)
{
  if (param)
  {
    CType param_value_type = param->type;
    CType construct_type;
    CType arg_value_type = vtop->type;

    param_value_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&param_value_type))
      param_value_type = *pointed_type(&param_value_type);
    arg_value_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&arg_value_type))
      arg_value_type = *pointed_type(&arg_value_type);
    construct_type = param_value_type;
    construct_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    if ((construct_type.t & VT_BTYPE) == VT_STRUCT
        && (!is_reference_type(&param->type)
            || (param->type.t & VT_RVALUE_REFERENCE)
            || (param_value_type.t & VT_CONSTANT))
        && !is_compatible_unqualified_types(&construct_type,
                                            &arg_value_type)
        && !same_concrete_template_instantiation(&construct_type,
                                                 &arg_value_type)
        && class_has_single_arg_constructor_for(&construct_type,
                                                &arg_value_type))
    {
      try_materialize_constructor_conversion(&construct_type);
    }
  }
  gfunc_param_typed(func_type, param);
}

static Sym *resolve_free_func_by_arg_count(int name_tok, int explicit_arg_count,
                                           int template_deduction_failed)
{
  FreeFuncOverload *o;
  Sym *s, *match = NULL;
  int matches = 0;

  for (o = free_func_overloads; o; o = o->next)
  {
    if (o->name_tok != name_tok || !free_func_accepts_argument_count(o, explicit_arg_count))
      continue;
    if (template_deduction_failed
        && find_template_inst_owner_entry(o->mangled_tok))
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
  if (match) deduce_pending_member_return(match->v);
  return match;
}

static int btype_is_arithmetic_scalar(int bt)
{
  return bt >= VT_BYTE && bt <= VT_LLONG
         || bt >= VT_FLOAT && bt <= VT_LDOUBLE
         || bt == VT_BOOL;
}

static int class_value_is_derived_from(CType *derived, CType *base)
{
  int derived_tok, base_tok;

  if (!derived || !base
      || (derived->t & VT_BTYPE) != VT_STRUCT
      || (base->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  derived_tok = get_struct_type_name_tok(derived);
  base_tok = get_struct_type_name_tok(base);
  return derived_tok && base_tok && class_has_base(derived_tok, base_tok);
}

static int call_arg_exact_param_type(CType *param_type, CType *arg_type);
static int cpp_enum_promotion_rank(CType *target, CType *source);

static int call_arg_reference_qualification(CType *parameter, CType *argument)
{
  CType source = *argument;
  CType *target;
  int cv = VT_CONSTANT | VT_VOLATILE;
  if (!is_reference_type(parameter)) return 0;
  target = pointed_type(parameter);
  source.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&source)) source = *pointed_type(&source);
  /* Array cv-qualification belongs to its element type. */
  while ((target->t & VT_ARRAY) && (source.t & VT_ARRAY))
  {
    target = pointed_type(target);
    source = *pointed_type(&source);
  }
  if (source.t & cv & ~target->t) return -1;
  if (!(parameter->t & VT_RVALUE_REFERENCE)
      && (argument->t & VT_RVALUE_REFERENCE)
      && (!(target->t & VT_CONSTANT) || (target->t & VT_VOLATILE))
      && (target->t & VT_BTYPE) != VT_FUNC) return -1;
  return (target->t & cv & ~source.t) != 0;
}

/* Zero means a non-array reference; positive values are conversion ranks. */
static int call_arg_array_reference_rank(CType *param_type, CType *arg_type)
{
  CType *array;
  CType argument = *arg_type;
  if (!is_reference_type(param_type))
    return 0;
  array = pointed_type(param_type);
  if (!(array->t & VT_ARRAY))
    return 0;
  argument.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&argument))
    argument = *pointed_type(&argument);
  while (array->t & VT_ARRAY)
  {
    if (!(argument.t & VT_ARRAY) || !array->ref || !argument.ref
        || (array->ref->c >= 0 && array->ref->c != argument.ref->c))
      return -1;
    array = pointed_type(array);
    argument = *pointed_type(&argument);
  }
  if ((argument.t & VT_ARRAY)
      || (argument.t & (VT_CONSTANT | VT_VOLATILE) & ~array->t)
      || !is_compatible_unqualified_types(array, &argument))
    return -1;
  return ((array->t ^ argument.t) & (VT_CONSTANT | VT_VOLATILE)) ? 2 : 1;
}

/* A pointer (including a decayed array or function) has a standard
   conversion to bool. Reference parameters bind the resulting temporary,
   so the source pointer's value category does not constrain that binding. */
static int cpp_pointer_to_bool_conversion(CType *parameter, CType *argument)
{
  CType target = *parameter, source = *argument;
  int reference = is_reference_type(&target);
  if (reference) target = *pointed_type(&target);
  source.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&source)) source = *pointed_type(&source);
  if ((target.t & VT_BTYPE) != VT_BOOL
      || ((source.t & VT_BTYPE) != VT_PTR
          && (source.t & VT_BTYPE) != VT_FUNC
          && !find_cpp_member_pointer_type(&source))) return 0;
  /* nullptr_t requires direct initialization to convert to bool. */
  if (argument->t & VT_NULLPTR_TYPE) return -1;
  if (reference && !(parameter->t & VT_RVALUE_REFERENCE)
      && (!(target.t & VT_CONSTANT) || (target.t & VT_VOLATILE))) return -1;
  return 1;
}

static int call_arg_matches_param_type(CType *param_type, CType *arg_type)
{
  if (arg_type->t == VT_BRACED_LIST)
    return saved_list_argument_rank(param_type, arg_type->ref->default_arg) >= 0;
  {
    CType *source = arg_type;
    if ((source->t & VT_BTYPE) == VT_PTR) source = pointed_type(source);
    if ((source->t & VT_BTYPE) == VT_FUNC && source->ref
        && source->ref->f.func_unresolved_overload)
      return resolve_function_address(source, param_type) != NULL;
  }
  CType decayed_arg, arg_value_type;
  CType param_value_type;
  CType normalized_arg;
  CType *param_pointed, *arg_pointed;
  int array_rank = call_arg_array_reference_rank(param_type, arg_type);
  int boolean_conversion = cpp_pointer_to_bool_conversion(param_type, arg_type);

  if (boolean_conversion) return boolean_conversion > 0;
  if (call_arg_reference_qualification(param_type, arg_type) < 0)
    return 0;
  if (array_rank)
    return array_rank > 0
           && (!(param_type->t & VT_RVALUE_REFERENCE)
               || (arg_type->t & VT_RVALUE_REFERENCE));

  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  param_value_type = *param_type;
  param_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&param_value_type))
    param_value_type = *pointed_type(&param_value_type);
  if ((arg_value_type.t & VT_BTYPE) == VT_NULLPTR
      && (param_value_type.t & VT_BTYPE) == VT_PTR)
    return !is_reference_type(param_type) || (param_value_type.t & VT_CONSTANT)
           || (param_type->t & VT_RVALUE_REFERENCE);
  if (arg_type->t & VT_NULLPTR_TYPE)
  {
    if ((param_value_type.t & VT_BTYPE) == VT_PTR)
      return 1;
    normalized_arg = *arg_type;
    normalized_arg.t &= ~VT_NULLPTR_TYPE;
    arg_type = &normalized_arg;
    arg_value_type = normalized_arg;
    arg_value_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&arg_value_type))
      arg_value_type = *pointed_type(&arg_value_type);
  }
  if (call_arg_exact_param_type(param_type, arg_type))
  {
    if (is_reference_type(param_type)
        && (param_type->t & VT_RVALUE_REFERENCE)
        && !(arg_type->t & VT_RVALUE_REFERENCE))
      return 0;
    return 1;
  }
  /* Unscoped enums convert to arithmetic types, but an arithmetic value (or
     a different enum) does not implicitly convert to an enum parameter. */
  if (IS_ENUM(param_value_type.t) || IS_ENUM_VAL(param_value_type.t))
    return 0;
  if ((IS_ENUM(arg_value_type.t) || IS_ENUM_VAL(arg_value_type.t))
      && btype_is_arithmetic_scalar(param_value_type.t & VT_BTYPE))
    return (!is_reference_type(param_type)
            || (param_type->t & VT_RVALUE_REFERENCE)
            || (param_value_type.t & VT_CONSTANT))
           && cpp_enum_promotion_rank(&param_value_type, &arg_value_type) >= 0;
  if (is_reference_type(param_type))
  {
    if ((param_type->t & VT_RVALUE_REFERENCE)
        && !(arg_type->t & VT_RVALUE_REFERENCE))
      return 0;
    param_pointed = pointed_type(param_type);
    if ((arg_value_type.t & VT_ARRAY)
        && (param_pointed->t & VT_BTYPE) != VT_PTR
        && (param_pointed->t & VT_BTYPE) != VT_STRUCT
        && !(param_pointed->t & VT_ARRAY))
      return 0;
    if (is_compatible_unqualified_types(param_pointed, &arg_value_type)
        || class_value_is_derived_from(&arg_value_type, param_pointed))
      return 1;
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & (VT_CONSTANT | VT_VOLATILE)) == VT_CONSTANT
        && (param_pointed->t & VT_BTYPE) == VT_PTR
        && (arg_value_type.t & VT_BTYPE) == VT_PTR)
      return call_arg_matches_param_type(param_pointed, &arg_value_type);
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && btype_is_arithmetic_scalar(param_pointed->t & VT_BTYPE)
        && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
      return 1;
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && class_has_single_arg_constructor_for(param_pointed, &arg_value_type))
      return 1;
    /* References use pointer storage internally, but a failed reference
       binding must not fall through to ordinary pointer conversion.  Doing
       so made `Base&` appear viable for a `Derived*` argument. */
    return 0;
  }
  if ((param_type->t & VT_BTYPE) == VT_PTR
      && (arg_value_type.t & VT_BTYPE) == VT_PTR)
  {
    param_pointed = pointed_type(param_type);
    arg_pointed = pointed_type(&arg_value_type);
    /* An object pointer converts to cv void* without discarding any of the
       pointed-to object's qualifiers. Function pointers do not. */
    if ((param_pointed->t & VT_BTYPE) == VT_VOID
        && (arg_pointed->t & VT_BTYPE) != VT_FUNC
        && !(arg_pointed->t & (VT_CONSTANT | VT_VOLATILE)
             & ~(param_pointed->t & (VT_CONSTANT | VT_VOLATILE))))
      return 1;
    if (is_compatible_unqualified_types(param_pointed, arg_pointed)
        && !(arg_pointed->t & (VT_CONSTANT | VT_VOLATILE)
             & ~(param_pointed->t & (VT_CONSTANT | VT_VOLATILE))))
      return 1;
    if (class_value_is_derived_from(arg_pointed, param_pointed)
        && !(arg_pointed->t & (VT_CONSTANT | VT_VOLATILE)
             & ~(param_pointed->t & (VT_CONSTANT | VT_VOLATILE))))
      return 1;
    return 0;
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
      && (arg_value_type.t & VT_BTYPE) == VT_PTR
      && (arg_value_type.t & VT_ARRAY))
  {
    decayed_arg = arg_value_type;
    decayed_arg.t &= ~VT_ARRAY;
    return call_arg_matches_param_type(param_type, &decayed_arg);
  }
  return 0;
}

static Sym *find_cpp_conversion_operator(CType *source, CType *target,
                                        int allow_explicit, int *method,
                                        int *conversion_rank);

static int cpp_enum_promotion_rank(CType *target, CType *source)
{
  Sym *enumeration = source->ref, *value;
  CType promoted, underlying;
  int fits_int = 1, fits_unsigned = 1, fits_signed_long_long = 1;
  if (!enumeration || enumeration->a.scoped_enum) return -1;
  underlying = enumeration->type;
  underlying.t &= ~(VT_STRUCT_MASK | VT_CONSTANT | VT_VOLATILE
                    | VT_STORAGE | VT_RVALUE_REFERENCE);
  underlying.ref = NULL;
  promoted = underlying;
  if (enumeration->c == 1) {
    /* An enum without a fixed underlying type promotes according to the
       range of all its enumerators, even when its storage type is unsigned. */
    for (value = enumeration->next; value; value = value->next) {
      long long number = value->enum_val;
      if ((value->type.t & VT_UNSIGNED) && number < 0) {
        fits_int = fits_unsigned = fits_signed_long_long = 0;
      } else {
        if (number < -2147483647LL - 1 || number > 2147483647LL)
          fits_int = 0;
        if (number < 0 || (unsigned long long)number > 4294967295ULL)
          fits_unsigned = 0;
      }
    }
    promoted.t = fits_int ? VT_INT : fits_unsigned ? VT_INT | VT_UNSIGNED
      : fits_signed_long_long ? VT_LLONG : VT_LLONG | VT_UNSIGNED;
    if (!fits_int && !fits_unsigned && LONG_SIZE == 8)
      promoted.t |= VT_LONG;
  } else {
    if (call_arg_exact_param_type(target, &underlying)) return 3;
    if ((underlying.t & VT_BTYPE) == VT_BOOL
        || (underlying.t & VT_BTYPE) == VT_BYTE
        || (underlying.t & VT_BTYPE) == VT_SHORT)
      promoted.t = VT_INT;
  }
  return call_arg_exact_param_type(target, &promoted) ? 3 : 4;
}

/* For a fixed enum, conversion to its underlying type is preferred over
   conversion to the promoted underlying type; both have promotion rank. */
static int cpp_compare_enum_promotion_targets(CType *left, CType *right,
                                               CType *source)
{
  CType from = *source, l = *left, r = *right, underlying;
  int left_underlying, right_underlying;
  if (is_reference_type(&from)) from = *pointed_type(&from);
  if (!(IS_ENUM(from.t) || IS_ENUM_VAL(from.t)) || !from.ref
      || from.ref->a.scoped_enum || from.ref->c == 1)
    return 0;
  if (is_reference_type(&l)) l = *pointed_type(&l);
  if (is_reference_type(&r)) r = *pointed_type(&r);
  if (cpp_enum_promotion_rank(&l, &from) != 3
      || cpp_enum_promotion_rank(&r, &from) != 3)
    return 0;
  underlying = from.ref->type;
  underlying.t &= ~(VT_STRUCT_MASK | VT_CONSTANT | VT_VOLATILE | VT_STORAGE);
  underlying.ref = NULL;
  left_underlying = call_arg_exact_param_type(&l, &underlying);
  right_underlying = call_arg_exact_param_type(&r, &underlying);
  return left_underlying - right_underlying;
}

static int cpp_arithmetic_conversion_rank(CType *target, CType *source)
{
  int from = source->t & VT_BTYPE;
  int to = target->t & VT_BTYPE;
  if (CHARACTER_TYPE_KIND(source->t) == 3 && to == VT_INT
      && (target->t & VT_UNSIGNED) && !(target->t & (VT_LONG | VT_WCHAR_T)))
    return 3;
  if ((from == VT_BYTE || from == VT_SHORT || from == VT_BOOL)
      && to == VT_INT && !(target->t & (VT_UNSIGNED | VT_LONG | VT_WCHAR_T)))
    return 3;
  if (from == VT_FLOAT && to == VT_DOUBLE && !(target->t & VT_LONG))
    return 3;
  return 4;
}

static int call_arg_match_rank_standard(CType *param_type, CType *arg_type)
{
  if (arg_type->t == VT_BRACED_LIST)
    return saved_list_argument_rank(param_type, arg_type->ref->default_arg);
  {
    CType *source = arg_type;
    if ((source->t & VT_BTYPE) == VT_PTR) source = pointed_type(source);
    if ((source->t & VT_BTYPE) == VT_FUNC && source->ref
        && source->ref->f.func_unresolved_overload)
      return resolve_function_address(source, param_type) ? 0 : -1;
  }
  CType arg_value_type;
  CType param_value_type;
  CType normalized_arg;
  CType *param_pointed;
  int qualification = call_arg_reference_qualification(param_type, arg_type);
  int array_rank = call_arg_array_reference_rank(param_type, arg_type);
  int boolean_conversion = cpp_pointer_to_bool_conversion(param_type, arg_type);

  if (boolean_conversion) return boolean_conversion > 0 ? 4 : -1;
  if (qualification < 0) return -1;

  if (array_rank)
    return (param_type->t & VT_RVALUE_REFERENCE)
              && !(arg_type->t & VT_RVALUE_REFERENCE) ? -1 : array_rank;

  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  param_value_type = *param_type;
  param_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&param_value_type))
    param_value_type = *pointed_type(&param_value_type);
  if ((arg_value_type.t & VT_BTYPE) == VT_NULLPTR
      && (param_value_type.t & VT_BTYPE) == VT_PTR)
    return !is_reference_type(param_type) || (param_value_type.t & VT_CONSTANT)
           || (param_type->t & VT_RVALUE_REFERENCE) ? 2 : -1;
  if (arg_type->t & VT_NULLPTR_TYPE)
  {
    if ((param_value_type.t & VT_BTYPE) == VT_PTR)
      return 2;
    normalized_arg = *arg_type;
    normalized_arg.t &= ~VT_NULLPTR_TYPE;
    arg_type = &normalized_arg;
    arg_value_type = normalized_arg;
    arg_value_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&arg_value_type))
      arg_value_type = *pointed_type(&arg_value_type);
  }
  if ((!is_reference_type(param_type)
       || (param_value_type.t & VT_CONSTANT)
       || (param_type->t & VT_RVALUE_REFERENCE))
      && (param_value_type.t & VT_BTYPE) == VT_PTR
      && (arg_value_type.t & VT_BTYPE) == VT_FUNC)
    return is_compatible_unqualified_types(pointed_type(&param_value_type),
                                             &arg_value_type) ? 1 : -1;
  if (call_arg_exact_param_type(param_type, arg_type))
  {
    if (is_reference_type(param_type))
    {
      if ((param_type->t & VT_RVALUE_REFERENCE)
          && !(arg_type->t & VT_RVALUE_REFERENCE))
        return -1;
      if ((param_type->t & VT_RVALUE_REFERENCE)
          && (arg_type->t & VT_RVALUE_REFERENCE))
        return qualification ? 1 : 0;
      if ((arg_type->t & VT_RVALUE_REFERENCE)
          && (pointed_type(param_type)->t & VT_CONSTANT))
        return 2;
    }
    return qualification ? 2 : 1;
  }
  if (IS_ENUM(param_value_type.t) || IS_ENUM_VAL(param_value_type.t))
    return -1;
  if (!is_reference_type(param_type)) {
    CppMemberPointerType *target_member = find_cpp_member_pointer_type(&param_value_type);
    CppMemberPointerType *source_member = find_cpp_member_pointer_type(&arg_value_type);
    if (target_member && source_member
        && is_compatible_unqualified_types(&target_member->member_type, &source_member->member_type)
        && !((source_member->member_type.t & ~target_member->member_type.t)
             & (VT_CONSTANT | VT_VOLATILE))) {
      int offset;
      if (target_member->owner_tok == source_member->owner_tok) return 2;
      if (class_subobject_offset(target_member->owner_tok, source_member->owner_tok, &offset)) return 3;
    }
  }
  if ((IS_ENUM(arg_value_type.t) || IS_ENUM_VAL(arg_value_type.t))
      && btype_is_arithmetic_scalar(param_value_type.t & VT_BTYPE)) {
    if (is_reference_type(param_type)
        && !(param_type->t & VT_RVALUE_REFERENCE)
        && !(param_value_type.t & VT_CONSTANT))
      return -1;
    return cpp_enum_promotion_rank(&param_value_type, &arg_value_type);
  }

  if (is_reference_type(param_type))
  {
    if ((param_value_type.t & VT_BTYPE) == VT_STRUCT
        && ((param_type->t & VT_RVALUE_REFERENCE) || (param_value_type.t & VT_CONSTANT))
        && !is_compatible_unqualified_types(&param_value_type, &arg_value_type)
        && class_has_single_arg_constructor_for(&param_value_type, &arg_value_type))
      return 8;
    if ((param_type->t & VT_RVALUE_REFERENCE)
        && !(arg_type->t & VT_RVALUE_REFERENCE))
      return -1;
    param_pointed = pointed_type(param_type);
    if ((arg_value_type.t & VT_ARRAY)
        && (param_pointed->t & VT_BTYPE) != VT_PTR
        && (param_pointed->t & VT_BTYPE) != VT_STRUCT
        && !(param_pointed->t & VT_ARRAY))
      return -1;
    if (is_compatible_unqualified_types(param_pointed, &arg_value_type))
    {
      if ((param_type->t & VT_RVALUE_REFERENCE)
          && (arg_type->t & VT_RVALUE_REFERENCE))
        return qualification ? 1 : 0;
      if ((arg_type->t & VT_RVALUE_REFERENCE)
          && (param_pointed->t & VT_CONSTANT))
        return 2;
      return qualification ? 2 : 1;
    }
    if (class_value_is_derived_from(&arg_value_type, param_pointed))
      return 3;
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & (VT_CONSTANT | VT_VOLATILE)) == VT_CONSTANT
        && (param_pointed->t & VT_BTYPE) == VT_PTR
        && (arg_value_type.t & VT_BTYPE) == VT_PTR)
      return call_arg_match_rank_standard(param_pointed, &arg_value_type);
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && btype_is_arithmetic_scalar(param_pointed->t & VT_BTYPE)
        && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
      return cpp_arithmetic_conversion_rank(param_pointed, &arg_value_type);
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && class_has_single_arg_constructor_for(param_pointed, &arg_value_type))
      return 8;
    /* Do not reinterpret the internal pointer-shaped representation of a
       reference as a pointer parameter after reference binding failed. */
    return -1;
  }
  if ((param_type->t & VT_BTYPE) == VT_PTR
      && (arg_value_type.t & VT_BTYPE) == VT_PTR)
  {
    if (!call_arg_matches_param_type(param_type, arg_type))
      return -1;
    /* Array decay and pointed-to qualification are exact conversion
       sequences. They beat object-to-void and derived-to-base pointer
       conversions, even when adding const prevents a literal type match. */
    if (is_compatible_unqualified_types(pointed_type(param_type),
                                        pointed_type(&arg_value_type)))
      return 2;
    return 3;
  }
  if (compare_types(param_type, &arg_value_type, 1))
    return 1;
  if (btype_is_arithmetic_scalar(param_value_type.t & VT_BTYPE)
      && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
    return cpp_arithmetic_conversion_rank(&param_value_type, &arg_value_type);
  if ((param_type->t & VT_BTYPE) == VT_STRUCT
      && class_has_single_arg_constructor_for(param_type, &arg_value_type))
    return 8;
  return call_arg_matches_param_type(param_type, arg_type) ? 3 : -1;
}

static int call_arg_match_rank(CType *param_type, CType *arg_type)
{
  int rank = call_arg_match_rank_standard(param_type, arg_type);
  int conversion_rank;
  CType source = *arg_type;
  if (is_reference_type(&source)) source = *pointed_type(&source);
  if (!cpp_standard_conversion_only && rank < 0 && (source.t & VT_BTYPE) == VT_STRUCT
      && find_cpp_conversion_operator(&source, param_type, 0, NULL,
                                      &conversion_rank))
    return 8 + conversion_rank;
  return rank;
}

static Sym *find_cpp_conversion_operator(CType *source, CType *target,
                                        int allow_explicit, int *method,
                                        int *conversion_rank)
{
  MemberFuncOverload *candidate;
  int local_classes[32], *classes = local_classes;
  int class_count = 1, class_capacity = 32, class_index;
  Sym *best = NULL;
  int source_class = get_struct_type_name_tok(source);
  int best_rank = 0x7fffffff, best_const = 0, ambiguous = 0;
  if (!source_class) return NULL;
  if (allow_explicit != 2) {
    CType destination = is_reference_type(target) ? *pointed_type(target) : *target;
    int destination_class = get_struct_type_name_tok(&destination);
    if ((destination.t & VT_BTYPE) == VT_VOID
        || (destination_class && (destination_class == source_class
            || class_has_base(source_class, destination_class)))) return NULL;
  }
  classes[0] = source_class;
  for (class_index = 0; class_index < class_count; ++class_index) {
    ClassBaseInfo *base;
    int current_class = classes[class_index];
    for (base = class_base_candidates(current_class); base; base = base->bucket_next) {
      int i;
      if (base->class_tok != current_class) continue;
      for (i = 0; i < class_count; ++i) if (classes[i] == base->base_tok) break;
      if (i < class_count) continue;
      if (class_count == class_capacity) {
        int *grown;
        class_capacity *= 2;
        grown = cprime_malloc(class_capacity * sizeof(*grown));
        memcpy(grown, classes, class_count * sizeof(*grown));
        if (classes != local_classes) cprime_free(classes);
        classes = grown;
      }
      classes[class_count++] = base->base_tok;
    }
  instantiate_cpp_conversion_templates(current_class, target);
  for (candidate = conversion_overloads[(unsigned)current_class & 4095];
       candidate; candidate = candidate->conversion_next) {
    Sym *function;
    CType converted;
    int rank;
    if (!candidate->func_type.ref
        || !candidate->func_type.ref->f.func_cpp_conversion
        || (candidate->func_type.ref->f.func_cpp_explicit && !allow_explicit)
        || candidate->struct_tok != current_class
        || ((source->t & VT_CONSTANT) && !candidate->is_const))
      continue;
    converted = candidate->func_type.ref->type;
    if (allow_explicit == 2 && !is_compatible_types(&converted, target))
      continue;
    if (is_reference_type(&converted)) {
      int rvalue = converted.t & VT_RVALUE_REFERENCE;
      converted = *pointed_type(&converted);
      converted.t |= rvalue;
    } else {
      if (is_reference_type(target)
          && !(target->t & VT_RVALUE_REFERENCE)
          && !(pointed_type(target)->t & VT_CONSTANT))
        continue;
      converted.t |= VT_RVALUE_REFERENCE;
    }
    cpp_standard_conversion_only++;
    rank = call_arg_match_rank_standard(target, &converted);
    cpp_standard_conversion_only--;
    /* A conversion function can be followed only by a standard conversion. */
    if (rank < 0 || rank >= 8) continue;
    function = find_global_symbol(candidate->mangled_tok);
    if (!function) continue;
    if (rank < best_rank
        || (rank == best_rank && best_const && !candidate->is_const)
        || (rank == best_rank && candidate->is_const == best_const && best
            && member_function_is_template(best->v)
            && !member_function_is_template(function->v))) {
      best = function;
      best_rank = rank;
      best_const = candidate->is_const;
      if (method) *method = candidate->method_tok;
      ambiguous = 0;
    } else if (rank == best_rank && candidate->is_const == best_const
               && function != best
               && member_function_is_template(best->v)
                    == member_function_is_template(function->v))
      ambiguous = 1;
  }
  }
  if (classes != local_classes) cprime_free(classes);
  if (conversion_rank) *conversion_rank = best_rank;
  return ambiguous ? NULL : best;
}

static int template_deduced_inst_for_call(TemplateDef *definition,
                                         CType *arguments, int argument_count);

static int cpp_compare_argument_conversion_overloads(Sym *left, Sym *right,
                                                 CType *arguments, int count, int skip_this)
{
  Sym *l = left->type.ref->next, *r = right->type.ref->next;
  int i, preference = 0;
  if (skip_this) { if (l) l = l->next; if (r) r = r->next; }
  for (i = 0; i < count && l && r; ++i, l = l->next, r = r->next) {
    int current;
    if (call_arg_match_rank(&l->type, &arguments[i])
        != call_arg_match_rank(&r->type, &arguments[i]))
      return 0;
    current = cpp_compare_enum_promotion_targets(&l->type, &r->type, &arguments[i]);
    if (arguments[i].t == VT_BRACED_LIST) {
      CType left_target = is_reference_type(&l->type) ? *pointed_type(&l->type) : l->type;
      CType right_target = is_reference_type(&r->type) ? *pointed_type(&r->type) : r->type;
      current = type_is_std_initializer_list(&left_target)
              - type_is_std_initializer_list(&right_target);
    }
    if (is_reference_type(&l->type) && is_reference_type(&r->type)
        && is_compatible_unqualified_types(pointed_type(&l->type),
                                            pointed_type(&r->type))) {
      CType source = arguments[i];
      int temporary = source.t == VT_BRACED_LIST
                      || (source.t & VT_RVALUE_REFERENCE);
      int left_rvalue = (l->type.t & VT_RVALUE_REFERENCE) != 0;
      int right_rvalue = (r->type.t & VT_RVALUE_REFERENCE) != 0;
      source.t &= ~VT_RVALUE_REFERENCE;
      if (is_reference_type(&source)) source = *pointed_type(&source);
      /* A conversion to an unrelated reference target also creates a
         temporary, even when the original expression is an lvalue enum.
         Equal conversions to the same target prefer its rvalue reference. */
      if (!temporary && !is_compatible_unqualified_types(&source, pointed_type(&l->type))
          && !class_value_is_derived_from(&source, pointed_type(&l->type)))
        temporary = 1;
      if (temporary) current = left_rvalue - right_rvalue;
    }
    if (current && preference && current != preference) return 0;
    if (current) preference = current;
  }
  return preference;
}

static int saved_list_argument_rank(CType *parameter, TokenString *argument)
{
  CType target = *parameter;
  int accepted;
  if (is_reference_type(&target)) {
    CType source;
    TokenString *direct = saved_list_direct_reference_argument(parameter,
                                                               argument, &source);
    if (direct) {
      /* Bind a singleton naming an existing object directly, preserving its
         value category instead of manufacturing a temporary. */
      int rank = call_arg_match_rank_standard(parameter, &source);
      tok_str_free(direct);
      return rank;
    }
    if (!(target.t & VT_RVALUE_REFERENCE)
        && !(pointed_type(&target)->t & VT_CONSTANT)) return -1;
    target = *pointed_type(&target);
  }
  target.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_STORAGE);
  accepted = template_probe_type_ex(argument, 7, current_namespace_scope_tok(), 0, 0, &target);
  if (!accepted) return -1;
  /* A viable initializer_list conversion precedes other list conversions. */
  if (type_is_std_initializer_list(&target)) return 0;
  return (target.t & VT_BTYPE) == VT_STRUCT ? 4 : 1;
}

static int compare_free_func_registration(const void *left, const void *right)
{
  const FreeFuncOverload *a = *(FreeFuncOverload *const *)left;
  const FreeFuncOverload *b = *(FreeFuncOverload *const *)right;
  return a->registration_order > b->registration_order ? -1
       : a->registration_order < b->registration_order;
}

static Sym *resolve_free_func_by_arg_types_filtered(int name_tok, CType *arg_types,
                                           int explicit_arg_count,
                                           int template_deduction_failed,
                                           int *candidates, int candidate_count,
                                           TokenString **saved_arguments)
{
  FreeFuncOverload *o;
  struct TemplateCandidate { TemplateDef *definition; int instance; };
  struct TemplateCandidate local_templates[16], *templates = local_templates;
  int template_count = 0, template_capacity = 16;
  FreeFuncOverload **filtered = NULL;
  int filtered_count = 0, filtered_index = 0;
  Sym *s, *arg, *match = NULL;
  int i, ok, matches = 0, best_arg_rank = 0x7fffffff;
  int best_is_provisional = 0;
  int best_is_template = 0;

  if (candidates) {
    /* Lookup is indexed, but retain registration order when comparing ties
       and aliases of the same function. */
    for (i = 0; i < candidate_count; ++i)
      for (o = free_mangled_buckets[(unsigned)candidates[i] & (MEMBER_CANDIDATE_BUCKETS - 1)];
           o; o = o->mangled_next)
        if (o->mangled_tok == candidates[i])
          dynarray_add(&filtered, &filtered_count, o);
    if (filtered_count > 1)
      qsort(filtered, filtered_count, sizeof(*filtered), compare_free_func_registration);
  }
  for (o = candidates ? (filtered_count ? filtered[0] : NULL) : free_func_candidates(name_tok);
       o; o = candidates ? (++filtered_index < filtered_count ? filtered[filtered_index] : NULL)
                         : o->name_next)
  {
    int arg_rank = 0;
    int candidate_is_template;
    if ((o->name_tok != name_tok && !candidates) || !free_func_accepts_argument_count(o, explicit_arg_count))
      continue;
    if (template_deduction_failed
        && find_template_inst_owner_entry(o->mangled_tok))
      continue;
    {
      TemplateInstOwner *owner = find_template_inst_owner_entry(o->mangled_tok);
      candidate_is_template = owner != NULL;
      if (owner) {
        int family;
        /* Resolve each template family once for this overload set. Previously
           instantiated bodies are not independent overload candidates. */
        for (family = 0; family < template_count; ++family)
          if (templates[family].definition == owner->owner) break;
        if (family == template_count) {
          if (template_count == template_capacity) {
            struct TemplateCandidate *grown;
            template_capacity *= 2;
            grown = cprime_malloc(template_capacity * sizeof(*grown));
            memcpy(grown, templates, template_count * sizeof(*grown));
            if (templates != local_templates) cprime_free(templates);
            templates = grown;
          }
          templates[family].definition = owner->owner;
          templates[family].instance = template_deduced_inst_for_call(
            owner->owner, arg_types, explicit_arg_count);
          ++template_count;
        }
        if (templates[family].instance != o->mangled_tok) continue;
      }
    }
    s = find_global_symbol(o->mangled_tok);
    if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
      continue;
    use_overload_func_type(s, &o->func_type);
    ok = 1;
    arg = s->type.ref->next;
    for (i = 0; i < explicit_arg_count; ++i)
    {
      int rank;
      if (!arg)
      {
        if (s->type.ref->f.func_type == FUNC_ELLIPSIS) {
          arg_rank += 32; /* Ellipsis loses to every standard or user conversion. */
          continue;
        }
        ok = 0;
        break;
      }
      /* A conversion between a class and a non-class consumes a user
         conversion (rank >= 8). Once a cheaper complete candidate exists,
         probing its constructors or conversion templates cannot improve it.
         Braced arguments have their own list-initialization ranking. */
      if (best_arg_rank - arg_rank < 8
          && arg_types[i].t != VT_BRACED_LIST
          && !(saved_arguments && saved_arg_is_braced(saved_arguments[i]))) {
        CType parameter = arg->type, actual = arg_types[i];
        if (is_reference_type(&parameter)) parameter = *pointed_type(&parameter);
        if (is_reference_type(&actual)) actual = *pointed_type(&actual);
        if (((parameter.t & VT_BTYPE) == VT_STRUCT)
            != ((actual.t & VT_BTYPE) == VT_STRUCT)) {
          ok = 0;
          break;
        }
      }
      rank = saved_arguments && saved_arg_is_braced(saved_arguments[i])
           ? saved_list_argument_rank(&arg->type, saved_arguments[i])
           : call_arg_match_rank(&arg->type, &arg_types[i]);
      if (rank < 0)
      {
        ok = 0;
        break;
      }
      arg_rank += rank;
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
    if (arg_rank == best_arg_rank && match) {
      int preference = cpp_compare_argument_conversion_overloads(s, match, arg_types,
                                                            explicit_arg_count, 0);
      if (preference < 0) continue;
      if (preference > 0) {
        match = NULL;
        matches = 0;
        best_is_provisional = o->is_provisional_auto_member;
        best_is_template = candidate_is_template;
      }
    }
    if (arg_rank > best_arg_rank)
      continue;
    if (arg_rank < best_arg_rank)
    {
      match = NULL;
      matches = 0;
      best_arg_rank = arg_rank;
      best_is_provisional = o->is_provisional_auto_member;
      best_is_template = candidate_is_template;
    }
    else if (best_is_template && !candidate_is_template)
    {
      match = NULL;
      matches = 0;
      best_is_template = 0;
    }
    else if (!best_is_template && candidate_is_template)
      continue;
    else if (!o->is_provisional_auto_member && best_is_provisional)
      continue;
    else if (o->is_provisional_auto_member && !best_is_provisional)
    {
      match = NULL;
      matches = 0;
      best_is_provisional = 1;
    }
    /* One function can be registered under its declaration name and an
       instantiated interface alias. It is still a single candidate. */
    if (match == s)
      continue;
    match = s;
    matches++;
  }
  cprime_free(filtered);
  if (templates != local_templates) cprime_free(templates);
  if (matches > 1)
    cprime_error("ambiguous overloaded function '%s'",
              get_tok_str(name_tok, NULL));
  if (match) deduce_pending_member_return(match->v);
  return match;
}

static Sym *resolve_free_func_by_arg_types(int name_tok, CType *arg_types,
                                           int explicit_arg_count,
                                           int template_deduction_failed)
{
  return resolve_free_func_by_arg_types_filtered(name_tok, arg_types,
                   explicit_arg_count, template_deduction_failed, NULL, 0, NULL);
}

static int call_arg_exact_param_type(CType *param_type, CType *arg_type)
{
  CType param = *param_type;
  CType arg = *arg_type;

  if (call_arg_array_reference_rank(param_type, arg_type) < 0)
    return 0;

  if (param.t & VT_NULLPTR_TYPE)
    return 0;
  arg.t &= ~VT_NULLPTR_TYPE;

  param.t &= ~VT_RVALUE_REFERENCE;
  arg.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&param))
    param = *pointed_type(&param);
  if (is_reference_type(&arg))
    arg = *pointed_type(&arg);
  /* Qualification addition remains an exact conversion sequence; its
     reference-binding tie-break is ranked separately. Dropping cv is invalid. */
  if (call_arg_reference_qualification(param_type, arg_type) < 0)
    return 0;
  param.t &= ~(VT_CONSTANT | VT_VOLATILE);
  arg.t &= ~(VT_CONSTANT | VT_VOLATILE);
  /* The generic comparator intentionally unwraps enum representations.  An
     exact C++ overload match still requires the same enum identity. */
  if (IS_ENUM(param.t) || IS_ENUM_VAL(param.t)
      || IS_ENUM(arg.t) || IS_ENUM_VAL(arg.t))
    return (IS_ENUM(param.t) || IS_ENUM_VAL(param.t))
           && (IS_ENUM(arg.t) || IS_ENUM_VAL(arg.t))
           && param.ref == arg.ref;
  if ((param.t & VT_BTYPE) == VT_PTR && (arg.t & VT_BTYPE) == VT_PTR)
  {
    CType param_pointed = *pointed_type(&param);
    CType arg_pointed = *pointed_type(&arg);
    if ((param_pointed.t & (VT_CONSTANT | VT_VOLATILE))
        != (arg_pointed.t & (VT_CONSTANT | VT_VOLATILE)))
      return 0;
    return call_arg_exact_param_type(&param_pointed, &arg_pointed);
  }
  /* `char` and `signed char` are distinct overload types.  VT_DEFSIGN is
     merely redundant for unsigned char, where template substitution can
     canonicalize an unsigned typedef differently from its declaration. */
  if (!((param.t | arg.t) & VT_WCHAR_T)
      && ((param.t & VT_BTYPE) != VT_BYTE
          || ((param.t & VT_UNSIGNED) && (arg.t & VT_UNSIGNED))))
  {
    param.t &= ~VT_DEFSIGN;
    arg.t &= ~VT_DEFSIGN;
  }
  return is_compatible_unqualified_types(&param, &arg)
         || same_concrete_template_instantiation(&param, &arg);
}

static int free_func_has_exact_concrete_match(int name_tok,
                                              CType *arg_types,
                                              int arg_count)
{
  Sym *s = find_global_symbol(name_tok);
  FreeFuncOverload *o;

  if (s && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref)
  {
    Sym *arg = s->type.ref->next;
    int i, exact = 1;
    for (i = 0; i < arg_count; ++i)
    {
      if (!arg || !call_arg_exact_param_type(&arg->type, &arg_types[i]))
      {
        exact = 0;
        break;
      }
      arg = arg->next;
    }
    for (; exact && arg; arg = arg->next)
      if (!arg->default_arg)
        exact = 0;
    if (exact)
      return 1;
  }

  for (o = free_func_candidates(name_tok); o; o = o->name_next)
  {
    Sym *arg;
    int i, exact = 1;
    if (o->name_tok != name_tok || arg_count < o->min_arg_count
        || arg_count > o->explicit_arg_count)
      continue;
    s = find_global_symbol(o->mangled_tok);
    if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
      continue;
    use_overload_func_type(s, &o->func_type);
    arg = s->type.ref->next;
    for (i = 0; i < arg_count; ++i)
    {
      if (!arg || !call_arg_exact_param_type(&arg->type, &arg_types[i]))
      {
        exact = 0;
        break;
      }
      arg = arg->next;
    }
    if (exact)
      return 1;
  }
  return 0;
}

static int instantiate_function_templates_for_call(int name_tok,
                                                   CType *arg_types,
                                                   int arg_count)
{
  TemplateDef *td;
  TemplateArgList args;
  int inst_tok;
  Sym *s;

  /* Do not instantiate a function-template specialization when an already
     published concrete overload is an exact match.  Besides implementing the
     C++ non-template tie-break, this prevents a generic fallback template
     (such as stream read/write) from firing its body-level static_assert for
     a type with a concrete overload. */
  if (free_func_has_exact_concrete_match(name_tok, arg_types, arg_count))
    return 0;

  td = find_function_template_for_call(name_tok, arg_types, arg_count, &args);

  if (!td)
    return 0;
  inst_tok = instantiate_template_if_needed(td, &args);
  compile_pending_template_specs_without_member_flush();
  s = sym_find(inst_tok);
  if (!s)
    s = global_symbol_find(inst_tok);
  if (s && (s->type.t & VT_BTYPE) == VT_FUNC && s->type.ref)
    note_free_func_overload(name_tok, inst_tok, &s->type);
  return 0;
}

static int count_param_tokens(TokenString *params)
{
  int i, n = 0, saw = 0, level = 0;

  if (!params)
    return 0;
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    int t = params->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(params->str, params->len, i);
      continue;
    }
    if (level == 0 && t == ',')
    {
      if (saw)
        ++n;
      saw = 0;
      continue;
    }
    if (t == '(' || t == '[' || t == '{')
      ++level;
    else if (t == ')' || t == ']' || t == '}')
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
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      if (level == 0)
        saw = 1;
      continue;
    }
    if (level == 0 && t == TOK_DOTS)
      continue;
    if (level == 0 && t == ')')
      return saw ? n + 1 : 0;
    if (level == 0 && t == ',')
    {
      ++n;
      saw = 0;
      continue;
    }
    if (t == '(' || t == '[' || t == '{')
      ++level;
    else if (t == ')' || t == ']' || t == '}')
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
  CString name;
  int base_tok, result;
  int i, has_suffix = 0, param_count, same_count = 0, other_count = 0;
  int same_mangled_tok = 0;
  MemberFuncOverload *o;

  base_tok = make_joined_name_tok(struct_tok, method_tok, "_", "");
  param_count = count_param_tokens(params);
  for (o = member_func_candidates(struct_tok, method_tok); o; o = o->bucket_next)
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
  if (!same_count && !other_count && !sym_find(base_tok)
      && !global_symbol_find(base_tok))
    return base_tok;
  cstr_new(&name);
  cstr_cat(&name, get_tok_str(base_tok, NULL), -1);
  if (!params || params->len <= 1)
  {
    cstr_cat(&name, "_0args", -1);
    goto finish;
  }
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    if (TOK_HAS_VALUE(params->str[i]))
    {
      i += tok_str_value_extra_words(params->str, params->len, i);
      continue;
    }
    if (params->str[i] >= TOK_IDENT && struct_find(params->str[i]))
    {
      cstr_cat(&name, "_struct_", -1);
      cstr_cat(&name, get_tok_str(params->str[i], NULL), -1);
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
      cstr_cat(&name, "_const", -1);
      has_suffix = 1;
      break;
    case '&':
      cstr_cat(&name, "_ref", -1);
      has_suffix = 1;
      break;
    case TOK_LAND:
      cstr_cat(&name, "_rref", -1);
      has_suffix = 1;
      break;
    case TOK_INT:
      cstr_cat(&name, "_int", -1);
      has_suffix = 1;
      break;
    case TOK_FLOAT:
      cstr_cat(&name, "_float", -1);
      has_suffix = 1;
      break;
    case TOK_DOUBLE:
      cstr_cat(&name, "_double", -1);
      has_suffix = 1;
      break;
    case TOK_CHAR:
      cstr_cat(&name, "_char", -1);
      has_suffix = 1;
      break;
    case TOK_BOOL:
    case TOK_BOOL2:
      cstr_cat(&name, "_bool", -1);
      has_suffix = 1;
      break;
    case TOK_STRUCT:
      if (i + 1 < params->len)
      {
        cstr_cat(&name, "_struct_", -1);
        cstr_cat(&name, get_tok_str(params->str[++i], NULL), -1);
        has_suffix = 1;
      }
      break;
    case '*':
      cstr_cat(&name, "_ptr", -1);
      break;
    default:
      break;
    }
  }
  if (!has_suffix)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "_%dargs", count_param_tokens(params));
    cstr_cat(&name, buf, -1);
  }
finish:
  result = tok_alloc(name.data, name.size)->tok;
  cstr_free(&name);
  return result;
}

static void note_raw_lifecycle_overload(int struct_tok, int method_tok,
                                        int mangled_tok, TokenString *params)
{
  MemberFuncOverload *o;


  for (o = member_func_candidates(struct_tok, method_tok);
       o; o = o->bucket_next)
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
  index_member_func_overload(o);
}

static const char *cpp_operator_name_from_tok(int op)
{
  switch (op)
  {
  case '+': return "operator+";
  case ',': return "operator,";
  case '-': return "operator-";
  case '*': return "operator*";
  case '/': return "operator/";
  case '%':
  case TOK_UMOD: return "operator%";
  case '&': return "operator&";
  case '|': return "operator|";
  case TOK_LAND: return "operator&&";
  case TOK_LOR: return "operator||";
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
  case TOK_INC: return "operator++";
  case TOK_DEC: return "operator--";
  case '!': return "operator!";
  case '~': return "operator~";
  case TOK_ARROW: return "operator->";
  case TOK_SHR:
  case TOK_SAR: return "operator>>";
  case TOK_SHL: return "operator<<";
  default:
    return NULL;
  }
}

static TokenString *take_cpp_conversion_operator_type_tokens(void)
{
  TokenString *tokens = last_cpp_conversion_operator_type_tokens;
  last_cpp_conversion_operator_type_tokens = NULL;
  return tokens;
}

static int parse_cpp_operator_method_tok(void)
{
  const char *name;

  if (last_cpp_conversion_operator_type_tokens)
    tok_str_free(last_cpp_conversion_operator_type_tokens);
  last_cpp_conversion_operator_type_tokens = NULL;
  if (tok != TOK_OPERATOR)
    cprime_error("operator");
  next();
  if (tok >= TOK_UIDENT
      && (!strcmp(get_tok_str(tok, NULL), "new")
          || !strcmp(get_tok_str(tok, NULL), "delete"))) {
    int allocation = !strcmp(get_tok_str(tok, NULL), "new"), array = 0;
    next();
    if (tok == '[') { next(); skip(']'); array = 1; }
    return tok_alloc_const(allocation ? (array ? "operator new[]" : "operator new")
                                     : (array ? "operator delete[]" : "operator delete"));
  }
  if (tok == '[')
  {
    next();
    skip(']');
    return tok_alloc_const("operator[]");
  }
  if (tok == '(')
  {
    next();
    skip(')');
    return tok_alloc_const("operator()");
  }
  name = cpp_operator_name_from_tok(tok);
  if (!name)
  {
    char opname[256];
    int first = 1, angle_depth = 0;
    if (tok == TOK_EOF || tok == '(')
      cprime_error("unsupported operator overload");
    pstrcpy(opname, sizeof(opname), "operator ");
    last_cpp_conversion_operator_type_tokens = tok_str_alloc();
    while (tok != TOK_EOF && tok != '(')
    {
      if (forming_cpp_member_address && !angle_depth
          && (tok == ';' || tok == ',' || tok == ')' || tok == '}' || tok == ']')) break;
      if (tok == TOK_LT) ++angle_depth;
      else if (tok == TOK_GT && angle_depth) --angle_depth;
      else if (tok == TOK_SAR && angle_depth) angle_depth = angle_depth > 1 ? angle_depth - 2 : 0;
      if (!first)
        pstrcat(opname, sizeof(opname), " ");
      pstrcat(opname, sizeof(opname), get_tok_str(tok, &tokc));
      first = 0;
      tok_str_add_tok(last_cpp_conversion_operator_type_tokens);
      next();
    }
    tok_str_add(last_cpp_conversion_operator_type_tokens, TOK_EOF);
    return tok_alloc_const(opname);
  }
  next();
  return tok_alloc_const(name);
}

static int get_cpp_binary_operator_method_tok(int op)
{
  const char *name = cpp_operator_name_from_tok(op);
  return name ? tok_alloc_const(name) : 0;
}

static void finish_cpp_member_func_call_to(Sym *func_type, int nb_args,
                                           int direct_result,
                                           int result_storage,
                                           int result_address)
{
  SValue ret;
  int ret_nregs, ret_align, regsize, variadic;
  int size, align, n, r, t;

  ret.r2 = VT_CONST;
  ret_align = regsize = 0;
  if ((func_type->type.t & VT_BTYPE) == VT_STRUCT)
  {
    variadic = (func_type->f.func_type == FUNC_ELLIPSIS);
    ret_nregs = cpp_function_return_regs(&func_type->type, func_type, variadic, &ret.type,
                           &ret_align, &regsize);
    if (ret_nregs <= 0)
    {
      size = type_size(&func_type->type, &align);
#ifdef CPRIME_TARGET_ARM64
      if (size < 16)
        while (size & (size - 1))
          size = (size | (size - 1)) + 1;
#endif
      if (!direct_result) {
        loc = (loc - size) & -align;
        result_address = loc;
      }
      ret.type = func_type->type;
      ret.r = result_storage;
      ret.c.i = result_address;
      vset(&ret.type, result_storage, result_address);
      mk_pointer(&vtop->type);
      gaddrof();
#ifdef CONFIG_CPRIME_BCHECK
      if (cprime_state->do_bounds_check)
        --loc;
#endif
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
      if (direct_result) addr = result_address;
      else {
        loc = (loc - size) & -align;
        addr = loc;
      }
      offset = 0;
      for (;;)
      {
        push_cpp_construction_lvalue(&ret.type, result_storage, addr, offset);
        vswap();
        vstore();
        vtop--;
        if (--ret_nregs == 0)
          break;
        offset += regsize;
      }
      vset(&func_type->type, result_storage, addr);
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
  maybe_indir_call_result();
  if (!direct_result && (func_type->type.t & VT_BTYPE) == VT_STRUCT)
    cpp_register_class_temporary(vtop);
}

static void finish_cpp_member_func_call(Sym *func_type, int nb_args)
{
  finish_cpp_member_func_call_to(func_type, nb_args, 0, VT_LOCAL | VT_LVAL, 0);
}

static int try_call_cpp_binary_operator(int op)
{
  int method_tok;
  int instantiated_member_tok;
  Sym *func_sym, *func_type, *sa;
  Sym stable_func_type;
  CType saved_ret_type;
  SValue saved_lhs, saved_rhs;
  SValue *saved_operand_vtop;
  int lhs_was_nonstruct;

  method_tok = get_cpp_binary_operator_method_tok(op);
  if (!method_tok)
    return 0;

  lhs_was_nonstruct = (vtop[-1].type.t & VT_BTYPE) != VT_STRUCT;
  if (lhs_was_nonstruct)
  {
    CType rhs_type = vtop->type;
    if (is_reference_type(&rhs_type))
      rhs_type = *pointed_type(&rhs_type);
    if ((rhs_type.t & VT_BTYPE) != VT_STRUCT)
      return 0;
    vswap();
    if (!try_materialize_constructor_conversion(&rhs_type))
    {
      vswap();
      return 0;
    }
    vswap();
  }

  saved_operand_vtop = vtop;
  saved_lhs = vtop[-1];
  saved_rhs = *vtop;
  if (!is_reference_type(&saved_rhs.type)
      && ((!(saved_rhs.r & VT_LVAL) && !(saved_rhs.type.t & VT_ARRAY)
           && (saved_rhs.type.t & VT_BTYPE) != VT_FUNC)
          || (saved_rhs.r & VT_CXX_PRVALUE)))
    saved_rhs.type.t |= VT_RVALUE_REFERENCE;
  instantiate_template_member_for_call(&saved_lhs.type, method_tok,
                                       &saved_rhs.type, 1);
  vtop = saved_operand_vtop;
  vtop[-1] = saved_lhs;
  *vtop = saved_rhs;
  instantiated_member_tok = last_instantiated_member_func_tok;
  func_sym = resolve_member_func_by_arg_types(&saved_lhs.type, method_tok,
                                              &saved_rhs.type, 1);
  vtop = saved_operand_vtop;
  vtop[-1] = saved_lhs;
  *vtop = saved_rhs;
  if (!func_sym)
    func_sym = resolve_member_field_func_by_arg_types(&saved_lhs.type,
                                                      method_tok,
                                                      &saved_rhs.type, 1);
  vtop = saved_operand_vtop;
  vtop[-1] = saved_lhs;
  *vtop = saved_rhs;
  if (!func_sym)
  {
    func_sym = resolve_member_func(&vtop[-1].type, method_tok);
  }
  if (!func_sym && instantiated_member_tok)
  {
    func_sym = global_symbol_find(instantiated_member_tok);
    if (!func_sym)
      func_sym = sym_find(instantiated_member_tok);
  }
  if (func_sym)
  {
    vtop = saved_operand_vtop;
    vtop[-1] = saved_lhs;
    *vtop = saved_rhs;
  }
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");

  stable_func_type = *func_sym->type.ref;
  func_type = &stable_func_type;
  saved_ret_type = func_type->type;
  sa = func_type->next;
  if (!sa || !sa->next || sa->next->next)
    cprime_error("operator overload must have exactly one explicit parameter");
  /* A name-only fallback is not viable merely because the class declares
     this operator. Nonmember candidates still participate when its explicit
     parameter cannot accept the right operand. */
  if (call_arg_match_rank(&sa->next->type, &saved_rhs.type) < 0)
    return 0;

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

/* Function and operator candidates combine ordinary lookup and the namespaces
   associated with the argument types. Keep one overload set so conversions, rather than
   namespace discovery order, decide the selected function. */
typedef struct CppOperatorLookup {
  int method_tok;
  int *names, name_count;
  Sym **visited;
  int visited_count;
} CppOperatorLookup;

static void cpp_operator_lookup_name(CppOperatorLookup *lookup, int name)
{
  int i;
  for (i = 0; i < lookup->name_count; ++i)
    if (lookup->names[i] == name) return;
  lookup->names = cprime_realloc(lookup->names,
                                (lookup->name_count + 1) * sizeof(int));
  lookup->names[lookup->name_count++] = name;
}

static void cpp_operator_associated_type(CppOperatorLookup *lookup, CType *type, int complete)
{
  int i, name = 0, namespace_tok;
  int bt = type->t & VT_BTYPE;
  Sym *symbol;
  /* Fundamental types have no associated declarations. Their incidental ref
     field is not a record identity and must not be inspected as one. */
  if (bt != VT_PTR && bt != VT_FUNC && bt != VT_STRUCT
      && !IS_ENUM(type->t) && !IS_ENUM_VAL(type->t)) return;
  symbol = type->ref;
  if (!symbol) return;
  for (i = 0; i < lookup->visited_count; ++i)
    if (lookup->visited[i] == symbol) return;
  dynarray_add(&lookup->visited, &lookup->visited_count, symbol);
  if (bt == VT_STRUCT && complete) {
    materialize_incomplete_template_type(type);
    symbol = type->ref;
  }
  if ((type->t & VT_BTYPE) == VT_PTR) {
    cpp_operator_associated_type(lookup, pointed_type(type), complete);
    return;
  }
  if ((type->t & VT_BTYPE) == VT_FUNC) {
    cpp_operator_associated_type(lookup, &symbol->type, 0);
    for (symbol = symbol->next; symbol; symbol = symbol->next)
      cpp_operator_associated_type(lookup, &symbol->type, 0);
    return;
  }
  if (IS_ENUM(type->t) || IS_ENUM_VAL(type->t))
    name = symbol->v & ~SYM_STRUCT;
  else if ((type->t & VT_BTYPE) == VT_STRUCT)
    name = get_struct_type_name_tok(type);
  if (!name) name = cpp_unnamed_tag_typedef_tok(type);
  if (!name) return;
  namespace_tok = symbol_namespace_scope_tok(name);
  if (nb_inline_namespace_scopes) {
    int root = namespace_tok, changed;
    do {
      changed = 0;
      for (i = 0; i < nb_namespace_scopes; ++i)
        if (namespace_scopes[i].name_tok == root && namespace_scopes[i].is_inline) {
          root = namespace_scopes[i].parent_tok;
          changed = 1;
          break;
        }
    } while (changed);
    cpp_operator_lookup_name(lookup, root
      ? make_static_member_tok(root, lookup->method_tok) : lookup->method_tok);
    for (i = 0; i < nb_namespace_scopes; ++i)
      if (namespace_is_inline_child(namespace_scopes[i].name_tok, root))
        cpp_operator_lookup_name(lookup,
          make_static_member_tok(namespace_scopes[i].name_tok, lookup->method_tok));
  }
  cpp_operator_lookup_name(lookup, namespace_tok
    ? make_static_member_tok(namespace_tok, lookup->method_tok)
    : lookup->method_tok);
  if ((type->t & VT_BTYPE) == VT_STRUCT) {
    ClassBaseInfo *base;
    TemplateInstOwner *owner = find_template_inst_owner_entry(name);
    for (base = class_base_candidates(name); base; base = base->bucket_next) {
      CType base_type;
      if (base->class_tok == name
          && make_class_type_from_tok(&base_type, base->base_tok))
        cpp_operator_associated_type(lookup, &base_type, 0);
    }
    if (owner && owner->owner->is_class) {
      TemplateDef *definition = owner->owner;
      int inst;
      for (inst = 0; inst < definition->nb_inst; ++inst)
        if (definition->inst_name_toks[inst] == name) {
          TemplateArgList *arguments = &definition->inst_arg_lists[inst];
          for (i = 0; i < arguments->nb; ++i) {
            CType argument_type;
            if (!(definition->value_param_mask & (1u << i))
                && make_type_from_type_arg_tok(&argument_type, arguments->toks[i]))
              cpp_operator_associated_type(lookup, &argument_type, 0);
          }
          break;
        }
    }
  }
}

static Sym *cpp_resolve_associated_function(int method_tok, int ordinary_tok,
    CType *arguments, int count, TokenString **call_args, int operator_call)
{
  CppOperatorLookup lookup;
  FreeFuncOverload *overload;
  int i, j, name_index, *candidates = NULL, candidate_count = 0, has_class_operand = 0;
  Sym *function;
  memset(&lookup, 0, sizeof lookup);
  lookup.method_tok = method_tok;
  cpp_operator_lookup_name(&lookup, ordinary_tok);
  if (nb_inline_namespace_scopes && ordinary_tok) {
    int parent = symbol_namespace_scope_tok(ordinary_tok);
    const char *spelling = get_tok_str(ordinary_tok, NULL);
    int member;
    if (parent) spelling += strlen(get_tok_str(parent, NULL)) + 1;
    member = tok_alloc(spelling, strlen(spelling))->tok;
    for (i = 0; i < nb_namespace_scopes; ++i)
      if (namespace_is_inline_child(namespace_scopes[i].name_tok, parent))
        cpp_operator_lookup_name(&lookup,
          make_static_member_tok(namespace_scopes[i].name_tok, member));
  }
  for (i = 0; operator_call >= 0 && i < count; ++i) {
    CType value = arguments[i];
    if (is_reference_type(&value)) value = *pointed_type(&value);
    if ((value.t & VT_BTYPE) == VT_STRUCT) has_class_operand = 1;
    cpp_operator_associated_type(&lookup, &arguments[i], 1);
  }
  for (i = 0; i < lookup.name_count; ++i) {
    Sym **saved_adl_classes = template_adl_classes;
    int saved_adl_count = template_adl_class_count;
    template_adl_classes = lookup.visited;
    template_adl_class_count = operator_call >= 0 ? lookup.visited_count : 0;
    instantiate_function_templates_for_call(lookup.names[i], arguments, count);
    template_adl_classes = saved_adl_classes;
    template_adl_class_count = saved_adl_count;
    function = find_global_symbol(lookup.names[i]);
    if (function && !function->cpp_using_target
        && (function->type.t & VT_BTYPE) == VT_FUNC)
      note_free_func_overload(lookup.names[i], function->v, &function->type);
  }
  for (name_index = 0; name_index < lookup.name_count; ++name_index)
  for (overload = free_func_candidates(lookup.names[name_index]); overload; overload = overload->name_next) {
    if (lookup.names[name_index] != overload->name_tok) continue;
    if (operator_call < 0) {
      Sym *candidate = global_symbol_find(overload->mangled_tok);
      if (candidate && candidate->cpp_hidden_friend) continue;
    }
    if (operator_call > 0 && !has_class_operand) {
      /* With enum operands and no class operand, a non-member operator must
         have a corresponding parameter of that enum type. Converting both
         operands to unrelated classes cannot displace a builtin operator. */
      Sym *parameter = overload->func_type.ref ? overload->func_type.ref->next : NULL;
      int matches_enum = 0;
      for (i = 0; i < count && parameter; ++i, parameter = parameter->next) {
        CType value = arguments[i], expected = parameter->type;
        if (is_reference_type(&value)) value = *pointed_type(&value);
        if (is_reference_type(&expected)) expected = *pointed_type(&expected);
        if ((IS_ENUM(value.t) || IS_ENUM_VAL(value.t))
            && call_arg_exact_param_type(&expected, &value)) matches_enum = 1;
      }
      if (!matches_enum) continue;
    }
    for (j = 0; j < candidate_count; ++j)
      if (candidates[j] == overload->mangled_tok) break;
    if (j < candidate_count) continue;
    candidates = cprime_realloc(candidates, (candidate_count + 1) * sizeof(int));
    candidates[candidate_count++] = overload->mangled_tok;
  }
  function = candidate_count
    ? resolve_free_func_by_arg_types_filtered(method_tok, arguments, count,
                                              0, candidates, candidate_count, call_args)
    : NULL;
  cprime_free(candidates);
  cprime_free(lookup.names);
  cprime_free(lookup.visited);
  return function;
}

static Sym *cpp_resolve_free_operator(int method_tok, CType *arguments, int count)
{
  return cpp_resolve_associated_function(method_tok,
    find_current_namespace_tok(method_tok), arguments, count, NULL, 1);
}

static int try_call_cpp_free_binary_operator(int op)
{
  int method_tok;
  CType arg_types[2];
  Sym *func_sym, *func_type, *sa;

  method_tok = get_cpp_binary_operator_method_tok(op);
  if (!method_tok)
    return 0;

  arg_types[0] = vtop[-1].type;
  arg_types[1] = vtop->type;
  func_sym = cpp_resolve_free_operator(method_tok, arg_types, 2);
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");

  func_type = func_sym->type.ref;
  sa = func_type->next;
  if (!sa || !sa->next || sa->next->next)
    cprime_error("operator overload must have exactly two parameters");

  /* Bind reference arguments before preserving values across the call;
     spilling a scalar lvalue first would bind the reference to a copy. */
  gfunc_param_typed_with_conversions(func_type, sa->next);
  vswap();
  gfunc_param_typed_with_conversions(func_type, sa);
  vswap();
  save_lvalues();
  vpushsym(&func_sym->type, func_sym);
  vrott(3);
  finish_cpp_member_func_call(func_type, 2);
  return 1;
}

static int try_call_cpp_postfix_operator(int op)
{
  if (!is_cpp_translation_unit()
      || (vtop->type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  /* C++ distinguishes postfix operators by a dummy int argument whose
     value is zero. Use ordinary typed member/free overload resolution and
     call-result handling, including class temporaries and reference returns. */
  vpushi(0);
  if (try_call_cpp_binary_operator(op) || try_call_cpp_free_binary_operator(op))
    return 1;
  vpop();
  return 0;
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
  {
    return 0;
  }
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");
  if (op == '-' && member_is_auto_return_tok(func_sym->v))
  {
    int struct_tok = get_struct_type_name_tok(&vtop->type);
    int typed_tok;
    Sym *typed_sym;
    CType ret_type = vtop->type;
    ret_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    func_sym->type.ref->type = ret_type;
    typed_tok = struct_tok
                ? make_member_func_tok_for_type(struct_tok, method_tok,
                                                &func_sym->type)
                : 0;
    typed_sym = typed_tok ? sym_find(typed_tok) : NULL;
    if (!typed_sym && typed_tok)
      typed_sym = global_symbol_find(typed_tok);
    if (typed_sym && (typed_sym->type.t & VT_BTYPE) == VT_FUNC
        && typed_sym->type.ref)
      func_sym = typed_sym;
  }
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

static void cpp_builtin_increment_candidate(CType *receiver, int owner,
                                              int operation, Sym **best,
                                              CType *target)
{
  MemberFuncOverload *candidate;
  ClassBaseInfo *base;
  for (candidate = conversion_overloads[(unsigned)owner & 4095]; candidate;
       candidate = candidate->conversion_next)
  {
    CType converted, value;
    Sym *function;
    if (candidate->struct_tok != owner || !candidate->func_type.ref)
      continue;
    converted = candidate->func_type.ref->type;
    if (!is_reference_type(&converted) || (converted.t & VT_RVALUE_REFERENCE))
      continue;
    value = *pointed_type(&converted);
    if ((value.t & VT_CONSTANT)
        || (!btype_is_arithmetic_scalar(value.t & VT_BTYPE)
            && (value.t & VT_BTYPE) != VT_PTR)
        || (operation == TOK_DEC && (value.t & VT_BTYPE) == VT_BOOL))
      continue;
    function = find_cpp_conversion_operator(receiver, &converted, 0, NULL, NULL);
    if (!function) continue;
    if (*best && *best != function)
      cprime_error("ambiguous conversion for built-in increment/decrement");
    *best = function;
    *target = converted;
  }
  for (base = class_base_candidates(owner); base; base = base->bucket_next)
    if (base->class_tok == owner)
      cpp_builtin_increment_candidate(receiver, base->base_tok, operation, best, target);
}

static int try_cpp_builtin_increment_conversion(int operation)
{
  CType receiver = vtop->type, target;
  Sym *best = NULL;
  cpp_builtin_increment_candidate(&receiver, get_struct_type_name_tok(&receiver),
                                   operation, &best, &target);
  if (!best)
    cprime_error("no conversion to a modifiable scalar reference for increment/decrement");
  return try_call_cpp_conversion_operator(&target, 0);
}

static void cpp_pointer_conversion_candidate(CType *receiver, int owner,
                                             Sym **best, CType *target, int deleting)
{
  MemberFuncOverload *candidate;
  ClassBaseInfo *base;
  for (candidate = conversion_overloads[(unsigned)owner & 4095]; candidate;
       candidate = candidate->conversion_next) {
    CType converted, value;
    Sym *function;
    if (candidate->struct_tok != owner || !candidate->func_type.ref)
      continue;
    converted = candidate->func_type.ref->type;
    value = is_reference_type(&converted) ? *pointed_type(&converted) : converted;
    if ((value.t & VT_BTYPE) != VT_PTR
        || (deleting && (pointed_type(&value)->t & VT_BTYPE) == VT_FUNC))
      continue;
    function = find_cpp_conversion_operator(receiver, &converted, 0, NULL, NULL);
    if (!function) continue;
    if (*best && *best != function)
      cprime_error("ambiguous pointer conversion for %s", deleting ? "delete" : "dereference");
    *best = function;
    *target = converted;
  }
  for (base = class_base_candidates(owner); base; base = base->bucket_next)
    if (base->class_tok == owner)
      cpp_pointer_conversion_candidate(receiver, base->base_tok, best, target, deleting);
}

static void try_cpp_pointer_conversion(int deleting)
{
  CType receiver = vtop->type, target;
  Sym *best = NULL;
  if (is_reference_type(&receiver)) receiver = *pointed_type(&receiver);
  if ((receiver.t & VT_BTYPE) != VT_STRUCT) return;
  cpp_pointer_conversion_candidate(&receiver, get_struct_type_name_tok(&receiver),
                                   &best, &target, deleting);
  if (best) {
    try_call_cpp_conversion_operator(&target, 0);
    maybe_indir_reference();
  }
}

static int try_call_cpp_bool_conversion_operator(void)
{
  int method_tok = tok_alloc_const("operator bool");
  Sym *func_sym;
  Sym *canonical_sym = NULL;
  Sym *func_type;
  Sym *sa;

  if ((vtop->type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  func_sym = resolve_member_func_by_arg_count(&vtop->type, method_tok, 0);
  if (!func_sym)
    func_sym = resolve_member_func(&vtop->type, method_tok);
  if (!func_sym)
  {
    CType boolean_type = { VT_BOOL, NULL };
    func_sym = find_cpp_conversion_operator(&vtop->type, &boolean_type,
                                             0, &method_tok, NULL);
    if (!func_sym)
      return 0;
  }
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("conversion operator target is not declared as function");
  func_type = func_sym->type.ref;
  sa = func_type->next;
  if (!sa || sa->next)
    cprime_error("conversion operator must have no explicit parameters");
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  save_lvalues();
  vpushsym(&func_sym->type, func_sym);
  vswap();
  finish_cpp_member_func_call(func_type, 1);
  return 1;
}

static int try_call_cpp_conversion_operator(CType *target, int allow_explicit)
{
  CType receiver = vtop->type;
  Sym *function, *signature;
  VirtualMethodInfo *virtual_method;
  int method = 0, owner;
  if (allow_explicit != 2 && (target->t & VT_BTYPE) == VT_VOID) return 0;
  if (is_reference_type(&receiver)) receiver = *pointed_type(&receiver);
  if ((receiver.t & VT_BTYPE) != VT_STRUCT) return 0;
  if (allow_explicit != 2) {
    CType destination = is_reference_type(target) ? *pointed_type(target) : *target;
    int destination_class = get_struct_type_name_tok(&destination);
    int source_class = get_struct_type_name_tok(&receiver);
    if (destination_class && (destination_class == source_class
        || class_has_base(source_class, destination_class))) return 0;
  }
  function = find_cpp_conversion_operator(&receiver, target, allow_explicit,
                                           &method, NULL);
  if (!function) return 0;
  instantiate_template_member_body_for_func_tok(&receiver, method, function->v);
  signature = function->type.ref;
  if (!signature || !signature->next || signature->next->next)
    cprime_error("conversion operator must have no explicit parameters");
  owner = get_struct_type_name_tok(&receiver);
  virtual_method = find_virtual_method_for_symbol(owner, function->v);
  maybe_indir_reference();
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  save_lvalues();
  if (virtual_method)
    push_virtual_call_target(owner, virtual_method, function);
  else
    vpushsym(&function->type, function);
  vswap();
  gfunc_param_typed(signature, signature->next);
  finish_cpp_member_func_call(signature, 1);
  return 1;
}

static int try_call_cpp_index_operator(void)
{
  int method_tok = tok_alloc_const("operator[]");
  Sym *func_sym;
  Sym *func_type;
  Sym *sa;

  if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  materialize_incomplete_template_type(&vtop[-1].type);
  func_sym = resolve_member_func_by_arg_types(&vtop[-1].type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
    func_sym = resolve_member_field_func_by_arg_types(&vtop[-1].type,
                                                      method_tok,
                                                      &vtop->type, 1);
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

static int try_call_cpp_call_operator(void)
{
  const int method_tok = tok_alloc_const("operator()");
  TokenString *call_args[CPC_MAX_CALL_ARGUMENTS];
  CType call_arg_types[CPC_MAX_CALL_ARGUMENTS];
  SValue saved_receiver;
  SValue *saved_vtop;
  Sym *func_sym, *func_type, *sa;
  int receiver_tok, call_arg_count, ai, nb_args;
  int direct_result, result_address = 0, result_storage = VT_LOCAL | VT_LVAL;

  if ((vtop->type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  materialize_incomplete_template_type(&vtop->type);
  receiver_tok = get_struct_type_name_tok(&vtop->type);
  if (!type_has_member_func_name(&vtop->type, method_tok)
      && (!receiver_tok
          || !class_or_inst_has_member_template_name(receiver_tok,
                                                     method_tok)))
    return 0;

  saved_vtop = vtop;
  saved_receiver = *vtop;
  next();
  call_arg_count = count_saved_call_args(call_args, CPC_MAX_CALL_ARGUMENTS);
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);

  instantiate_template_member_for_call(&saved_receiver.type, method_tok,
                                       call_arg_types, call_arg_count);
  vtop = saved_vtop;
  *vtop = saved_receiver;
  func_sym = resolve_member_func_by_arg_types(&saved_receiver.type, method_tok,
                                              call_arg_types,
                                              call_arg_count);
  if (!func_sym)
    func_sym = resolve_member_field_func_by_arg_types(&saved_receiver.type,
                                                      method_tok,
                                                      call_arg_types,
                                                      call_arg_count);
  if (!func_sym
      && member_overload_count_for_call(&saved_receiver.type, method_tok,
                                        call_arg_count) == 1)
    func_sym = resolve_member_func_by_arg_count(&saved_receiver.type,
                                                method_tok,
                                                call_arg_count);
  if (!func_sym)
    func_sym = resolve_member_func(&saved_receiver.type, method_tok);
  if (!func_sym)
    cprime_error("no matching call operator for '%s'",
                 receiver_tok ? get_tok_str(receiver_tok, NULL)
                              : "<anonymous>");
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("call operator target is not declared as function");
  {
    const CppMemberDeclInfo *declaration = lookup_cpp_member_decl(func_sym->v);
    if (declaration && declaration->access_known && declaration->access
        && active_member_class_tok != declaration->owner_tok
        && !(declaration->access == 1 && active_member_class_tok
             && class_has_base(active_member_class_tok, declaration->owner_tok)))
      cprime_error("call operator is inaccessible in this context");
  }

  vtop = saved_vtop;
  *vtop = saved_receiver;
  func_type = func_sym->type.ref;
  direct_result = take_cpp_construction_destination(&func_type->type,
                    &result_address, &result_storage);
  sa = func_type->next;
  if (!sa)
    cprime_error("call operator is missing its receiver parameter");

  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  save_lvalues();
  vpushsym(&func_sym->type, func_sym);
  vswap();
  gfunc_param_typed(func_type, sa);
  sa = sa->next;
  nb_args = 1;

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    emit_saved_arg_for_param(call_args[ai], sa);
    tok_str_free(call_args[ai]);
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
  finish_cpp_member_func_call_to(func_type, nb_args, direct_result,
                                result_storage, result_address);
  return 1;
}

static int try_call_cpp_assignment_operator(void)
{
  int method_tok = tok_alloc_const("operator=");
  Sym *func_sym;
  Sym *func_type;
  Sym *sa, *this_arg, *value_arg;
  CType receiver_type;

  if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  receiver_type = vtop[-1].type;
  if (!(vtop[-1].r & VT_LVAL)
      || ((vtop[-1].r & VT_VALMASK) < VT_CONST))
    receiver_type.t &= ~VT_CONSTANT;
  func_sym = resolve_member_func_by_arg_types(&receiver_type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
  {
    func_sym = resolve_member_field_func_by_arg_types(&vtop[-1].type,
                                                      method_tok,
                                                      &vtop->type, 1);
  }
  /* An implicit assignment still accepts a converting-constructor temporary.
     Materialize it before retrying so its class value category also selects
     the appropriate copy/move member, or the implicit memberwise path. */
  if (!func_sym
      && !is_compatible_unqualified_types(&receiver_type, &vtop->type)
      && try_call_cpp_conversion_operator(&receiver_type, 0)) {
    maybe_indir_reference();
    return try_call_cpp_assignment_operator();
  }
  if (!func_sym
      && !is_compatible_unqualified_types(&receiver_type, &vtop->type)
      && try_materialize_constructor_conversion(&receiver_type))
    return try_call_cpp_assignment_operator();
  if (!func_sym && receiver_type.ref
      && receiver_type.ref->a.cpp_nontrivial_copy_assignment)
    cprime_error("no matching user-declared copy assignment operator");
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
  vtop->type = receiver_type;
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  vswap();
  save_lvalues();
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
    return try_call_cpp_free_binary_operator(op);
  func_sym = resolve_member_func_by_arg_types(&vtop[-1].type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
    func_sym = resolve_member_field_func_by_arg_types(&vtop[-1].type, method_tok,
                                                      &vtop->type, 1);
  if (!func_sym)
    return try_call_cpp_free_binary_operator(op);
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
