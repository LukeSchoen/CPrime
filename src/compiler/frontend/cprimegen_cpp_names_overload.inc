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

static int find_current_class_nested_type_tok(int name_tok)
{
  int class_i;

  if (name_tok < TOK_UIDENT)
    return name_tok;
  if (active_member_class_tok)
  {
    int nested_tok;
    if (class_tok_matches_unqualified_name(active_member_class_tok, name_tok))
      return active_member_class_tok;
    nested_tok = make_static_member_tok(active_member_class_tok, name_tok);
    if (struct_find(nested_tok))
      return nested_tok;
  }
  for (class_i = nb_defining_class_stack - 1; class_i >= 0; --class_i)
  {
    if (class_tok_matches_unqualified_name(defining_class_stack[class_i], name_tok))
      return defining_class_stack[class_i];
    int nested_tok = make_static_member_tok(defining_class_stack[class_i],
                                            name_tok);
    if (struct_find(nested_tok))
      return nested_tok;
  }
  return name_tok;
}

static int make_namespace_tok_from_parts(const int *parts, int nb_parts)
{
  CString name;
  int i = 0, result;

  if (nb_parts <= 0)
    return 0;
  cstr_new(&name);
  if (!strncmp(get_tok_str(parts[0], NULL), "__cpc_ns_", 9))
  {
    cstr_cat(&name, get_tok_str(parts[0], NULL), -1);
    i = 1;
  }
  else
    cstr_cat(&name, "__cpc_ns", -1);
  for (; i < nb_parts; ++i)
  {
    cstr_ccat(&name, '_');
    cstr_cat(&name, get_tok_str(parts[i], NULL), -1);
  }
  cstr_ccat(&name, 0);
  result = tok_alloc_const(name.data);
  cstr_free(&name);
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
  memcpy(scope->parts, namespace_stack, sizeof(namespace_stack));
}

static int symbol_namespace_scope_tok(int symbol_tok)
{
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
  for (i = 0; i < nb_namespace_scopes; ++i)
    if (namespace_scopes[i].name_tok == t)
      return 1;
  return sym_find(t) || global_symbol_find(t) || struct_find(t)
         || find_class_template_def(t) || find_function_template_def(t);
}

static int find_namespace_tok_from_parts(const int *parts, int nb_parts)
{
  int t = make_namespace_tok_from_parts(parts, nb_parts);
  int expanded[18], i, candidate;
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
  if (local && local->sym_scope)
    return name_tok;
  for (i = 0; i < nb_namespace_stack; ++i)
    parts[i] = namespace_stack[i];
  for (depth = nb_namespace_stack; depth >= 0; --depth)
  {
    parts[depth] = name_tok;
    t = depth ? find_namespace_tok_from_parts(parts, depth + 1) : name_tok;
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

static int append_type_mangle_string(CString *name, CType *type)
{
  int bt = type->t & VT_BTYPE;
  CType pt;
  char part[128];

  if (bt == VT_PTR)
  {
    pt = *pointed_type(type);
    if (!append_type_mangle_string(name, &pt))
      return 0;
    if (type->t & VT_RVALUE_REFERENCE)
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
    pstrcpy(part, sizeof(part), (type->t & VT_LONG) ? "ldouble" : "double");
  else if (bt == VT_LDOUBLE)
    pstrcpy(part, sizeof(part), "ldouble");
  else if (bt == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
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
    if (!append_type_mangle_string(&name, &arg->type))
      cprime_error("unsupported overloaded function parameter type");
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
          && o->explicit_arg_count == 0
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

  note_member_function_scope(mangled_tok, struct_tok);
  overload_tok_set_note(member_func_mangled_toks, mangled_tok);

  if (lowered_type && lowered_type->ref)
  {
    this_arg = lowered_type->ref->next;
    if (this_arg && (this_arg->type.t & VT_BTYPE) == VT_PTR)
      is_const = pointed_type(&this_arg->type)->t & VT_CONSTANT;
  }

  for (o = member_func_candidates(struct_tok, method_tok);
       o; o = o->bucket_next)
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
  unsigned bucket = (unsigned)function_tok & 1023;
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

static int member_function_class_tok(int function_tok)
{
  MemberFunctionScope *scope;
  for (scope = member_function_scopes[(unsigned)function_tok & 1023];
       scope; scope = scope->next)
    if (scope->function_tok == function_tok)
      return scope->class_tok;
  return 0;
}

static void note_free_func_overload(int name_tok, int mangled_tok,
                                    CType *func_type)
{
  FreeFuncOverload *o;

  overload_tok_set_note(free_func_name_toks, name_tok);

  for (o = free_func_overloads; o; o = o->next)
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
  o->mangled_tok = mangled_tok;
  o->explicit_arg_count = func_explicit_arg_count(func_type);
  o->min_arg_count = func_min_arg_count(func_type);
  if (func_type)
    o->func_type = *func_type;
  o->next = free_func_overloads;
  free_func_overloads = o;
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
  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == tok && o->mangled_tok == tok
        && o->is_provisional_auto_member)
      return 1;
  return 0;
}

static int is_mangled_free_func_tok(int tok)
{
  FreeFuncOverload *o;
  for (o = free_func_overloads; o; o = o->next)
    if (o->mangled_tok == tok && o->name_tok != tok)
      return 1;
  return 0;
}

static int has_free_func_overload(int name_tok)
{
  FreeFuncOverload *o;
  int indexed = overload_tok_set_has(free_func_name_toks, name_tok);

  if (indexed >= 0)
    return indexed;

  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == name_tok)
      return 1;
  return 0;
}

static int is_member_func_mangled_tok(int tok)
{
  MemberFuncOverload *o;
  int indexed = overload_tok_set_has(member_func_mangled_toks, tok);

  if (indexed >= 0)
    return indexed;

  for (o = member_func_overloads; o; o = o->next)
    if (o->mangled_tok == tok)
      return 1;
  return 0;
}

static int free_func_overload_exists_for_arg_count(int name_tok,
                                                   int explicit_arg_count)
{
  FreeFuncOverload *o;

  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == name_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
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

static int prepare_free_func_declarator(int name_tok, CType *func_type)
{
  int mangled_tok;
  Sym *base;

  if (getenv("CPC_TRACE_FREE_DECL")
      && strstr(get_tok_str(name_tok, NULL), "clMaxComponent"))
    fprintf(stderr, "CPC_FREE_DECL_IN name=%s ret_t=%04x ret_s=%s argc=%d\n",
            get_tok_str(name_tok, NULL),
            func_type && func_type->ref ? func_type->ref->type.t : 0,
            func_type && func_type->ref
              ? get_tok_str(get_struct_type_name_tok(&func_type->ref->type), NULL)
              : "<none>",
            func_explicit_arg_count(func_type));
  base = find_global_symbol(name_tok);
  if (base && (base->type.t & VT_BTYPE) == VT_FUNC
      && same_func_param_signature(&base->type, func_type))
    preserve_func_default_args(func_type, &base->type);
  mangled_tok = make_free_func_tok_for_type(name_tok, func_type);
  if (getenv("CPC_TRACE_FREE_DECL")
      && strstr(get_tok_str(name_tok, NULL), "clMaxComponent"))
    fprintf(stderr, "CPC_FREE_DECL_OUT name=%s mangled=%s ret_t=%04x ret_s=%s base=%s\n",
            get_tok_str(name_tok, NULL), get_tok_str(mangled_tok, NULL),
            func_type && func_type->ref ? func_type->ref->type.t : 0,
            func_type && func_type->ref
              ? get_tok_str(get_struct_type_name_tok(&func_type->ref->type), NULL)
              : "<none>",
            base ? get_tok_str(base->v, NULL) : "<none>");
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
  if (getenv("CPC_TRACE_SHALLOW")
      && token_string_contains_tok(&macro, tok_alloc_const("clVector2")))
    fprintf(stderr, "CPC_SHALLOW default-arg func=%s len=%d\n",
            funcname ? funcname : "<none>", macro.len);
  begin_macro(&macro, 0);
  next();
  expr_eq();
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
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

static void infer_saved_arg_type(TokenString *arg, CType *type)
{
  SValue *saved_vtop = vtop;
  int saved_nocode_wanted = nocode_wanted;
  int saved_tok = tok;
  CValue saved_tokc = tokc;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;
  int is_single_cchar = saved_arg_is_single_cchar(arg);
  int is_null_pointer_constant;
  TokenString macro;

  if (saved_arg_is_braced(arg))
  {
    type->t = VT_INT;
    type->ref = NULL;
    return;
  }
  if (saved_arg_functional_temporary_type(arg, type))
    return;

  prepare_saved_cpp_lambdas(arg);
  nocode_wanted++;
  if (getenv("CPC_TRACE_SHALLOW")
      && token_string_contains_tok(arg, tok_alloc_const("clVector2")))
    fprintf(stderr, "CPC_SHALLOW infer-saved func=%s len=%d\n",
            funcname ? funcname : "<none>", arg->len);
  /* end_macro() resets a non-owning macro buffer.  Replay a shallow view so
     overload inference does not consume the saved argument that emission
     still needs. */
  macro = *arg;
  begin_macro(&macro, 0);
  next();
  expr_eq();
  is_null_pointer_constant =
    (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
    && is_integer_btype(vtop->type.t & VT_BTYPE) && vtop->c.i == 0;
  *type = vtop->type;
  if (!(vtop->r & VT_LVAL)
      || (((type->t & VT_BTYPE) == VT_STRUCT)
          && saved_arg_is_functional_temporary(arg)))
    type->t |= VT_RVALUE_REFERENCE;
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  if (is_single_cchar)
    type->t = VT_BYTE;
  if (is_null_pointer_constant)
    type->t |= VT_NULLPTR_TYPE;
  vtop = saved_vtop;
  nocode_wanted = saved_nocode_wanted;
  tok = saved_tok;
  tokc = saved_tokc;
}

static void infer_saved_arg_types(TokenString **args, CType *types, int nb_args)
{
  int i;

  for (i = 0; i < nb_args; ++i)
    infer_saved_arg_type(args[i], &types[i]);
}

static int emit_saved_arg_for_param(TokenString *arg, Sym *param)
{
  int saved_tok = tok;
  CValue saved_tokc = tokc;
  if (saved_arg_is_braced(arg) && param
      && (((param->type.t & VT_BTYPE) == VT_STRUCT)
          || (is_reference_type(&param->type)
              && ((pointed_type(&param->type)->t & VT_BTYPE) == VT_STRUCT))))
  {
    CType temp_type;
    TokenString macro;
    TokenString *saved_macro_stack = macro_stack;
    const int *saved_macro_ptr = macro_ptr;

    temp_type = is_reference_type(&param->type) ? *pointed_type(&param->type)
                                               : param->type;
    macro = *arg;
    if (getenv("CPC_TRACE_SHALLOW")
        && token_string_contains_tok(&macro, tok_alloc_const("clVector2")))
      fprintf(stderr, "CPC_SHALLOW braced-arg func=%s len=%d\n",
              funcname ? funcname : "<none>", macro.len);
    begin_macro(&macro, 0);
    next();
    materialize_braced_temporary(&temp_type);
    while (macro_stack && macro_stack != saved_macro_stack)
      end_macro();
    macro_ptr = saved_macro_ptr;
    tok = saved_tok;
    tokc = saved_tokc;
    return 1;
  }

  {
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;
  begin_macro(arg, 1);
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
    if (o->name_tok != name_tok || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
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

static int call_arg_array_reference_compatible(CType *param_type, CType *arg_type)
{
  CType *array;
  CType argument = *arg_type;
  if (!is_reference_type(param_type))
    return 1;
  array = pointed_type(param_type);
  if (!(array->t & VT_ARRAY))
    return 1;
  argument.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&argument))
    argument = *pointed_type(&argument);
  return (argument.t & VT_ARRAY) && array->ref && argument.ref
         && (array->ref->c < 0 || array->ref->c == argument.ref->c);
}

static int call_arg_matches_param_type(CType *param_type, CType *arg_type)
{
  CType decayed_arg, arg_value_type;
  CType param_value_type;
  CType normalized_arg;
  CType *param_pointed, *arg_pointed;

  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  param_value_type = *param_type;
  param_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&param_value_type))
    param_value_type = *pointed_type(&param_value_type);
  if (!call_arg_array_reference_compatible(param_type, arg_type))
    return 0;
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

static int call_arg_match_rank(CType *param_type, CType *arg_type)
{
  CType arg_value_type;
  CType param_value_type;
  CType normalized_arg;
  CType *param_pointed;

  if (!call_arg_array_reference_compatible(param_type, arg_type))
    return -1;

  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  param_value_type = *param_type;
  param_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&param_value_type))
    param_value_type = *pointed_type(&param_value_type);
  if (is_reference_type(param_type) && (param_value_type.t & VT_ARRAY))
  {
    CType *target_element = pointed_type(&param_value_type);
    CType *source_element = pointed_type(&arg_value_type);
    int cv = VT_CONSTANT | VT_VOLATILE;
    if ((source_element->t & cv & ~target_element->t)
        || !is_compatible_unqualified_types(target_element, source_element))
      return -1;
    return ((target_element->t ^ source_element->t) & cv) ? 2 : 1;
  }
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
  if (call_arg_exact_param_type(param_type, arg_type))
  {
    if (is_reference_type(param_type))
    {
      if ((param_type->t & VT_RVALUE_REFERENCE)
          && !(arg_type->t & VT_RVALUE_REFERENCE))
        return -1;
      if ((param_type->t & VT_RVALUE_REFERENCE)
          && (arg_type->t & VT_RVALUE_REFERENCE))
        return 0;
      if ((arg_type->t & VT_RVALUE_REFERENCE)
          && (pointed_type(param_type)->t & VT_CONSTANT))
        return 2;
    }
    return 1;
  }
  if (IS_ENUM(param_value_type.t) || IS_ENUM_VAL(param_value_type.t))
    return -1;
  if (is_reference_type(param_type))
  {
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
        return 0;
      if ((arg_type->t & VT_RVALUE_REFERENCE)
          && (param_pointed->t & VT_CONSTANT))
        return 2;
      return 1;
    }
    if (class_value_is_derived_from(&arg_value_type, param_pointed))
      return 3;
    if (!(param_type->t & VT_RVALUE_REFERENCE)
        && (param_pointed->t & VT_CONSTANT)
        && btype_is_arithmetic_scalar(param_pointed->t & VT_BTYPE)
        && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
      return 4;
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
    return call_arg_matches_param_type(param_type, arg_type) ? 3 : -1;
  if (compare_types(param_type, &arg_value_type, 1))
    return 1;
  if (btype_is_arithmetic_scalar(param_value_type.t & VT_BTYPE)
      && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
    return 4;
  if ((param_type->t & VT_BTYPE) == VT_STRUCT
      && class_has_single_arg_constructor_for(param_type, &arg_value_type))
    return 8;
  return call_arg_matches_param_type(param_type, arg_type) ? 3 : -1;
}

static Sym *resolve_free_func_by_arg_types_filtered(int name_tok, CType *arg_types,
                                           int explicit_arg_count,
                                           int template_deduction_failed,
                                           int *candidates, int candidate_count)
{
  FreeFuncOverload *o;
  Sym *s, *arg, *match = NULL;
  int i, ok, matches = 0, best_arg_rank = 0x7fffffff;
  int best_is_provisional = 0;
  int best_is_template = 0;

  for (o = free_func_overloads; o; o = o->next)
  {
    int arg_rank = 0;
    int candidate_is_template;
    if (o->name_tok != name_tok || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      continue;
    if (candidates)
    {
      for (i = 0; i < candidate_count; ++i)
        if (candidates[i] == o->mangled_tok)
          break;
      if (i == candidate_count)
        continue;
    }
    if (template_deduction_failed
        && find_template_inst_owner_entry(o->mangled_tok))
      continue;
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
        ok = 0;
        break;
      }
      rank = call_arg_match_rank(&arg->type, &arg_types[i]);
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
    candidate_is_template = find_template_inst_owner_entry(o->mangled_tok) != NULL;
    for (; arg; arg = arg->next)
      if (!arg->default_arg)
      {
        ok = 0;
        break;
      }
    if (!ok)
      continue;
    if (getenv("CPC_DUMP_FREE_RESOLVE")
        && name_tok >= TOK_UIDENT)
    {
      CType *pt = s->type.ref->next ? &s->type.ref->next->type : NULL;
      CType *at = explicit_arg_count ? &arg_types[0] : NULL;
      fprintf(stderr,
              "CPC_FREE_OK name=%s rank=%d cand=%s p0=%04x/%04x a0=%04x/%04x\n",
              get_tok_str(name_tok, NULL), arg_rank, get_tok_str(s->v, NULL),
              pt ? pt->t : 0,
              pt && (pt->t & VT_BTYPE) == VT_PTR ? pointed_type(pt)->t : 0,
              at ? at->t : 0,
              at && (at->t & VT_BTYPE) == VT_PTR ? pointed_type(at)->t : 0);
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
    if (getenv("CPC_DUMP_FREE_RESOLVE")
        && name_tok >= TOK_UIDENT)
      fprintf(stderr, "CPC_FREE_CAND name=%s rank=%d cand=%s\n",
              get_tok_str(name_tok, NULL), arg_rank, get_tok_str(s->v, NULL));
    match = s;
    matches++;
  }
  if (matches > 1)
    cprime_error("ambiguous overloaded function '%s'",
              get_tok_str(name_tok, NULL));
  if (match && getenv("CPC_DUMP_FREE_RESOLVE")
      && name_tok >= TOK_UIDENT)
    fprintf(stderr, "CPC_FREE_RESOLVE name=%s argc=%d -> %s\n",
            get_tok_str(name_tok, NULL), explicit_arg_count,
            get_tok_str(match->v, NULL));
  return match;
}

static Sym *resolve_free_func_by_arg_types(int name_tok, CType *arg_types,
                                           int explicit_arg_count,
                                           int template_deduction_failed)
{
  return resolve_free_func_by_arg_types_filtered(name_tok, arg_types,
                   explicit_arg_count, template_deduction_failed, NULL, 0);
}

static int call_arg_exact_param_type(CType *param_type, CType *arg_type)
{
  CType param = *param_type;
  CType arg = *arg_type;

  if (!call_arg_array_reference_compatible(param_type, arg_type))
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
  /* Top-level cv qualification does not change an exact binding match.  In
     particular, a value argument is an exact match for `const T&`. */
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
  if ((param.t & VT_BTYPE) != VT_BYTE
      || ((param.t & VT_UNSIGNED) && (arg.t & VT_UNSIGNED)))
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

  for (o = free_func_overloads; o; o = o->next)
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

  td = find_function_template_for_call(name_tok, arg_types, arg_count);

  if (!td)
    return 0;
  if (!infer_template_args_from_call(td, arg_types, arg_count, &args))
    return 1;
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
        && !global_symbol_find(tok_alloc_const(name)))
      return tok_alloc_const(name);
    pstrcat(name, sizeof(name), "_0args");
    return tok_alloc_const(name);
  }
  if (!same_count && !other_count && !sym_find(tok_alloc_const(name))
      && !global_symbol_find(tok_alloc_const(name)))
    return tok_alloc_const(name);
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    if (TOK_HAS_VALUE(params->str[i]))
    {
      i += tok_str_value_extra_words(params->str, params->len, i);
      continue;
    }
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

  overload_tok_set_note(member_func_mangled_toks, mangled_tok);

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
  case TOK_INC: return "operator++";
  case TOK_DEC: return "operator--";
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

  last_cpp_conversion_operator_type_tokens = NULL;
  if (tok != TOK_OPERATOR)
    cprime_error("operator");
  next();
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
    int first = 1;
    if (tok == TOK_EOF || tok == '(')
      cprime_error("unsupported operator overload");
    pstrcpy(opname, sizeof(opname), "operator ");
    last_cpp_conversion_operator_type_tokens = tok_str_alloc();
    while (tok != TOK_EOF && tok != '(')
    {
      if (!first)
        pstrcat(opname, sizeof(opname), " ");
      pstrcat(opname, sizeof(opname), get_tok_str(tok, NULL));
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

static void finish_cpp_member_func_call(Sym *func_type, int nb_args)
{
  SValue ret;
  int ret_nregs, ret_align, regsize, variadic;
  int size, align, n, r, t;

  ret.r2 = VT_CONST;
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_FINISH enter ret=%04x/%s depth=%d\n",
            func_type->type.t,
            get_tok_str(get_struct_type_name_tok(&func_type->type), NULL),
            (int)(vtop - vstack));
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
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_FINISH called ret=%04x/%s ret_nregs=%d depth=%d\n",
            func_type->type.t,
            get_tok_str(get_struct_type_name_tok(&func_type->type), NULL),
            ret_nregs, (int)(vtop - vstack));

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
  maybe_indir_call_result();
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
    adjust_template_member_auto_return_for_call(&saved_lhs.type, method_tok,
                                                &saved_rhs.type, 1, func_sym);
    vtop = saved_operand_vtop;
    vtop[-1] = saved_lhs;
    *vtop = saved_rhs;
  }
  if (func_sym && getenv("CPC_DUMP_MEMBER_RESOLVE"))
    fprintf(stderr, "CPC_BINARY_RESOLVE lhs=%s rhs_t=%04x method=%s -> %s ret_t=%04x\n",
            get_tok_str(get_struct_type_name_tok(&vtop[-1].type), NULL),
            vtop->type.t, get_tok_str(method_tok, NULL),
            get_tok_str(func_sym->v, NULL),
            func_sym->type.ref ? func_sym->type.ref->type.t : 0);
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

  /* Convert lhs/rhs into member call args: operator(this, rhs). */
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT start below=%04x top=%04x depth=%d\n",
            vtop[-1].type.t, vtop->type.t, (int)(vtop - vstack));
  vswap();
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT swap below=%04x top=%04x depth=%d\n",
            vtop[-1].type.t, vtop->type.t, (int)(vtop - vstack));
  mk_pointer(&vtop->type);
  gaddrof();
  gv(RC_INT);
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT addr below=%04x top=%04x depth=%d\n",
            vtop[-1].type.t, vtop->type.t, (int)(vtop - vstack));
  vswap();
  save_lvalues();
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT saved below=%04x top=%04x ret=%s depth=%d\n",
            vtop[-1].type.t, vtop->type.t,
            get_tok_str(get_struct_type_name_tok(&func_type->type), NULL),
            (int)(vtop - vstack));
  gfunc_param_typed(func_type, sa->next);
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT param1 ret=%s depth=%d\n",
            get_tok_str(get_struct_type_name_tok(&func_type->type), NULL),
            (int)(vtop - vstack));
  vswap();
  gfunc_param_typed(func_type, sa);
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT param2 ret=%s depth=%d\n",
            get_tok_str(get_struct_type_name_tok(&func_type->type), NULL),
            (int)(vtop - vstack));
  vswap();
  vpushsym(&func_sym->type, func_sym);
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT pushed ret=%s depth=%d\n",
            get_tok_str(get_struct_type_name_tok(&func_type->type), NULL),
            (int)(vtop - vstack));
  vrott(3);
  finish_cpp_member_func_call(func_type, 2);
  if (getenv("CPC_TRACE_MATRIX"))
    fprintf(stderr, "CPC_MATRIX_EMIT finish top=%04x/%s depth=%d\n",
            vtop->type.t,
            get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
            (int)(vtop - vstack));
  return 1;
}

static int try_call_cpp_free_binary_operator(int op)
{
  int method_tok;
  int template_deduction_failed;
  CType arg_types[2];
  Sym *func_sym, *func_type, *sa;

  method_tok = get_cpp_binary_operator_method_tok(op);
  if (!method_tok)
    return 0;

  arg_types[0] = vtop[-1].type;
  arg_types[1] = vtop->type;
  template_deduction_failed =
    instantiate_function_templates_for_call(method_tok, arg_types, 2);
  func_sym = resolve_free_func_by_arg_types(method_tok, arg_types, 2,
                                            template_deduction_failed);
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");

  func_type = func_sym->type.ref;
  sa = func_type->next;
  if (!sa || !sa->next || sa->next->next)
    cprime_error("operator overload must have exactly two parameters");

  save_lvalues();
  gfunc_param_typed_with_conversions(func_type, sa->next);
  vswap();
  gfunc_param_typed_with_conversions(func_type, sa);
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
  {
    if (getenv("CPC_DUMP_MEMBER_RESOLVE") && op == '*')
      fprintf(stderr, "CPC_UNARY_RESOLVE op=* recv=%s t=%04x -> <none>\n",
              get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
              vtop->type.t);
    return 0;
  }
  if (getenv("CPC_DUMP_MEMBER_RESOLVE") && op == '*')
    fprintf(stderr, "CPC_UNARY_RESOLVE op=* recv=%s t=%04x -> %s\n",
            get_tok_str(get_struct_type_name_tok(&vtop->type), NULL),
            vtop->type.t, get_tok_str(func_sym->v, NULL));
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

static int try_call_cpp_bool_conversion_operator(void)
{
  int method_tok = tok_alloc_const("operator bool");
  Sym *func_sym;
  Sym *canonical_sym = NULL;
  Sym *func_type;
  Sym *sa;
  int trace_depth_before = 0;

  if ((vtop->type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  func_sym = resolve_member_func_by_arg_count(&vtop->type, method_tok, 0);
  if (!func_sym)
    func_sym = resolve_member_func(&vtop->type, method_tok);
  if (!func_sym)
    return 0;
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("conversion operator target is not declared as function");
  if (getenv("CPC_TRACE_CLBITREF")
      && get_struct_type_name_tok(&vtop->type)
      && strstr(get_tok_str(get_struct_type_name_tok(&vtop->type), NULL), "clBitRef"))
    trace_depth_before = (int)(vtop - vstack);
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
  if (trace_depth_before
      && getenv("CPC_TRACE_CLBITREF"))
    fprintf(stderr, "CPC_CLBITREF_BOOL_DEPTH before=%d after=%d ret_t=%04x ret_r=%04x\n",
            trace_depth_before, (int)(vtop - vstack), vtop->type.t, vtop->r);
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
  func_sym = resolve_member_func_by_arg_types(&vtop[-1].type, method_tok,
                                              &vtop->type, 1);
  if (!func_sym)
    func_sym = resolve_member_field_func_by_arg_types(&vtop[-1].type,
                                                      method_tok,
                                                      &vtop->type, 1);
  if (!func_sym)
    return 0;
  if (getenv("CPC_DUMP_MEMBER_RESOLVE"))
    fprintf(stderr, "CPC_COMPOUND_RESOLVE lhs=%s rhs_t=%04x method=%s -> %s\n",
            get_tok_str(get_struct_type_name_tok(&vtop[-1].type), NULL),
            vtop->type.t, get_tok_str(method_tok, NULL),
            get_tok_str(func_sym->v, NULL));
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
  TokenString *call_args[32];
  CType call_arg_types[32];
  SValue saved_receiver;
  SValue *saved_vtop;
  Sym *func_sym, *func_type, *sa;
  int receiver_tok, call_arg_count, ai, nb_args;

  if ((vtop->type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  receiver_tok = get_struct_type_name_tok(&vtop->type);
  if (!type_has_member_func_name(&vtop->type, method_tok)
      && (!receiver_tok
          || !class_or_inst_has_member_template_name(receiver_tok,
                                                     method_tok)))
    return 0;

  saved_vtop = vtop;
  saved_receiver = *vtop;
  next();
  call_arg_count = count_saved_call_args(call_args, 32);
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

  adjust_template_member_auto_return_for_call(&saved_receiver.type,
                                              method_tok,
                                              call_arg_types,
                                              call_arg_count, func_sym);
  vtop = saved_vtop;
  *vtop = saved_receiver;
  func_type = func_sym->type.ref;
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
  finish_cpp_member_func_call(func_type, nb_args);
  return 1;
}

static int try_call_cpp_assignment_operator(void)
{
  int method_tok = tok_alloc_const("operator=");
  Sym *func_sym;
  Sym *func_type;
  Sym *sa, *this_arg, *value_arg;
  CType receiver_type;
  int trace_depth_before = 0;

  if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  receiver_type = vtop[-1].type;
  if (!(vtop[-1].r & VT_LVAL)
      || ((vtop[-1].r & VT_VALMASK) < VT_CONST))
    receiver_type.t &= ~VT_CONSTANT;
  func_sym = resolve_member_func_by_arg_types(&receiver_type, method_tok,
                                              &vtop->type, 1);
  if (getenv("CPC_TRACE_CLBITREF")
      && get_struct_type_name_tok(&vtop[-1].type)
      && strstr(get_tok_str(get_struct_type_name_tok(&vtop[-1].type), NULL), "clBitRef"))
    fprintf(stderr, "CPC_CLBITREF_ASSIGN first=%s lhs_t=%04x lhs_r=%04x recv_t=%04x\n",
            func_sym ? get_tok_str(func_sym->v, NULL) : "<none>",
            vtop[-1].type.t, vtop[-1].r, receiver_type.t);
  if (!func_sym)
  {
    func_sym = resolve_member_field_func_by_arg_types(&vtop[-1].type,
                                                      method_tok,
                                                      &vtop->type, 1);
  }
  if (!func_sym)
    return 0;
  if (getenv("CPC_DUMP_MEMBER_RESOLVE"))
    fprintf(stderr, "CPC_COMPOUND_RESOLVE lhs=%s rhs_t=%04x method=%s -> %s\n",
            get_tok_str(get_struct_type_name_tok(&vtop[-1].type), NULL),
            vtop->type.t, get_tok_str(method_tok, NULL),
            get_tok_str(func_sym->v, NULL));
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("operator overload target is not declared as function");
  if (getenv("CPC_TRACE_CLBITREF")
      && get_struct_type_name_tok(&vtop[-1].type)
      && strstr(get_tok_str(get_struct_type_name_tok(&vtop[-1].type), NULL), "clBitRef"))
    trace_depth_before = (int)(vtop - vstack);
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
  if (trace_depth_before
      && getenv("CPC_TRACE_CLBITREF"))
    fprintf(stderr, "CPC_CLBITREF_ASSIGN_DEPTH before=%d after=%d ret_t=%04x ret_r=%04x below_t=%04x below_r=%04x\n",
            trace_depth_before, (int)(vtop - vstack), vtop->type.t, vtop->r,
            vtop > vstack ? vtop[-1].type.t : 0,
            vtop > vstack ? vtop[-1].r : 0);
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
  if (getenv("CPC_DUMP_MEMBER_RESOLVE"))
    fprintf(stderr, "CPC_COMPOUND_RESOLVE lhs=%s rhs_t=%04x method=%s -> %s\n",
            get_tok_str(get_struct_type_name_tok(&vtop[-1].type), NULL),
            vtop->type.t, get_tok_str(method_tok, NULL),
            get_tok_str(func_sym->v, NULL));
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
