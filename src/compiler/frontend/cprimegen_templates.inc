static int infer_template_args_from_call_with_explicit(TemplateDef *, CType *, int, TemplateArgList *, TemplateArgList *);
static int template_typedef_is_type_pos(TokenString *spec);
static int template_probe_type_ex(TokenString *, int, int, int, int, CType *);
static int infer_saved_template_body_return(TemplateDef *, TemplateArgList *, CType *);
static int template_class_argument_for_deduction(TemplateDef *, int, int);
static int template_intern_address(Sym *target, CType *type);

static int template_substituted_pattern_argument(TemplateDef *definition,
                                                 TemplateArgList *bindings);
static int template_probe_type(TokenString *tokens, int argument,
                               int namespace_tok, CType *result);
static int template_member_def_first_param_class_kind(TemplateMemberDef *md,
                                                      int method_tok);
static int template_member_def_first_param_type_tok(TemplateMemberDef *md,
                                                    int method_tok);

static TemplateDef *find_template_def(int name_tok)
{
  TemplateDef *td;
  for (td = template_def_candidates(name_tok); td;
       td = td->lookup_bucket_next)
  {
    if (template_def_lookup_tok(td) == name_tok)
      return td;
  }
  return NULL;
}

static TemplateDef *find_class_template_def(int name_tok)
{
  TemplateDef *td;
  Sym *binding = sym_find(name_tok);
  if (binding && binding->cpp_using_target)
    name_tok = binding->cpp_using_target;

  for (td = template_def_candidates(name_tok); td;
       td = td->lookup_bucket_next)
  {
    if (!td->is_class)
      continue;
    if ((td->lookup_tok ? td->lookup_tok : td->name_tok) == name_tok)
      return td;
  }
  return NULL;
}

static TemplateDef *find_class_template_def_in_context(TemplateDef *context,
                                                        int name_tok)
{
  TemplateDef *td = find_class_template_def(name_tok);

  if (!td && context && context->namespace_tok && name_tok >= TOK_UIDENT)
    td = find_class_template_def(
           make_static_member_tok(context->namespace_tok, name_tok));
  return td;
}

static int template_def_first_param_matches_call(TemplateDef *td,
                                                 CType *arg_types,
                                                 int arg_count)
{
  int i, n, name_index = -1, paren_index = -1, depth = 0;
  CType arg_value_type;
  int pointer_pattern = 0;

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
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(td->def_str->str, n, i);
      continue;
    }
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
    if (t == '*' && depth == 0)
      pointer_pattern = i > 1 && td->def_str->str[i - 1] == ':'
                        && td->def_str->str[i - 2] == ':' ? 2 : 1;
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
        CType matched_type = arg_value_type;
        if ((matched_type.t & VT_BTYPE) == VT_PTR)
          matched_type = *pointed_type(&matched_type);
        if ((matched_type.t & VT_BTYPE) == VT_STRUCT)
          arg_tok = get_struct_type_name_tok(&matched_type);
        if (!arg_tok)
          return 0;
        for (j = 0; j < class_td->nb_inst; ++j)
          if (class_td->inst_name_toks[j] == arg_tok)
            return 1;
        return template_class_argument_for_deduction(class_td, arg_tok, 0) > 0;
      }
    }
  }
  if (pointer_pattern)
    return pointer_pattern == 2 ? find_cpp_member_pointer_type(&arg_value_type) != NULL
                                : (arg_value_type.t & VT_BTYPE) == VT_PTR;
  return 1;
}

static TemplateDef *template_pattern_class_definition(TemplateDef *context,
                                        TokenString *tokens, int begin, int end)
{
  int i, name = 0, colons = 0, parts[18], count = 0;
  TemplateDef *definition;
  for (i = begin; i <= end; ++i)
  {
    int token = tokens->str[i];
    if (token == TOK_LINENUM) { ++i; continue; }
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(tokens->str, tokens->len, i);
      name = colons = count = 0;
    }
    else if (token >= TOK_UIDENT)
    {
      if (colons != 2) count = 0;
      if (count == 17) cprime_error("qualified template name is too deep");
      parts[++count] = token;
      name = colons == 2 && name ? make_static_member_tok(name, token) : token;
      colons = 0;
    }
    else if (token == ':') ++colons;
    else name = colons = count = 0;
  }
  if (count > 1)
  {
    if (context && context->namespace_tok)
    {
      parts[0] = context->namespace_tok;
      definition = find_class_template_def(make_namespace_tok_from_parts(parts, count + 1));
      if (definition) return definition;
    }
    definition = find_class_template_def(find_namespace_tok_from_parts(parts + 1, count));
    if (definition) return definition;
  }
  return find_class_template_def_in_context(context, name);
}

static int template_def_first_param_pointer_pattern(TemplateDef *td)
{
  int i, n, paren_index = -1, depth = 0;

  if (!td || !td->def_str)
    return 0;
  n = td->def_str->len;
  for (i = 0; i + 1 < n; ++i)
    if (td->def_str->str[i] == td->name_tok
        && td->def_str->str[i + 1] == '(')
    {
      paren_index = i + 1;
      break;
    }
  if (paren_index < 0)
    return 0;
  for (i = paren_index + 1; i < n; ++i)
  {
    int t = td->def_str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(td->def_str->str, n, i);
      continue;
    }
    if (t == '(' || t == '[' || t == TOK_LT || t == '<')
      ++depth;
    else if ((t == ')' || t == ']'
              || t == TOK_GT || t == '>' || t == TOK_SAR)
             && depth > 0)
      --depth;
    else if ((t == ',' || t == ')') && depth == 0)
      break;
    else if (t == '*' && depth == 0)
      return 1;
  }
  return 0;
}

static int template_def_first_param_class_tok(TemplateDef *td)
{
  int i, n, name_index = -1, paren_index = -1, depth = 0;

  if (!td || !td->def_str)
    return 0;
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
    return 0;

  for (i = paren_index + 1; i + 1 < n; ++i)
  {
    int t = td->def_str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(td->def_str->str, n, i);
      continue;
    }
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
        return t;
    }
  }
  return 0;
}

static int template_first_parameter_specificity(TemplateDef *td,
                                                CType *arg_types, int arg_count)
{
  int i, j, start = -1, type_tok = 0;
  CType parameter;
  if (!arg_count || !arg_types || !td->def_str) return 0;
  for (i = 0; i + 1 < td->def_str->len; ++i)
    if (td->def_str->str[i] == td->name_tok && td->def_str->str[i + 1] == '(')
    { start = i + 2; break; }
  if (start < 0) return 0;
  for (i = start; i < td->def_str->len; ++i)
  {
    int token = td->def_str->str[i];
    if (TOK_HAS_VALUE(token))
    { i += tok_str_value_extra_words(td->def_str->str, td->def_str->len, i); continue; }
    /* This fast path recognizes plain types, pointers and references.
       Parenthesized/array declarators require the complete substituted
       signature; do not misread char (&buffer)[N] as char&. */
    if (token == '(' || token == '[') return 0;
    if (token == ',' || token == ')') break;
    for (j = 0; j < td->nb_type_params; ++j)
      if (token == td->type_param_toks[j]) return 0;
    if (token == TOK_LT || token == '<') return 0;
    if (!type_tok)
    {
      int candidate = token;
      if (td->namespace_tok && token >= TOK_UIDENT)
        candidate = make_static_member_tok(td->namespace_tok, token);
      if (make_type_from_type_arg_tok(&parameter, candidate)
          || (candidate != token && make_type_from_type_arg_tok(&parameter, token)))
        type_tok = token;
    }
    else if (token == '*') mk_pointer(&parameter);
    else if (token == '&' || token == TOK_LAND)
    { mk_reference(&parameter); if (token == TOK_LAND) parameter.t |= VT_RVALUE_REFERENCE; }
  }
  if (type_tok)
  {
    int rank = call_arg_match_rank(&parameter, &arg_types[0]);
    if (rank < 0) return -1000000;
    /* Conversion quality precedes partial ordering. A concrete first
       parameter wins an equally good generic match, not an exact generic
       match that would require a conversion in this candidate. */
    return 4 - (rank > 1 ? rank - 1 : 0) * 16;
  }
  return 0;
}

static int parse_template_type_parameter_tok(int index, int *is_variadic,
                                             int *has_default,
                                             TokenString **default_str);
static int template_parameter_is_type(void);
static int parse_template_non_type_parameter_tok(int parameter_index, int *has_default,
                                                 TokenString **default_str, TokenString **type_str);
static int template_return_substitution_matches_bound(TemplateDef *, TemplateArgList *);
static int template_call_signature_rank_bound(TemplateDef *, CType *, int, TemplateArgList *, int);


static int template_function_matches_namespace(TemplateDef *definition, int name)
{
  int parent;
  const char *spelling;
  if (definition->friend_class_tok) {
    int i;
    for (i = 0; i < template_adl_class_count; ++i)
      if ((template_adl_classes[i]->v & ~SYM_STRUCT) == definition->friend_class_tok) break;
    if (i == template_adl_class_count) return 0;
  }
  if (template_def_lookup_tok(definition) == name) return 1;
  if (!nb_inline_namespace_scopes) return 0;
  parent = symbol_namespace_scope_tok(name);
  if (!namespace_is_inline_child(definition->namespace_tok, parent)) return 0;
  spelling = get_tok_str(name, NULL);
  if (parent) spelling += strlen(get_tok_str(parent, NULL)) + 1;
  return !strcmp(spelling, get_tok_str(definition->name_tok, NULL));
}

static TemplateDef *find_function_template_for_call(int name_tok,
                                                    CType *arg_types,
                                                    int arg_count, TemplateArgList *selected_args)
{
  TemplateDef *td;
  TemplateArgList fixed_args, pack_args;
  TemplateDef *variadic = NULL;
  TemplateDef *exact_unpreferred = NULL;
  int exact_specificity = -1;
  int variadic_specificity = -1000000;
  int template_count = 0;
  int definition_index;

  for (definition_index = 0, td = nb_inline_namespace_scopes
         ? (nb_template_defs ? template_defs[0] : NULL) : template_def_candidates(name_tok);
       td; td = nb_inline_namespace_scopes
         ? (++definition_index < nb_template_defs ? template_defs[definition_index] : NULL)
         : td->lookup_bucket_next)
    if (!td->is_class && !td->is_alias && template_function_matches_namespace(td, name_tok))
      ++template_count;

  for (definition_index = 0, td = nb_inline_namespace_scopes
         ? (nb_template_defs ? template_defs[0] : NULL) : template_def_candidates(name_tok);
       td; td = nb_inline_namespace_scopes
         ? (++definition_index < nb_template_defs ? template_defs[definition_index] : NULL)
         : td->lookup_bucket_next)
  {
    TemplateArgList deduced;
    if (td->is_class || td->is_alias)
      continue;
    if (!template_function_matches_namespace(td, name_tok))
      continue;
    /* Arity is part of candidate eligibility. Do not instantiate a dependent
       return type for a declaration that cannot accept this call. */
    if (arg_count < td->func_min_args
        || (!td->func_is_variadic && arg_count > td->func_max_args))
      continue;
    {
      /* All fixed and pack parameters must deduce before a candidate can
         compete. A more specific first parameter cannot rescue a mismatch
         in a later pointer/reference parameter. */
      if (!infer_template_args_from_call(td, arg_types, arg_count, &deduced))
        continue;
    }
    if (!template_return_substitution_matches_bound(td, &deduced))
      continue;
    /* Default arguments affect eligibility, not conversion ranking. Every
       supplied argument must match the substituted fixed signature. */
    if (!td->func_is_variadic)
    {
      int first_matches =
        template_def_first_param_matches_call(td, arg_types, arg_count);
      if (first_matches)
      {
        int specificity = template_def_first_param_class_tok(td) ? 2
                          : template_def_first_param_pointer_pattern(td) ? 1
                          : 0;
        if (template_count > 1) {
          int rank = template_call_signature_rank_bound(td, arg_types, arg_count, &deduced, 0);
          if (rank == -1) continue;
          if (rank >= 0) specificity -= (rank - arg_count) * 16;
        }
        if (!exact_unpreferred || specificity > exact_specificity)
        {
          exact_unpreferred = td;
          fixed_args = deduced;
          exact_specificity = specificity;
        }
      }
      continue;
    }
    if (td->func_is_variadic && arg_count >= td->func_min_args
        && template_def_first_param_matches_call(td, arg_types, arg_count))
    {
      int specificity = template_first_parameter_specificity(td, arg_types, arg_count);
      int signature_rank = template_call_signature_rank_bound(td, arg_types, arg_count, &deduced, 0);
      if (signature_rank == -1) continue;
      if (signature_rank >= 0) specificity -= (signature_rank - arg_count) * 16;
      if (specificity == -1000000) continue;
      if (!variadic || specificity > variadic_specificity
          || (specificity == variadic_specificity
              && td->func_min_args > variadic->func_min_args))
      {
        variadic = td;
        pack_args = deduced;
        variadic_specificity = specificity;
      }
    }
  }
  if (exact_unpreferred) {
    if (variadic) {
      int fixed_rank = template_call_signature_rank_bound(exact_unpreferred, arg_types, arg_count, &fixed_args, 1);
      int pack_rank = template_call_signature_rank_bound(variadic, arg_types, arg_count, &pack_args, 1);
      /* A fixed-arity template is not preferred over a parameter pack when
         its supplied arguments require worse conversions. */
      if (fixed_rank >= 0 && pack_rank >= 0 && pack_rank < fixed_rank) {
        *selected_args = pack_args;
        return variadic;
      }
    }
    *selected_args = fixed_args;
    return exact_unpreferred;
  }
  if (variadic) *selected_args = pack_args;
  return variadic;
}

static TemplateDef *find_function_template_def(int name_tok)
{
  TemplateDef *td;

  for (td = template_def_candidates(name_tok); td;
       td = td->lookup_bucket_next)
  {
    if (td->is_class || td->is_alias || td->friend_class_tok)
      continue;
    if ((td->lookup_tok ? td->lookup_tok : td->name_tok) == name_tok)
      return td;
  }
  return NULL;
}

static TemplateDef *find_function_template_def_for_arg_count(int name_tok,
                                                             int arg_count,
                                                             int first_class_tok)
{
  TemplateDef *td;
  TemplateDef *variadic = NULL;
  TemplateDef *best_defaulted = NULL;
  int best_min = 0;
  TemplateDef *exact_unpreferred = NULL;

  for (td = template_def_candidates(name_tok); td;
       td = td->lookup_bucket_next)
  {
    int td_first_class_tok;
    if (td->is_class || td->is_alias)
      continue;
    if ((td->lookup_tok ? td->lookup_tok : td->name_tok) != name_tok)
      continue;
    td_first_class_tok = template_def_first_param_class_tok(td);
    if (!td->func_is_variadic && td->func_min_args == arg_count
        && td->func_max_args == arg_count)
    {
      if (!first_class_tok || td_first_class_tok == first_class_tok)
        return td;
      if (!exact_unpreferred)
        exact_unpreferred = td;
      continue;
    }
    if (first_class_tok && td_first_class_tok
        && td_first_class_tok != first_class_tok)
      continue;
    if (!td->func_is_variadic && arg_count >= td->func_min_args
        && arg_count <= td->func_max_args
        && (!best_defaulted || td->func_min_args > best_min))
    {
      best_defaulted = td;
      best_min = td->func_min_args;
    }
    if (td->func_is_variadic && arg_count >= td->func_min_args)
      variadic = td;
  }
  if (best_defaulted)
    return best_defaulted;
  if (variadic)
    return variadic;
  return exact_unpreferred;
}

static int count_template_return_call_args(TemplateDef *td, int open_paren_index)
{
  int i, depth = 0, saw_arg = 0, count = 0;

  if (!td || !td->def_str || open_paren_index < 0
      || open_paren_index >= td->def_str->len
      || td->def_str->str[open_paren_index] != '(')
    return -1;
  for (i = open_paren_index + 1; i < td->def_str->len; ++i)
  {
    int t = td->def_str->str[i];
    if (t == TOK_EOF)
      return -1;
    if (t == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (depth == 0 && t == ')')
      return saw_arg ? count + 1 : 0;
    if (depth == 0 && t == ',')
    {
      ++count;
      saw_arg = 0;
      continue;
    }
    saw_arg = 1;
    if (t == '(' || t == '[' || t == '{' || t == TOK_LT || t == '<')
      ++depth;
    else if ((t == ')' || t == ']' || t == '}' || t == TOK_GT || t == '>')
             && depth > 0)
      --depth;
    else if (t == TOK_SAR && depth > 0)
    {
      --depth;
      if (depth > 0)
        --depth;
    }
  }
  return -1;
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

/* A function-template signature includes its return type. This distinguishes
   dependent return constraints even when the function parameters coincide. */
/* Identifier token numbers depend on what each translation unit parsed
   earlier. Signature identities must depend on their spelling instead. */
static unsigned template_signature_hash_token(unsigned hash, int token)
{
  if (token >= TOK_UIDENT)
  {
    const unsigned char *name = (const unsigned char *)get_tok_str(token, NULL);
    while (*name)
    {
      hash ^= *name++;
      hash *= 16777619u;
    }
    token = 0;
  }
  hash ^= (unsigned)token;
  return hash * 16777619u;
}

static unsigned template_function_return_signature_hash(TokenString *str,
                                                         int name_tok)
{
  unsigned hash = 2166136261u;
  int i, end = str->len;
  for (i = 0; i + 1 < str->len; ++i) {
    if ((str->str[i] == name_tok && str->str[i + 1] == '(')
        || (str->str[i] == TOK_OPERATOR && i + 2 < str->len
            && get_cpp_binary_operator_method_tok(str->str[i + 1]) == name_tok
            && str->str[i + 2] == '(')) { end = i; break; }
    if (TOK_HAS_VALUE(str->str[i]))
      i += tok_str_value_extra_words(str->str, str->len, i);
  }
  /* The nested-name-specifier of an out-of-class declarator is not part of
     its return type. Strip only the qualified chain immediately before the
     function name, retaining qualified and dependent return types. */
  for (;;) {
    int last = tok_str_prev_token_index(str->str, str->len, end);
    int first, depth = 0;
    if (last < 0 || str->str[last] != ':') break;
    first = tok_str_prev_token_index(str->str, str->len, last);
    if (first < 0 || str->str[first] != ':') break;
    first = tok_str_prev_token_index(str->str, str->len, first);
    if (first >= 0 && (str->str[first] == TOK_GT || str->str[first] == TOK_SAR)) {
      do {
        int token = str->str[first];
        if (token == TOK_GT) ++depth;
        else if (token == TOK_SAR) depth += 2;
        else if (token == TOK_LT) --depth;
        first = tok_str_prev_token_index(str->str, str->len, first);
      } while (first >= 0 && depth > 0);
    }
    if (first < 0 || str->str[first] < TOK_UIDENT) break;
    end = first;
  }
  for (i = 0; i < end; ++i) {
    int token = str->str[i];
    if (token == TOK_LINENUM) { ++i; continue; }
    if (token >= TOK_UIDENT && !strcmp(get_tok_str(token, NULL), "template")
        && i + 1 < end && str->str[i + 1] == TOK_LT) {
      int depth = 0;
      for (++i; i < end; ++i) {
        token = str->str[i];
        if (TOK_HAS_VALUE(token)) {
          i += tok_str_value_extra_words(str->str, str->len, i);
          continue;
        }
        if (token == TOK_LT) ++depth;
        else if (token == TOK_GT) --depth;
        else if (token == TOK_SAR) depth -= 2;
        if (depth <= 0) break;
      }
      continue;
    }
    if (token == TOK_INLINE1 || token == TOK_INLINE2 || token == TOK_INLINE3
        || token == TOK_STATIC || token == TOK_EXTERN || token == tok_constexpr)
      continue;
    hash = template_signature_hash_token(hash, token);
    if (TOK_HAS_VALUE(token)) {
      int extra = tok_str_value_extra_words(str->str, str->len, i);
      while (extra-- > 0) { hash ^= (unsigned)str->str[++i]; hash *= 16777619u; }
    }
  }
  return hash;
}

static int template_void_parameter_list(TokenString *str, int start)
{
  int at = tok_str_next_non_linenum_index(str->str, str->len, start);
  if (at >= str->len || str->str[at] != TOK_VOID) return 0;
  at = tok_str_next_non_linenum_index(str->str, str->len, at + 1);
  return at < str->len && str->str[at] == ')';
}

static void analyze_template_function_signature(TokenString *str, int name_tok,
                                                int *min_args,
                                                int *max_args,
                                                int *is_variadic,
                                                unsigned *sig_hash)
{
  int i, n = str->len, paren = 0, angle = 0;
  int in_param = 0, param_has_pack = 0, param_has_default = 0;
  int skip_default = 0;

  *min_args = 0;
  *max_args = 0;
  *is_variadic = 0;
  *sig_hash = template_function_return_signature_hash(str, name_tok);
  for (i = 0; i + 1 < n; ++i)
  {
    if (str->str[i] == name_tok && str->str[i + 1] == '(')
    {
      i += 2;
      break;
    }
    if (str->str[i] == TOK_OPERATOR && i + 2 < n
        && get_cpp_binary_operator_method_tok(str->str[i + 1]) == name_tok
        && str->str[i + 2] == '(')
    {
      i += 3;
      break;
    }
    if (TOK_HAS_VALUE(str->str[i]))
      i += tok_str_value_extra_words(str->str, n, i);
  }
  if (i >= n)
    return;
  if (template_void_parameter_list(str, i))
    return;
  for (; i < n; ++i)
  {
    int t = str->str[i];
    if (t == TOK_EOF)
      break;
    if (t == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (TOK_HAS_VALUE(t))
    {
      int extra = tok_str_value_extra_words(str->str, n, i);
      if (!skip_default)
        *sig_hash = template_signature_hash_token(*sig_hash, t);
      while (extra-- > 0)
      {
        unsigned word = (unsigned)str->str[++i];
        if (!skip_default)
          *sig_hash = (*sig_hash ^ word) * 16777619u;
      }
      in_param = 1;
      continue;
    }
    if (t == TOK_LT)
    {
      angle++;
      if (!skip_default)
      {
        *sig_hash = template_signature_hash_token(*sig_hash, t);
      }
    }
    else if ((t == TOK_GT || t == TOK_SAR) && angle > 0)
    {
      angle--;
      if (!skip_default)
      {
        *sig_hash = template_signature_hash_token(*sig_hash, t);
      }
    }
    else if (angle == 0)
    {
      if (t == '(')
      {
        paren++;
        if (!skip_default)
        {
          *sig_hash = template_signature_hash_token(*sig_hash, t);
        }
      }
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
      {
        paren--;
        if (!skip_default)
        {
          *sig_hash = template_signature_hash_token(*sig_hash, t);
        }
      }
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
        skip_default = 0;
        *sig_hash = template_signature_hash_token(*sig_hash, t);
        continue;
      }
      else if (t == '=')
      {
        param_has_default = 1;
        skip_default = 1;
      }
      else if (t == TOK_DOTS)
      {
        *is_variadic = 1;
        param_has_pack = 1;
        if (!skip_default)
        {
          *sig_hash = template_signature_hash_token(*sig_hash, t);
        }
      }
      else if (!skip_default)
      {
        *sig_hash = template_signature_hash_token(*sig_hash, t);
      }
    }
    else if (!skip_default)
    {
      *sig_hash = template_signature_hash_token(*sig_hash, t);
    }
    if (t != TOK_LINENUM)
      in_param = 1;
  }
}

static unsigned template_function_merge_signature_hash(TokenString *str,
                                                       int name_tok)
{
  unsigned hash = template_function_return_signature_hash(str, name_tok);
  int i, n = str->len, paren = 0, angle = 0, bracket = 0;
  int skip_default = 0;

  for (i = 0; i + 1 < n; ++i)
    if (str->str[i] == name_tok && str->str[i + 1] == '(')
    {
      i += 2;
      break;
    }
  if (template_void_parameter_list(str, i))
    return hash;
  for (; i < n; ++i)
  {
    int t = str->str[i];
    int next = i + 1;

    if (t == TOK_EOF)
      break;
    if (t == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    while (next + 1 < n && str->str[next] == TOK_LINENUM)
      next += 2;
    if (t == ')' && paren == 0 && angle == 0 && bracket == 0)
      break;
    if (t == ',' && paren == 0 && angle == 0 && bracket == 0)
      skip_default = 0;
    else if (t == '=' && paren == 0 && angle == 0 && bracket == 0)
    {
      skip_default = 1;
      continue;
    }

    /* Parameter names are not part of a function template signature.  The
       common declarator form ends the name at the top-level comma, closing
       parenthesis, or default argument.  Excluding that token lets a forward
       declaration such as `const T &value` merge with a definition spelling
       the same parameter `const T &input`, without relaxing overload type
       matching. */
    if (!skip_default && paren == 0 && angle == 0 && bracket == 0
        && t >= TOK_UIDENT && next < n
        && (str->str[next] == ',' || str->str[next] == ')'
            || str->str[next] == '=')) {
      int previous = tok_str_prev_token_index(str->str, str->len, i);
      int token = previous >= 0 ? str->str[previous] : 0;
      if (token != '(' && token != ',' && token != ':'
          && token != TOK_CONST1 && token != TOK_CONST2 && token != TOK_CONST3
          && token != TOK_VOLATILE1 && token != TOK_VOLATILE2 && token != TOK_VOLATILE3
          && token != TOK_STRUCT && token != TOK_CLASS && token != TOK_ENUM)
        continue;
    }

    if (!skip_default)
    {
      hash = template_signature_hash_token(hash, t);
    }
    if (t == '(')
      ++paren;
    else if (t == ')' && paren > 0)
      --paren;
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
    if (TOK_HAS_VALUE(t))
      i += tok_str_value_extra_words(str->str, n, i);
  }
  return hash;
}

static int find_template_name_in_str(TokenString *str)
{
  int i, n = str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int t = str->str[i];
    if (t == TOK_OPERATOR && i + 2 < n
        && str->str[i + 2] == '(')
      return get_cpp_binary_operator_method_tok(str->str[i + 1]);
    if (t >= TOK_UIDENT && str->str[i + 1] == '(')
      return t;
  }
  return 0;
}

static int is_class_template_partial_specialization(TokenString *str,
                                                    int name_tok)
{
  int i, n = str->len, prev = 0;

  for (i = 0; i + 1 < n; ++i)
  {
    int t = str->str[i];
    if (t == '{' || t == ';' || t == TOK_EOF)
      break;
    if (t == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (t == name_tok && str->str[i + 1] == TOK_LT
        && (prev == TOK_CLASS || prev == TOK_STRUCT))
      return 1;
    prev = t;
  }
  return 0;
}

static int template_qualified_declarator_follows(TokenString *str, int index)
{
  int n = str->len;
  while (index < n)
  {
    int angle = 0;
    while (index + 1 < n && str->str[index] == TOK_LINENUM) index += 2;
    if (index < n && str->str[index] == '~') ++index;
    if (index < n && str->str[index] == TOK_OPERATOR) return 1;
    if (index >= n || str->str[index] < TOK_UIDENT) return 0;
    ++index;
    if (index < n && (str->str[index] == TOK_LT || str->str[index] == '<'))
    {
      angle = 1;
      for (++index; index < n && angle; ++index)
      {
        int t = str->str[index];
        if (TOK_HAS_VALUE(t))
          index += tok_str_value_extra_words(str->str, n, index);
        else if (t == TOK_LT || t == '<') ++angle;
        else if (t == TOK_GT || t == '>') --angle;
        else if (t == TOK_SAR) angle = angle > 1 ? angle - 2 : 0;
      }
    }
    while (index + 1 < n && str->str[index] == TOK_LINENUM) index += 2;
    if (index + 1 < n && str->str[index] == ':' && str->str[index + 1] == ':')
    {
      index += 2;
      continue;
    }
    return index < n && (str->str[index] == '(' || str->str[index] == '='
                         || str->str[index] == '{' || str->str[index] == ';');
  }
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
        && str->str[i + 1] == ':' && str->str[i + 2] == ':'
        && ((str->str[i + 3] >= TOK_UIDENT
             && str->str[i + 4] == '(')
            || (str->str[i + 3] == TOK_OPERATOR
                && template_qualified_declarator_follows(str, i + 3))))
    {
      int class_tok = find_current_namespace_tok(str->str[i]);
      /* A member function template of a non-template class has no
         Class<T> qualifier, but it still needs the same staged declaration
         and body replay as a class-template member. */
      if (struct_find(class_tok))
      {
        /* Saved namespace bodies retain the source's unqualified class
           spelling.  Canonicalize the qualifier so later lookup and replay
           refer to the same class token as the receiver. */
        str->str[i] = class_tok;
        return class_tok;
      }
    }
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
              || str->str[j + 2] == TOK_OPERATOR)
          && template_qualified_declarator_follows(str, j + 2))
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
static int token_string_has_variadic_pack(TokenString *str);
static int template_member_is_call_specialized(TemplateMemberDef *md);

static int template_member_has_variadic_pack(TemplateMemberDef *md)
{
  if (!md->method_tok_known) return token_string_has_variadic_pack(md->def_str);
  if (!(md->syntax_known & (1u << TMS_VARIADIC))) {
    md->syntax_cache[TMS_VARIADIC] = token_string_has_variadic_pack(md->def_str);
    md->syntax_known |= 1u << TMS_VARIADIC;
  }
  return md->syntax_cache[TMS_VARIADIC];
}

static int template_member_has_body(TemplateMemberDef *md)
{
  if (!md->method_tok_known) return token_string_has_top_level_body(md->def_str);
  if (!(md->syntax_known & (1u << TMS_BODY))) {
    md->syntax_cache[TMS_BODY] = token_string_has_top_level_body(md->def_str);
    md->syntax_known |= 1u << TMS_BODY;
  }
  return md->syntax_cache[TMS_BODY];
}

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

static int is_explicit_function_specialization(int function_tok)
{
  int i;
  for (i = 0; i < nb_explicit_function_specializations; ++i)
    if (explicit_function_specializations[i] == function_tok) return 1;
  return 0;
}

static int template_member_lookup_inst(TemplateMemberDef *md, int type_tok,
                                       int pack_tok, int class_tok)
{
  int i;
  int is_variadic = template_member_has_variadic_pack(md);
  int is_member_template = template_member_is_call_specialized(md);

  for (i = 0; i < md->nb_inst; ++i)
    if (md->inst_type_toks[i] == type_tok && md->inst_owner_toks[i] == class_tok
        && (!(is_variadic || is_member_template)
            || (md->inst_pack_toks && md->inst_pack_toks[i] == pack_tok)))
      return md->inst_func_toks && md->inst_func_toks[i]
             ? md->inst_func_toks[i] : 1;
  return 0;
}

/* A member-template call owns its complete explicit argument list. Interning
   it gives deferred interface/body requests a stable cache key, independent
   of whichever call is being parsed when the worklist is drained. */
typedef struct ExplicitMemberArguments {
  int key;
  TemplateArgList args;
  struct ExplicitMemberArguments *next;
  struct ExplicitMemberArguments *key_next, *args_next;
} ExplicitMemberArguments;
static ExplicitMemberArguments *explicit_member_arguments;
static ExplicitMemberArguments *explicit_member_keys[4096], *explicit_member_args[4096];

static void free_explicit_member_arguments(void)
{
  memset(explicit_member_keys, 0, sizeof(explicit_member_keys));
  memset(explicit_member_args, 0, sizeof(explicit_member_args));
  while (explicit_member_arguments)
  {
    ExplicitMemberArguments *next_entry = explicit_member_arguments->next;
    cprime_free(explicit_member_arguments);
    explicit_member_arguments = next_entry;
  }
}

static int intern_explicit_member_arguments(TemplateArgList *args)
{
  ExplicitMemberArguments *entry;
  CString name;
  int i;
  unsigned hash = args->nb;
  for (i = 0; i < args->nb; ++i) hash = hash * 16777619u ^ (unsigned)args->toks[i];
  hash &= 4095;
  for (entry = explicit_member_args[hash]; entry; entry = entry->args_next)
    if (entry->args.nb == args->nb
        && !memcmp(entry->args.toks, args->toks, args->nb * sizeof(int))) return entry->key;
  cstr_new(&name);
  cstr_cat(&name, "__cpc_explicit_member_arguments", -1);
  for (i = 0; i < args->nb; ++i)
    cstr_printf(&name, "_%s", get_tok_str(args->toks[i], NULL));
  cstr_ccat(&name, 0);
  i = tok_alloc_const(name.data);
  cstr_free(&name);
  for (entry = explicit_member_keys[(unsigned)i & 4095]; entry; entry = entry->key_next)
    if (entry->key == i) return i;
  entry = cprime_malloc(sizeof(*entry));
  entry->key = i;
  entry->args = *args;
  entry->next = explicit_member_arguments;
  explicit_member_arguments = entry;
  entry->key_next = explicit_member_keys[(unsigned)i & 4095];
  explicit_member_keys[(unsigned)i & 4095] = entry;
  entry->args_next = explicit_member_args[hash];
  explicit_member_args[hash] = entry;
  return i;
}

static int get_explicit_member_arguments(int key, TemplateArgList *args)
{
  ExplicitMemberArguments *entry;
  for (entry = explicit_member_keys[(unsigned)key & 4095]; entry; entry = entry->key_next)
    if (entry->key == key) { *args = entry->args; return 1; }
  args->nb = 0;
  return 0;
}

static int template_member_lookup_inst_index(TemplateMemberDef *md,
                                             int type_tok, int pack_tok,
                                             int class_tok)
{
  int i;
  int is_variadic = template_member_has_variadic_pack(md);
  int is_member_template = template_member_is_call_specialized(md);

  for (i = 0; i < md->nb_inst; ++i)
    if (md->inst_type_toks[i] == type_tok && md->inst_owner_toks[i] == class_tok
        && (!(is_variadic || is_member_template)
            || (md->inst_pack_toks && md->inst_pack_toks[i] == pack_tok)))
      return i;
  return -1;
}

static int template_member_cached_param_type(TemplateMemberDef *md,
                                             int class_tok, int type_tok,
                                             int member_tok, CType *out)
{
  int i;

  for (i = 0; i < md->nb_param_types; ++i)
    if (md->param_class_toks[i] == class_tok
        && md->param_type_toks[i] == type_tok
        && md->param_member_toks[i] == member_tok)
    {
      *out = md->param_types[i];
      return 1;
    }
  return 0;
}

static void template_member_note_param_type(TemplateMemberDef *md,
                                            int class_tok, int type_tok,
                                            int member_tok, CType *type)
{
  int i = md->nb_param_types;

  if (i >= md->al_param_types)
  {
    md->al_param_types = md->al_param_types ? md->al_param_types * 2 : 4;
    md->param_class_toks = cprime_realloc(
      md->param_class_toks, md->al_param_types * sizeof(int));
    md->param_type_toks = cprime_realloc(
      md->param_type_toks, md->al_param_types * sizeof(int));
    md->param_member_toks = cprime_realloc(
      md->param_member_toks, md->al_param_types * sizeof(int));
    md->param_types = cprime_realloc(
      md->param_types, md->al_param_types * sizeof(CType));
  }
  md->param_class_toks[i] = class_tok;
  md->param_type_toks[i] = type_tok;
  md->param_member_toks[i] = member_tok;
  md->param_types[i] = *type;
  ++md->nb_param_types;
}

static int is_currently_defining_class(int type_tok)
{
  int i;
  Sym *definition = struct_find(type_tok);
  if (definition && definition->c == -2) return 1;

  for (i = 0; i < nb_defining_class_stack; ++i)
    if (defining_class_stack[i] == type_tok)
      return 1;
  return 0;
}

static void template_member_note_inst(TemplateMemberDef *md, int type_tok,
                                      int pack_tok, int class_tok)
{
  if (md->nb_inst >= md->al_inst)
  {
    md->al_inst = md->al_inst ? md->al_inst * 2 : 4;
    md->inst_type_toks = cprime_realloc(md->inst_type_toks,
                                     md->al_inst *sizeof(int));
    md->inst_pack_toks = cprime_realloc(md->inst_pack_toks,
                                     md->al_inst *sizeof(int));
    md->inst_owner_toks = cprime_realloc(md->inst_owner_toks,
                                     md->al_inst * sizeof(int));
    md->inst_func_toks = cprime_realloc(md->inst_func_toks,
                                     md->al_inst *sizeof(int));
    md->inst_states = cprime_realloc(md->inst_states,
                                     md->al_inst * sizeof(unsigned char));
    md->inst_call_arg_offsets = cprime_realloc(
      md->inst_call_arg_offsets, md->al_inst * sizeof(int));
    md->inst_call_arg_counts = cprime_realloc(
      md->inst_call_arg_counts, md->al_inst * sizeof(unsigned char));
  }
  md->inst_type_toks[md->nb_inst++] = type_tok;
  md->inst_pack_toks[md->nb_inst - 1] = pack_tok;
  md->inst_owner_toks[md->nb_inst - 1] = class_tok;
  md->inst_func_toks[md->nb_inst - 1] = 0;
  md->inst_states[md->nb_inst - 1] = 0;
  md->inst_call_arg_counts[md->nb_inst - 1] =
    nb_template_member_call_arg_types < CPC_MAX_CALL_ARGUMENTS
      ? nb_template_member_call_arg_types : CPC_MAX_CALL_ARGUMENTS;
  {
    int i;
    int count = md->inst_call_arg_counts[md->nb_inst - 1];
    int offset = md->nb_inst_call_args;
    /* Store only the live argument types. Most specializations have zero or
       one argument, so reserving the maximum arity per entry wastes memory
       and copies unused slots whenever the specialization table grows. */
    if (offset + count > md->al_inst_call_args) {
      md->al_inst_call_args = md->al_inst_call_args ? md->al_inst_call_args : 8;
      while (offset + count > md->al_inst_call_args) md->al_inst_call_args *= 2;
      md->inst_call_arg_types = cprime_realloc(md->inst_call_arg_types,
        md->al_inst_call_args * sizeof(CType));
    }
    md->inst_call_arg_offsets[md->nb_inst - 1] = offset;
    md->nb_inst_call_args += count;
    for (i = 0; i < md->inst_call_arg_counts[md->nb_inst - 1]; ++i)
    {
      CType saved = template_member_call_arg_types[i];
      /* Body materialization can happen after the caller's local scope has
         been released. Own the pointer/reference nodes in cached call types. */
      if ((saved.t & VT_BTYPE) == VT_PTR)
        saved.ref = template_stable_pointer_ref(&saved);
      md->inst_call_arg_types[offset + i] = saved;
    }
  }
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
  TemplateInstOwner *owner;
  int i, j;

  if (!md || class_mangled_tok < TOK_UIDENT || param_tok < TOK_UIDENT)
    return 0;
  /* The concrete instance owns the argument bindings. Looking up the
     member's source class name can select another namespace declaration or
     the primary template instead of the selected partial specialization. */
  owner = find_template_inst_owner_entry(class_mangled_tok);
  td = owner ? owner->owner : find_template_def(md->class_tok);
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

static int is_bound_member_template_parameter(int name, int first)
{
  int i;
  if (first && name == first) return 1;
  for (i = 0; i < nb_template_member_extra_params; ++i)
    if (name == template_member_extra_param_toks[i]) return 1;
  return 0;
}

static void tok_str_add_template_member_subst(TokenString *str,
    TemplateMemberDef *md, int class_mangled_tok, int type_param_tok,
    int type_tok, int member_type_param_tok, int member_type_arg_tok,
    int value_param_tok)
{
  int i;
  int class_arg_tok = template_member_class_arg_subst_tok(md,
                                                          class_mangled_tok,
                                                          tok);
  if (class_arg_tok)
    tok_str_add(str, class_arg_tok);
  else if (tok == type_param_tok)
    tok_str_add(str, type_tok);
  else if (member_type_param_tok && tok == member_type_param_tok)
    tok_str_add(str, member_type_arg_tok ? member_type_arg_tok : type_tok);
  else
  {
    for (i = 0; i < nb_template_member_extra_params; ++i)
      if (tok == template_member_extra_param_toks[i])
      {
        tok_str_add(str, template_member_extra_arg_toks[i]
                          ? template_member_extra_arg_toks[i] : type_tok);
        return;
      }
    tok_str_add_tok(str);
  }
}

static void note_template_alias_inst(int class_tok, int alias_tok,
                                     int scoped_tok)
{
  int i;

  if (class_tok < TOK_UIDENT || alias_tok < TOK_UIDENT)
    return;
  for (i = template_alias_buckets[member_candidate_bucket(class_tok, alias_tok)] - 1;
       i >= 0; i = template_alias_insts[i].alias_next - 1)
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
  {
    unsigned h = member_candidate_bucket(class_tok, alias_tok);
    unsigned scoped = member_candidate_bucket(class_tok, scoped_tok);
    template_alias_insts[nb_template_alias_insts].alias_next = template_alias_buckets[h];
    template_alias_insts[nb_template_alias_insts].scoped_next = template_scoped_alias_buckets[scoped];
    template_alias_buckets[h] = template_scoped_alias_buckets[scoped] = nb_template_alias_insts + 1;
  }
  nb_template_alias_insts++;
}

static int template_member_scoped_alias_tok(int class_tok, int alias_tok)
{
  int i;

  if (class_tok < TOK_UIDENT || alias_tok < TOK_UIDENT)
    return 0;
  for (i = template_alias_buckets[member_candidate_bucket(class_tok, alias_tok)] - 1;
       i >= 0; i = template_alias_insts[i].alias_next - 1)
    if (template_alias_insts[i].class_tok == class_tok
        && template_alias_insts[i].alias_tok == alias_tok)
      return template_alias_insts[i].scoped_tok;
  return 0;
}

static int class_alias_storage_tok(int class_tok, int declared_tok)
{
  int i;

  for (i = template_scoped_alias_buckets[member_candidate_bucket(class_tok, declared_tok)] - 1;
       i >= 0; i = template_alias_insts[i].scoped_next - 1)
    if (template_alias_insts[i].class_tok == class_tok
        && template_alias_insts[i].scoped_tok == declared_tok)
      return declared_tok;
  return make_static_member_tok(class_tok, declared_tok);
}

static void tok_str_add_template_member_subst_scoped(TokenString *str,
    TemplateMemberDef *md, int class_mangled_tok, int type_param_tok,
    int type_tok, int member_type_param_tok, int member_type_arg_tok,
    int value_param_tok)
{
  int i, alias_tok, class_arg_tok;

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
  for (i = 0; i < nb_template_member_extra_params; ++i)
    if (tok == template_member_extra_param_toks[i])
    {
      tok_str_add(str, template_member_extra_arg_toks[i]
                        ? template_member_extra_arg_toks[i] : type_tok);
      return;
    }
  alias_tok = tok >= TOK_UIDENT
              ? template_member_scoped_alias_tok(class_mangled_tok, tok)
              : 0;
  if (!alias_tok && tok >= TOK_UIDENT)
  {
    int nested = make_static_member_tok(class_mangled_tok, tok);
    if (struct_find(nested) || find_class_template_def(nested)) alias_tok = nested;
  }
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
    if (TOK_HAS_VALUE(t))
    {
      tok_str_add_record(dst, src->str, src->len, &i);
      continue;
    }
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
    if (TOK_HAS_VALUE(src->str[i]))
    {
      i += tok_str_value_extra_words(src->str, src->len, i);
      continue;
    }
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
  if (!cpp_template_keyword_tok)
    cpp_template_keyword_tok = tok_alloc_const("template");
  return t == cpp_template_keyword_tok;
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

static int token_string_template_is_friend_decl(TokenString *str)
{
  int i = 0;

  if (!token_string_starts_template(str))
    return 0;
  for (i = 0; i < str->len && str->str[i] != TOK_EOF; ++i)
  {
    if (TOK_HAS_VALUE(str->str[i]))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      continue;
    }
    if (str->str[i] == '{') return 0;
    if (str->str[i] >= TOK_UIDENT
        && !strcmp(get_tok_str(str->str[i], NULL), "friend"))
      return 1;
  }
  return 0;
}

static void queue_friend_template_declaration(TokenString *source, int owner)
{
  TokenString *declaration = tok_str_alloc();
  int i, removed = 0;
  for (i = 0; i + 1 < source->len; ++i)
    if (source->str[i] == tok_alloc_const("friend")
        && (source->str[i + 1] == TOK_CLASS || source->str[i + 1] == TOK_STRUCT)) {
      tok_str_free(declaration); tok_str_free(source); return;
    }
  tok_str_add(declaration, tok_alloc_const("friend"));
  for (i = 0; i < source->len; ++i) {
    int token = source->str[i];
    if (TOK_HAS_VALUE(token)) {
      tok_str_add_record(declaration, source->str, source->len, &i);
      continue;
    }
    if (!removed && token == tok_alloc_const("friend")) { removed = 1; continue; }
    tok_str_add(declaration, token);
  }
  tok_str_free(source);
  queue_pending_template_spec_in_namespace(declaration, owner, symbol_namespace_scope_tok(owner));
}

static int token_string_has_variadic_pack(TokenString *str)
{
  int i;

  if (!str)
    return 0;
  for (i = 0; i < str->len && str->str[i] != TOK_EOF; ++i)
  {
    if (TOK_HAS_VALUE(str->str[i]))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      continue;
    }
    if (str->str[i] == TOK_DOTS)
      return 1;
  }
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
    if (TOK_HAS_VALUE(str->str[i]))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      continue;
    }
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
  int angle = 0, paren = 0, brace = 0, brace_is_body = 0, prev_tok = 0;
  int header_paren = 0;
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
        if (str) {
          /* In the template parameter grammar an unparenthesized >> closes
             two lists. Preserve separate closes for later signature readers;
             parenthesized shifts remain ordinary expression tokens. */
          if (tok == TOK_SAR && !header_paren) {
            tok_str_add(str, TOK_GT);
            tok_str_add(str, TOK_GT);
          } else tok_str_add_tok(str);
        }
        if (tok == '(') ++header_paren;
        else if (tok == ')' && header_paren) --header_paren;
        else if (!header_paren && tok == TOK_LT)
          ++angle;
        else if (!header_paren && tok == TOK_GT)
          --angle;
        else if (!header_paren && tok == TOK_SAR)
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
          if (brace_is_body)
          {
            next();
            break;
          }
          next();
          prev_tok = '}';
          continue;
        }
      }
      prev_tok = tok;
      next();
      continue;
    }
    if (prev_tok != TOK_OPERATOR && (tok == TOK_LT || tok == '<'))
      ++angle;
    else if (prev_tok != TOK_OPERATOR && tok == TOK_GT && angle > 0)
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
      {
        brace_is_body = paren == 0;
        ++brace;
      }
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

static void apply_function_noexcept(CType *type, int specification)
{
  if (specification && type && (type->t & VT_BTYPE) == VT_FUNC && type->ref) {
    type->ref->f.func_noexcept_specified = 1;
    type->ref->f.func_noexcept = specification == 1;
  }
}

static void append_function_exception_spec(TokenString *str, const struct FuncAttr *attr)
{
  if (!attr || !attr->func_noexcept_specified) return;
  tok_str_add(str, tok_alloc_const("noexcept"));
  tok_str_add(str, '(');
  tok_str_add(str, attr->func_noexcept ? TOK_TRUE : TOK_FALSE);
  tok_str_add(str, ')');
}

static int skip_member_func_cv_qualifiers(void)
{
  int qualifiers = 0;
  last_member_noexcept_spec = 0;
  for (;;)
  {
    if (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
      qualifiers |= VT_CONSTANT;
    else if (tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
             || tok == TOK_VOLATILE3)
      qualifiers |= VT_VOLATILE;
    else if (tok == '&' || tok == TOK_LAND)
    {
      if (qualifiers & (VT_FUNC_LREF | VT_FUNC_RREF))
        cprime_error("duplicate member function ref qualifier");
      qualifiers |= tok == '&' ? VT_FUNC_LREF : VT_FUNC_RREF;
    }
    else if (tok >= TOK_UIDENT
             && (!strcmp(get_tok_str(tok, NULL), "override")
                 || !strcmp(get_tok_str(tok, NULL), "final")))
    {
      next();
      continue;
    }
    else if (tok == TOK_THROW)
    {
      /* Since C++17, the empty dynamic exception specification is exactly
         the non-throwing specification retained by legacy native headers. */
      next();
      skip('(');
      skip(')');
      last_member_noexcept_spec = 1;
      continue;
    }
    else if (tok >= TOK_UIDENT
             && !strcmp(get_tok_str(tok, NULL), "noexcept"))
    {
      next();
      last_member_noexcept_spec = 1;
      if (tok == '(')
      {
        next();
        last_member_noexcept_spec = expr_const() ? 1 : 2;
        skip(')');
      }
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

static int class_or_inst_has_member_template_name(int class_tok, int member_tok);

static int type_has_member_func_name(CType *type, int member_tok)
{
  Sym *func_sym, *field;
  MemberFuncOverload *o;
  ClassBaseInfo *info;
  int dummy_ofs, struct_tok;

  if (!type || ((type->t & VT_BTYPE) != VT_STRUCT))
    return 0;
  struct_tok = get_struct_type_name_tok(type);
  if (class_or_inst_has_member_template_name(struct_tok, member_tok))
    return 1;
  for (o = member_func_candidates(struct_tok, member_tok); o; o = o->bucket_next)
    if (o->struct_tok == struct_tok && o->method_tok == member_tok)
      return 1;
  field = find_field_try(type, member_tok, &dummy_ofs);
  if (!field)
    field = find_field_try(type, member_tok | SYM_FIELD, &dummy_ofs);
  if (field && ((field->type.t & VT_BTYPE) == VT_FUNC))
    return 1;
  func_sym = resolve_member_func(type, member_tok);
  if (func_sym && !IS_ASM_SYM(func_sym))
    return 1;
  for (info = class_base_candidates(struct_tok); info; info = info->bucket_next)
  {
    CType base_type;

    if (info->class_tok != struct_tok)
      continue;
    if (!make_class_type_from_tok(&base_type, info->base_tok))
      continue;
    base_type.t |= type->t & VT_CONSTANT;
    if (type_has_member_func_name(&base_type, member_tok))
      return 1;
  }
  return 0;
}

static int template_member_def_method_tok(TemplateMemberDef *md)
{
  int i, j, tok0, angle = 0, paren = 0;
  int result = 0;

  if (!md || !md->def_str)
    return 0;
  if (md->method_tok_known || md->method_tok)
    return md->method_tok;
  for (i = 0; i + 2 < md->def_str->len; ++i)
  {
    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    if (md->def_str->str[i] == '(') { ++paren; continue; }
    if (md->def_str->str[i] == ')') { --paren; continue; }
    if (paren) continue;
    /* Qualified calls in an inline body or constructor initializer are
       expressions, not the declaration's member name. */
    if (md->def_str->str[i] == '{') break;
    if (md->def_str->str[i] == ':' && md->def_str->str[i + 1] == ':')
    {
      int after_tok0;
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
        if (operator_tok == TOK_BOOL || operator_tok == TOK_BOOL2)
          return tok_alloc_const("operator bool");
        return get_cpp_binary_operator_method_tok(operator_tok);
      }
      if (tok0 == '~' && i + 3 < md->def_str->len)
      {
        int destructor_tok = md->def_str->str[i + 3];
        after_tok0 = i + 4;
        while (after_tok0 + 1 < md->def_str->len
               && md->def_str->str[after_tok0] == TOK_LINENUM)
          after_tok0 += 2;
        if (after_tok0 < md->def_str->len
            && md->def_str->str[after_tok0] == '(')
          return class_tok_matches_unqualified_name(md->class_tok, destructor_tok)
                 ? TOK_DESTRUCTOR1 : destructor_tok;
        continue;
      }
      after_tok0 = i + 3;
      while (after_tok0 + 1 < md->def_str->len
             && md->def_str->str[after_tok0] == TOK_LINENUM)
        after_tok0 += 2;
      /* A scoped identifier in the return type (Class<T>::Alias) is not
         the method.  Keep scanning until the scoped name that actually
         introduces the function parameter list. */
      if (after_tok0 >= md->def_str->len
          || md->def_str->str[after_tok0] != '(')
        continue;
      if (class_tok_matches_unqualified_name(md->class_tok, tok0))
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
        if (TOK_HAS_VALUE(t))
        {
          i += 1 + tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
          continue;
        }
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
      if (TOK_HAS_VALUE(t))
      {
        i += 1 + tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
        continue;
      }
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
        if (t == tok_explicit
            || (t >= TOK_UIDENT && !strcmp(get_tok_str(t, NULL), "explicit")))
        {
          ++i;
          continue;
        }
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
          return class_tok_matches_unqualified_name(md->class_tok, t)
                   ? TOK_CONSTRUCTOR1 : t;
      }
      ++i;
    }
  }
  return 0;
}

static int template_member_def_is_scoped_uncached(TemplateMemberDef *md);
static int template_member_def_is_scoped(TemplateMemberDef *md)
{
  if (!md || !md->method_tok_known)
    return template_member_def_is_scoped_uncached(md);
  if (!(md->syntax_known & (1u << TMS_SCOPED))) {
    md->syntax_cache[TMS_SCOPED] = template_member_def_is_scoped_uncached(md);
    md->syntax_known |= 1u << TMS_SCOPED;
  }
  return md->syntax_cache[TMS_SCOPED];
}

static int template_member_def_is_scoped_uncached(TemplateMemberDef *md)
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
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
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
  {
    if (TOK_HAS_VALUE(str->str[i]))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      continue;
    }
    if (str->str[i] >= TOK_UIDENT
        && strstr(get_tok_str(str->str[i], NULL), "initializer_list"))
      return 1;
  }
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

static int token_string_has_top_level_body(TokenString *str)
{
  int i, paren = 0, bracket = 0;

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
    if (t == '(')
      ++paren;
    else if (t == ')' && paren > 0)
      --paren;
    else if (t == '[')
      ++bracket;
    else if (t == ']' && bracket > 0)
      --bracket;
    else if (t == '{' && paren == 0 && bracket == 0)
      return 1;
    else if (t == ';' && paren == 0 && bracket == 0)
      return 0;
  }
  return 0;
}

/* Publish a generated member-template prototype while a specialization body
   is already being compiled.  Re-entering the general specialization
   worklist here would replay whole function bodies inside the suspended
   parser.  A prototype is deliberately body-free, so it can be parsed in an
   isolated global scope and make overload resolution continue immediately. */
static void publish_template_member_proto(TokenString *proto, int class_tok)
{
  Sym **saved_local_bindings = NULL;
  int nb_saved_local_bindings = 0;
  int saved_tok = tok;
  int saved_namespace_tok = current_namespace_scope_tok();
  int saved_local_scope = local_scope;
  int saved_defer = defer_pending_member_funcs;
  int saved_vstack_depth = (int)(vtop - vstack + 1);
  CValue saved_tokc = tokc;
  Sym *saved_local_stack = local_stack;
  SValue *saved_vstack = NULL;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;

  if (saved_vstack_depth > 0)
  {
    saved_vstack = cprime_malloc(saved_vstack_depth * sizeof(SValue));
    memcpy(saved_vstack, vstack, saved_vstack_depth * sizeof(SValue));
  }
  vtop = vstack - 1;
  if (saved_local_stack)
  {
    saved_local_bindings = sym_hide_local_bindings(saved_local_stack,
                                                   &nb_saved_local_bindings);
    sym_relink_local_types(saved_local_bindings, nb_saved_local_bindings);
  }
  local_stack = NULL;
  local_scope = 0;
  defer_pending_member_funcs = 1;
  if (class_tok != -1)
    enter_namespace_scope(symbol_namespace_scope_tok(class_tok));
  begin_macro(proto, 1);
  next();
  decl(VT_CONST);
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  if (saved_local_stack)
  {
    sym_unlink_local_types(saved_local_bindings, nb_saved_local_bindings);
    sym_relink_stack(saved_local_bindings, nb_saved_local_bindings);
  }
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  defer_pending_member_funcs = saved_defer;
  if (saved_vstack_depth > 0)
  {
    memcpy(vstack, saved_vstack, saved_vstack_depth * sizeof(SValue));
    cprime_free(saved_vstack);
  }
  vtop = vstack + saved_vstack_depth - 1;
  enter_namespace_scope(saved_namespace_tok);
  tok = saved_tok;
  tokc = saved_tokc;
}

static int template_member_method_end_index(TemplateMemberDef *md, int index,
                                            int method_tok);

static int template_member_def_param_count_uncached(TemplateMemberDef *md, int method_tok);
static int template_member_def_param_count(TemplateMemberDef *md, int method_tok)
{
  if (!md || !md->method_tok_known || method_tok != md->method_tok)
    return template_member_def_param_count_uncached(md, method_tok);
  if (!(md->syntax_known & (1u << TMS_PARAM_COUNT))) {
    md->syntax_cache[TMS_PARAM_COUNT] = template_member_def_param_count_uncached(md, method_tok);
    md->syntax_known |= 1u << TMS_PARAM_COUNT;
  }
  return md->syntax_cache[TMS_PARAM_COUNT];
}

static int template_member_def_param_count_uncached(TemplateMemberDef *md, int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return -1;
  if (method_tok == TOK_DESTRUCTOR1)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0, count = 0, saw = 0;

    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
      if (TOK_HAS_VALUE(t))
      {
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
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

static int template_member_def_min_param_count_uncached(TemplateMemberDef *md, int method_tok);
static int template_member_def_min_param_count(TemplateMemberDef *md, int method_tok)
{
  if (!md || !md->method_tok_known || method_tok != md->method_tok)
    return template_member_def_min_param_count_uncached(md, method_tok);
  if (!(md->syntax_known & (1u << TMS_MIN_PARAM_COUNT))) {
    md->syntax_cache[TMS_MIN_PARAM_COUNT] = template_member_def_min_param_count_uncached(md, method_tok);
    md->syntax_known |= 1u << TMS_MIN_PARAM_COUNT;
  }
  return md->syntax_cache[TMS_MIN_PARAM_COUNT];
}

static int template_member_def_min_param_count_uncached(TemplateMemberDef *md, int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return -1;
  if (method_tok == TOK_DESTRUCTOR1)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0, brace = 0;
    int required = 0, saw = 0, has_default = 0;

    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT
                  || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
    angle = 0;
    for (++j; j < n; ++j)
    {
      int t = md->def_str->str[j];
      if (TOK_HAS_VALUE(t))
      {
        if (t != TOK_LINENUM) saw = 1;
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
      if (t == TOK_EOF)
        return -1;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if (t == ')' && !paren && !angle && !bracket && !brace)
      {
        if (saw && !has_default)
          ++required;
        return required;
      }
      else if (t == '[')
        ++bracket;
      else if (t == ']' && bracket > 0)
        --bracket;
      else if (t == '{')
        ++brace;
      else if (t == '}' && brace > 0)
        --brace;
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
      else if (t == '=' && !paren && !angle && !bracket && !brace)
        has_default = 1;
      else if (t == ',' && !paren && !angle && !bracket && !brace)
      {
        if (saw && !has_default)
          ++required;
        saw = 0;
        has_default = 0;
        continue;
      }
      if (t != TOK_LINENUM)
        saw = 1;
    }
  }
  return -1;
}

static int template_member_def_first_param_is_array_ref_uncached(TemplateMemberDef *md, int method_tok);
static int template_member_def_first_param_is_array_ref(TemplateMemberDef *md, int method_tok)
{
  if (!md || !md->method_tok_known || method_tok != md->method_tok)
    return template_member_def_first_param_is_array_ref_uncached(md, method_tok);
  if (!(md->syntax_known & (1u << TMS_ARRAY_REF))) {
    md->syntax_cache[TMS_ARRAY_REF] = template_member_def_first_param_is_array_ref_uncached(md, method_tok);
    md->syntax_known |= 1u << TMS_ARRAY_REF;
  }
  return md->syntax_cache[TMS_ARRAY_REF];
}

static int template_member_def_first_param_is_array_ref_uncached(TemplateMemberDef *md, int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0;
    int saw_ref = 0, saw_array = 0;

    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
      if (TOK_HAS_VALUE(t)) {
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
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

/* Classify the first parameter's top-level declarator shape:
   1 for pointer, 2 for reference, 0 for neither, -1 when unknown. */
static int template_member_def_first_param_decl_kind(TemplateMemberDef *md,
                                                     int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return -1;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0;

    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
      if (TOK_HAS_VALUE(t)) {
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
      if (t == TOK_EOF)
        return -1;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if ((t == ')' || t == ',') && !paren && !angle && !bracket)
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
      else if (t == '*' && !paren && !angle && !bracket)
        return 1;
      else if ((t == '&' || t == TOK_LAND) && !paren && !angle && !bracket)
        return 2;
    }
  }
  return -1;
}

static int template_member_def_first_param_type_tok_uncached(TemplateMemberDef *md, int method_tok);
static int template_member_def_first_param_type_tok(TemplateMemberDef *md, int method_tok)
{
  if (!md || !md->method_tok_known || method_tok != md->method_tok)
    return template_member_def_first_param_type_tok_uncached(md, method_tok);
  if (!(md->syntax_known & (1u << TMS_PARAM_TYPE))) {
    md->syntax_cache[TMS_PARAM_TYPE] = template_member_def_first_param_type_tok_uncached(md, method_tok);
    md->syntax_known |= 1u << TMS_PARAM_TYPE;
  }
  return md->syntax_cache[TMS_PARAM_TYPE];
}

static int template_member_def_first_param_type_tok_uncached(TemplateMemberDef *md, int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0;

    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
      if (TOK_HAS_VALUE(t)) {
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
      if (t == TOK_EOF)
        return 0;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if ((t == ')' || t == ',') && !paren && !angle && !bracket)
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
      else if (!paren && !angle && !bracket && t >= TOK_UIDENT
               && strcmp(get_tok_str(t, NULL), "const")
               && strcmp(get_tok_str(t, NULL), "volatile")
               && strcmp(get_tok_str(t, NULL), "struct")
               && strcmp(get_tok_str(t, NULL), "class"))
        return t;
    }
  }
  return 0;
}

static int template_member_def_is_const_qualified_uncached(TemplateMemberDef *md, int method_tok);
static int template_member_def_is_const_qualified(TemplateMemberDef *md, int method_tok)
{
  if (!md || !md->method_tok_known || method_tok != md->method_tok)
    return template_member_def_is_const_qualified_uncached(md, method_tok);
  if (!(md->syntax_known & (1u << TMS_CONST))) {
    md->syntax_cache[TMS_CONST] = template_member_def_is_const_qualified_uncached(md, method_tok);
    md->syntax_known |= 1u << TMS_CONST;
  }
  return md->syntax_cache[TMS_CONST];
}

static int template_member_def_is_const_qualified_uncached(TemplateMemberDef *md, int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i < n; ++i)
  {
    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    int j = template_member_method_end_index(md, i, method_tok);
    int paren = 0;
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j >= n || md->def_str->str[j] != '(')
      continue;
    paren = 1;
    for (++j; j < n && paren > 0; ++j)
    {
      int t = md->def_str->str[j];
      if (TOK_HAS_VALUE(t)) {
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
      if (t == '(')
        ++paren;
      else if (t == ')')
        --paren;
    }
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    return j < n && (md->def_str->str[j] == TOK_CONST1
                     || md->def_str->str[j] == TOK_CONST2
                     || md->def_str->str[j] == TOK_CONST3);
  }
  return 0;
}

static int concrete_type_tok_struct_tok(int type_tok)
{
  Sym *s;

  if (type_tok < TOK_UIDENT)
    return 0;
  s = sym_find(type_tok);
  if (!s)
    s = global_symbol_find(type_tok);
  if (s && (s->type.t & VT_BTYPE) == VT_STRUCT)
    return get_struct_type_name_tok(&s->type);
  if (struct_find(type_tok))
    return type_tok;
  return 0;
}

static int template_member_method_end_index(TemplateMemberDef *md, int index,
                                            int method_tok)
{
  int operator_tok, candidate_tok;

  if (md->def_str->str[index]
      == (method_tok == TOK_CONSTRUCTOR1 ? md->class_tok : method_tok)
      || (method_tok == TOK_CONSTRUCTOR1
          && class_tok_matches_unqualified_name(md->class_tok,
                                                md->def_str->str[index])))
    return index + 1;
  if (md->def_str->str[index] != TOK_OPERATOR
      || index + 1 >= md->def_str->len)
    return 0;
  operator_tok = md->def_str->str[index + 1];
  if (operator_tok == '[' && index + 2 < md->def_str->len
      && md->def_str->str[index + 2] == ']')
  {
    candidate_tok = tok_alloc_const("operator[]");
    return candidate_tok == method_tok ? index + 3 : 0;
  }
  if (operator_tok == '(' && index + 2 < md->def_str->len
      && md->def_str->str[index + 2] == ')')
  {
    candidate_tok = tok_alloc_const("operator()");
    return candidate_tok == method_tok ? index + 3 : 0;
  }
  if (operator_tok == TOK_BOOL || operator_tok == TOK_BOOL2)
    candidate_tok = tok_alloc_const("operator bool");
  else
    candidate_tok = get_cpp_binary_operator_method_tok(operator_tok);
  return candidate_tok == method_tok ? index + 2 : 0;
}

static int template_member_type_param_tok_uncached(TemplateMemberDef *md);
static int template_member_type_param_tok(TemplateMemberDef *md)
{
  if (!md || !md->method_tok_known)
    return template_member_type_param_tok_uncached(md);
  if (!(md->syntax_known & (1u << TMS_TYPE_PARAM))) {
    md->syntax_cache[TMS_TYPE_PARAM] = template_member_type_param_tok_uncached(md);
    md->syntax_known |= 1u << TMS_TYPE_PARAM;
  }
  return md->syntax_cache[TMS_TYPE_PARAM];
}

static int template_member_type_param_tok_uncached(TemplateMemberDef *md)
{
  int i = 0, angle = 0, expect_name = 0;

  while (i + 1 < md->def_str->len && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= md->def_str->len || !is_template_keyword_tok(md->def_str->str[i]))
    return 0;
  ++i;
  while (i + 1 < md->def_str->len && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= md->def_str->len
      || (md->def_str->str[i] != TOK_LT && md->def_str->str[i] != '<'))
    return 0;
  angle = 1;
  for (++i; i < md->def_str->len && angle > 0; ++i)
  {
    int t = md->def_str->str[i];
    if (TOK_HAS_VALUE(t)) {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    if (angle == 1 && (t == TOK_CLASS
        || (t >= TOK_UIDENT && !strcmp(get_tok_str(t, NULL), "typename"))))
      expect_name = 1;
    else if (angle == 1 && expect_name && t >= TOK_UIDENT)
      return t;
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
  return 0;
}

static int template_member_value_param_tok(TemplateMemberDef *md)
{
  int i = 0;

  if (!md || !md->def_str)
    return 0;
  if (md->nb_stripped_member_value_params > 0)
    return md->stripped_member_value_param_toks[0];
  while (i < md->def_str->len)
  {
    int angle, saw_type_keyword = 0, last_identifier = 0;
    while (i + 1 < md->def_str->len
           && md->def_str->str[i] == TOK_LINENUM)
      i += 2;
    if (i >= md->def_str->len
        || !is_template_keyword_tok(md->def_str->str[i]))
      break;
    ++i;
    while (i + 1 < md->def_str->len
           && md->def_str->str[i] == TOK_LINENUM)
      i += 2;
    if (i >= md->def_str->len
        || (md->def_str->str[i] != TOK_LT
            && md->def_str->str[i] != '<'))
      break;
    angle = 1;
    for (++i; i < md->def_str->len && angle > 0; ++i)
    {
      int t = md->def_str->str[i];
      if (TOK_HAS_VALUE(t)) {
        i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
        continue;
      }
      if (angle == 1
          && (t == TOK_CLASS
              || (t >= TOK_UIDENT
                  && !strcmp(get_tok_str(t, NULL), "typename"))))
        saw_type_keyword = 1;
      else if (angle == 1 && t == ',')
      {
        saw_type_keyword = 0;
        last_identifier = 0;
      }
      else if (angle == 1 && t >= TOK_UIDENT)
        last_identifier = t;
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
    if (!saw_type_keyword && last_identifier)
      return last_identifier;
  }
  return 0;
}

/* A class-template member definition starts with the class template's own
   parameter list, but that does not make the member itself a function
   template.  Treating every leading `template<...>` as call-specialized made
   the cache key depend on the call argument and replayed ordinary members
   while overload resolution was in progress. */
static int template_member_is_call_specialized_uncached(TemplateMemberDef *md);
static int template_member_is_call_specialized(TemplateMemberDef *md)
{
  if (!md || !md->method_tok_known)
    return template_member_is_call_specialized_uncached(md);
  if (!(md->syntax_known & (1u << TMS_CALL_SPECIALIZED))) {
    md->syntax_cache[TMS_CALL_SPECIALIZED] = template_member_is_call_specialized_uncached(md);
    md->syntax_known |= 1u << TMS_CALL_SPECIALIZED;
  }
  return md->syntax_cache[TMS_CALL_SPECIALIZED];
}

static int template_member_is_call_specialized_uncached(TemplateMemberDef *md)
{
  int first_param, i = 0, template_groups = 0;
  int method_tok;

  if (!md)
    return 0;
  if (md->nb_stripped_member_type_params > 0
      || md->nb_stripped_member_value_params > 0)
    return 1;
  if (!token_string_starts_template(md->def_str))
    return 0;
  method_tok = template_member_def_method_tok(md);
  if (method_tok
      && template_member_def_first_param_is_array_ref(md, method_tok))
    return 1;
  while (i + 1 < md->def_str->len && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  while (i < md->def_str->len
         && is_template_keyword_tok(md->def_str->str[i]))
  {
    int angle = 0;
    ++template_groups;
    ++i;
    while (i + 1 < md->def_str->len && md->def_str->str[i] == TOK_LINENUM)
      i += 2;
    if (i >= md->def_str->len
        || (md->def_str->str[i] != TOK_LT
            && md->def_str->str[i] != '<'))
      break;
    angle = 1;
    for (++i; i < md->def_str->len && angle > 0; ++i)
    {
      int t = md->def_str->str[i];
      if (TOK_HAS_VALUE(t)) {
        i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
        continue;
      }
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
    while (i + 1 < md->def_str->len && md->def_str->str[i] == TOK_LINENUM)
      i += 2;
  }
  if (template_groups > 1)
    return 1;
  first_param = template_member_type_param_tok(md);
  /* A single value parameter (template<i64 N>) is also call-specialized;
     there is no typename token for template_member_type_param_tok to return. */
  return !first_param || first_param != md->type_param_tok;
}

static int template_member_collect_first_param(TemplateMemberDef *md,
                                               int method_tok,
                                               TokenString *out)
{
  int i, n, name_tok;

  if (!md || !md->def_str || !out)
    return 0;
  n = md->def_str->len;
  name_tok = method_tok == TOK_CONSTRUCTOR1 ? md->class_tok : method_tok;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0;

    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
      if (TOK_HAS_VALUE(t)) {
        int extra = tok_str_value_extra_words(md->def_str->str, n, j), k;
        /* This is a copied declarator, so preserve literal records as well
           as their payload. Source-line records carry no type information. */
        if (t != TOK_LINENUM)
          for (k = 0; k <= extra; ++k)
            tok_str_add(out, md->def_str->str[j + k]);
        j += extra;
        continue;
      }
      if (t == TOK_EOF)
        return 0;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
      else if ((t == ')' || t == ',') && !paren && !angle && !bracket)
      {
        tok_str_add(out, TOK_EOF);
        return out->len > 1;
      }
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
      tok_str_add(out, t);
    }
  }
  return 0;
}

/* Find the member-template parameter's position in the class template-id
   used by the first function parameter.  For example, N is position 1 in
   FixedArray<char, N>&. */
static int template_member_first_param_arg_index(TemplateMemberDef *md,
                                                 int method_tok,
                                                 int param_tok)
{
  TokenString *raw;
  int i, angle = 0, arg_index = 0, result = -1;

  if (!param_tok)
    return -1;
  raw = tok_str_alloc();
  if (!template_member_collect_first_param(md, method_tok, raw))
  {
    tok_str_free(raw);
    return -1;
  }
  for (i = 0; i < raw->len; ++i)
  {
    int t = raw->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(raw->str, raw->len, i);
      continue;
    }
    if (t == TOK_LT || t == '<')
    {
      if (!angle)
        arg_index = 0;
      ++angle;
    }
    else if ((t == TOK_GT || t == '>') && angle > 0)
      --angle;
    else if (t == TOK_SAR && angle > 0)
    {
      --angle;
      if (angle > 0)
        --angle;
    }
    else if (t == ',' && angle == 1)
      ++arg_index;
    else if (t == param_tok && angle > 0)
    {
      result = arg_index;
      break;
    }
  }
  tok_str_free(raw);
  return result;
}

static int template_member_first_param_matches_arg(TemplateMemberDef *md,
                                                   int method_tok,
                                                   int class_mangled_tok,
                                                   int type_tok,
                                                   int member_type_arg_tok,
                                                   CType *arg_type)
{
  int saved_tok, v = 0, parsed = 0;
  CValue saved_tokc;
  AttributeDef ad;
  CType param_type;
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;
  Sym *saved_local_stack;
  TokenString *raw, *subst;
  int member_type_param_tok;
  int value_param_tok;

  if (!arg_type)
    return 1;
  /* A member-template parameter such as Vector2<U> accepts every concrete
     instantiation of that class-template family.  Replaying it in the
     receiver's Vector2<T> context must not rewrite the parameter to the
     receiver specialization before overload matching. */
  if ((arg_type->t & VT_BTYPE) == VT_STRUCT
      && template_member_is_call_specialized(md)
      && template_member_def_first_param_class_kind(md, method_tok) == 1)
  {
    int param_class_tok =
      template_member_def_first_param_type_tok(md, method_tok);
    int arg_struct_tok = get_struct_type_name_tok(arg_type);
    TemplateDef *param_td = find_class_template_def(param_class_tok);
    int i;

    if (param_td && param_td->is_class && arg_struct_tok)
      for (i = 0; i < param_td->nb_inst; ++i)
        if (param_td->inst_name_toks[i] == arg_struct_tok)
          return 1;
  }
  if (template_member_cached_param_type(md, class_mangled_tok, type_tok,
                                        member_type_arg_tok, &param_type))
    parsed = 1;
  if (parsed)
    goto compare_types;
  raw = tok_str_alloc();
  if (!template_member_collect_first_param(md, method_tok, raw))
  {
    tok_str_free(raw);
    return 1;
  }
  subst = tok_str_alloc();
  member_type_param_tok = template_member_type_param_tok(md);
  value_param_tok = template_member_value_param_tok(md);
  saved_tok = tok;
  saved_tokc = tokc;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  begin_macro(raw, 3);
  next();
  while (tok != TOK_EOF)
  {
    if (tok == md->class_tok)
      tok_str_add(subst, class_mangled_tok);
    else
      tok_str_add_template_member_subst_scoped(subst, md,
                                               class_mangled_tok,
                                               md->type_param_tok, type_tok,
                                               member_type_param_tok,
                                               member_type_arg_tok,
                                               value_param_tok);
    next();
  }
  tok_str_add(subst, TOK_EOF);
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;

  memset(&ad, 0, sizeof ad);
  saved_tok = tok;
  saved_tokc = tokc;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  saved_local_stack = local_stack;
  /* Parsed parameter metadata is cached across function scopes.  Pointer and
     reference derivation nodes therefore need global lifetime just like the
     cached replay return types. */
  local_stack = NULL;
  begin_macro(subst, 3);
  next();
  if (parse_btype(&param_type, &ad, 0))
  {
    type_decl(&param_type, &ad, &v, TYPE_DIRECT | TYPE_ABSTRACT | TYPE_PARAM);
    convert_parameter_type(&param_type);
    parsed = 1;
  }
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  local_stack = saved_local_stack;
  tok = saved_tok;
  tokc = saved_tokc;
  tok_str_free(raw);
  tok_str_free(subst);
  if (!parsed)
    return 0;
  template_member_note_param_type(md, class_mangled_tok, type_tok,
                                  member_type_arg_tok, &param_type);

compare_types:
  {
    CType param_value_type = param_type;
    CType arg_value_type = *arg_type;
    param_value_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&param_value_type))
      param_value_type = *pointed_type(&param_value_type);
    arg_value_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&arg_value_type))
      arg_value_type = *pointed_type(&arg_value_type);
    if (btype_is_arithmetic_scalar(param_value_type.t & VT_BTYPE)
        && btype_is_arithmetic_scalar(arg_value_type.t & VT_BTYPE))
      return 1;
    /* Array references permit element qualification without decaying or
       losing their bounds. Other dependent patterns still require deduction
       compatibility before conversion ranking considers their interfaces. */
    if (is_reference_type(&param_type)
        && (pointed_type(&param_type)->t & VT_ARRAY))
      return call_arg_match_rank(&param_type, arg_type) >= 0;
    if (!is_reference_type(&param_type))
      convert_parameter_type(&arg_value_type);
    return is_compatible_unqualified_types(&param_value_type,
                                           &arg_value_type);
  }
}

/* Classify the definition's first parameter type: 1 for an explicit
   class/struct type (e.g. const List<i64> &), 0 for an explicit scalar
   type, -1 when the type is a generic template parameter or otherwise
   unknown.  Used to keep an unrelated arity match (Erase(c--) with i64)
   from instantiating a member whose signature takes a different class
   argument. */
static int template_member_def_first_param_class_kind(TemplateMemberDef *md,
                                                      int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return -1;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0;

    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
      if (TOK_HAS_VALUE(t)) {
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
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
               || (t >= TOK_UIDENT && find_class_template_def(t))
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

/* Build a stable cache key for every concrete type in a variadic member pack.
   Keying only on the first element aliases calls such as Emplace(int, int)
   and Emplace(int, float), causing the later replay to reuse the wrong
   signature and body.  The pack parameter position is determined by the
   top-level parameter index of the first `...` token. */
static int template_member_def_pack_arg_tok(TemplateMemberDef *md,
                                             int method_tok,
                                             CType *arg_types,
                                             int explicit_arg_count)
{
  int i, n, j, paren = 0, angle = 0, bracket = 0;
  int param_index = 0;

  if (!md || !md->def_str || !arg_types || explicit_arg_count <= 0)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < n && (md->def_str->str[j] == TOK_LT || md->def_str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < n && angle > 0; ++j)
      {
        int t = md->def_str->str[j];
        if (TOK_HAS_VALUE(t)) {
          j += tok_str_value_extra_words(md->def_str->str, n, j);
          continue;
        }
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
      if (TOK_HAS_VALUE(t)) {
        j += tok_str_value_extra_words(md->def_str->str, n, j);
        continue;
      }
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
      {
        char name[512];
        int k;
        snprintf(name, sizeof(name), "__cpc_member_pack");
        for (k = param_index; k < explicit_arg_count; ++k)
        {
          pstrcat(name, sizeof(name),
                  (arg_types[k].t & VT_RVALUE_REFERENCE) ? "_rvalue_" : "_lvalue_");
          if (!append_type_mangle(name, sizeof(name), &arg_types[k]))
            return template_type_tok_from_ctype(&arg_types[param_index]);
        }
        return tok_alloc_const(name);
      }
    }
  }
  return 0;
}

static int template_member_def_pack_param_index(TemplateMemberDef *md,
                                                int method_tok,
                                                int *pattern_start,
                                                int *pattern_end)
{
  int i, n, j, paren = 0, angle = 0, bracket = 0;
  int param_index = 0, start = 0;

  if (!md || !md->def_str)
    return -1;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j >= n || md->def_str->str[j] != '(')
      continue;
    start = j + 1;
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
      else if (t == ',' && !paren && !angle && !bracket)
      { ++param_index; start = j + 1; }
      else if (t == TOK_DOTS && !paren && !angle && !bracket)
      {
        if (pattern_start) *pattern_start = start;
        if (pattern_end) *pattern_end = j;
        return param_index;
      }
    }
  }
  return -1;
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
                                                   CType *func_type,
                                                   TokenString *def_str,
                                                   int member_type_arg_tok)
{
  CString name;
  int result;
  Sym *arg;
  int min_args, max_args, is_variadic;
  unsigned sig_hash;

  cstr_new(&name);
  cstr_cat(&name, get_tok_str(struct_tok, NULL), -1);
  cstr_ccat(&name, '_');
  cstr_cat(&name, get_tok_str(method_tok, NULL), -1);
  if (func_type && (func_type->t & VT_CONSTANT))
    cstr_cat(&name, "_const", -1);
  if (def_str)
  {
    char suffix[24];
    analyze_template_function_signature(def_str, method_tok,
                                        &min_args, &max_args,
                                        &is_variadic, &sig_hash);
    snprintf(suffix, sizeof suffix, "_h%08x", sig_hash);
    cstr_cat(&name, suffix, -1);
  }
  if (member_type_arg_tok > 0)
  {
    CType member_arg_type;
    cstr_cat(&name, "__cpc_template_type_", -1);
    if (make_type_from_type_arg_tok(&member_arg_type,
                                    member_type_arg_tok))
    {
      if (!append_type_mangle_string(&name, &member_arg_type))
        cprime_error("unsupported member-template type argument");
    }
    else
      cstr_cat(&name, get_tok_str(member_type_arg_tok, NULL), -1);
  }
  if (func_type && func_type->ref)
    for (arg = func_type->ref->next; arg; arg = arg->next)
      if (!append_type_mangle_string(&name, &arg->type))
        cprime_error("unsupported member-template parameter type");
  cstr_ccat(&name, 0);
  result = tok_alloc_const(name.data);
  cstr_free(&name);
  return result;
}

static int template_arg_tok_from_inst_type(CType *type, int arg_index);
static int first_template_arg_tok_from_inst_type(CType *type);
static int template_type_tok_from_ctype(CType *type);

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
    else if (str->str[i] != TOK_CONST1 && str->str[i] != TOK_CONST2
             && str->str[i] != TOK_CONST3 && str->str[i] != TOK_VOLATILE1
             && str->str[i] != TOK_VOLATILE2 && str->str[i] != TOK_VOLATILE3
             && str->str[i] != '*' && str->str[i] != '&'
             && str->str[i] != TOK_LAND && str->str[i] != TOK_DECLTYPE
             && str->str[i] != '(' && str->str[i] != ')'
             && str->str[i] != TOK_EOF)
      return 0;
  }
  return saw_auto;
}

/* Build an abstract declaration retaining the placeholder's cv/pointer/ref
   structure. The temporary int marks the single deduced leaf. */
static int append_placeholder_return_pattern(TokenString *output,
                                               TokenString *source, int end)
{
  int i, kind = 1;
  for (i = 0; i < end; ++i)
  {
    int token = source->str[i];
    if (token == TOK_DECLTYPE) {
      int at = tok_str_next_non_linenum_index(source->str, end, i + 1);
      int value = at < end
        ? tok_str_next_non_linenum_index(source->str, end, at + 1) : end;
      int close = value < end
        ? tok_str_next_non_linenum_index(source->str, end, value + 1) : end;
      if (at < end && source->str[at] == '(' && value < end
          && source->str[value] == TOK_AUTO && close < end
          && source->str[close] == ')') {
        tok_str_add(output, TOK_INT);
        i = close;
        kind = 2;
        continue;
      }
    }
    if (token == TOK_AUTO) tok_str_add(output, TOK_INT);
    else if (token != TOK_EOF)
      tok_str_add_record(output, source->str, source->len, &i);
  }
  return kind;
}

static int token_string_is_auto_type_token_prefix(TokenString *str)
{
  int i = 0, angle = 0;

  if (!str)
    return 0;
  while (i < str->len && str->str[i] == TOK_LINENUM)
    i += 2;
  if (i < str->len && is_template_keyword_tok(str->str[i]))
  {
    ++i;
    if (i >= str->len || (str->str[i] != TOK_LT && str->str[i] != '<'))
      return 0;
    angle = 1;
    ++i;
    while (i < str->len && str->str[i] != TOK_EOF && angle > 0)
    {
      int t = str->str[i];
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
  while (i < str->len && str->str[i] == TOK_LINENUM)
    i += 2;
  return i < str->len && (str->str[i] == TOK_AUTO || str->str[i] == TOK_DECLTYPE);
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
      if (j >= md->def_str->len)
        continue;
      if (method_tok == tok_alloc_const("operator bool"))
      {
        if (md->def_str->str[j] != TOK_BOOL
            && md->def_str->str[j] != TOK_BOOL2)
          continue;
      }
      else if (get_cpp_binary_operator_method_tok(md->def_str->str[j])
               != method_tok)
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

static int template_member_def_is_static_signature_uncached(TemplateMemberDef *md);
static int template_member_def_is_static_signature(TemplateMemberDef *md)
{
  if (!md || !md->method_tok_known)
    return template_member_def_is_static_signature_uncached(md);
  if (!(md->syntax_known & (1u << TMS_STATIC))) {
    md->syntax_cache[TMS_STATIC] = template_member_def_is_static_signature_uncached(md);
    md->syntax_known |= 1u << TMS_STATIC;
  }
  return md->syntax_cache[TMS_STATIC];
}

static int template_member_def_is_static_signature_uncached(TemplateMemberDef *md)
{
  int j;

  if (!md || !md->def_str)
    return 0;
  for (j = 0; j + 1 < md->def_str->len; ++j)
  {
    int token = md->def_str->str[j];
    if (TOK_HAS_VALUE(token)) {
      j += tok_str_value_extra_words(md->def_str->str, md->def_str->len, j);
      continue;
    }
    if (token == '{' || token == ';') break;
    if (token == TOK_STATIC) return 1;
  }
  return 0;
}

static int class_has_static_member_template_name(int class_tok, int member_tok)
{
  TemplateDef *td = find_class_template_def_for_class_tok(class_tok);
  TemplateMemberDef *md;

  for (md = td ? template_member_candidates(td->name_tok, member_tok)
                : template_member_lookup_candidates(class_tok, member_tok);
       md; md = td ? md->bucket_next : md->lookup_next)
  {
    if ((td ? md->class_tok == td->name_tok : md->lookup_class_tok == class_tok)
        && template_member_def_method_tok(md) == member_tok
        && template_member_is_call_specialized(md)
        && template_member_def_is_static_signature(md))
      return 1;
  }
  return 0;
}

/* An out-of-class definition omits the static specifier. Recover it from
   the declaration with the same parameter signature, not merely its name:
   a class may overload static and non-static functions. */
static int template_member_definition_is_static(TemplateMemberDef *definition)
{
  TemplateMemberDef *declaration;
  int method = template_member_def_method_tok(definition);
  unsigned signature;
  if (template_member_def_is_static_signature(definition))
    return 1;
  signature = template_function_merge_signature_hash(definition->def_str, method);
  for (declaration = template_member_candidates(definition->class_tok, method);
       declaration; declaration = declaration->bucket_next)
    if (declaration->class_tok == definition->class_tok
        && template_member_def_method_tok(declaration) == method
        && template_member_def_is_static_signature(declaration)
        && template_function_merge_signature_hash(declaration->def_str, method) == signature)
      return 1;
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
   (e.g. template<typename T> static Matrix4x4<T> CreateMatrix(...)),
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
  int canonical_tok;
  TemplateInstOwner *owner;
  TemplateDef *td = find_template_def(class_tok);

  if (td && td->is_class)
    return td;
  owner = find_template_inst_owner_entry(class_tok);
  if (owner && owner->owner->is_class)
    return owner->owner;
  if (class_tok >= TOK_IDENT && class_tok < SYM_FIRST_ANOM) return NULL;
  /* Equal spellings intern to the same token. Canonicalize unusual token
     kinds directly instead of comparing against every instantiation. */
  canonical_tok = tok_alloc_const(get_tok_str(class_tok, NULL));
  owner = canonical_tok != class_tok
            ? find_template_inst_owner_entry(canonical_tok) : NULL;
  return owner && owner->owner->is_class ? owner->owner : NULL;
}

static int is_class_template_instantiation_tok(int class_tok)
{
  int canonical_tok;
  TemplateInstOwner *owner = find_template_inst_owner_entry(class_tok);
  /* Partial specializations have their own definition records; the
     instantiation index is authoritative even when those records are not
     present in the primary-template declaration list. */
  if (owner && owner->owner->is_class) return 1;
  if (class_tok >= TOK_IDENT && class_tok < SYM_FIRST_ANOM) return 0;
  canonical_tok = tok_alloc_const(get_tok_str(class_tok, NULL));
  owner = canonical_tok != class_tok
            ? find_template_inst_owner_entry(canonical_tok) : NULL;
  return owner && owner->owner->is_class;
}

static int class_has_member_template_name(int class_tok, int member_tok)
{
  TemplateMemberDef *md;

  for (md = template_member_candidates(class_tok, member_tok);
       md; md = md->bucket_next)
  {
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
  TemplateInstOwner *owner;
  ClassBaseInfo *base;

  if (class_has_member_template_name(class_tok, member_tok))
    return 1;
  owner = find_template_inst_owner_entry(class_tok);
  if (owner && owner->owner->is_class
      && class_has_member_template_name(owner->owner->name_tok, member_tok)) return 1;
  for (base = class_base_candidates(class_tok); base; base = base->bucket_next)
    if (base->class_tok == class_tok
        && class_or_inst_has_member_template_name(base->base_tok, member_tok)) return 1;
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

static void skip_or_save_member_initializer(TokenString **str)
{
  int paren = 0, brace = 0, bracket = 0;

  if (str)
    *str = tok_str_alloc();
  while (tok != TOK_EOF)
  {
    if (!paren && !brace && !bracket && (tok == ',' || tok == ';'))
      break;
    if (str)
      tok_str_add_tok(*str);
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
  if (str)
    tok_str_add(*str, TOK_EOF);
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
    if (t == TOK_INLINE1 || t == TOK_INLINE2
        || t == TOK_INLINE3 || t == TOK_INLINE4)
      continue;
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
        || t == TOK_BOOL2
        || t == TOK_DECLTYPE || t == TOK_TYPEOF1 || t == TOK_TYPEOF2 || t == TOK_TYPEOF3
        || t == TOK_UNSIGNED || t == TOK_CONST1 || t == TOK_VOLATILE1
        || t >= TOK_UIDENT)
      return 1;
    return 0;
  }
  return 0;
}

static int parse_ctype_from_token_string(TokenString *str, CType *out)
{
  int i, last_tok = 0, saved_tok, v, ok = 0, btype_ok;
  CValue saved_tokc;
  AttributeDef ad;
  CType type;
  Sym *saved_local_stack;
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;

  if (!str || !token_string_starts_like_type(str))
    return 0;
  saved_tok = tok;
  saved_tokc = tokc;
  saved_local_stack = local_stack;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  /* Interface types outlive the expression or function scope that first
     requested them.  Keep pointer/reference derivation symbols on the
     global stack so cached overload metadata cannot dangle after scope pop. */
  local_stack = NULL;
  begin_macro(str, 3);
  next();
  memset(&type, 0, sizeof type);
  memset(&ad, 0, sizeof ad);
  btype_ok = parse_btype(&type, &ad, 0);
  if (btype_ok)
  {
    v = 0;
    type_decl(&type, &ad, &v, TYPE_ABSTRACT);
    if (tok == TOK_EOF || tok == 0)
    {
      for (i = 0; i < str->len; ++i)
      {
        if (str->str[i] == TOK_LINENUM)
        {
          ++i;
          continue;
        }
        if (str->str[i] == TOK_EOF || str->str[i] == 0)
          break;
        last_tok = str->str[i];
      }
      /* Abstract-declarator parsing can inherit VT_RVALUE_REFERENCE from a
         previously materialized class typedef.  The saved declaration is
         authoritative: a trailing '&' is an lvalue reference and must not
         later conflict with the concrete body definition. */
      if (last_tok == '&')
      {
        while (is_reference_type(&type)
               && is_reference_type(pointed_type(&type)))
        {
          CType collapsed = *pointed_type(pointed_type(&type));
          mk_reference(&collapsed);
          type = collapsed;
        }
        type.t &= ~VT_RVALUE_REFERENCE;
      }
      *out = type;
      ok = 1;
    }
  }
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  local_stack = saved_local_stack;
  return ok;
}

static TokenString *parse_substituted_template_constructor_initializers(
    TemplateMemberDef *md, int class_mangled_tok, int type_tok,
    int member_type_param_tok, int member_type_arg_tok, int value_param_tok,
    CType *struct_type)
{
  int saved_tok;
  CValue saved_tokc;
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;
  TokenString *subst, *prefix;

  /* Unlike the parameter list and function body, a constructor's member
     initializer list is parsed immediately during replay.  Materialize a
     substituted view of that slice so every enclosing class parameter (and
     every member-template parameter) is concrete before expression parsing.
     Keep the original stream positioned at the opening body brace. */
  subst = tok_str_alloc();
  while (tok != TOK_EOF && tok != '{')
  {
    tok_str_add_template_member_subst_scoped(subst, md, class_mangled_tok,
                                             md->type_param_tok, type_tok,
                                             member_type_param_tok,
                                             member_type_arg_tok,
                                             value_param_tok);
    next();
  }
  tok_str_add(subst, TOK_EOF);

  saved_tok = tok;
  saved_tokc = tokc;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  begin_macro(subst, 3);
  next();
  prefix = parse_constructor_member_initializers(struct_type);
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  tok_str_free(subst);
  return prefix;
}

static int deduce_direct_template_type(TokenString *definition, int start,
                                       int parameter, int end, CType *argument);

/* Expand the saved source pattern before ordinary substitution can erase which
   names belong to the pack. Every element must replay the entire expression,
   including casts, calls, subscripts and operators, with its own deduced type. */
static TokenString *expand_template_expression_packs(TokenString *source,
    int type_pack, int value_pack, int first_value_tok, int *types, int count)
{
  TokenString *result = tok_str_alloc();
  int i, skip_separator = 0;
  int declared_value_pack = value_pack;
  for (i = 0; i < source->len; ++i)
  {
    int token = source->str[i];
    if (token == TOK_EOF) break;
    if (TOK_HAS_VALUE(token)) {
      tok_str_add_record(result, source->str, source->len, &i);
      continue;
    }
    if (skip_separator) {
      skip_separator = 0;
      if (token == ',') continue;
    }
    if (token == TOK_DOTS)
    {
      int last = tok_str_prev_token_index(result->str, result->len, result->len);
      int begin = result->len, position = last;
      int parens = 0, brackets = 0, braces = 0, angles = 0, uses_pack = 0;
      if (last >= 0 && result->str[last] == TOK_SIZEOF) {
        int j = i + 1;
        CValue size;
        while (j < source->len && source->str[j] == TOK_LINENUM) j += 2;
        if (j < source->len && source->str[j] == '(') {
          ++j;
          while (j < source->len && source->str[j] == TOK_LINENUM) j += 2;
          if (j < source->len && (source->str[j] == type_pack
                                  || source->str[j] == declared_value_pack)) {
            ++j;
            while (j < source->len && source->str[j] == TOK_LINENUM) j += 2;
            if (j < source->len && source->str[j] == ')') {
              result->len = last;
              size.i = count;
              tok_str_add2(result, TOK_CULLONG, &size);
              i = j;
              continue;
            }
          }
        }
      }
      while (position >= 0)
      {
        int t = result->str[position];
        if (t == TOK_LINENUM) {
          position = tok_str_prev_token_index(result->str, result->len, position);
          continue;
        }
        if (!parens && !brackets && !braces && !angles
            && (t == ',' || t == ';' || t == TOK_RETURN || t == '=')) break;
        if (t == ')') ++parens;
        else if (t == '(') { if (!parens) break; --parens; }
        else if (t == ']') ++brackets;
        else if (t == '[') { if (!brackets) break; --brackets; }
        else if (t == '}') ++braces;
        else if (t == '{') { if (!braces) break; --braces; }
        /* Template argument commas are inside the postfix expression too. */
        else if (!parens && !brackets && !braces) {
          if (t == TOK_GT || t == '>') ++angles;
          else if (t == TOK_SAR) angles += 2;
          else if ((t == TOK_LT || t == '<') && angles) --angles;
          else if ((t == TOK_LT || t == '<') && !value_pack)
            break; /* type-id pack: the enclosing template argument list */
        }
        if (t == type_pack || t == declared_value_pack) uses_pack = 1;
        begin = position;
        position = tok_str_prev_token_index(result->str, result->len, position);
      }
      if (uses_pack)
      {
        TokenString *pattern = tok_str_alloc();
        int element, j;
        char parameter[256];
        for (j = begin; j < result->len; ++j)
          tok_str_add_record(pattern, result->str, result->len, &j);
        /* In a class definition, a function parameter pack includes the
           identifier after the ellipsis. Expand that declarator and retain
           its name for subsequent uses in the member's body. */
        if (!value_pack) {
          int next_index = i + 1;
          while (next_index < source->len && source->str[next_index] == TOK_LINENUM)
            next_index += 2;
          if (next_index < source->len && source->str[next_index] >= TOK_UIDENT) {
            declared_value_pack = source->str[next_index];
            tok_str_add(pattern, declared_value_pack);
            i = next_index;
          }
        }
        result->len = begin;
        for (element = 0; element < count; ++element) {
          if (element) tok_str_add(result, ',');
          snprintf(parameter, sizeof(parameter), "%s__pack%d",
                   declared_value_pack ? get_tok_str(declared_value_pack, NULL) : "", element);
          for (j = 0; j < pattern->len; ++j) {
            int t = pattern->str[j];
            if (t == type_pack) tok_str_add(result, types[element]);
            else if (declared_value_pack && t == declared_value_pack)
              tok_str_add(result, element == 0 && first_value_tok
                                    ? first_value_tok : tok_alloc_const(parameter));
            else tok_str_add_record(result, pattern->str, pattern->len, &j);
          }
        }
        tok_str_free(pattern);
        if (!count) {
          int previous = tok_str_prev_token_index(result->str, result->len,
                                                   result->len);
          if (previous >= 0 && result->str[previous] == ',') result->len = previous;
          else skip_separator = 1;
        }
        continue;
      }
    }
    tok_str_add(result, token);
  }
  tok_str_add(result, TOK_EOF);
  return result;
}

static TokenString *capture_template_pack_body(int type_pack, int value_pack,
                        int first_value_tok, int *types, int count)
{
  TokenString *source = tok_str_alloc(), *expanded;
  while (tok != TOK_EOF) {
    tok_str_add_tok(source);
    next();
  }
  tok_str_add(source, TOK_EOF);
  expanded = expand_template_expression_packs(source, type_pack, value_pack,
                                              first_value_tok, types, count);
  tok_str_free(source);
  begin_macro(expanded, 3);
  next();
  return expanded;
}

static void instantiate_template_member_if_needed(TemplateMemberDef *md,
                                                  int type_tok,
                                                  int class_mangled_tok,
                                                  int member_type_arg_tok)
{
  int saved_tok, method_tok, member_func_tok, paren, i, cv_qualifiers, noexcept_spec;
  int member_decl_exists = 0;
  int substitution_failed = 0;
  int qualifier_already_parsed = 0;
  int method_already_parsed = 0;
  CValue saved_tokc;
  CType struct_type;
  Sym *class_sym;
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;
  CType replay_ret_type, replay_func_type;
  TokenString *ret_str, *sig, *params_str, *proto, *body_def;
  TokenString definition_replay;
  TokenString *typed_params = NULL;
  TokenString *init_prefix = NULL;
  int init_prefix_emitted = 0;
  TokenString *clean_ret = NULL;
  TokenString *expanded_pack_body = NULL;
  PendingMemberFunc *pm;
  int value_param_tok = 0;
  int member_type_param_tok = 0;
  int auto_return_struct_tok = 0, deduction_owner_tok = 0;
  CType inferred_auto_ret;
  int has_inferred_auto_ret = 0;
  int is_static_member = 0;
  int is_explicit_constructor = 0;
  int is_conversion_operator = 0;
  int is_member_template_replay = 0;
  int orig_pack_tok = member_type_arg_tok;
  TemplateArgList explicit_arguments;
  TemplateArgList linkage_arguments = {{0}, 0};
  unsigned explicit_type_mask = 0, linkage_value_mask = 0;
  int explicit_param_names[16] = {0}, explicit_param_index = 0;
  int pack_is_empty = member_type_arg_tok == -1;
  int pack_name_tok = 0, pack_type_param_tok = 0, pack_type_arg_tok = 0;
  int pack_type_param_index = -1;
  int pack_pattern_start = -1, pack_pattern_end = -1, pack_pattern_parameter = -1;
  int member_type_param_count = 0;
  int pack_arg_start = -1;
  int cached_index = -1;
  int cached_inst = 0;
  CType replay_call_arg_types[CPC_MAX_CALL_ARGUMENTS];
  int replay_nb_call_arg_types = nb_template_member_call_arg_types < CPC_MAX_CALL_ARGUMENTS
                                   ? nb_template_member_call_arg_types : CPC_MAX_CALL_ARGUMENTS;
  int saved_nb_extra_params;
  int saved_extra_param_toks[15];
  int saved_extra_arg_toks[15];
  int materialize_body = requested_template_member_body == md
                         && !materializing_template_interface;
  int saved_namespace_tok;

  get_explicit_member_arguments(orig_pack_tok, &explicit_arguments);
  if (explicit_arguments.nb)
    member_type_arg_tok = explicit_arguments.toks[0];

  if (replay_nb_call_arg_types)
    memcpy(replay_call_arg_types, template_member_call_arg_types,
           replay_nb_call_arg_types * sizeof(CType));

  if (md->nested_class_template_tok) {
    TokenString *definition = tok_str_alloc(), *source = md->def_str;
    TokenString *parent = macro_stack;
    const int *parent_ptr = macro_ptr;
    int saved_token = tok, saved_namespace = current_namespace_scope_tok();
    CValue saved_value = tokc;
    int nested = make_static_member_tok(class_mangled_tok, md->nested_class_template_tok);
    if (template_member_lookup_inst(md, type_tok, 0, class_mangled_tok)) {
      tok_str_free(definition); return;
    }
    template_member_note_inst(md, type_tok, 0, class_mangled_tok);
    for (i = 0; i < source->len; ++i) {
      int token = source->str[i], replacement;
      if (TOK_HAS_VALUE(token)) { tok_str_add_record(definition, source->str, source->len, &i); continue; }
      if (token == md->class_tok && i + 1 < source->len && source->str[i + 1] == TOK_LT) {
        int cursor = i + 2, angle = 1;
        while (cursor < source->len && angle) {
          int part = source->str[cursor++];
          if (part == TOK_LT) ++angle;
          else if (part == TOK_GT) --angle;
          else if (part == TOK_SAR) angle -= 2;
        }
        if (!angle && cursor + 2 < source->len && source->str[cursor] == ':'
            && source->str[cursor + 1] == ':' && source->str[cursor + 2] == md->nested_class_template_tok) {
          tok_str_add(definition, nested); i = cursor + 2; continue;
        }
      }
      replacement = template_member_class_arg_subst_tok(md, class_mangled_tok, token);
      if (!replacement && token == md->type_param_tok) replacement = type_tok;
      if (!replacement && token == md->nested_class_template_tok) replacement = nested;
      tok_str_add(definition, replacement ? replacement : token);
    }
    tok_str_add(definition, TOK_EOF);
    enter_namespace_scope(0);
    begin_macro(definition, 1); next();
    if (is_template_keyword_tok(tok)) parse_template_decl();
    else decl(VT_CONST);
    while (macro_stack && macro_stack != parent) end_macro();
    macro_ptr = parent_ptr; tok = saved_token; tokc = saved_value;
    enter_namespace_scope(saved_namespace);
    return;
  }
  if (md->nested_class_tok) {
    TemplateMemberDef *bound = NULL;
    TemplateInstOwner *outer = find_template_inst_owner_entry(class_mangled_tok);
    int nested_tok = make_static_member_tok(class_mangled_tok, md->nested_class_tok);
    TokenString *source = md->def_str, *definition;
    if (!outer || !struct_find(nested_tok)) return;
    for (i = 0; i < nb_template_member_defs; ++i)
      if (template_member_defs[i]->nested_source == md
          && template_member_defs[i]->nested_outer_tok == class_mangled_tok) return;
    definition = tok_str_alloc();
    for (i = 0; i < source->len; ++i) {
      int token = source->str[i], substituted;
      if (TOK_HAS_VALUE(token)) {
        tok_str_add_record(definition, source->str, source->len, &i);
        continue;
      }
      if (token == md->class_tok && i + 1 < source->len
          && source->str[i + 1] == TOK_LT) {
        int at = i + 2, parameter = 0, matches = 1;
        for (;;) {
          int expected;
          if (parameter >= outer->owner->nb_type_params) { matches = 0; break; }
          expected = parameter == 0 ? md->type_param_tok
            : outer->owner->type_param_toks[parameter];
          while (at < source->len && source->str[at] == TOK_LINENUM) at += 2;
          if (parameter >= outer->owner->nb_type_params || at >= source->len
              || source->str[at++] != expected) { matches = 0; break; }
          ++parameter;
          while (at < source->len && source->str[at] == TOK_LINENUM) at += 2;
          if (at < source->len && source->str[at] == ',') { ++at; continue; }
          break;
        }
        if (matches && parameter == outer->owner->nb_type_params
            && at < source->len && source->str[at] == TOK_GT) {
          i = at;
          if (i + 3 < source->len && source->str[i + 1] == ':'
              && source->str[i + 2] == ':' && source->str[i + 3] == md->nested_class_tok) {
            i += 3;
            tok_str_add(definition, nested_tok);
          } else tok_str_add(definition, class_mangled_tok);
          continue;
        }
      }
      substituted = template_member_class_arg_subst_tok(md, class_mangled_tok, token);
      if (!substituted && token == md->type_param_tok) substituted = type_tok;
      if (!substituted && token == md->nested_class_tok) substituted = nested_tok;
      tok_str_add(definition, substituted ? substituted : token);
    }
    bound = cprime_mallocz(sizeof(*bound));
    bound->class_tok = bound->lookup_class_tok = nested_tok;
    bound->nested_source = md;
    bound->nested_outer_tok = class_mangled_tok;
    bound->def_str = definition;
    bound->method_tok = template_member_def_method_tok(bound);
    bound->method_tok_known = 1;
    dynarray_add(&template_member_defs, &nb_template_member_defs, bound);
    index_template_member(bound);
    instantiate_template_member_if_needed(bound, 0, nested_tok, 0);
    return;
  }

  method_tok = template_member_def_method_tok(md);
  if (template_member_has_variadic_pack(md))
    pack_arg_start = template_member_def_pack_param_index(md, method_tok,
                         &pack_pattern_start, &pack_pattern_end);

  cached_index = template_member_lookup_inst_index(md, type_tok, orig_pack_tok,
                                                   class_mangled_tok);
  cached_inst = template_member_lookup_inst(md, type_tok, orig_pack_tok,
                                            class_mangled_tok);
  /* Interface discovery may be separated from body materialization by the
     template worklist.  Preserve the complete call type vector on the cache
     entry so a later body replay expands the same pack, rather than falling
     back to its first element. */
  if (materialize_body && cached_index >= 0 && md->inst_call_arg_counts)
  {
    replay_nb_call_arg_types = md->inst_call_arg_counts[cached_index];
    if (replay_nb_call_arg_types)
      memcpy(replay_call_arg_types,
             &md->inst_call_arg_types[md->inst_call_arg_offsets[cached_index]],
             replay_nb_call_arg_types * sizeof(CType));
  }
  if (pack_arg_start >= 0 && replay_nb_call_arg_types == pack_arg_start)
  {
    /* A call can deduce an empty pack without an explicit argument-list
       marker, including a trailing pack after fixed parameters. */
    pack_is_empty = 1;
    if (!explicit_arguments.nb) member_type_arg_tok = 0;
  }
  if (pack_arg_start >= 0 && pack_arg_start < replay_nb_call_arg_types)
    pack_type_arg_tok = template_type_tok_from_ctype(
      &replay_call_arg_types[pack_arg_start]);
  else if (member_type_arg_tok == -1)
    member_type_arg_tok = 0;

  if (method_tok == TOK_CONSTRUCTOR1
      && replay_nb_call_arg_types == 1
      && (replay_call_arg_types[0].t & VT_BTYPE) == VT_STRUCT
      && get_struct_type_name_tok(&replay_call_arg_types[0])
           == class_mangled_tok
      && (template_member_is_call_specialized(md)
          || requested_template_member_body != md))
  {
    /* Constructor templates do not replace the implicit copy constructor.
       A same-class initialization can otherwise spuriously deduce the
       enclosing type into a nested parameter such as Base<U> and replay an
       unrelated Base<Derived> body. */
    return;
  }
  if (cached_inst
      && (!materialize_body || is_explicit_function_specialization(cached_inst)
          || (cached_index >= 0 && md->inst_states
              && md->inst_states[cached_index] >= 2)))
  {
    if (cached_inst > 1)
      last_instantiated_member_func_tok = cached_inst;
    return;
  }
  if (md->instantiating)
    return;
  if ((md->class_tok >= TOK_UIDENT
       && strstr(get_tok_str(md->class_tok, NULL), "initializer_list"))
      || (class_mangled_tok >= TOK_UIDENT
          && strstr(get_tok_str(class_mangled_tok, NULL), "initializer_list")))
  {
    template_member_note_inst(md, type_tok, orig_pack_tok, class_mangled_tok);
    return;
  }
  if (token_string_starts_template(md->def_str)
      && token_string_body_has_variadic_pack(md->def_str)
      && !pack_is_empty
      && member_type_arg_tok == 0)
  {
    template_member_note_inst(md, type_tok, orig_pack_tok, class_mangled_tok);
    return;
  }

  /* Mark before replaying the saved definition.  Constructor initializer
     parsing and overload resolution can ask for the same member again while
     this instantiation is still being built; without an in-progress mark, the
     compiler recursively replays the same template body until timeout. */
  if (!template_member_lookup_inst(md, type_tok, orig_pack_tok, class_mangled_tok))
    template_member_note_inst(md, type_tok, orig_pack_tok, class_mangled_tok);
  md->instantiating = 1;

  saved_nb_extra_params = nb_template_member_extra_params;
  memcpy(saved_extra_param_toks, template_member_extra_param_toks,
         sizeof(saved_extra_param_toks));
  memcpy(saved_extra_arg_toks, template_member_extra_arg_toks,
         sizeof(saved_extra_arg_toks));
  nb_template_member_extra_params = 0;

  /* A call may demand this signature from outside the declaration's
     namespace. Non-dependent parameter/default names retain the member's
     lexical context while concrete argument types are substituted. */
  saved_namespace_tok = current_namespace_scope_tok();
  enter_namespace_scope(symbol_namespace_scope_tok(class_mangled_tok));
  saved_tok = tok;
  saved_tokc = tokc;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  ret_str = tok_str_alloc();
  sig = tok_str_alloc();
  params_str = tok_str_alloc();

  /* A template definition is persistent and may be replayed recursively.
     begin_macro() stores its parent cursor in the TokenString itself, so
     using md->def_str directly would let a nested instantiation overwrite
     the suspended outer replay.  Give every replay its own cursor frame
     while sharing the immutable token buffer. */
  definition_replay = *md->def_str;
  begin_macro(&definition_replay, 3);
  next();
  if (is_template_keyword_tok(tok))
  {
    next();
    skip(TOK_LT);
    while (tok != TOK_GT && tok != TOK_EOF)
    {
      TokenString *default_tokens = NULL;
      int parameter, is_pack = 0, has_default = 0;
      int is_type = template_parameter_is_type();
      if (is_template_keyword_tok(tok)) explicit_type_mask |= 1u << explicit_param_index;
      if (explicit_param_index == 16)
        cprime_error("too many member template parameters");
      if (is_type) {
        int inferred_arg_tok = 0;
        parameter = parse_template_type_parameter_tok(explicit_param_index,
                      &is_pack, &has_default, &default_tokens);
        explicit_type_mask |= 1u << explicit_param_index;
        if (is_pack) {
          pack_type_param_tok = parameter;
          pack_type_param_index = explicit_param_index;
        }
        if (member_type_param_count < replay_nb_call_arg_types) {
          int call_arg_index = member_type_param_count;
          if (pack_arg_start >= 0 && member_type_param_count == 0)
            call_arg_index = pack_arg_start;
          if (call_arg_index < replay_nb_call_arg_types) {
            inferred_arg_tok = first_template_arg_tok_from_inst_type(
              &replay_call_arg_types[call_arg_index]);
            if (!inferred_arg_tok)
              inferred_arg_tok = template_type_tok_from_ctype(
                &replay_call_arg_types[call_arg_index]);
          }
        }
        if (member_type_param_count == 0)
          member_type_param_tok = parameter;
        else if (nb_template_member_extra_params < 15) {
          int extra = nb_template_member_extra_params++;
          template_member_extra_param_toks[extra] = parameter;
          template_member_extra_arg_toks[extra] = inferred_arg_tok
                                                   ? inferred_arg_tok : member_type_arg_tok;
        }
        ++member_type_param_count;
      } else {
        parameter = parse_template_non_type_parameter_tok(explicit_param_index, &has_default,
                                                           &default_tokens, NULL);
        value_param_tok = parameter;
      }
      explicit_param_names[explicit_param_index] = parameter;
      if (default_tokens) tok_str_free(default_tokens);
      if (tok != ',') break;
      ++explicit_param_index;
      next();
    }
    skip(TOK_GT);
  }

  if (value_param_tok && replay_nb_call_arg_types > 0
      && nb_template_member_extra_params < 15)
  {
    int arg_index = template_member_first_param_arg_index(
      md, method_tok, value_param_tok);
    int inferred_arg_tok = template_arg_tok_from_inst_type(
      &replay_call_arg_types[0], arg_index);
    if (inferred_arg_tok)
    {
      int extra = nb_template_member_extra_params++;
      template_member_extra_param_toks[extra] = value_param_tok;
      template_member_extra_arg_toks[extra] = inferred_arg_tok;
    }
  }

  if (!member_type_param_tok && md->nb_stripped_member_type_params > 0)
  {
    /* parse_template_decl consumes the outer template<> header.  For an
       out-of-class member template of a non-template class, retain those
       parameter names on the saved definition so replay can substitute the
       call-deduced types just like an inline member-template declaration. */
    member_type_param_tok = md->stripped_member_type_param_toks[0];
    member_type_param_count = md->nb_stripped_member_type_params;
    for (i = 1; i < md->nb_stripped_member_type_params
                && nb_template_member_extra_params < 15; ++i)
    {
      int inferred_arg_tok = 0;
      int extra = nb_template_member_extra_params++;
      if (i < replay_nb_call_arg_types)
      {
        inferred_arg_tok = first_template_arg_tok_from_inst_type(
          &replay_call_arg_types[i]);
        if (!inferred_arg_tok)
          inferred_arg_tok = template_type_tok_from_ctype(
            &replay_call_arg_types[i]);
      }
      template_member_extra_param_toks[extra] =
        md->stripped_member_type_param_toks[i];
      template_member_extra_arg_toks[extra] = inferred_arg_tok
                                               ? inferred_arg_tok
                                               : member_type_arg_tok;
    }
  }

  if (!value_param_tok && md->nb_stripped_member_value_params > 0)
  {
    value_param_tok = md->stripped_member_value_param_toks[0];
    if (replay_nb_call_arg_types > 0
        && nb_template_member_extra_params < 15)
    {
      int arg_index = template_member_first_param_arg_index(
        md, method_tok, value_param_tok);
      int inferred_arg_tok = template_arg_tok_from_inst_type(
        &replay_call_arg_types[0], arg_index);
      if (inferred_arg_tok)
      {
        int extra = nb_template_member_extra_params++;
        template_member_extra_param_toks[extra] = value_param_tok;
        template_member_extra_arg_toks[extra] = inferred_arg_tok;
      }
    }
  }

  if (pack_arg_start >= 0 && pack_arg_start < replay_nb_call_arg_types
      && pack_type_param_tok)
  {
    for (i = pack_pattern_start; i >= 0 && i < pack_pattern_end; ++i)
      if (md->def_str->str[i] == pack_type_param_tok)
      {
        int deduced;
        pack_pattern_parameter = i;
        deduced = deduce_direct_template_type(md->def_str, pack_pattern_start,
                      i, pack_pattern_end, &replay_call_arg_types[pack_arg_start]);
        if (deduced) pack_type_arg_tok = deduced;
        break;
      }
  }

  if (pack_type_param_tok == member_type_param_tok)
    member_type_arg_tok = pack_type_arg_tok;
  else if (pack_type_param_tok)
  {
    int extra;
    for (extra = 0; extra < nb_template_member_extra_params; ++extra)
      if (template_member_extra_param_toks[extra] == pack_type_param_tok)
        template_member_extra_arg_toks[extra] = pack_type_arg_tok;
  }

  if (explicit_arguments.nb)
  {
    int argument_index;
    for (argument_index = 0; argument_index < explicit_arguments.nb; ++argument_index)
    {
      int parameter = argument_index < 16 ? explicit_param_names[argument_index] : 0;
      int extra;
      if (pack_type_param_index >= 0 && argument_index > pack_type_param_index)
        continue; /* Later pack elements have their own replayed declarators. */
      if (!parameter && argument_index < md->nb_stripped_member_params)
        parameter = md->stripped_member_param_toks[argument_index];
      if (!parameter && argument_index < md->nb_stripped_member_type_params)
        parameter = md->stripped_member_type_param_toks[argument_index];
      if (!parameter && argument_index < md->nb_stripped_member_value_params)
        parameter = md->stripped_member_value_param_toks[argument_index];
      if (!parameter)
        cprime_error("too many explicit member-template arguments for '%s::%s'",
          get_tok_str(class_mangled_tok, NULL), get_tok_str(method_tok, NULL));
      if (parameter == member_type_param_tok)
      {
        member_type_arg_tok = explicit_arguments.toks[argument_index];
        continue;
      }
      for (extra = 0; extra < nb_template_member_extra_params; ++extra)
        if (template_member_extra_param_toks[extra] == parameter) break;
      if (extra == nb_template_member_extra_params)
      {
        if (extra == 15) cprime_error("too many member-template parameters");
        ++nb_template_member_extra_params;
        template_member_extra_param_toks[extra] = parameter;
      }
      template_member_extra_arg_toks[extra] = explicit_arguments.toks[argument_index];
    }
  }

  /* Explicit argument keys can use a different canonical alias for the
     same first pack type (notably an array reference). Expansion must
     replace the token actually substituted into the first declarator. */
  if (pack_type_param_tok == member_type_param_tok)
    pack_type_arg_tok = member_type_arg_tok;
  else if (pack_type_param_tok)
    for (i = 0; i < nb_template_member_extra_params; ++i)
      if (template_member_extra_param_toks[i] == pack_type_param_tok)
        pack_type_arg_tok = template_member_extra_arg_toks[i];

  for (;;)
  {
    if (tok == TOK_EOF)
      cprime_error("unsupported template member declaration %s::%s for %s", get_tok_str(md->class_tok,NULL),get_tok_str(method_tok,NULL),get_tok_str(class_mangled_tok,NULL));
    if (method_tok == TOK_CONSTRUCTOR1
        && (tok == TOK_INLINE1 || tok == TOK_INLINE2 || tok == TOK_INLINE3
            || tok == TOK_INLINE4
            || (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "constexpr"))))
    {
      next();
      continue;
    }
    if (tok == tok_explicit
        || (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "explicit")))
    {
      is_explicit_constructor = 1;
      next();
      continue;
    }
    if ((tok == md->class_tok && !is_bound_member_template_parameter(tok, member_type_param_tok))
        || (method_tok == TOK_CONSTRUCTOR1 && ret_str->len == 0
            && class_tok_matches_unqualified_name(md->class_tok, tok)))
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
      if (tok == ':')
      {
        int expected_method_tok = template_member_def_method_tok(md);
        int introduces_method;
        skip(':');
        skip(':');
        introduces_method = tok == expected_method_tok
          || (expected_method_tok == TOK_CONSTRUCTOR1
              && class_tok_matches_unqualified_name(md->class_tok, tok))
          || (expected_method_tok == TOK_DESTRUCTOR1 && tok == '~')
          || (tok == TOK_OPERATOR && expected_method_tok >= TOK_UIDENT
              && !strncmp(get_tok_str(expected_method_tok, NULL),
                          "operator", 8));
        if (introduces_method)
        {
          tok_str_free(candidate);
          qualifier_already_parsed = 2;
          break;
        }
        {
          int scoped_alias_tok = tok >= TOK_UIDENT
            ? template_member_scoped_alias_tok(class_mangled_tok, tok) : 0;
          if (scoped_alias_tok)
          {
            tok_str_free(candidate);
            tok_str_add(ret_str, scoped_alias_tok);
            next();
            continue;
          }
        }
        tok_str_free(candidate);
        tok_str_add(ret_str, md->class_tok);
        tok_str_add(ret_str, ':');
        tok_str_add(ret_str, ':');
        continue;
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
          int expected_method_tok = template_member_def_method_tok(md);
          int introduces_method;
          skip(':');
          skip(':');
          introduces_method = tok == expected_method_tok
            || (expected_method_tok == TOK_CONSTRUCTOR1
                && tok == md->class_tok)
            || (expected_method_tok == TOK_DESTRUCTOR1 && tok == '~')
            || (tok == TOK_OPERATOR && expected_method_tok >= TOK_UIDENT
                && !strncmp(get_tok_str(expected_method_tok, NULL),
                            "operator", 8));
          if (introduces_method)
          {
            tok_str_free(candidate);
            /* Both scope tokens were consumed while distinguishing a nested
               return alias from the actual member qualifier. */
            qualifier_already_parsed = 2;
            break;
          }
          {
            int scoped_alias_tok = tok >= TOK_UIDENT
              ? template_member_scoped_alias_tok(class_mangled_tok, tok)
              : 0;
            if (scoped_alias_tok)
            {
              int ret_end = ret_str->len;
              /* The instantiated class interface already owns a canonical
                 token for this nested alias.  Use it directly instead of
                 replaying Class<Args>::Alias through the template parser. */
              while (ret_end >= 2
                     && ret_str->str[ret_end - 2] == TOK_LINENUM)
                ret_end -= 2;
              if (ret_end > 0 && ret_str->str[ret_end - 1] >= TOK_UIDENT
                  && !strcmp(get_tok_str(ret_str->str[ret_end - 1], NULL),
                             "typename"))
                ret_str->len = ret_end - 1;
              tok_str_free(candidate);
              tok_str_add(ret_str, scoped_alias_tok);
              next();
              continue;
            }
          }
          tok_str_append(ret_str, candidate);
          tok_str_add(ret_str, ':');
          tok_str_add(ret_str, ':');
          tok_str_free(candidate);
          continue;
        }
        tok_str_append(ret_str, candidate);
        tok_str_free(candidate);
        continue;
      }
      tok_str_free(candidate);
      /* An unadorned class name in its member declaration denotes this
         specialization, including inside a member-template return type. */
      tok_str_add(ret_str, class_mangled_tok);
      continue;
    }
    if (tok == TOK_INLINE1 || tok == TOK_INLINE2 || tok == TOK_INLINE3)
    {
      /* Inline is a declaration specifier, not a return type. Template
         member definitions already receive ODR linkage when materialized. */
      next();
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
        && tok != member_type_param_tok
        && tok == template_member_def_method_tok(md))
    {
      /* Inline member templates have no Class:: qualifier: after the return
         type, the identifier that introduces the parameter list is the
         method name.  Out-of-class plain members can have qualified return
         types (const Vector4<T>& ...), so only treat an identifier as the
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
    if (tok == TOK_OPERATOR)
    {
      method_tok = parse_cpp_operator_method_tok();
      if (last_cpp_conversion_operator_type_tokens) {
        TokenString *conversion = take_cpp_conversion_operator_type_tokens();
        TokenString conversion_replay = *conversion;
        TokenString *parent = macro_stack;
        const int *parent_ptr = macro_ptr;
        int parent_tok = tok;
        CValue parent_tokc = tokc;
        is_conversion_operator = 1;
        /* The class qualifier was consumed while the method name was being
           identified; the conversion target is the only return type. */
        ret_str->len = 0;
        begin_macro(&conversion_replay, 3);
        next();
        while (tok != TOK_EOF) {
          tok_str_add_template_member_subst_scoped(ret_str, md, class_mangled_tok,
            md->type_param_tok, type_tok, member_type_param_tok,
            member_type_arg_tok, value_param_tok);
          next();
        }
        while (macro_stack && macro_stack != parent) end_macro();
        macro_ptr = parent_ptr; tok = parent_tok; tokc = parent_tokc;
        tok_str_free(conversion);
      }
      method_already_parsed = 1;
      break;
    }
    tok_str_add_template_member_subst_scoped(ret_str, md, class_mangled_tok,
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
    if (qualifier_already_parsed == 1)
    {
      skip(':');
      skip(':');
    }
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
      if (method_tok == md->class_tok
          || method_tok == class_mangled_tok
          || class_tok_matches_unqualified_name(class_mangled_tok, method_tok))
        method_tok = TOK_CONSTRUCTOR1;
      next();
    }
  }
  skip('(');
  is_static_member = !is_lifecycle_member_tok(method_tok)
                     && (template_member_definition_is_static(md)
                         || (!template_member_is_call_specialized(md)
                             && class_has_static_member_func(class_mangled_tok,
                                                             method_tok)));

  if (ret_str->len == 0
      && method_tok == tok_alloc_const("operator bool"))
    tok_str_add(sig, TOK_BOOL);
  else if (ret_str->len == 0)
  {
    if (!is_lifecycle_member_tok(method_tok))
      cprime_error("template member function requires a return type");
    tok_str_add(sig, TOK_VOID);
  }
  else if (token_string_is_auto_type(ret_str))
    tok_str_add(sig, materialize_body ? TOK_AUTO : TOK_VOID);

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
      if (tok == md->class_tok && !is_bound_member_template_parameter(tok, member_type_param_tok))
      {
        next();
        /* An injected class name denotes this specialization. An explicit
           template-id retains its arguments, including other specializations
           of the same primary template (Container<long> in Container<char>). */
        tok_str_add(params_str, (tok == TOK_LT || tok == '<')
                                 ? md->class_tok : class_mangled_tok);
        continue;
      }
      if (tok == TOK_DOTS) {
        int previous = tok_str_prev_token_index(params_str->str, params_str->len,
                                                params_str->len);
        if (previous < 0 || params_str->str[previous] == ',') {
          /* A standalone ellipsis is a variadic function parameter,
             independent of any template parameter packs. */
          tok_str_add_tok(params_str); next(); continue;
        }
      }
      if (tok == TOK_DOTS && !pack_is_empty && pack_type_arg_tok)
      {
        int pattern_start = params_str->len;
        int pattern_end = params_str->len;
        int pack_call_index;
        while (pattern_start > 0 && params_str->str[pattern_start - 1] != ',')
          --pattern_start;
        next();
        while (tok == TOK_LINENUM)
          next();
        if (tok >= TOK_UIDENT)
        {
          char expanded_name[256];
          int source_name_tok = tok;

          pack_name_tok = source_name_tok;
          tok_str_add(params_str, source_name_tok);
          next();
          for (pack_call_index = pack_arg_start + 1;
               pack_arg_start >= 0
                 && pack_call_index < replay_nb_call_arg_types;
               ++pack_call_index)
          {
            int concrete_tok = pack_pattern_parameter >= 0
              ? deduce_direct_template_type(md->def_str, pack_pattern_start,
                    pack_pattern_parameter, pack_pattern_end,
                    &replay_call_arg_types[pack_call_index])
              : template_type_tok_from_ctype(&replay_call_arg_types[pack_call_index]);
            int pi;
            tok_str_add(params_str, ',');
            for (pi = pattern_start; pi < pattern_end; ++pi)
            {
              int pt = params_str->str[pi];
              tok_str_add(params_str,
                          pt == pack_type_arg_tok && concrete_tok
                            ? concrete_tok : pt);
            }
            snprintf(expanded_name, sizeof(expanded_name), "%s__pack%d",
                     get_tok_str(source_name_tok, NULL),
                     pack_call_index - pack_arg_start);
            tok_str_add(params_str, tok_alloc_const(expanded_name));
          }
        }
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
  if (pack_name_tok && pack_arg_start >= 0) {
    int types[CPC_MAX_CALL_ARGUMENTS], element, count = replay_nb_call_arg_types - pack_arg_start;
    for (element = 0; element < count; ++element)
      types[element] = deduce_direct_template_type(md->def_str,
          pack_pattern_start, pack_pattern_parameter, pack_pattern_end,
          &replay_call_arg_types[pack_arg_start + element]);
    expanded_pack_body = capture_template_pack_body(pack_type_param_tok,
                               pack_name_tok, pack_name_tok, types, count);
  }
  {
    int depth = 0, at;
    for (at = 0; at < ret_str->len; ++at) {
      if (TOK_HAS_VALUE(ret_str->str[at])) {
        at += tok_str_value_extra_words(ret_str->str, ret_str->len, at);
        continue;
      }
      if (ret_str->str[at] == '(') ++depth;
      else if (ret_str->str[at] == ')') --depth;
    }
    if (depth > 0) {
      CType grouped_return;
      int return_name;
      /* The name and parameter list sit inside a grouped declarator such
         as char (&f(U*))[3]. Complete its abstract return declarator before
         publishing the ordinary lowered function signature. */
      while (depth || tok == '[' || tok == '(') {
        if (tok == TOK_EOF) cprime_error("incomplete member return declarator");
        if (tok == '(' || tok == '[') ++depth;
        else if (tok == ')' || tok == ']') --depth;
        tok_str_add_template_member_subst_scoped(ret_str, md, class_mangled_tok,
            md->type_param_tok, type_tok, member_type_param_tok, member_type_arg_tok, value_param_tok);
        next();
      }
      tok_str_free(clean_ret);
      clean_ret = tok_str_alloc();
      tok_str_append_without_eof(clean_ret, ret_str);
      tok_str_add(clean_ret, TOK_EOF);
      if (!parse_ctype_from_token_string(clean_ret, &grouped_return))
        cprime_error("invalid member return declarator");
      return_name = template_exact_ctype_typedef_tok(&grouped_return);
      sig->len = 0;
      tok_str_add(sig, return_name);
    }
  }
  {
    TokenString *suffix = tok_str_alloc();
    int have_suffix = 0;
    while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
           || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3
           || (tok >= TOK_UIDENT
               && (!strcmp(get_tok_str(tok, NULL), "noexcept")
                   || !strcmp(get_tok_str(tok, NULL), "override")
                   || !strcmp(get_tok_str(tok, NULL), "final")))) {
      int has_expression = tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "noexcept");
      have_suffix = 1;
      tok_str_add_template_member_subst_scoped(suffix, md, class_mangled_tok,
          md->type_param_tok, type_tok, member_type_param_tok, member_type_arg_tok, value_param_tok);
      next();
      if (has_expression && tok == '(') {
        int depth = 0;
        do {
          if (tok == '(') ++depth;
          else if (tok == ')') --depth;
          if (tok == TOK_EOF) cprime_error("unterminated exception specification");
          tok_str_add_template_member_subst_scoped(suffix, md, class_mangled_tok,
              md->type_param_tok, type_tok, member_type_param_tok, member_type_arg_tok, value_param_tok);
          next();
        } while (depth);
      }
    }
    if (have_suffix) {
      tok_str_add2(suffix, tok, &tokc);
      tok_str_add(suffix, 0);
      begin_macro(suffix, 1);
      next();
    } else tok_str_free(suffix);
  }
  cv_qualifiers = skip_member_func_cv_qualifiers();
  noexcept_spec = last_member_noexcept_spec;
  is_member_template_replay = template_member_is_call_specialized(md);
  if (is_member_template_replay) {
    int parameter_count = explicit_param_names[0] ? explicit_param_index + 1
                                                : md->nb_stripped_member_params;
    for (i = 0; i < parameter_count && i < 16; ++i) {
      int parameter = explicit_param_names[0] ? explicit_param_names[i]
                                            : md->stripped_member_param_toks[i];
      int is_value = explicit_param_names[0] ? !(explicit_type_mask & (1u << i))
                                            : !!(md->stripped_member_value_mask & (1u << i));
      int argument = i < explicit_arguments.nb ? explicit_arguments.toks[i] : 0;
      int extra;
      if (pack_arg_start >= 0 && parameter == pack_type_param_tok) {
        int element;
        for (element = pack_arg_start; element < replay_nb_call_arg_types; ++element) {
          int concrete = deduce_direct_template_type(md->def_str,
              pack_pattern_start, pack_pattern_parameter, pack_pattern_end,
              &replay_call_arg_types[element]);
          if (!concrete) concrete = template_type_tok_from_ctype(&replay_call_arg_types[element]);
          if (linkage_arguments.nb == CPC_MAX_TEMPLATE_ARGUMENTS) cprime_error("too many template linkage arguments");
          linkage_arguments.toks[linkage_arguments.nb++] = concrete;
        }
        continue;
      }
      if (!argument && parameter == member_type_param_tok) argument = member_type_arg_tok;
      for (extra = 0; !argument && extra < nb_template_member_extra_params; ++extra)
        if (template_member_extra_param_toks[extra] == parameter)
          argument = template_member_extra_arg_toks[extra];
      if (!argument && parameter == value_param_tok) argument = member_type_arg_tok;
      if (!argument) cprime_error("unresolved member template argument for native linkage");
      if (is_value) linkage_value_mask |= 1u << linkage_arguments.nb;
      linkage_arguments.toks[linkage_arguments.nb++] = argument;
    }
  }
  typed_params = tok_str_alloc();
  if (params_str->len > 0)
  {
    int start = params_str->str[0] == ',' ? 1 : 0;
    for (i = start; i < params_str->len; ++i)
      tok_str_add(typed_params, params_str->str[i]);
  }
  tok_str_add(typed_params, TOK_EOF);

  if (tok == TOK_ARROW)
  {
    TokenString *trailing = tok_str_alloc();
    TokenString *scope_parameters = tok_str_alloc();
    TokenString *declarator = tok_str_alloc();
    CType trailing_function;
    int depth = 0;
    next();
    while (tok != TOK_EOF)
    {
      if (!depth && (tok == '{' || tok == ';')) break;
      if (tok == '(') ++depth;
      else if (tok == ')') --depth;
      tok_str_add_template_member_subst_scoped(trailing, md, class_mangled_tok,
          md->type_param_tok, type_tok, member_type_param_tok,
          member_type_arg_tok, value_param_tok);
      next();
    }
    tok_str_add(trailing, TOK_EOF);
    if (!is_static_member) {
      tok_str_add(scope_parameters, TOK_STRUCT);
      tok_str_add(scope_parameters, class_mangled_tok);
      if (cv_qualifiers & VT_CONSTANT) tok_str_add(scope_parameters, TOK_CONST1);
      tok_str_add(scope_parameters, '*');
      tok_str_add(scope_parameters, tok_alloc_const("this"));
      if (typed_params->len > 1) tok_str_add(scope_parameters, ',');
    }
    tok_str_append_without_eof(scope_parameters, typed_params);
    tok_str_add(scope_parameters, TOK_EOF);
    tok_str_add(declarator, TOK_AUTO);
    tok_str_add(declarator, '(');
    tok_str_append_without_eof(declarator, scope_parameters);
    tok_str_add(declarator, ')');
    tok_str_add(declarator, TOK_ARROW);
    tok_str_append_without_eof(declarator, trailing);
    tok_str_add(declarator, TOK_EOF);
    if (!template_probe_type(declarator, 0,
          symbol_namespace_scope_tok(class_mangled_tok), &trailing_function))
      cprime_error("invalid member template trailing return type");
    inferred_auto_ret = trailing_function.ref->type;
    tok_str_free(declarator);
    tok_str_free(scope_parameters);
    tok_str_free(trailing);
    has_inferred_auto_ret = 1;
    auto_return_struct_tok = 0;
    sig->len = 0;
    add_ctype_tokens(sig, &inferred_auto_ret);
  }

  if (materialize_body && token_string_is_auto_type(ret_str) && !has_inferred_auto_ret && tok == '{') {
    TokenString *probe = tok_str_alloc();
    TokenString *stack = macro_stack;
    const int *ptr = macro_ptr;
    int token = tok;
    CValue value = tokc;
    int deduction_kind = append_placeholder_return_pattern(probe, ret_str, ret_str->len);
    tok_str_add(probe, '(');
    if (!is_static_member) {
      tok_str_add(probe, TOK_STRUCT);
      tok_str_add(probe, class_mangled_tok);
      if (cv_qualifiers & VT_CONSTANT) tok_str_add(probe,TOK_CONST1);
      tok_str_add(probe, '*');
      tok_str_add(probe, tok_alloc_const("this"));
      if (typed_params->len > 1) tok_str_add(probe, ',');
    }
    tok_str_append_without_eof(probe, typed_params);
    tok_str_add(probe, ')');
    {
      TokenString *remaining = tok_str_alloc();
      int at = 0, parens = 0, brackets = 0;
      /* Default arguments may contain braces. Only a brace outside the
         declarator's parentheses and brackets starts the function body. */
      for (; at < md->def_str->len; ++at)
      {
        int current = md->def_str->str[at];
        if (TOK_HAS_VALUE(current))
        {
          at += tok_str_value_extra_words(md->def_str->str, md->def_str->len, at);
          continue;
        }
        if (current == '(') ++parens;
        else if (current == ')') --parens;
        else if (current == '[') ++brackets;
        else if (current == ']') --brackets;
        else if (current == '{' && !parens && !brackets) break;
      }
      while (at < md->def_str->len && md->def_str->str[at]
             && md->def_str->str[at] != TOK_EOF)
      {
        tok_str_add_record(remaining, md->def_str->str, md->def_str->len, &at);
        ++at;
      }
      tok_str_add(remaining, TOK_EOF);
      begin_macro(remaining,3);
      next();
    }
    while(tok != TOK_EOF) {
      if (tok >= TOK_UIDENT && class_has_static_member_func(class_mangled_tok,tok)) {
        int previous=tok_str_prev_token_index(probe->str,probe->len,probe->len);
        int pt=previous>=0?probe->str[previous]:0;
        if(pt!='.' && pt!=TOK_ARROW && pt!=':') {
          tok_str_add(probe,class_mangled_tok);tok_str_add(probe,':');tok_str_add(probe,':');
        }
      }
      tok_str_add_template_member_subst_scoped(probe,md,class_mangled_tok,
        md->type_param_tok,type_tok,member_type_param_tok,member_type_arg_tok,value_param_tok);
      next();
    }
    tok_str_add(probe,TOK_EOF);
    while (macro_stack && macro_stack != stack) end_macro();
    macro_ptr=ptr;tok=token;tokc=value;
    {
      CType temporary_return=int_type;
      CType identity_type=make_member_func_type_from_saved_params(&temporary_return,typed_params, class_mangled_tok);
      identity_type.t |= cv_qualifiers;
      deduction_owner_tok=make_member_template_func_tok_for_type(class_mangled_tok,
          method_tok,&identity_type,md->def_str,member_type_arg_tok);
    }
    if (!template_probe_type_ex(probe, deduction_kind == 2 ? 5 : 4,
                                symbol_namespace_scope_tok(class_mangled_tok),
                                class_mangled_tok,deduction_owner_tok,&inferred_auto_ret))
      cprime_error("cannot deduce member return from saved body");
    tok_str_free(probe);
    has_inferred_auto_ret=1;
    auto_return_struct_tok=(inferred_auto_ret.t&VT_BTYPE)==VT_STRUCT
                            ?get_struct_type_name_tok(&inferred_auto_ret):0;
    sig->len=0;
    add_ctype_tokens(sig,&inferred_auto_ret);
  }

  class_sym = NULL;
  if (tok == ':' && (method_tok == TOK_CONSTRUCTOR1
                     || method_tok == md->class_tok
                     || method_tok == class_mangled_tok
                     || class_tok_matches_unqualified_name(class_mangled_tok,
                                                           method_tok)))
  {
    method_tok = TOK_CONSTRUCTOR1;
    class_sym = struct_find(class_mangled_tok);
    if (!class_sym)
      cprime_error("template member replay requires instantiated class '%s'",
                   get_tok_str(class_mangled_tok, NULL));
    class_sym->type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    struct_type.t = class_sym->type.t & ~(VT_CONSTANT | VT_VOLATILE);
    struct_type.ref = class_sym;
    if (!materialize_body)
    {
      int depth, open_tok, close_tok;
      next();
      while (tok != TOK_EOF && tok != '{')
      {
        while (tok != TOK_EOF && tok != '(' && tok != '{')
          next();
        if (tok == TOK_EOF)
          break;
        open_tok = tok;
        close_tok = open_tok == '(' ? ')' : '}';
        depth = 1;
        next();
        while (tok != TOK_EOF && depth > 0)
        {
          if (tok == open_tok)
            ++depth;
          else if (tok == close_tok)
            --depth;
          next();
        }
        if (tok == ',')
          next();
      }
    }
    else
    {
      Sym *saved_param_locals;
      int saved_param_scope;
      int saved_member_class = active_member_class_tok;
      active_member_class_tok = class_mangled_tok;
      /* Initializer argument types are resolved while the prefix is lowered.
         Give that probe the constructor's concrete parameter scope, just as
         the ordinary out-of-class member path does. */
      push_saved_param_scope(typed_params, &saved_param_locals,
                             &saved_param_scope);
      init_prefix = parse_substituted_template_constructor_initializers(
        md, class_mangled_tok, type_tok, member_type_param_tok,
        member_type_arg_tok, value_param_tok, &struct_type);
      pop_saved_param_scope(saved_param_locals, saved_param_scope);
      active_member_class_tok = saved_member_class;
    }
  }
  if (init_prefix && init_prefix->len > 2)
    note_struct_member_init_list(class_mangled_tok);

  if (is_lifecycle_member_tok(method_tok))
  {
    CType lowered_type;

    replay_ret_type.t = VT_VOID;
    replay_ret_type.ref = NULL;
    replay_func_type = make_member_func_type_from_saved_params(&replay_ret_type,
                                                        typed_params, class_mangled_tok);
    replay_func_type.ref->f.func_cpp_explicit = is_explicit_constructor;
    class_sym = struct_find(class_mangled_tok);
    if (!class_sym)
      cprime_error("template member replay requires instantiated class '%s'",
                   get_tok_str(class_mangled_tok, NULL));
    class_sym->type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    struct_type.t = class_sym->type.t & ~(VT_CONSTANT | VT_VOLATILE);
    struct_type.ref = class_sym;
    note_lifecycle_exception_spec(&struct_type, &replay_func_type,
                                  method_tok, noexcept_spec);
    lowered_type = make_lowered_member_func_type(&struct_type,
                                                 &replay_func_type);
    member_func_tok = cached_inst > 1 ? cached_inst
                     : is_member_template_replay
                     ? make_member_template_func_tok_for_type(class_mangled_tok,
                         method_tok, &replay_func_type, md->def_str,
                         explicit_arguments.nb ? orig_pack_tok : member_type_arg_tok)
                     : make_lifecycle_func_tok_for_type(class_mangled_tok,
                                                       method_tok,
                                                       &lowered_type,
                                                       0);
    if (is_member_template_replay)
      member_func_tok = canonical_cpp_member_template_tok(member_func_tok,
          class_mangled_tok, method_tok, &replay_func_type, &linkage_arguments,
          linkage_value_mask);
    note_raw_lifecycle_overload(class_mangled_tok, method_tok, member_func_tok,
                                params_str);
    note_cpp_template_member_decl(md, class_mangled_tok, method_tok,
        member_func_tok, &replay_func_type, 0, 0);
    if (is_member_template_replay)
    {
      note_cpp_member_template_arguments(member_func_tok, &linkage_arguments, linkage_value_mask);
      last_instantiated_member_func_tok = member_func_tok;
    }
    {
      Sym *function = external_global_sym(member_func_tok, &lowered_type);
      /* An instantiated constructor/destructor may be emitted by every
         translation unit that uses it, including templates on ordinary
         non-template classes. Coalesce those definitions at link time. */
      if (template_type_has_internal_linkage(&struct_type, 0))
        function->type.t |= VT_STATIC;
      else
        function->a.weak = 1;
      update_storage(function);
    }
    tok_str_add(sig, member_func_tok);
  }
  else
  {
    Sym *canonical_sym;
    replay_ret_type.t = VT_VOID;
    replay_ret_type.ref = NULL;
    if (has_inferred_auto_ret)
    {
      replay_ret_type = inferred_auto_ret;
    }
    else if (method_tok == tok_alloc_const("operator bool")
        && ret_str->len == 0)
    {
      replay_ret_type.t = VT_BOOL;
    }
    else if (auto_return_struct_tok
        && template_return_ctype_from_struct_tok(&replay_ret_type,
                                                 auto_return_struct_tok))
    {
      /* Auto-return member templates deduced through the return expression
         (return Class<T>, operators, or a same-class helper call) must
         carry the concrete struct return in the overload metadata. */
    }
    else if (is_member_template_replay && materializing_template_interface
        && clean_ret && !token_string_is_auto_type(ret_str))
    {
      if (!template_probe_type_ex(clean_ret, 0,
            symbol_namespace_scope_tok(class_mangled_tok), class_mangled_tok,
            0, &replay_ret_type)) {
        substitution_failed = 1;
        goto finish_member_signature_replay;
      }
    }
    else if (clean_ret
        && parse_ctype_from_token_string(clean_ret, &replay_ret_type))
    {
      /* Explicit member-template return types are parsed from the clean
         replay tokens so the overload metadata (and therefore call-site
         typing) sees the real return type instead of void. */
    }
    replay_func_type = make_member_func_type_from_saved_params(&replay_ret_type,
                                                        typed_params, class_mangled_tok);
    if (is_conversion_operator) {
      replay_func_type.ref->f.func_cpp_conversion = 1;
      replay_func_type.ref->f.func_cpp_explicit = is_explicit_constructor;
    }
    replay_func_type.t |= cv_qualifiers;
    apply_function_noexcept(&replay_func_type, noexcept_spec);
    class_sym = struct_find(class_mangled_tok);
    if (!class_sym)
      cprime_error("template member replay requires instantiated class '%s'",
                   get_tok_str(class_mangled_tok, NULL));
    class_sym->type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    struct_type.t = class_sym->type.t & ~(VT_CONSTANT | VT_VOLATILE);
    struct_type.ref = class_sym;
    if (cv_qualifiers & VT_CONSTANT)
      struct_type.t |= VT_CONSTANT;
    if (is_static_member)
    {
      int static_base_tok = make_static_member_tok(class_mangled_tok,
                                                   method_tok);
      CType static_type = replay_func_type;
      static_type.t &= ~VT_STATIC;
      if (is_member_template_replay)
      {
        member_func_tok = make_member_template_func_tok_for_type(class_mangled_tok,
                          method_tok, &static_type, md->def_str,
                          explicit_arguments.nb ? orig_pack_tok : member_type_arg_tok);
        member_func_tok = canonical_cpp_member_template_tok(member_func_tok,
            class_mangled_tok, method_tok, &static_type, &linkage_arguments,
            linkage_value_mask);
        note_cpp_template_member_decl(md, class_mangled_tok, method_tok,
            member_func_tok, &static_type, 1, 0);
        note_cpp_member_template_arguments(member_func_tok, &linkage_arguments, linkage_value_mask);
        external_global_sym(member_func_tok, &static_type);
        note_free_func_overload(static_base_tok, member_func_tok, &static_type);
        {
          FreeFuncOverload *overload;
          for (overload = free_func_overloads; overload; overload = overload->next)
            if (overload->mangled_tok == member_func_tok)
              overload->is_member_template = 1;
        }
      }
      else
      {
        declare_static_member_func(&struct_type, method_tok, &static_type);
        member_func_tok = make_static_member_func_tok_for_type(static_base_tok,
                                                               &static_type);
      }
    }
    else if (is_member_template_replay)
    {
      member_func_tok = make_member_template_func_tok_for_type(class_mangled_tok,
                                                               method_tok,
                                                               &replay_func_type,
                                                               md->def_str,
                                                               explicit_arguments.nb ? orig_pack_tok : member_type_arg_tok);
    }
    else
    {
      canonical_sym = auto_return_struct_tok
                      ? NULL
                      : resolve_member_func_by_param_signature(&struct_type,
                                                               method_tok,
                                                               &replay_func_type);
      if (canonical_sym)
      {
        /* The concrete out-of-class definition is the authoritative
           interface for this specialization.  Keep the canonical member
           symbol's return type in sync before its body is queued; otherwise
           later declaration replay can compare the body against stale
           reference metadata from the class-pattern parse. */
        if (canonical_sym->type.ref && replay_func_type.ref)
          canonical_sym->type.ref->type = replay_func_type.ref->type;
        member_func_tok = canonical_sym->v;
      }
      else
        member_func_tok = make_member_func_tok_for_type(class_mangled_tok,
                                                        method_tok,
                                                        &replay_func_type);
    }
    if (is_member_template_replay)
      member_func_tok = canonical_cpp_member_template_tok(member_func_tok,
          class_mangled_tok, method_tok, &replay_func_type, &linkage_arguments,
          linkage_value_mask);
    if (!materialize_body && token_string_is_auto_type(ret_str)
        && !has_inferred_auto_ret) {
      Sym *existing = global_symbol_find(member_func_tok);
      /* Another enclosing-class instantiation can rediscover this member
         through a fresh declaration record. Interface discovery has not
         deduced a replacement return type and must retain the completed
         declaration already associated with the canonical specialization. */
      if (existing && (existing->type.t & VT_BTYPE) == VT_FUNC
          && existing->type.ref) {
        replay_ret_type = existing->type.ref->type;
        replay_func_type.ref->type = replay_ret_type;
      }
    }
    note_cpp_template_member_decl(md, class_mangled_tok, method_tok,
        member_func_tok, &replay_func_type, is_static_member, 0);
    if (is_member_template_replay)
      note_cpp_member_template_arguments(member_func_tok, &linkage_arguments, linkage_value_mask);
    if (auto_return_struct_tok || is_member_template_replay)
      last_instantiated_member_func_tok = member_func_tok;
    {
      MemberFuncOverload *o;
      Sym *interface_sym = sym_find(member_func_tok);
      if (!interface_sym)
        interface_sym = global_symbol_find(member_func_tok);
      if (interface_sym && interface_sym->type.ref)
        interface_sym->type.ref->type = replay_ret_type;
      for (o = member_func_candidates(class_mangled_tok, method_tok);
           o; o = o->bucket_next)
        if (o->mangled_tok == member_func_tok && o->func_type.ref)
          o->func_type.ref->type = replay_ret_type;
    }
    note_local_type_function_owner(member_func_tok, deduction_owner_tok);
    tok_str_add(sig, member_func_tok);
  }
  if (template_type_has_internal_linkage(&struct_type, 0))
  {
    TokenString *internal_sig = tok_str_alloc();
    tok_str_add(internal_sig, TOK_STATIC);
    tok_str_append(internal_sig, sig);
    tok_str_free(sig);
    sig = internal_sig;
  }
  if (is_static_member)
  {
    int i2;
    TokenString *static_sig = tok_str_alloc();
    TokenString *static_params = tok_str_alloc();
    int param_start = (params_str->len > 0 && params_str->str[0] == ',')
                      ? 1 : 0;
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
  {
    struct FuncAttr exception_attr = {0};
    exception_attr.func_noexcept_specified = noexcept_spec != 0;
    exception_attr.func_noexcept = noexcept_spec == 1;
    append_function_exception_spec(sig, &exception_attr);
  }
  if (sym_find(member_func_tok) || global_symbol_find(member_func_tok))
  {
    member_decl_exists = 1;
  }

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
      for (i = first_pending_template_spec; i < nb_pending_template_specs;
           i = next_pending_template_spec(i + 1))
      {
        if (!pending_template_specs[i])
          continue;
        if (token_string_contains_tok(pending_template_specs[i], member_func_tok))
          break;
      }
      if (i < nb_pending_template_specs)
        tok_str_free(proto);
      else if (compiling_pending_template_specs)
        publish_template_member_proto(proto, class_mangled_tok);
      else
        queue_pending_template_spec_in_namespace(proto, class_mangled_tok,
          symbol_namespace_scope_tok(class_mangled_tok));
    }
  }

  body_def = tok_str_alloc();
  tok_str_append(body_def, sig);

  while (tok != TOK_EOF)
  {
    int prev_body_tok = 0, prev_prev_body_tok = 0, pi;
    for (pi = body_def->len - 1; pi >= 0; --pi)
    {
      if (body_def->str[pi] == TOK_LINENUM)
      {
        --pi;
        continue;
      }
      if (!prev_body_tok)
        prev_body_tok = body_def->str[pi];
      else
      {
        prev_prev_body_tok = body_def->str[pi];
        break;
      }
    }
    if (tok >= TOK_UIDENT
        && prev_body_tok != '.'
        && prev_body_tok != TOK_ARROW
        && !struct_find(tok)
        && !struct_find(make_static_member_tok(class_mangled_tok, tok))
        && !is_template_keyword_tok(prev_body_tok)
        && !(prev_body_tok == ':' && prev_prev_body_tok == ':')
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
        /* Preserve the qualified call for typed overload resolution. The
           enclosing specialization cannot determine a nested call's template
           arguments or select between scalar and aggregate overloads. */
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
              tok_str_add_template_member_subst_scoped(body_def, md, class_mangled_tok,
                                  md->type_param_tok, type_tok, member_type_param_tok,
                                  member_type_arg_tok, value_param_tok);
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
    if (tok == md->class_tok && !is_bound_member_template_parameter(tok, member_type_param_tok))
    {
      int self_type_pos = template_typedef_is_type_pos(body_def);
      int previous = body_def->len > 0 ? body_def->str[body_def->len - 1] : 0;
      if (previous == '.' || previous == TOK_ARROW) {
        tok_str_add_tok(body_def);
        next();
        continue;
      }
      next();
      while (tok == TOK_LINENUM)
        next();
      if (tok == TOK_LT || tok == '<')
      {
        int angle = 1;
        int all_class_params = 1;
        TokenString *explicit_args = tok_str_alloc();
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
          {
            if (angle == 1 && tok != ','
                && (tok < TOK_UIDENT
                    || !template_member_class_arg_subst_tok(md,
                                                            class_mangled_tok,
                                                            tok)))
              all_class_params = 0;
            tok_str_add_template_member_subst(explicit_args, md,
                                              class_mangled_tok,
                                              md->type_param_tok, type_tok,
                                              member_type_param_tok,
                                              member_type_arg_tok,
                                              value_param_tok);
            next();
          }
        }
        if (tok != TOK_GT)
          cprime_error("member-template body type close expected before '%s'",
                       get_tok_str(tok, &tokc));
        next();
        if (self_type_pos && tok != '(' && tok != '{' && tok != ':')
          tok_str_add(body_def, TOK_STRUCT);
        if (all_class_params)
        {
          /* Spelled entirely with this template's own parameters: the
             injected class name still denotes this instantiation. */
          tok_str_add(body_def, class_mangled_tok);
          tok_str_free(explicit_args);
          continue;
        }
        /* An explicit template-id in a member body names that specialization of
           the enclosing template, not the current instantiation. Keep the
           arguments, substituting the enclosing class and member parameters the
           same way the member's written return type does. */
        tok_str_add(body_def, md->class_tok);
        tok_str_add(body_def, TOK_LT);
        tok_str_append(body_def, explicit_args);
        tok_str_add(body_def, TOK_GT);
        tok_str_free(explicit_args);
        continue;
      }
      if (self_type_pos && tok != '(' && tok != '{' && tok != ':')
        tok_str_add(body_def, TOK_STRUCT);
      tok_str_add(body_def, class_mangled_tok);
      continue;
    }
    tok_str_add_template_member_subst_scoped(body_def, md, class_mangled_tok,
                                             md->type_param_tok, type_tok,
                                             member_type_param_tok,
                                             member_type_arg_tok,
                                             value_param_tok);
    if (init_prefix && !init_prefix_emitted && tok == '{')
    {
      tok_str_append(body_def, init_prefix);
      init_prefix_emitted = 1;
    }
    next();
  }
  tok_str_add(body_def, TOK_EOF);

finish_member_signature_replay:
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  if (expanded_pack_body) tok_str_free(expanded_pack_body);
  macro_ptr = saved_macro_ptr;
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
  if (substitution_failed) goto restore_member_instantiation;
  if (is_conversion_operator) {
    Sym *function = find_global_symbol(member_func_tok);
    if (!function) {
      CType lowered = make_lowered_member_func_type(&struct_type, &replay_func_type);
      function = external_global_sym(member_func_tok, &lowered);
    }
    if (function && function->type.ref) {
      function->type.ref->f.func_cpp_conversion = 1;
      function->type.ref->f.func_cpp_explicit = is_explicit_constructor;
      note_member_func_overload(class_mangled_tok, method_tok,
                                member_func_tok, &function->type);
    }
  }
  if (!is_static_member && !is_lifecycle_member_tok(method_tok)
      && (!member_type_param_tok || member_type_param_tok == md->type_param_tok)
      && method_tok == tok_alloc_const("operator=")
      && replay_func_type.ref && replay_func_type.ref->next)
  {
    CType param_type = replay_func_type.ref->next->type;
    int bt;
    if (is_reference_type(&param_type))
      param_type = *pointed_type(&param_type);
    bt = param_type.t & VT_BTYPE;
    if (btype_is_arithmetic_scalar(bt))
    {
      CType lowered_type = make_lowered_member_func_type(&struct_type,
                                                         &replay_func_type);
      note_member_func_overload(class_mangled_tok, method_tok,
                                member_func_tok, &lowered_type);
    }
  }
  if (is_explicit_function_specialization(member_func_tok)) materialize_body = 0;
  if (!materialize_body)
    tok_str_free(body_def);
  else if (pending_member_func_has_body_tok(member_func_tok))
    tok_str_free(body_def);
  else
  {
    Sym *function = find_global_symbol(member_func_tok);
    if (function) {
      /* Instantiated members are ODR definitions, including ordinary
         out-of-class members. Preserve their external identity. */
      if (template_type_has_internal_linkage(&struct_type, 0))
        function->type.t |= VT_STATIC;
      else
        function->a.weak = 1;
      update_storage(function);
    }
    pm = cprime_mallocz(sizeof(*pm));
    pm->str = body_def;
    pm->func_tok = member_func_tok;
    pm->struct_tok = class_mangled_tok;
    pm->is_template_member = 1;
    pm->is_lifecycle_member = is_lifecycle_member_tok(method_tok);
    pm->is_static_member = is_static_member;
    note_member_function_scope(member_func_tok, class_mangled_tok);
    queue_pending_member(pm);
  }
  if (template_member_is_call_specialized(md))
    note_member_function_template(member_func_tok, class_mangled_tok, md);
  /* The first enclosing argument alone cannot identify a class instance:
     members of Box<T, A> and Box<T, B> have different bindings and bodies. */
  for (i = 0; i < md->nb_inst; ++i)
    if (md->inst_type_toks[i] == type_tok
        && md->inst_owner_toks[i] == class_mangled_tok
        && (!(template_member_has_variadic_pack(md)
              || template_member_is_call_specialized(md))
            || md->inst_pack_toks[i] == orig_pack_tok))
    {
      md->inst_func_toks[i] = member_func_tok;
      if (md->inst_states)
      {
        md->inst_states[i] = materialize_body || is_explicit_function_specialization(member_func_tok) ? 2 : 1;
        if (materialize_body)
          materialized_member_body_note(member_func_tok);
      }
      break;
    }
restore_member_instantiation:
  md->instantiating = 0;
  nb_template_member_extra_params = saved_nb_extra_params;
  enter_namespace_scope(saved_namespace_tok);
  memcpy(template_member_extra_param_toks, saved_extra_param_toks,
         sizeof(saved_extra_param_toks));
  memcpy(template_member_extra_arg_toks, saved_extra_arg_toks,
         sizeof(saved_extra_arg_toks));
}

static int template_member_func_body_materialized(int func_tok)
{
  return materialized_member_body_has(func_tok);
}

static void instantiate_template_member_body_if_needed(TemplateMemberDef *md,
                                                         int type_tok,
                                                         int class_mangled_tok,
                                                         int member_type_arg_tok)
{
  TemplateMemberDef *saved_request = requested_template_member_body;
  int i;
  if (compiling_pending_member_funcs && !draining_template_member_body_requests
      && !token_string_is_auto_type_token_prefix(md->def_str))
  {
    /* Keep overload resolution usable in the current body: the interface
       instance already exists in the normal case, and this call publishes its
       concrete function token without upgrading it to a body replay. */
    instantiate_template_member_if_needed(md, type_tok, class_mangled_tok,
                                          member_type_arg_tok);
    for (i = 0; i < nb_pending_template_member_body_requests; ++i)
    {
      PendingTemplateMemberBodyRequest *request =
        &pending_template_member_body_requests[i];
      if (request->md == md && request->type_tok == type_tok
          && request->class_mangled_tok == class_mangled_tok
          && request->member_type_arg_tok == member_type_arg_tok)
        return;
    }
    if (nb_pending_template_member_body_requests
        >= al_pending_template_member_body_requests)
    {
      al_pending_template_member_body_requests =
        al_pending_template_member_body_requests
          ? al_pending_template_member_body_requests * 2 : 16;
      pending_template_member_body_requests = cprime_realloc(
        pending_template_member_body_requests,
        al_pending_template_member_body_requests
          * sizeof(*pending_template_member_body_requests));
    }
    pending_template_member_body_requests[
      nb_pending_template_member_body_requests].md = md;
    pending_template_member_body_requests[
      nb_pending_template_member_body_requests].type_tok = type_tok;
    pending_template_member_body_requests[
      nb_pending_template_member_body_requests].class_mangled_tok =
        class_mangled_tok;
    pending_template_member_body_requests[
      nb_pending_template_member_body_requests].member_type_arg_tok =
        member_type_arg_tok;
    ++nb_pending_template_member_body_requests;
    return;
  }
  requested_template_member_body = md;
  instantiate_template_member_if_needed(md, type_tok, class_mangled_tok,
                                        member_type_arg_tok);
  requested_template_member_body = saved_request;
}

static void drain_template_member_body_requests(void)
{
  int i, count = nb_pending_template_member_body_requests;

  if (!count || draining_template_member_body_requests)
    return;
  nb_pending_template_member_body_requests = 0;
  draining_template_member_body_requests = 1;
  for (i = 0; i < count; ++i)
  {
    PendingTemplateMemberBodyRequest request =
      pending_template_member_body_requests[i];
    TemplateMemberDef *saved_request = requested_template_member_body;
    requested_template_member_body = request.md;
    instantiate_template_member_if_needed(request.md, request.type_tok,
                                          request.class_mangled_tok,
                                          request.member_type_arg_tok);
    requested_template_member_body = saved_request;
  }
  draining_template_member_body_requests = 0;
}

static void instantiate_template_member_body_for_func_tok_impl(CType *type,
                                                          int method_tok,
                                                          int func_tok)
{
  TemplateDef *td;
  TemplateMemberDef *md;
  int struct_tok = get_struct_type_name_tok(type);
  int type_tok = 0;
  int explicit_specialization_owner = 0;
  int saved_pending = nb_pending_member_funcs;
  int i, j;
  MemberFunctionScope *scope;

  if (!struct_tok || !func_tok)
    return;
  /* An inline constructor template can belong to an ordinary class, and
     its selected interface already identifies the exact definition/cache
     entry. Demand that body before falling back to scoped class members. */
  for (scope = member_function_scopes[(unsigned)func_tok & (MEMBER_SCOPE_BUCKETS - 1)];
       scope; scope = scope->next)
    if (scope->function_tok == func_tok && scope->template_definition) {
      md = scope->template_definition;
      if (!template_member_has_body(md)) {
        if (md->class_tok == struct_tok) return;
        break;
      }
      for (j = 0; j < md->nb_inst; ++j)
        if (md->inst_owner_toks[j] == struct_tok && md->inst_func_toks
            && md->inst_func_toks[j] == func_tok) {
          instantiate_template_member_body_if_needed(md, md->inst_type_toks[j],
              struct_tok, md->inst_pack_toks ? md->inst_pack_toks[j] : 0);
          if (!compiling_pending_member_funcs && nb_pending_member_funcs != saved_pending)
            compile_pending_member_funcs(saved_pending);
          return;
        }
      break;
    }
  td = find_class_template_def_for_class_tok(struct_tok);
  if (!td)
    return;
  for (i = 0; i < td->nb_inst; ++i)
    if (td->inst_name_toks[i] == struct_tok)
    {
      if (td->inst_states && (td->inst_states[i] & 64))
        explicit_specialization_owner = 1;
      type_tok = td->inst_type_toks[i * td->nb_type_params];
      break;
    }
  if (!type_tok || explicit_specialization_owner)
    return;

  for (md = template_member_candidates(td->name_tok, method_tok);
       md; md = md->bucket_next)
  {
    if (md->class_tok != td->name_tok
        || template_member_def_method_tok(md) != method_tok
        || !template_member_def_is_scoped(md)
        || !template_member_has_body(md))
      continue;
    /* The class declaration can have been instantiated while an enclosing
       argument was incomplete. Its out-of-class interfaces were deferred
       then, but the selected ordinary member still needs its definition.
       Attach those interfaces now before looking up the selected cache key.
       Member templates already have their deduced interface above. */
    if (!template_member_is_call_specialized(md))
    {
      ++materializing_template_interface;
      instantiate_template_member_if_needed(md, type_tok, struct_tok, 0);
      --materializing_template_interface;
    }
    for (j = 0; j < md->nb_inst; ++j)
      if (md->inst_type_toks[j] == type_tok && md->inst_owner_toks[j] == struct_tok
          && md->inst_func_toks
          && md->inst_func_toks[j] == func_tok)
      {
        int pack_tok = md->inst_pack_toks ? md->inst_pack_toks[j] : 0;
        instantiate_template_member_body_if_needed(md, type_tok, struct_tok,
                                                   pack_tok);
        if (!compiling_pending_member_funcs
            && nb_pending_member_funcs != saved_pending)
          compile_pending_member_funcs(saved_pending);
        return;
      }
  }
}

static void instantiate_template_member_body_for_func_tok(CType *type,
                                                          int method_tok,
                                                          int func_tok)
{
  /* The selected interface already identifies this call. Completing its
     body can instantiate other receivers' members during return deduction;
     publish this selected target after all nested work has completed. */
  instantiate_template_member_body_for_func_tok_impl(type, method_tok, func_tok);
  last_instantiated_member_func_tok = func_tok;
}

static int template_member_def_first_param_is_rvalue_ref(
  TemplateMemberDef *md, int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    if (TOK_HAS_VALUE(md->def_str->str[i]))
    {
      i += tok_str_value_extra_words(md->def_str->str, md->def_str->len, i);
      continue;
    }
    int j = template_member_method_end_index(md, i, method_tok);
    int paren = 0, angle = 0, bracket = 0;
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
    if (j >= n || md->def_str->str[j] != '(')
      continue;
    for (++j; j < n; ++j)
    {
      int t = md->def_str->str[j];
      if (t == TOK_EOF || ((t == ')' || t == ',')
                           && !paren && !angle && !bracket))
        return 0;
      if (t == TOK_LAND && !paren && !angle && !bracket)
        return 1;
      if (t == '(')
        ++paren;
      else if (t == ')' && paren > 0)
        --paren;
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
    }
  }
  return 0;
}

static int instantiate_template_self_constructor_body(CType *type,
                                                      int move_source)
{
  TemplateDef *td;
  TemplateMemberDef *md;
  int struct_tok = get_struct_type_name_tok(type);
  int type_tok = 0;
  int i, result_tok = 0;

  if (!struct_tok)
    return 0;
  td = find_class_template_def_for_class_tok(struct_tok);
  if (!td)
    return 0;
  for (i = 0; i < td->nb_inst; ++i)
    if (td->inst_name_toks[i] == struct_tok)
    {
      type_tok = td->inst_type_toks[i * td->nb_type_params];
      break;
    }
  if (!type_tok)
    return 0;
  for (md = template_member_candidates(td->name_tok, TOK_CONSTRUCTOR1);
       md; md = md->bucket_next)
    if (md->class_tok == td->name_tok
        && template_member_def_method_tok(md) == TOK_CONSTRUCTOR1
        && template_member_def_is_scoped(md)
        && template_member_has_body(md)
        && !template_member_is_call_specialized(md)
        && template_member_def_param_count(md, TOK_CONSTRUCTOR1) == 1
        && template_member_def_first_param_class_kind(md,
                                                       TOK_CONSTRUCTOR1) == 1
        && template_member_def_first_param_is_rvalue_ref(
             md, TOK_CONSTRUCTOR1) == move_source)
    {
      int j;
      instantiate_template_member_body_if_needed(md, type_tok, struct_tok, 0);
      for (j = 0; j < md->nb_inst; ++j)
        if (md->inst_type_toks[j] == type_tok && md->inst_owner_toks[j] == struct_tok
            && md->inst_func_toks
            && md->inst_func_toks[j])
          result_tok = md->inst_func_toks[j];
    }
  return result_tok;
}

static void instantiate_template_members_for_class(TemplateDef *td, int type_tok,
    int class_mangled_tok)
{
  int i, inst_index = -1;
  Sym *arg_sym, *owner;

  if (!td->is_class)
    return;
  /* A forward tag can be published while a nested field's layout is being
     requested. Its class declaration must establish the canonical member
     overloads before out-of-class definitions attach their interfaces. */
  owner = struct_find(class_mangled_tok);
  if (!owner || owner->c < 0 || is_currently_defining_class(class_mangled_tok))
    return;
  arg_sym = struct_find(type_tok);
  if ((arg_sym && arg_sym->c < 0) || is_currently_defining_class(type_tok))
    return;
  for (i = 0; i < td->nb_inst; ++i)
    if (td->inst_name_toks[i] == class_mangled_tok)
    {
      inst_index = i;
      break;
    }
  if (inst_index >= 0 && td->inst_states
      && (td->inst_states[inst_index] & 64))
    return;
  if (inst_index >= 0 && td->inst_states)
  {
    if (td->inst_states[inst_index] >= 2)
      return;
    td->inst_states[inst_index] = 1;
  }
  materializing_template_interface++;
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (md->class_tok == td->name_tok
        && (md->nested_class_template_tok || (template_member_def_is_scoped(md)
        && !template_member_is_call_specialized(md)
        && !token_string_is_auto_type_token_prefix(md->def_str))))
      instantiate_template_member_if_needed(md, type_tok,
                                            class_mangled_tok, 0);
  }
  materializing_template_interface--;
  if (inst_index >= 0 && td->inst_states)
    td->inst_states[inst_index] = 2;
}

static int template_arg_tok_from_inst_type(CType *type, int arg_index)
{
  CType value_type;
  TemplateInstOwner *owner;
  TemplateDef *td;
  int struct_tok;

  if (arg_index < 0)
    return 0;
  value_type = *type;
  value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&value_type))
    value_type = *pointed_type(&value_type);
  struct_tok = get_struct_type_name_tok(&value_type);
  if (!struct_tok)
    return 0;
  owner = find_template_inst_owner_entry(struct_tok);
  if (!owner || !owner->owner->is_class)
    return 0;
  td = owner->owner;
  if (arg_index >= td->nb_type_params)
    return 0;
  return td->inst_type_toks[owner->inst_index * td->nb_type_params + arg_index];
}

static int first_template_arg_tok_from_inst_type(CType *type)
{
  return template_arg_tok_from_inst_type(type, 0);
}

static void instantiate_static_template_member_for_call(int class_mangled_tok,
                                                        int member_tok)
{
  TemplateInstOwner *owner;
  TemplateDef *td;
  TemplateMemberDef *md;

  if (!class_mangled_tok || !member_tok)
    return;
  owner = find_template_inst_owner_entry(class_mangled_tok);
  if (!owner || !owner->owner->is_class
      || !class_has_static_member_func(class_mangled_tok, member_tok))
    return;
  td = owner->owner;
  for (md = template_member_candidates(td->name_tok, member_tok);
       md; md = md->bucket_next)
    if (md->class_tok == td->name_tok
        && template_member_def_method_tok(md) == member_tok
        && !template_member_is_call_specialized(md)
        && template_member_has_body(md))
    {
      /* This lookup precedes argument parsing and overload selection.
         Publish every interface, then let the selected function symbol
         demand its body. Uncalled overloads may depend on incomplete
         types and must never be instantiated just because they share
         the selected member's name. */
      ++materializing_template_interface;
      instantiate_template_member_if_needed(
          md, td->inst_type_toks[owner->inst_index * td->nb_type_params],
          class_mangled_tok, 0);
      --materializing_template_interface;
    }
}

static int parse_template_type_parameter_tok(int index, int *is_variadic,
                                             int *has_default,
                                             TokenString **default_str);
static int parse_template_non_type_parameter_tok(int parameter_index, int *has_default,
                                                 TokenString **default_str, TokenString **type_str);

/* Deduce a non-type parameter only when it is the complete bound of an
   array dimension. Expressions such as N + 1 are non-deduced contexts.
   Keep dimensions and repeated occurrences paired with their actual array
   arguments instead of applying ordinary pointer decay. */
static int deduce_braced_array_type(TokenString *initializer, CType *result)
{
  TokenString **elements;
  CType element, common;
  int i, valid = 1, count = split_saved_array_initializer(initializer, &elements);
  if (!count) { free_saved_array_elements(elements, count); return 0; }
  for (i = 0; i < count; ++i)
  {
    if (saved_arg_is_braced(elements[i]))
      valid = deduce_braced_array_type(elements[i], &element);
    else
    {
      infer_saved_arg_type_and_category(elements[i], &element, NULL);
      if (is_reference_type(&element)) element = *pointed_type(&element);
      element.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_STORAGE | VT_RVALUE_REFERENCE
                     | VT_NULLPTR_TYPE);
    }
    if (!valid || (i && !is_compatible_types(&common, &element)))
    { valid = 0; break; }
    common = element;
  }
  free_saved_array_elements(elements, count);
  if (!valid) return 0;
  result->t = VT_PTR | VT_ARRAY;
  result->ref = sym_push2(&global_stack, SYM_FIELD, 0, count);
  result->ref->type = common;
  return 1;
}

static CType array_deduction_argument(CType argument)
{
  if (is_reference_type(&argument)) argument = *pointed_type(&argument);
  if (argument.t == VT_BRACED_LIST && argument.ref)
  {
    CType shape;
    if (deduce_braced_array_type(argument.ref->default_arg, &shape)) return shape;
  }
  return argument;
}

static int deduce_function_array_argument(TokenString *source, int start, int parameter,
                                         int value_parameter,
                                         CType *arguments, int argument_count)
{
  int i, argument = 0, paren = 0, angle = 0, bracket = 0;
  int found = 0;
  int result_type = 0, saw_type = 0, dimensions = 0, qualifiers = 0;
  int64_t bound = 0;
  CType actual;
  if (!arguments || !argument_count) return 0;
  if (!token_string_signature_has_array_brackets(source)) return 0;
  for (i = 0; i < argument_count; ++i) {
    actual = array_deduction_argument(arguments[i]);
    if (actual.t & VT_ARRAY) break;
  }
  if (i == argument_count) return 0;
  if (start < 0) return 0;
  actual = array_deduction_argument(arguments[0]);
  for (i = start; i < source->len && argument < argument_count; ++i) {
    int token = source->str[i];
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    if (!paren && !angle && !bracket && (token == ',' || token == ')')) {
      if (!value_parameter && saw_type && dimensions) {
        int type_tok;
        actual.t &= ~qualifiers;
        type_tok = actual.t & VT_ARRAY ? template_ctype_typedef_tok(&actual)
                                       : template_type_tok_from_ctype(&actual);
        if (result_type && !template_type_arg_toks_match(result_type, type_tok)) return -1;
        result_type = type_tok;
      }
      if (token == ')' || ++argument == argument_count) break;
      actual = array_deduction_argument(arguments[argument]);
      saw_type = dimensions = qualifiers = 0;
      continue;
    }
    if (!paren && !angle && !bracket) {
      if (token == parameter) saw_type = 1;
      if (token == TOK_CONST1 || token == TOK_CONST2 || token == TOK_CONST3) qualifiers |= VT_CONSTANT;
      if (token == TOK_VOLATILE1 || token == TOK_VOLATILE2 || token == TOK_VOLATILE3) qualifiers |= VT_VOLATILE;
    }
    if (token == '[' && !bracket && !angle) {
      int name = tok_str_next_non_linenum_index(source->str, source->len, i + 1);
      int close = tok_str_next_non_linenum_index(source->str, source->len, name + 1);
      if (value_parameter && name < source->len && source->str[name] == parameter
          && close < source->len && source->str[close] == ']') {
        if (!(actual.t & VT_ARRAY) || !actual.ref || actual.ref->c < 0) return 0;
        if (found && bound != actual.ref->c) return -1;
        found = 1; bound = actual.ref->c;
      }
      if ((actual.t & VT_ARRAY) && actual.ref) {
        actual = *pointed_type(&actual);
        ++dimensions;
      }
    }
    if (token == '(') ++paren;
    else if (token == ')' && paren) --paren;
    else if (token == '[') ++bracket;
    else if (token == ']' && bracket) --bracket;
    else if (token == TOK_LT || token == '<') ++angle;
    else if ((token == TOK_GT || token == '>') && angle) --angle;
    else if ((token == TOK_SAR || token == TOK_SHR) && angle) angle = angle > 1 ? angle - 2 : 0;
  }
  return value_parameter ? (found ? template_integer_expression_arg_tok(bound) : 0)
                         : result_type;
}

static int deduce_member_array_argument(TemplateMemberDef *member, int parameter,
                                         int value_parameter,
                                         CType *arguments, int argument_count)
{
  TokenString *source = member->def_str;
  int method = template_member_def_method_tok(member), i;
  for (i = 0; i < source->len; ++i) {
    int after;
    if (TOK_HAS_VALUE(source->str[i])) {
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    after = template_member_method_end_index(member, i, method);
    if (!after) continue;
    after = tok_str_next_non_linenum_index(source->str, source->len, after);
    if (after < source->len && source->str[after] == '(')
      return deduce_function_array_argument(source, after + 1, parameter,
                       value_parameter, arguments, argument_count);
  }
  return 0;
}

static int template_member_deduction_end(TemplateMemberDef *md)
{
  TokenString *source = md->def_str;
  int i, depth = 0, opening = -1, last = 0, end = source->len;
  if (md->method_tok_known && (md->syntax_known & (1u << TMS_DEDUCTION_END)))
    return md->syntax_cache[TMS_DEDUCTION_END];
  /* Remove only a final complete brace block. Earlier blocks may be lambdas
     or braced defaults in the signature and must remain available to SFINAE. */
  for (i = 0; i < source->len; ++i) {
    int token = source->str[i];
    if (token == TOK_EOF) break;
    if (token != TOK_LINENUM) last = token;
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    if (token == '{') { if (!depth) opening = i; ++depth; }
    else if (token == '}') --depth;
  }
  if (last == '}' && !depth && opening >= 0) end = opening;
  if (md->method_tok_known) {
    md->syntax_cache[TMS_DEDUCTION_END] = end;
    md->syntax_known |= 1u << TMS_DEDUCTION_END;
  }
  return end;
}

/* Deduction follows the complete function parameter list. Use the same
   template parameter parser as free functions so unnamed/defaulted parameters
   do not acquire spurious identities from tokens in their defaults. */
static int deduce_member_template_argument_key(TemplateMemberDef *md,
                                               int owner_tok,
                                               CType *argument_types,
                                               int argument_count)
{
  TemplateDef function_view;
  TemplateArgList arguments, explicit_arguments;
  int has_explicit_arguments = 0;
  TokenString replay, *saved_stack, *specialized_definition;
  const int *saved_ptr;
  CValue saved_tokc;
  int names[16], value_parameters[16] = {0}, count = 0, required = 0, i, saved_tok;
  int has_pack = 0, pack_index = -1, key = 0;
  int definition_end;
  if (!argument_types || argument_count <= 0
      || !template_member_is_call_specialized(md))
    return 0;
  if (explicit_member_template_arg_tok)
  {
    has_explicit_arguments = 1;
    if (!get_explicit_member_arguments(explicit_member_template_arg_tok, &explicit_arguments))
    {
      explicit_arguments.nb = 1;
      explicit_arguments.toks[0] = explicit_member_template_arg_tok;
    }
  }
  memset(&function_view, 0, sizeof(function_view));
  saved_tok = tok; saved_tokc = tokc;
  saved_stack = macro_stack; saved_ptr = macro_ptr;
  /* Do not substitute the body for every overload probe. */
  definition_end = template_member_deduction_end(md);
  specialized_definition = tok_str_alloc();
  for (i = 0; i < definition_end; ++i) {
    int token = md->def_str->str[i];
    int replacement;
    if (TOK_HAS_VALUE(token)) {
      tok_str_add_record(specialized_definition, md->def_str->str, md->def_str->len, &i);
      continue;
    }
    replacement = template_member_class_arg_subst_tok(md, owner_tok, token);
    if (!replacement && token >= TOK_UIDENT)
      replacement = template_member_scoped_alias_tok(owner_tok, token);
    tok_str_add(specialized_definition, replacement ? replacement : token);
  }
  tok_str_add(specialized_definition, TOK_EOF);
  replay = *specialized_definition;
  begin_macro(&replay, 3);
  next();
  if (is_template_keyword_tok(tok))
  {
    next(); skip(TOK_LT);
    while (tok != TOK_GT && tok != TOK_EOF)
    {
      int is_pack = 0, has_default = 0;
      if (count == 16) cprime_error("too many member template parameters");
      if (template_parameter_is_type())
        names[count] = parse_template_type_parameter_tok(count, &is_pack,
                           &has_default, &function_view.default_arg_strs[count]);
      else
      {
        value_parameters[count] = !is_template_keyword_tok(tok);
        names[count] = parse_template_non_type_parameter_tok(count, &has_default,
                            &function_view.default_arg_strs[count],
                            &function_view.value_type_strs[count]);
      }
      if (is_pack) pack_index = count;
      ++count;
      has_pack |= is_pack;
      if (!has_default && !is_pack) required = count;
      if (tok != ',') break;
      next();
    }
  }
  while (macro_stack && macro_stack != saved_stack) end_macro();
  macro_ptr = saved_ptr; tok = saved_tok; tokc = saved_tokc;
  if (!count && md->nb_stripped_member_params) {
    count = required = md->nb_stripped_member_params;
    memcpy(names, md->stripped_member_param_toks, count * sizeof(int));
    for (i = 0; i < count; ++i)
      value_parameters[i] = (md->stripped_member_value_mask >> i) & 1;
  }
  if (!count && md->nb_stripped_member_type_params)
  {
    count = required = md->nb_stripped_member_type_params;
    memcpy(names, md->stripped_member_type_param_toks, count * sizeof(int));
  }
  if (!count && md->nb_stripped_member_value_params)
  {
    count = required = md->nb_stripped_member_value_params;
    memcpy(names, md->stripped_member_value_param_toks, count * sizeof(int));
    for (i = 0; i < count; ++i) value_parameters[i] = 1;
  }
  if (count)
  {
    for (i = 0; i < count; ++i)
      if (value_parameters[i]) function_view.value_param_mask |= 1u << i;
    function_view.name_tok = template_member_def_method_tok(md);
    if (function_view.name_tok == TOK_CONSTRUCTOR1) {
      const CppRecordDeclInfo *record = lookup_cpp_record_decl(md->class_tok);
      function_view.name_tok = token_string_contains_tok(specialized_definition, md->class_tok)
          ? md->class_tok : record ? record->source_name_tok : md->class_tok;
    }
    function_view.type_param_toks = names;
    function_view.nb_type_params = count;
    function_view.nb_required_type_params = required;
    function_view.variadic_param_index = pack_index;
    if (pack_index >= 0)
      function_view.func_min_args = template_member_def_pack_param_index(md,
                 template_member_def_method_tok(md), NULL, NULL);
    function_view.def_str = specialized_definition;
    function_view.namespace_tok = symbol_namespace_scope_tok(owner_tok);
    function_view.member_class_tok = owner_tok;
    key = -1; /* Failed complete deduction removes this candidate. */
    if (infer_template_args_from_call_with_explicit(&function_view, argument_types,
          argument_count, &arguments, has_explicit_arguments ? &explicit_arguments : NULL)) {
      int valid = 1;
      for (i = 0; i < count; ++i) {
        if (has_explicit_arguments && i < explicit_arguments.nb) continue;
        int value = deduce_member_array_argument(md, names[i], value_parameters[i],
                                                  argument_types, argument_count);
        if (value < 0 || (value_parameters[i] && !value && !arguments.toks[i])) { valid = 0; break; }
        if (value) arguments.toks[i] = value;
      }
      if (valid) {
        int rank = template_call_signature_rank_bound(&function_view, argument_types,
                                  argument_count, &arguments, 2);
        if (rank != -1) key = intern_explicit_member_arguments(&arguments);
      }
    }
  }
  for (i = 0; i < count; ++i)
    if (function_view.default_arg_strs[i]) tok_str_free(function_view.default_arg_strs[i]);
  for (i = 0; i < count; ++i)
    if (function_view.value_type_strs[i]) tok_str_free(function_view.value_type_strs[i]);
  tok_str_free(specialized_definition);
  return key;
}

static int class_has_concrete_member_template_declaration(int class_tok,
                                                          int method_tok)
{
  TemplateMemberDef *member;
  for (member = template_member_candidates(class_tok, method_tok);
       member; member = member->bucket_next)
    if (member->lookup_class_tok == class_tok
        && template_member_def_method_tok(member) == method_tok)
      return 1;
  return 0;
}

static int resolve_qualified_static_template_call(int class_tok, int method_tok)
{
  TemplateInstOwner *owner = find_template_inst_owner_entry(class_tok);
  TemplateDef *class_template = owner ? owner->owner : NULL;
  TemplateMemberDef *member;
  TokenString *saved_arguments[CPC_MAX_CALL_ARGUMENTS];
  CType argument_types[CPC_MAX_CALL_ARGUMENTS], saved_types[CPC_MAX_CALL_ARGUMENTS];
  TemplateArgList explicit_arguments;
  int argument_count, key = 0, i, class_argument = 0, base_tok, address;
  int candidates[64], candidate_count = 0;
  TemplateMemberDef *candidate_defs[64];
  int candidate_keys[64];
  int member_owner_tok = class_template ? class_template->name_tok : class_tok;
  int saved_count = nb_template_member_call_arg_types;
  Sym *selected;
  if (class_has_concrete_member_template_declaration(class_tok, method_tok))
  {
    class_template = NULL;
    member_owner_tok = class_tok;
  }
  if (tok == TOK_LT)
  {
    parse_template_type_args(&explicit_arguments);
    key = intern_explicit_member_arguments(&explicit_arguments);
  }
  address = tok != '(';
  if (address && !key) return 0;
  base_tok = make_static_member_tok(class_tok, method_tok);
  if (!key)
  {
    FreeFuncOverload *overload;
    for (overload = free_func_overloads; overload; overload = overload->next)
      if (overload->name_tok == base_tok && !overload->is_member_template)
      {
        if (candidate_count == 64)
          cprime_error("too many static member candidates");
        candidate_defs[candidate_count] = NULL;
        candidate_keys[candidate_count] = 0;
        candidates[candidate_count++] = overload->mangled_tok;
      }
  }
  argument_count = address ? 0
    : probe_template_call_args(saved_arguments, argument_types, CPC_MAX_CALL_ARGUMENTS);
  memcpy(saved_types, template_member_call_arg_types, saved_count * sizeof(CType));
  nb_template_member_call_arg_types = argument_count < CPC_MAX_CALL_ARGUMENTS ? argument_count : CPC_MAX_CALL_ARGUMENTS;
  memcpy(template_member_call_arg_types, argument_types,
         nb_template_member_call_arg_types * sizeof(CType));
  if (class_template)
    for (i = 0; i < class_template->nb_inst; ++i)
      if (class_template->inst_name_toks[i] == class_tok)
        class_argument = class_template->inst_type_toks[i * class_template->nb_type_params];
  for (member = template_member_candidates(member_owner_tok, method_tok);
       member; member = member->bucket_next)
  {
    int arguments_key;
    if ((class_template ? member->class_tok != member_owner_tok
                        : member->lookup_class_tok != member_owner_tok)
        || template_member_def_method_tok(member) != method_tok
        || !template_member_definition_is_static(member)
        || !template_member_is_call_specialized(member)) continue;
    if (!address && !template_member_has_variadic_pack(member)
        && (argument_count < template_member_def_min_param_count(member, method_tok)
            || argument_count > template_member_def_param_count(member, method_tok)))
      continue;
    arguments_key = key ? key : deduce_member_template_argument_key(member, class_tok,
                                      argument_types, argument_count);
    if (arguments_key <= 0) continue;
    ++materializing_template_interface;
    instantiate_template_member_if_needed(member, class_argument, class_tok, arguments_key);
    --materializing_template_interface;
    {
      int candidate = template_member_lookup_inst(member, class_argument,
                                                  arguments_key, class_tok);
      if (candidate > 1)
      {
        if (candidate_count == 64)
          cprime_error("too many static member-template candidates");
        candidate_defs[candidate_count] = member;
        candidate_keys[candidate_count] = arguments_key;
        candidates[candidate_count++] = candidate;
      }
    }
  }
  nb_template_member_call_arg_types = saved_count;
  memcpy(template_member_call_arg_types, saved_types, saved_count * sizeof(CType));
  base_tok = make_static_member_tok(class_tok, method_tok);
  selected = address
    ? (candidate_count == 1 ? global_symbol_find(candidates[0]) : NULL)
    : resolve_free_func_by_arg_types_filtered(base_tok, argument_types,
                     argument_count, 0, candidates, candidate_count, NULL);
  for (i = 0; i < argument_count; ++i) tok_str_free(saved_arguments[i]);
  if (!selected) cprime_error("no matching static member-template call '%s'",
                              get_tok_str(base_tok, NULL));
  for (i = 0; i < candidate_count; ++i)
    if (candidates[i] == selected->v && candidate_defs[i]
        && template_member_has_body(candidate_defs[i]))
    {
      instantiate_template_member_body_if_needed(candidate_defs[i], class_argument,
                                                  class_tok, candidate_keys[i]);
      break;
    }
  return selected->v;
}

static int register_materialized_member_template_interfaces(int class_tok,
                                                              int method_tok)
{
  TemplateDef *owner = find_class_template_def_for_class_tok(class_tok);
  int pass, valid_last = 0;
  for (pass = 0; pass < (owner ? 2 : 1); ++pass)
  {
    int lookup_tok = pass ? owner->name_tok : class_tok;
    TemplateMemberDef *member;
    for (member = template_member_candidates(lookup_tok, method_tok);
         member; member = member->bucket_next)
    {
      int i;
      if (template_member_def_method_tok(member) != method_tok) continue;
      for (i = 0; i < member->nb_inst; ++i)
      {
        Sym *function;
        int function_tok = member->inst_func_toks[i];
        if (!function_tok || member->inst_owner_toks[i] != class_tok) continue;
        if (function_tok == last_instantiated_member_func_tok) valid_last = function_tok;
        function = global_symbol_find(function_tok);
        if (!function) function = sym_find(function_tok);
        if (function && (function->type.t & VT_BTYPE) == VT_FUNC)
          note_member_func_overload(class_tok, method_tok, function_tok,
                                     &function->type);
      }
    }
  }
  return valid_last;
}

/* Conversion templates deduce from the destination type. Express that
   deduction as a single synthetic function parameter, sharing the ordinary
   structural deduction and template-parameter validation machinery. */
static void instantiate_cpp_conversion_templates(int class_tok, CType *target)
{
  TemplateInstOwner *owner = find_template_inst_owner_entry(class_tok);
  int count = nb_template_member_defs;
  TemplateMemberDef *direct = template_member_owner_buckets[
    (unsigned)class_tok & (MEMBER_CANDIDATE_BUCKETS - 1)];
  TemplateMemberDef *primary = owner ? template_member_owner_buckets[
    (unsigned)owner->owner->name_tok & (MEMBER_CANDIDATE_BUCKETS - 1)] : NULL;
  /* Merge owner buckets in declaration order. A shared bucket must be visited
     once, and declarations created during deduction belong to the next call. */
  if (primary == direct) primary = NULL;
  while (direct || primary) {
    TemplateMemberDef *member, view;
    if (!primary || (direct && direct->definition_index < primary->definition_index)) {
      member = direct; direct = direct->owner_next;
    } else {
      member = primary; primary = primary->owner_next;
    }
    if (member->definition_index >= count) continue;
    TokenString *source = member->def_str, *signature;
    int at, start = 0, end, header_end = 0, depth = 0, key, fake_name;
    if (member->instantiating || !source
        || (member->class_tok != class_tok
            && (!owner || member->class_tok != owner->owner->name_tok))
        || !template_member_is_call_specialized(member)
        || !template_member_has_body(member)) continue;
    for (at = 0; at < source->len; ++at) {
      int token = source->str[at];
      if (TOK_HAS_VALUE(token)) {
        at += tok_str_value_extra_words(source->str, source->len, at);
        continue;
      }
      if (token == TOK_LT) ++depth;
      if (token == TOK_GT && depth && !--depth) header_end = at + 1;
      if (token == '{') break;
      if (token == TOK_OPERATOR) { start = at + 1; break; }
    }
    if (!start) continue;
    start = tok_str_next_non_linenum_index(source->str, source->len, start);
    if (start >= source->len || cpp_operator_name_from_tok(source->str[start])
        || source->str[start] == '(' || source->str[start] == '[') continue;
    for (end = start; end < source->len && source->str[end] != '('; ++end) {
      if (TOK_HAS_VALUE(source->str[end]))
        end += tok_str_value_extra_words(source->str, source->len, end);
    }
    if (end == source->len) continue;
    for (at = start; at + 2 < end; ++at)
      if (source->str[at] == ':' && source->str[at + 1] == ':'
          && source->str[at + 2] == '*' && !find_cpp_member_pointer_type(target)) break;
    if (at + 2 < end) continue;
    signature = tok_str_alloc();
    for (at = 0; at < header_end; ++at)
      tok_str_add_record(signature, source->str, source->len, &at);
    fake_name = tok_alloc_const("__cpc_conversion_deduction");
    tok_str_add(signature, TOK_VOID); tok_str_add(signature, fake_name);
    tok_str_add(signature, '(');
    for (at = start; at < end; ++at)
      tok_str_add_record(signature, source->str, source->len, &at);
    tok_str_add(signature, ')'); tok_str_add(signature, ';');
    tok_str_add(signature, TOK_EOF);
    view = *member;
    view.def_str = signature; view.method_tok = fake_name;
    view.method_tok_known = 1; view.syntax_known = 0;
    key = deduce_member_template_argument_key(&view, class_tok, target, 1);
    tok_str_free(signature);
    if (key > 0)
      instantiate_template_member_if_needed(member,
        owner && owner->owner->nb_type_params
          ? owner->owner->inst_type_toks[owner->inst_index * owner->owner->nb_type_params]
          : 0, class_tok, key);
  }
}

static void instantiate_template_member_for_call(CType *type, int method_tok,
                                                 CType *arg_types,
                                                 int explicit_arg_count)
{
  int i, j, struct_tok, type_tok = 0;
  int entry_tok = tok;
  CValue entry_tokc = tokc;
  TokenString *entry_macro_stack = macro_stack;
  const int *entry_macro_ptr = macro_ptr;
  int saved_nb_pending_template_specs;
  int saved_nb_pending_member_funcs;
  int saved_nb_call_arg_types;
  int selected_member_func_tok;
  CType saved_call_arg_types[CPC_MAX_CALL_ARGUMENTS];

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return;
  saved_nb_call_arg_types = nb_template_member_call_arg_types;
  memcpy(saved_call_arg_types, template_member_call_arg_types,
         saved_nb_call_arg_types * sizeof(CType));
  nb_template_member_call_arg_types = 0;
  if (arg_types && explicit_arg_count > 0)
  {
    nb_template_member_call_arg_types = explicit_arg_count < CPC_MAX_CALL_ARGUMENTS
                                          ? explicit_arg_count : CPC_MAX_CALL_ARGUMENTS;
    memcpy(template_member_call_arg_types, arg_types,
           nb_template_member_call_arg_types * sizeof(CType));
  }
  /* Allow nested member instantiation from template member bodies so
     helper members (TryReserve/Realloc/Move) are emitted too.  The
     pending-member machinery defers compilation to a safe point. */
  if (class_has_concrete_member_template_declaration(struct_tok, method_tok))
    goto concrete_member_templates;
  {
    TemplateInstOwner *owner = find_template_inst_owner_entry(struct_tok);
    TemplateDef *td;
    if (!owner || !owner->owner->is_class)
      goto concrete_member_templates;
    td = owner->owner;
    if (td->inst_states
        && (td->inst_states[owner->inst_index] & 64))
      goto concrete_member_templates;
    type_tok = td->inst_type_toks[owner->inst_index * td->nb_type_params];
    if (!type_tok)
      goto concrete_member_templates;
    saved_nb_pending_template_specs = nb_pending_template_specs;
    saved_nb_pending_member_funcs = nb_pending_member_funcs;
    last_instantiated_member_func_tok = 0;
    if (method_tok == tok_alloc_const("operator=")
        && arg_types && explicit_arg_count == 1
        && btype_is_arithmetic_scalar(arg_types[0].t & VT_BTYPE))
    {
      int scalar_arg_tok = template_type_tok_from_ctype(&arg_types[0]);
      {
      TemplateMemberDef *md;
      for (md = template_member_candidates(td->name_tok, method_tok);
           md; md = md->bucket_next)
      {
        if (profile_scans_enabled)
          ++profile_template_member_scans;
        if (md->class_tok == td->name_tok
            && template_member_def_method_tok(md) == method_tok
            && !(template_member_def_first_param_is_array_ref(md, method_tok)
                 && !(arg_types[0].t & VT_ARRAY))
            && template_member_def_is_scoped(md)
            && template_member_has_body(md))
        {
          instantiate_template_member_if_needed(md, type_tok, struct_tok,
                                                     scalar_arg_tok);
        }
      }
      }
    }
    {
      /* Only instantiate the overloads whose declared signatures actually
         accept the call's argument types.  Arity alone is not enough:
         e.g. Erase(c--) (i64) must not instantiate Erase(List<i64>&). */
      int compatible_counts[16], nb_compatible = 0;
      int prefer_class_first_param = 0;
      int has_preexisting_exact_match = 0;
      MemberFuncOverload *o2;
      if (arg_types && explicit_arg_count >= 0)
        for (o2 = member_func_candidates(struct_tok, method_tok);
             o2; o2 = o2->bucket_next)
        {
          Sym *os;
          if (o2->struct_tok != struct_tok || o2->method_tok != method_tok
              || explicit_arg_count < o2->min_arg_count
              || explicit_arg_count > o2->explicit_arg_count)
            continue;
          os = global_symbol_find(o2->mangled_tok);
          if (!os)
            os = sym_find(o2->mangled_tok);
          if (!os)
            continue;
          use_overload_func_type(os, &o2->func_type);
          if (member_func_matches_arg_types(os, arg_types,
                                            explicit_arg_count))
          {
            if (!member_function_is_template(os->v)
                && member_func_exact_arg_types(os, arg_types,
                                            explicit_arg_count))
              has_preexisting_exact_match = 1;
            if (nb_compatible < 16)
              compatible_counts[nb_compatible++] = o2->explicit_arg_count;
          }
        }
      if (arg_types && explicit_arg_count > 0
          && (arg_types[0].t & VT_BTYPE) == VT_STRUCT)
        {
        TemplateMemberDef *md;
        for (md = template_member_candidates(td->name_tok, method_tok);
             md; md = md->bucket_next)
        {
          int param_count;
          if (profile_scans_enabled)
            ++profile_template_member_scans;
          if (md->class_tok != td->name_tok
              || template_member_def_method_tok(md) != method_tok
              || !template_member_def_is_scoped(md))
            continue;
          if (template_member_def_first_param_is_array_ref(md, method_tok)
              && !(arg_types[0].t & VT_ARRAY))
            continue;
          param_count = template_member_def_param_count(md, method_tok);
          if (explicit_arg_count >= template_member_def_min_param_count(
                                      md, method_tok)
              && explicit_arg_count <= param_count
              && template_member_def_first_param_class_kind(md, method_tok) == 1)
          {
            prefer_class_first_param = 1;
            break;
          }
        }
        }
    {
    TemplateMemberDef *md;
    for (md = template_member_candidates(td->name_tok, method_tok);
         md; md = md->bucket_next)
    {
      int member_type_arg_tok = explicit_member_template_arg_tok;
      if (!arg_types && !explicit_member_template_arg_tok
          && template_member_is_call_specialized(md))
        continue;
      if (profile_scans_enabled)
        ++profile_template_member_scans;
      if (md->class_tok == td->name_tok
          && template_member_def_method_tok(md) == method_tok)
      {
        int receiver_const = member_receiver_is_const(type);
        int md_is_const = template_member_def_is_const_qualified(md,
                                                                 method_tok);
        if (receiver_const && !md_is_const)
          continue;
        if (!receiver_const && md_is_const)
        {
          TemplateMemberDef *alt;
          int md_param_count = template_member_def_param_count(md, method_tok);
          int has_nonconst_alt = 0;
          for (alt = template_member_candidates(td->name_tok, method_tok);
               alt; alt = alt->bucket_next)
            if (alt != md && alt->class_tok == td->name_tok
                && template_member_def_method_tok(alt) == method_tok
                && !template_member_def_is_const_qualified(alt, method_tok)
                && template_member_def_param_count(alt, method_tok)
                     == md_param_count)
            {
              has_nonconst_alt = 1;
              break;
            }
          if (has_nonconst_alt)
            continue;
        }
        if (explicit_arg_count < 0
            && template_member_is_call_specialized(md))
          continue;
        /* C++ prefers an otherwise-equivalent non-template member over a
           member-function-template specialization.  Avoid materializing the
           specialization at all when a concrete exact match was already
           published; doing so would also generate the same lowered symbol. */
        if (explicit_arg_count >= 0 && !explicit_member_template_arg_tok
            && has_preexisting_exact_match
            && template_member_is_call_specialized(md))
          continue;
        /* A call asks for executable code, not merely the declaration that
           introduced the overload during the interface prepass.  Keep
           scanning so an out-of-class definition of the same member wins. */
        if ((explicit_arg_count >= 0 || is_lifecycle_member_tok(method_tok))
            && !template_member_is_call_specialized(md)
            && !template_member_has_body(md))
          continue;
        /* Reject dependent array-reference overloads before any signature
           parsing.  Their bounds (template<i64 N>) are intentionally
           unresolved until an array call supplies N; parsing them while
           considering a non-array overload candidate diagnoses bare N. */
        if (arg_types && explicit_arg_count > 0
            && template_member_def_first_param_is_array_ref(md, method_tok)
            && !(arg_types[0].t & VT_ARRAY))
          continue;
        /* Most inline member templates are emitted during class layout.
           Dependent auto-return operators still need a call-specific replay
           so the concrete return type is available before postfix lookup.
           Converting constructor templates also require call-specific replay:
           their member template argument comes from the source class, and a
           class-layout replay cannot know or emit that specialization. */
        if (!template_member_def_is_scoped(md)
            && !template_member_is_call_specialized(md)
            && !(method_tok == TOK_CONSTRUCTOR1
                 && arg_types && explicit_arg_count == 1
                 && template_member_is_same_family_conversion_ctor(md)
                 && is_same_template_family_conversion_ctor(type,
                                                            &arg_types[0]))
            && !(token_string_is_auto_type_token_prefix(md->def_str)
                 && !strncmp(get_tok_str(method_tok, NULL), "operator", 8)))
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
            for (o = member_func_candidates(struct_tok, method_tok);
                 o; o = o->bucket_next)
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
            /* Defaults on another overload do not make a member-template
               candidate viable. Only an out-of-class non-template definition
               can inherit defaults from its previously declared interface. */
            if ((!overload_ok || template_member_is_call_specialized(md))
                && !template_member_has_variadic_pack(md))
            {
              int min_param_count =
                template_member_def_min_param_count(md, method_tok);
              if (explicit_arg_count < min_param_count
                  || explicit_arg_count > param_count)
                continue;
            }
          }
          if (param_count < 0 && explicit_arg_count == 0
              && !template_member_operator_has_no_params(md, method_tok))
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
             instantiate Erase(List<i64>&)).  Generic template parameters
             and scalar signatures stay untouched. */
          if (first_kind == 1 && !arg_is_struct)
            continue;
          if (!template_member_is_call_specialized(md)
              && method_tok == TOK_CONSTRUCTOR1 && arg_is_struct
              && first_kind == 0)
            continue;
          if (!template_member_is_call_specialized(md)
              && prefer_class_first_param && arg_is_struct && first_kind != 1)
            continue;
          /* The substituted signature determines compatibility. A raw parameter
             spelling may name a global class hidden by a template parameter. */
        }
        if (method_tok == TOK_CONSTRUCTOR1 && arg_types
            && explicit_arg_count == 1
            && template_member_is_same_family_conversion_ctor(md)
            && !is_same_template_family_conversion_ctor(type, &arg_types[0]))
          continue;
        if (method_tok == TOK_CONSTRUCTOR1 && arg_types
            && explicit_arg_count == 1
            && (arg_types[0].t & VT_BTYPE) == VT_STRUCT
            && get_struct_type_name_tok(&arg_types[0]) == struct_tok)
        {
          /* A constructor template is not a copy constructor.  Do not
             instantiate it as the body for same-class initialization; the
             implicit copy path must copy the base/member subobjects instead.
             This is especially important for signatures such as
             Derived(const Base<U>&), where substituting U=Derived produces
             a syntactically valid but non-viable Base<Derived> overload. */
          continue;
        }
        if (arg_types && explicit_arg_count > 0)
        {
          int first_kind =
            template_member_def_first_param_class_kind(md, method_tok);
          const char *method_name = get_tok_str(method_tok, NULL);
          if (!member_type_arg_tok
              && !(first_kind != 1 && !strcmp(method_name, "Read")))
          {
            int member_param_tok = template_member_type_param_tok(md);
            int first_param_tok =
              template_member_def_first_param_type_tok(md, method_tok);
            /* `template<class R> f(R&)` deduces R as the complete argument
               type.  Only a pattern such as Wrapper<R>& deduces the first
               template argument of the receiver type. */
            if (member_param_tok && first_param_tok == member_param_tok)
              member_type_arg_tok =
                template_type_tok_from_ctype(&arg_types[0]);
            else
              member_type_arg_tok =
                first_template_arg_tok_from_inst_type(&arg_types[0]);
          }
          if (!member_type_arg_tok)
            member_type_arg_tok = template_type_tok_from_ctype(&arg_types[0]);
          if (!explicit_member_template_arg_tok
              && template_member_def_first_param_decl_kind(md, method_tok) == 1
              && first_kind != 1
              && (arg_types[0].t & VT_BTYPE) == VT_PTR)
          {
            CType pointee = *pointed_type(&arg_types[0]);
            int pointee_tok = template_type_tok_from_ctype(&pointee);
            if (pointee_tok)
              member_type_arg_tok = pointee_tok;
          }
          /* Inference resolves typedef spellings (f32 -> float), which would
             instantiate the member against a duplicate class token such as
             Matrix4x4__float instead of the real Matrix4x4__f32.  When the
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
          if (template_member_has_variadic_pack(md))
          {
            int pack_arg_tok =
              template_member_def_pack_arg_tok(md, method_tok, arg_types,
                                               explicit_arg_count);
            if (pack_arg_tok)
              member_type_arg_tok = pack_arg_tok;
            else
              member_type_arg_tok = -1; /* variadic pack with zero elements */
          }
          if (!explicit_member_template_arg_tok
              && template_member_def_first_param_is_array_ref(md, method_tok))
          {
            int array_key = deduce_member_template_argument_key(md, struct_tok, arg_types,
                                                                explicit_arg_count);
            if (array_key <= 0) continue;
            member_type_arg_tok = array_key;
          }
          else if (!explicit_member_template_arg_tok
                   && !template_member_is_call_specialized(md)
                   && !template_member_has_variadic_pack(md))
          {
            int first_matches = template_member_first_param_matches_arg(
              md, method_tok, struct_tok, type_tok,
              member_type_arg_tok, &arg_types[0]);
            if (!first_matches)
              continue;
          }
        }
        {
          int deduced_key = deduce_member_template_argument_key(md, struct_tok, arg_types, explicit_arg_count);
      if (deduced_key < 0) continue;
          if (deduced_key) member_type_arg_tok = deduced_key;
          else if (!explicit_member_template_arg_tok
                   && template_member_value_param_tok(md)
                   && token_string_signature_has_array_brackets(md->def_str))
            continue;
        }
        /* Conversion ranking chooses the body to instantiate. Candidate
           discovery publishes only the parameter/return interface. */
        instantiate_template_member_if_needed(md, type_tok, struct_tok,
                                              member_type_arg_tok);
      }
    }
    }
    }
    /* Nested specialization compilation may instantiate unrelated members
       and overwrite the shared last-result token.  Capture this call's
       interface before draining that worklist so overload registration keeps
       the receiver/name paired with the correct concrete function. */
    selected_member_func_tok = last_instantiated_member_func_tok;
    if (nb_pending_template_specs != saved_nb_pending_template_specs)
      compile_pending_template_specs_without_member_flush();
    last_instantiated_member_func_tok = selected_member_func_tok;
    last_instantiated_member_func_tok =
      register_materialized_member_template_interfaces(struct_tok, method_tok);
    if (saved_nb_pending_member_funcs != nb_pending_member_funcs)
      compile_pending_member_funcs(saved_nb_pending_member_funcs);
    nb_template_member_call_arg_types = saved_nb_call_arg_types;
    memcpy(template_member_call_arg_types, saved_call_arg_types,
           saved_nb_call_arg_types * sizeof(CType));
    tok = entry_tok;
    tokc = entry_tokc;
    if (macro_stack == entry_macro_stack)
      macro_ptr = entry_macro_ptr;
    return;
  }
concrete_member_templates:
  saved_nb_pending_template_specs = nb_pending_template_specs;
  saved_nb_pending_member_funcs = nb_pending_member_funcs;
  last_instantiated_member_func_tok = 0;
  {
  int has_preexisting_exact_match = 0;
  int has_preexisting_void_pointer_match = 0;
  MemberFuncOverload *o;
  if (arg_types && explicit_arg_count >= 0)
    for (o = member_func_candidates(struct_tok, method_tok);
         o; o = o->bucket_next)
    {
      Sym *os;
      if (o->struct_tok != struct_tok || o->method_tok != method_tok
          || explicit_arg_count < o->min_arg_count
          || explicit_arg_count > o->explicit_arg_count)
        continue;
      os = global_symbol_find(o->mangled_tok);
      if (!os)
        os = sym_find(o->mangled_tok);
      if (os)
        use_overload_func_type(os, &o->func_type);
      if (os && member_func_arg_match_rank(os, arg_types,
                                           explicit_arg_count) >= 0)
      {
        Sym *pa = os->type.ref ? os->type.ref->next : NULL;
        CType actual = arg_types[0];
        if (pa) pa = pa->next; /* implicit this */
        actual.t &= ~(VT_REFERENCE | VT_RVALUE_REFERENCE);
        if (pa && (pa->type.t & VT_BTYPE) == VT_PTR
            && (actual.t & VT_BTYPE) == VT_PTR
            && (pointed_type(&pa->type)->t & VT_BTYPE) == VT_VOID
            && (pointed_type(&actual)->t & VT_BTYPE) == VT_VOID)
          has_preexisting_void_pointer_match = 1;
      }
      if (os && !member_function_is_template(os->v)
          && member_func_exact_arg_types(os, arg_types, explicit_arg_count))
      {
        has_preexisting_exact_match = 1;
        break;
      }
    }
  TemplateMemberDef *md;
  for (md = template_member_lookup_candidates(struct_tok, method_tok);
       md; md = md->lookup_next)
  {
    int member_type_arg_tok = explicit_member_template_arg_tok;
    int param_count;
    int interface_only_for_concrete_void_pointer = 0;
    if (!arg_types && !explicit_member_template_arg_tok
        && template_member_is_call_specialized(md))
      continue;
    if (md->lookup_class_tok != struct_tok
        || template_member_def_method_tok(md) != method_tok)
      continue;
    /* Name-only lookup has no call arguments from which to deduce a member
       function template.  Its concrete specialization belongs to the later
       typed-call path, not the interface prepass. */
    if (explicit_arg_count < 0
        && template_member_is_call_specialized(md))
      continue;
    if (explicit_arg_count >= 0 && !explicit_member_template_arg_tok
        && has_preexisting_exact_match
        && template_member_is_call_specialized(md))
      continue;
    if (explicit_arg_count >= 0 && has_preexisting_void_pointer_match
        && template_member_is_call_specialized(md)
        && template_member_def_first_param_decl_kind(md, method_tok) == 1)
      interface_only_for_concrete_void_pointer = 1;
    if (explicit_arg_count >= 0
        && !template_member_is_call_specialized(md)
        && !template_member_has_body(md))
      continue;
    param_count = template_member_def_param_count(md, method_tok);
    if (explicit_arg_count >= 0
        && param_count >= 0
        && !template_member_has_variadic_pack(md))
    {
      int min_param_count = template_member_def_min_param_count(md,
                                                                method_tok);
      if (explicit_arg_count < min_param_count
          || explicit_arg_count > param_count)
        continue;
    }
    if (arg_types && explicit_arg_count > 0)
    {
      if (!member_type_arg_tok)
        member_type_arg_tok = template_type_tok_from_ctype(&arg_types[0]);
      if (!explicit_member_template_arg_tok
          && template_member_def_first_param_decl_kind(md, method_tok) == 1
          && template_member_def_first_param_class_kind(md, method_tok) != 1
          && (arg_types[0].t & VT_BTYPE) == VT_PTR)
      {
        CType pointee = *pointed_type(&arg_types[0]);
        int pointee_tok = template_type_tok_from_ctype(&pointee);
        if (pointee_tok)
          member_type_arg_tok = pointee_tok;
      }
    }
    if (template_member_is_call_specialized(md)
        && explicit_arg_count > 0
        && !member_type_arg_tok)
      continue;
    {
      int deduced_key = deduce_member_template_argument_key(md, struct_tok, arg_types, explicit_arg_count);
      if (deduced_key < 0) continue;
          if (deduced_key) member_type_arg_tok = deduced_key;
      else if (!explicit_member_template_arg_tok
               && template_member_has_variadic_pack(md))
      {
        int pack_key = template_member_def_pack_arg_tok(md, method_tok,
                                                        arg_types, explicit_arg_count);
        member_type_arg_tok = pack_key ? pack_key : -1;
      }
      else if (!explicit_member_template_arg_tok
               && template_member_value_param_tok(md)
               && token_string_signature_has_array_brackets(md->def_str))
        continue;
    }
    if (interface_only_for_concrete_void_pointer)
    {
      materializing_template_interface++;
      instantiate_template_member_if_needed(md, 0, struct_tok,
                                            member_type_arg_tok);
      materializing_template_interface--;
    }
    else
      instantiate_template_member_if_needed(md, 0, struct_tok,
                                                 member_type_arg_tok);
  }
  }
  selected_member_func_tok = last_instantiated_member_func_tok;
  if (nb_pending_template_specs != saved_nb_pending_template_specs)
    compile_pending_template_specs_without_member_flush();
  last_instantiated_member_func_tok = selected_member_func_tok;
  last_instantiated_member_func_tok =
    register_materialized_member_template_interfaces(struct_tok, method_tok);
  if (saved_nb_pending_member_funcs != nb_pending_member_funcs)
    compile_pending_member_funcs(saved_nb_pending_member_funcs);
  nb_template_member_call_arg_types = saved_nb_call_arg_types;
  memcpy(template_member_call_arg_types, saved_call_arg_types,
         saved_nb_call_arg_types * sizeof(CType));
  tok = entry_tok;
  tokc = entry_tokc;
  if (macro_stack == entry_macro_stack)
    macro_ptr = entry_macro_ptr;
}

static TokenString *merge_template_function_defaults(TokenString *definition,
                                                    TokenString *prior, int name);

static void add_template_member_def(int class_tok, int type_param_tok,
                                     int *stripped_member_type_param_toks,
                                     int nb_stripped_member_type_params,
                                     int *stripped_member_value_param_toks,
                                     int nb_stripped_member_value_params,
                                     int *ordered_params, int nb_ordered_params,
                                     unsigned ordered_value_mask,
                                     TokenString *str)
{
  int i;
  TemplateDef *td;
  TemplateMemberDef *md;

  md = cprime_mallocz(sizeof(*md));
  md->class_tok = class_tok;
  {
    CppMemberDeclContext *context;
    for (context = cpp_member_decl_context; context; context = context->previous)
      if (context->owner_tok == class_tok) {
        md->member_access = context->access;
        md->member_access_known = 1;
        break;
      }
  }
  md->lookup_class_tok = find_current_namespace_tok(class_tok);
  md->type_param_tok = type_param_tok;
  md->nb_stripped_member_params = nb_ordered_params;
  md->stripped_member_value_mask = ordered_value_mask;
  for (i = 0; i < nb_ordered_params && i < 16; ++i)
    md->stripped_member_param_toks[i] = ordered_params[i];
  if (nb_stripped_member_type_params > 16)
    nb_stripped_member_type_params = 16;
  md->nb_stripped_member_type_params = nb_stripped_member_type_params;
  for (i = 0; i < nb_stripped_member_type_params; ++i)
    md->stripped_member_type_param_toks[i] =
      stripped_member_type_param_toks[i];
  if (nb_stripped_member_value_params > 16)
    nb_stripped_member_value_params = 16;
  md->nb_stripped_member_value_params = nb_stripped_member_value_params;
  for (i = 0; i < nb_stripped_member_value_params; ++i)
    md->stripped_member_value_param_toks[i] =
      stripped_member_value_param_toks[i];
  prepare_saved_cpp_lambdas(str);
  md->def_str = str;
  for (i = 0; i + 4 < str->len; ++i)
  {
    int j, angle = 0;
    if (str->str[i] != class_tok)
      continue;
    j = i + 1;
    while (j + 1 < str->len && str->str[j] == TOK_LINENUM)
      j += 2;
    if (j < str->len && (str->str[j] == TOK_LT || str->str[j] == '<'))
    {
      angle = 1;
      for (++j; j < str->len && angle > 0; ++j)
      {
        int t = str->str[j];
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
    }
    if (j + 4 < str->len && str->str[j] == ':'
        && str->str[j + 1] == ':' && str->str[j + 2] >= TOK_UIDENT
        && str->str[j + 3] == ':' && str->str[j + 4] == ':')
    {
      md->nested_class_tok = str->str[j + 2];
      break;
    }
    if (i > 0 && (str->str[i - 1] == TOK_CLASS || str->str[i - 1] == TOK_STRUCT)
        && j + 2 < str->len && str->str[j] == ':' && str->str[j + 1] == ':'
        && str->str[j + 2] >= TOK_UIDENT) {
      md->nested_class_template_tok = str->str[j + 2];
      break;
    }
  }
  md->method_tok = template_member_def_method_tok(md);
  md->method_tok_known = 1;
  if (md->nb_stripped_member_params && template_member_has_body(md)) {
    TemplateMemberDef *prior;
    for (prior = template_member_candidates(class_tok, md->method_tok);
         prior; prior = prior->bucket_next) {
      TokenString *normalized;
      int at, parameter;
      if (prior->class_tok != class_tok || template_member_has_body(prior)
          || template_member_def_method_tok(prior) != md->method_tok
          || prior->nb_stripped_member_params != md->nb_stripped_member_params
          || prior->stripped_member_value_mask != md->stripped_member_value_mask
          || template_member_def_is_const_qualified(prior, md->method_tok)
             != template_member_def_is_const_qualified(md, md->method_tok))
        continue;
      normalized = tok_str_alloc();
      for (at = 0; at < str->len; ++at) {
        int token = str->str[at];
        if (TOK_HAS_VALUE(token)) {
          tok_str_add_record(normalized, str->str, str->len, &at);
          continue;
        }
        for (parameter = 0; parameter < md->nb_stripped_member_params; ++parameter)
          if (token == md->stripped_member_param_toks[parameter]) break;
        tok_str_add(normalized, parameter < md->nb_stripped_member_params
            ? prior->stripped_member_param_toks[parameter] : token);
      }
      if (template_function_merge_signature_hash(prior->def_str, md->method_tok)
          == template_function_merge_signature_hash(normalized, md->method_tok)) {
        /* Keep the forward declaration's specialization cache and demand
           records. The final body drain can now complete those same symbols. */
        normalized = merge_template_function_defaults(normalized, prior->def_str,
                                                       md->method_tok);
        tok_str_free(prior->def_str);
        prior->def_str = normalized;
        prior->syntax_known = 0;
        tok_str_free(str);
        cprime_free(md);
        return;
      }
      tok_str_free(normalized);
    }
  }
  if (md->method_tok == TOK_CONSTRUCTOR1
      && template_member_is_call_specialized(md))
  {
    Sym *owner = struct_find(class_tok);
    if (owner)
      owner->a.cpp_user_constructor = 1;
  }
  dynarray_add(&template_member_defs, &nb_template_member_defs, md);
  index_template_member(md);

  td = find_template_def(md->lookup_class_tok);
  if (!td || !td->is_class)
    return;
  for (i = 0; i < td->nb_inst; ++i)
  {
    int method_tok = template_member_def_method_tok(md);
    Sym *owner = struct_find(td->inst_name_toks[i]);
    if (td->inst_states && (td->inst_states[i] & 64))
      continue;
    /* A pointer/reference may have requested only a forward specialization.
       Its member aliases and declarations become available when the class
       definition is instantiated; publish interfaces after that point. */
    if (!owner || owner->c < 0)
      continue;
    if (md->nested_class_template_tok || (template_member_def_is_scoped(md)
        && !template_member_is_call_specialized(md)
        && !token_string_is_auto_type_token_prefix(md->def_str)))
    {
      materializing_template_interface++;
      instantiate_template_member_if_needed(md,
                                            td->inst_type_toks[i * td->nb_type_params],
                                            td->inst_name_toks[i], 0);
      materializing_template_interface--;
    }
  }
}

static int save_template_parameter_default(TokenString **default_str, int type_default)
{
  int angle = 0, paren = 0, bracket = 0;
  int previous = 0;
  TokenString *str;

  if (tok != '=')
    return 0;
  str = tok_str_alloc();
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
    else if (tok == TOK_LT && !paren && !bracket
             && (type_default || angle || find_template_def(previous)
                 || find_template_def(find_current_namespace_tok(previous))))
      ++angle;
    else if (tok == TOK_GT && angle > 0 && !paren && !bracket)
      --angle;
    else if (tok == TOK_SAR && angle > 0 && !paren && !bracket)
    {
      if (angle == 1)
      {
        /* One close belongs to the default; leave the second close for
           the enclosing template parameter list. */
        tok_str_add(str, TOK_GT);
        tok = TOK_GT;
        break;
      }
      angle -= 2;
    }
    else if (!angle && !paren && !bracket && (tok == ',' || tok == TOK_GT))
      break;
    tok_str_add_tok(str);
    previous = tok;
    next();
  }
  tok_str_add(str, TOK_EOF);
  *default_str = str;
  return 1;
}

static int parse_template_type_parameter_tok(int index, int *is_variadic,
                                             int *has_default,
                                             TokenString **default_str)
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
  *has_default = save_template_parameter_default(default_str, 1);
  return param_tok;
}

/* typename introduces either a type parameter or a dependent type in a
   value parameter declaration. Inspect its grammar before consuming it. */
static int template_parameter_is_type(void)
{
  TokenString *lookahead;
  int result;
  if (tok == TOK_CLASS) return 1;
  if (tok < TOK_UIDENT || strcmp(get_tok_str(tok, NULL), "typename")) return 0;
  lookahead = tok_str_alloc();
  tok_str_add_tok(lookahead); next();
  if (tok == TOK_DOTS) { tok_str_add_tok(lookahead); next(); }
  if (tok >= TOK_UIDENT) { tok_str_add_tok(lookahead); next(); }
  result = tok == ',' || tok == TOK_GT || tok == TOK_SAR || tok == '=';
  restore_cpp_lifecycle_probe(lookahead);
  return result;
}

static int parse_template_non_type_parameter_tok(int parameter_index,
                            int *has_default, TokenString **default_str,
                            TokenString **type_str)
{
  TokenString *declaration = tok_str_alloc();
  int paren = 0, angle = 0, bracket = 0, previous = 0;
  int param_tok = 0, name_index = -1;
  char name[64];
  *has_default = 0;
  while (tok != TOK_EOF) {
    if (!paren && !angle && !bracket
        && (tok == ',' || tok == TOK_GT || tok == TOK_SAR || tok == '=')) break;
    if (!angle && !bracket && (!paren || previous == '*' || previous == '&')
        && tok >= TOK_UIDENT && previous != ':') {
      param_tok = tok;
      name_index = declaration->len;
    }
    if (tok == '(') ++paren;
    else if (tok == ')' && paren) --paren;
    else if (tok == '[') ++bracket;
    else if (tok == ']' && bracket) --bracket;
    else if (tok == TOK_LT && !paren && !bracket) ++angle;
    else if (tok == TOK_GT && angle && !paren && !bracket) --angle;
    else if (tok == TOK_SAR && angle && !paren && !bracket) {
      if (angle == 1) { tok = TOK_GT; unget_tok(TOK_GT); }
      angle = angle > 1 ? angle - 2 : 0;
    }
    previous = tok;
    tok_str_add_tok(declaration); next();
  }
  /* A qualified terminal type name is not an unnamed parameter's name. */
  if (previous == ':' || (previous >= TOK_UIDENT
      && (name_index < 0 || declaration->str[declaration->len-1] != param_tok))
      || name_index == 0 || (name_index >= 0 && name_index + 1 < declaration->len
                            && declaration->str[name_index + 1] == ':')) name_index = -1;
  if (name_index >= 0) {
    memmove(declaration->str+name_index, declaration->str+name_index+1,
              (declaration->len-name_index-1)*sizeof(int));
    --declaration->len;
  } else {
    snprintf(name, sizeof(name), "__cpc_template_value_param_%d", parameter_index);
    param_tok = tok_alloc_const(name);
  }
  tok_str_add(declaration, TOK_EOF);
  if (type_str) *type_str = declaration;
  else tok_str_free(declaration);
  *has_default = save_template_parameter_default(default_str, 0);
  return param_tok;
}

typedef struct TemplateParameterDefault {
  int start, end;
} TemplateParameterDefault;

static int template_parameter_defaults(TokenString *tokens, int name,
                                        TemplateParameterDefault *parameters)
{
  int i, count = 0, paren = 0, bracket = 0, brace = 0, angle = 0, start = -1;
  for (i = 0; i + 1 < tokens->len; ++i) {
    if (tokens->str[i] == name && tokens->str[i + 1] == '(') { i += 2; break; }
    if (TOK_HAS_VALUE(tokens->str[i]))
      i += tok_str_value_extra_words(tokens->str, tokens->len, i);
  }
  for (; i < tokens->len; ++i) {
    int token = tokens->str[i];
    if (!paren && !bracket && !brace && !angle) {
      if (token == '=' && start < 0) start = i;
      if (token == ',' || token == ')') {
        if (count >= CPC_MAX_CALL_ARGUMENTS) cprime_error("too many template function parameters");
        parameters[count].start = start;
        parameters[count++].end = i;
        start = -1;
        if (token == ')') break;
      }
    }
    if (token == '(') ++paren;
    else if (token == ')') --paren;
    else if (token == '[') ++bracket;
    else if (token == ']') --bracket;
    else if (token == '{') ++brace;
    else if (token == '}') --brace;
    else if (!paren && !bracket && !brace) {
      if (token == TOK_LT) ++angle;
      else if (token == TOK_GT && angle) --angle;
      else if (token == TOK_SAR && angle) angle = angle > 1 ? angle - 2 : 0;
    }
    if (TOK_HAS_VALUE(token))
      i += tok_str_value_extra_words(tokens->str, tokens->len, i);
  }
  return count;
}

static TokenString *merge_template_function_defaults(TokenString *definition,
                                                       TokenString *prior, int name)
{
  TemplateParameterDefault old_parameters[CPC_MAX_CALL_ARGUMENTS];
  TemplateParameterDefault new_parameters[CPC_MAX_CALL_ARGUMENTS];
  TokenString *merged;
  int old_count = template_parameter_defaults(prior, name, old_parameters);
  int count = template_parameter_defaults(definition, name, new_parameters);
  int parameter = 0, i, j;
  if (count != old_count) return definition;
  merged = tok_str_alloc();
  for (i = 0; i < definition->len; ++i) {
    if (parameter < count && i == new_parameters[parameter].end) {
      if (new_parameters[parameter].start < 0 && old_parameters[parameter].start >= 0)
        for (j = old_parameters[parameter].start; j < old_parameters[parameter].end; ++j)
          tok_str_add_record(merged, prior->str, prior->len, &j);
      ++parameter;
    }
    tok_str_add_record(merged, definition->str, definition->len, &i);
  }
  tok_str_free(definition);
  return merged;
}

static void complete_function_template_definition(TemplateDef *td,
                                                  TokenString *definition)
{
  definition = merge_template_function_defaults(definition, td->def_str, td->name_tok);
  tok_str_free(td->def_str);
  prepare_saved_cpp_lambdas(definition);
  td->def_str = definition;
  td->has_body = 1;
}

#include "cprimegen_function_specializations.inc"

static int template_declares_static_data(TemplateDef *owner, int member_tok)
{
  int i, braces = 0, parens = 0, is_static = 0;
  TokenString *body = owner->def_str;
  if (!body) return 0;
  for (i = 0; i < body->len; ++i) {
    int token = body->str[i];
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(body->str, body->len, i);
      continue;
    }
    if (token == '{') ++braces;
    else if (token == '}') { --braces; if (braces == 1) is_static = 0; }
    else if (braces == 1) {
      if (token == '(') ++parens;
      else if (token == ')') --parens;
      else if (!parens) {
        if (token == TOK_STATIC) is_static = 1;
        else if (token == ';') is_static = 0;
        else if (is_static && token == member_tok && i + 1 < body->len
                 && body->str[i + 1] != '(')
          return 1;
      }
    }
  }
  return 0;
}

static int register_template_static_data(TokenString *declaration,
                                           int *parameters, int parameter_count)
{
  int i;
  for (i=0;i<declaration->len;++i) {
    int token=declaration->str[i], cursor, argument=0;
    TemplateDef *owner;
    TemplateStaticDataDef *definition;
    if(TOK_HAS_VALUE(token)){i+=tok_str_value_extra_words(declaration->str,declaration->len,i);continue;}
    if(token=='=' || token=='{' || token==';')break;
    if(token<TOK_UIDENT || i+1>=declaration->len || declaration->str[i+1]!=TOK_LT)continue;
    owner=find_class_template_def(find_current_namespace_tok(token));
    if(!owner || !owner->is_class)continue;
    cursor=i+2;
    while(cursor<declaration->len && argument<parameter_count) {
      if(declaration->str[cursor]!=parameters[argument])break;
      ++cursor;++argument;
      if(argument<parameter_count){if(declaration->str[cursor]!=',')break;++cursor;}
    }
    if(argument!=parameter_count || declaration->str[cursor]!=TOK_GT
        || cursor+4>=declaration->len || declaration->str[cursor+1]!=':'
        || declaration->str[cursor+2]!=':' || declaration->str[cursor+3]<TOK_UIDENT
        || (declaration->str[cursor+4]!='=' && declaration->str[cursor+4]!=';'
            && declaration->str[cursor+4]!='[' && declaration->str[cursor+4]!='{'
            && !(declaration->str[cursor+4]=='('
                 && template_declares_static_data(owner, declaration->str[cursor+3]))
            && !(declaration->str[cursor+4]==')' && i > 0
                 && (declaration->str[i-1]=='*' || declaration->str[i-1]=='&'))))continue;
    if(parameter_count!=owner->nb_type_params)
      cprime_error("static data definition template parameters do not match its class");
    for(definition=template_static_data_defs;definition;definition=definition->next)
      if(definition->owner==owner && definition->member_tok==declaration->str[cursor+3])
        cprime_error("duplicate static member template definition");
    definition=cprime_mallocz(sizeof(*definition));
    definition->owner=owner;definition->declaration=declaration;
    memcpy(definition->parameters,parameters,parameter_count*sizeof(int));
    definition->parameter_count=parameter_count;
    definition->class_begin=i;definition->class_end=cursor;
    definition->member_tok=declaration->str[cursor+3];
    definition->namespace_tok=current_namespace_scope_tok();
    definition->next=template_static_data_defs;template_static_data_defs=definition;
    return 1;
  }
  return 0;
}

static int template_template_parameter_kinds(TokenString *signature, int *kinds)
{
  int at, depth = 0, start = 0, count = 0;
  if (!signature || !token_string_starts_template(signature)) return -1;
  for (at = 0; at < signature->len; ++at) {
    int token = signature->str[at];
    if (TOK_HAS_VALUE(token)) {
      at += tok_str_value_extra_words(signature->str, signature->len, at);
      continue;
    }
    if (token == TOK_LT) { if (++depth == 1) start = 1; continue; }
    if (token == TOK_GT || token == TOK_SAR) {
      depth -= token == TOK_SAR ? 2 : 1;
      if (depth <= 0) break;
      continue;
    }
    if (depth != 1) continue;
    if (token == ',') { start = 1; continue; }
    if (start) {
      if (count == 16) return -1;
      kinds[count++] = is_template_keyword_tok(token) ? 2
        : token == TOK_CLASS || (token >= TOK_UIDENT
            && !strcmp(get_tok_str(token, NULL), "typename")) ? 1 : 0;
      start = 0;
    }
  }
  return count;
}

static int template_template_parameter_kinds_differ(TokenString *first, TokenString *second)
{
  int a[16], b[16], na = template_template_parameter_kinds(first, a);
  int nb = template_template_parameter_kinds(second, b), i;
  if (na < 0 || nb < 0) return (na < 0) != (nb < 0);
  if (na != nb) return 1;
  for (i = 0; i < na; ++i) if (a[i] != b[i]) return 1;
  return 0;
}

static TokenString *canonicalize_template_signature_types(TokenString *source,
                                    int *parameters, int count, int qualified_only)
{
  TokenString *result = tok_str_alloc();
  int i, body = 0;
  for (i = 0; i < source->len; ++i) {
    int token = source->str[i];
    TemplateDef *record;
    if (token == '{') body = 1;
    if (!body && token >= TOK_UIDENT && i + 1 < source->len
        && source->str[i + 1] == TOK_LT
        && (record = find_class_template_def(find_current_namespace_tok(token))) != NULL) {
      int end = i + 1, depth = 0, dependent = 0, j;
      for (; end < source->len; ++end) {
        int current = source->str[end];
        if (current == TOK_LT) ++depth;
        else if (current == TOK_GT) --depth;
        else if (current == TOK_SAR) depth -= 2;
        for (j = 0; j < count; ++j) if (current == parameters[j]) dependent = 1;
        if (!depth) break;
        if (TOK_HAS_VALUE(current)) end += tok_str_value_extra_words(source->str, source->len, end);
      }
      if (!dependent && end < source->len
          && (!qualified_only || (end + 2 < source->len
              && source->str[end + 1] == ':' && source->str[end + 2] == ':'))) {
        TokenString *argument_tokens = tok_str_alloc(), *parent = macro_stack;
        const int *parent_ptr = macro_ptr;
        int saved = tok, name;
        CValue saved_value = tokc;
        TemplateArgList arguments;
        for (j = i + 1; j <= end; ++j)
          tok_str_add_record(argument_tokens, source->str, source->len, &j);
        tok_str_add(argument_tokens, TOK_EOF);
        begin_macro(argument_tokens, 1); next();
        parse_template_type_args(&arguments);
        name = instantiate_template_if_needed(record, &arguments);
        while (macro_stack && macro_stack != parent) end_macro();
        macro_ptr = parent_ptr; tok = saved; tokc = saved_value;
        tok_str_add(result, name); i = end; continue;
      }
    }
    tok_str_add_record(result, source->str, source->len, &i);
  }
  tok_str_free(source);
  return result;
}

static void parse_template_decl(void)
{
  int friend_owner = friend_template_declaration_owner;
  int type_param_toks[16], nb_type_params = 0;
  int type_param_is_type[16] = { 0 };
  TokenString *default_arg_strs[16] = { 0 };
  TokenString *value_type_strs[16] = { 0 };
  int nb_required_type_params = 0;
  int variadic_param_index = -1;
  int name_tok = 0, member_class_tok, prev_ident = 0, prev_tok = 0;
  int brace = 0, saw_body = 0, parentheses = 0, brackets = 0;
  int function_try = 0;
  int saw_operator_declarator = 0;
  int is_class = 0, is_union = 0, is_alias = 0;
  const char *kw;
  TemplateDef *td;
  TokenString *str;

  friend_template_declaration_owner = 0;
  next();
  if (tok != TOK_LT && tok != '<') {
    if (tok == TOK_CLASS || tok == TOK_STRUCT || tok == TOK_UNION) {
      CType record;
      TemplateArgList arguments;
      TemplateDef *definition;
      MemberFuncOverload *member;
      int owner, name;
      int explicit_global_scope = 0;
      next();
      if (tok == ':') {
        explicit_global_scope = 1;
        next();
        skip(':');
      }
      if (tok < TOK_UIDENT) expect("class template name");
      if (is_namespace_tok(tok)) {
        int parts[16], nb_parts = 0;
        parts[nb_parts++] = tok;
        next();
        for (;;) {
          if (tok != ':')
            cprime_error("namespace-qualified class template name expected");
          next();
          skip(':');
          if (tok < TOK_UIDENT)
            expect("qualified class template name");
          if (nb_parts >= (int)(sizeof(parts) / sizeof(parts[0])))
            cprime_error("qualified template name too deep");
          parts[nb_parts++] = tok;
          next();
          if (tok != ':' || !is_namespace_tok(parts[nb_parts - 1]))
            break;
        }
        name = find_namespace_tok_from_parts(parts, nb_parts);
      } else {
        name = explicit_global_scope ? tok : find_current_namespace_tok(tok);
        next();
      }
      while (tok == ':') {
        next(); skip(':');
        if (tok < TOK_UIDENT) expect("qualified class template name");
        name = make_static_member_tok(name, tok);
        next();
      }
      definition = find_class_template_def(name);
      if (!definition || !definition->is_class)
        cprime_error("explicit instantiation requires a class template");
      parse_template_type_args(&arguments);
      owner = instantiate_template_if_needed(definition, &arguments);
      compile_pending_template_specs_without_member_flush();
      while (tok == ':') {
        next(); skip(':');
        if (is_template_keyword_tok(tok)) next();
        if (tok < TOK_UIDENT) expect("nested class name in explicit instantiation");
        name = make_static_member_tok(owner, tok);
        next();
        if (tok == TOK_LT || tok == '<') {
          definition = find_class_template_def(name);
          if (!definition || !definition->is_class)
            cprime_error("explicit instantiation requires a nested class template");
          parse_template_type_args(&arguments);
          owner = instantiate_template_if_needed(definition, &arguments);
          compile_pending_template_specs_without_member_flush();
        } else owner = name;
      }
      if (!make_class_type_from_tok(&record, owner) || record.ref->c < 0)
        cprime_error("explicit instantiation requires a complete class template");
      skip(';');
      if (!extern_template_explicit_declaration)
        for (member = member_func_overloads; member; member = member->next) {
          Sym *function;
          if (member->struct_tok != owner) continue;
          instantiate_template_member_body_for_func_tok(&record, member->method_tok,
                                                         member->mangled_tok);
          function = global_symbol_find(member->mangled_tok);
          /* Register the external demand only while the member still has no
             definition. put_extern_sym rewrites an existing entry to
             SHN_UNDEF/value 0, which would erase a body this explicit
             instantiation just materialized and leave calls jumping to
             address 0. */
          if (function && pending_member_func_has_body_tok(member->mangled_tok)
              && (!function->c || elfsym(function)->st_shndx == SHN_UNDEF))
            put_extern_sym(function, NULL, 0, 0);
        }
    } else {
      parse_explicit_function_specialization(1);
    }
    return;
  }
  skip(TOK_LT);
  if (tok == TOK_GT)
  {
    int class_tok, mangled_tok;
    TemplateArgList args;

    next();
    if (is_template_keyword_tok(tok)) {
      parse_template_decl();
      return;
    }
    if (tok != TOK_CLASS && tok != TOK_STRUCT && tok != TOK_UNION)
    {
      parse_explicit_function_specialization(0);
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
      if (tok == '(') ++parentheses;
      else if (tok == ')') --parentheses;
      else if (tok == '[') ++brackets;
      else if (tok == ']') --brackets;
      if (tok == '{')
      {
        if (!parentheses && !brackets && !brace) saw_body = 1;
        ++brace;
      }
      else if (tok == '}')
      {
        if (--brace == 0 && saw_body)
        {
          next();
          break;
        }
      }
      else if (!saw_body && !brace && !parentheses && !brackets && tok == ';')
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
    /* A full specialization declares its own members and never inherits
       out-of-class primary-template member definitions.  Keep that
       distinction on the instance so later definitions skip this entry. */
    td->inst_states[td->nb_inst - 1] |= 64;
    if (macro_stack)
    {
      queue_pending_template_spec_in_namespace(str, args.nb > 0 ? args.toks[0] : 0, td->namespace_tok);
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
    return;
  }
  if (template_parameter_is_type())
  {
    int is_variadic, has_default, param_index = nb_type_params;
    type_param_is_type[param_index] = 1;
    type_param_toks[nb_type_params++] =
      parse_template_type_parameter_tok(param_index, &is_variadic,
                                        &has_default,
                                        &default_arg_strs[param_index]);
    if (!has_default && !is_variadic)
      nb_required_type_params = nb_type_params;
    if (is_variadic)
      variadic_param_index = param_index;
  }
  else
  {
    int param_index = nb_type_params;
    int has_default, param_tok = parse_template_non_type_parameter_tok(
                                   param_index, &has_default,
                                   &default_arg_strs[param_index], &value_type_strs[param_index]);
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
    if (template_parameter_is_type())
    {
      int is_variadic, has_default, param_index = nb_type_params;
      type_param_is_type[param_index] = 1;
      type_param_toks[nb_type_params++] =
        parse_template_type_parameter_tok(param_index, &is_variadic,
                                          &has_default,
                                          &default_arg_strs[param_index]);
      if (!has_default && !is_variadic)
        nb_required_type_params = nb_type_params;
      if (is_variadic)
        variadic_param_index = param_index;
    }
    else
    {
      int param_index = nb_type_params;
      int has_default, param_tok = parse_template_non_type_parameter_tok(
                                     param_index, &has_default,
                                     &default_arg_strs[param_index], &value_type_strs[param_index]);
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
    if (tok == TOK_ATTRIBUTE1 || tok == TOK_ATTRIBUTE2)
    {
      /* Attribute arguments are not a class/function declarator. Preserve
         their tokens for instantiation without mistaking visibility(...) or
         another attribute name for the name of this template. */
      tok_str_add_tok(str);
      next();
      if (tok != '(') expect("'(' after attribute");
      skip_or_save_balanced_tokens(str);
      continue;
    }
    tok_str_add_tok(str);
    if (!is_class && !brace && !parentheses && !brackets && tok == TOK_TRY)
      function_try = 1;
    if (tok == TOK_OPERATOR && !brace && !parentheses && !brackets)
      saw_operator_declarator = 1;
    if (tok == '(') ++parentheses;
    else if (tok == ')') --parentheses;
    else if (tok == '[') ++brackets;
    else if (tok == ']') --brackets;
    if (!name_tok && (prev_tok == TOK_CLASS || prev_tok == TOK_STRUCT
                      || prev_tok == TOK_UNION)
        && tok >= TOK_UIDENT)
    {
      name_tok = tok;
      is_class = 1;
      is_union = prev_tok == TOK_UNION;
    }
    if (!name_tok && prev_tok >= TOK_UIDENT
        && !strcmp(get_tok_str(prev_tok, NULL), "using")
        && tok >= TOK_UIDENT)
    {
      name_tok = tok;
      is_class = 0;
      is_alias = 1;
    }
    if (!name_tok && prev_tok == TOK_OPERATOR)
    {
      int operator_name_tok = get_cpp_binary_operator_method_tok(tok);
      if (operator_name_tok)
        name_tok = operator_name_tok;
    }
    if (!saw_body && brace == 0 && parentheses == 1 && brackets == 0
        && tok == '(' && (prev_tok >= TOK_UIDENT || prev_tok == ']'
            || saw_operator_declarator) && !name_tok)
      name_tok = prev_tok >= TOK_UIDENT ? prev_tok : prev_ident;
    if (!saw_body && !brace && !parentheses && !brackets && tok == ';')
    {
      next();
      break;
    }
    if (tok == '{')
    {
      if (!parentheses && !brackets && !brace) saw_body = 1;
      ++brace;
    }
    else if (tok == '}')
    {
      if (--brace == 0 && saw_body)
      {
        next();
        if (function_try && tok == TOK_CATCH) continue;
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

  if (!is_class && !is_alias)
    /* Resolve non-dependent class qualifiers before classifying a member
       template. Value<int>::read still belongs to a class even when its
       outer template<> header leaves only read's parameters dependent. */
    str = canonicalize_template_signature_types(str, type_param_toks,
                                                nb_type_params, !friend_owner);
  if (!is_class && !is_alias && register_template_static_data(str,type_param_toks,nb_type_params))
  {
    int parameter;
    for(parameter=0;parameter<nb_type_params;++parameter){
      if(default_arg_strs[parameter])tok_str_free(default_arg_strs[parameter]);
      if(value_type_strs[parameter])tok_str_free(value_type_strs[parameter]);
    }
    return;
  }
  if (!name_tok)
    name_tok = find_template_name_in_str(str);

  member_class_tok = is_alias ? 0
    : find_template_member_class_in_str(str, type_param_toks[0]);
  if (member_class_tok)
  {
    TemplateDef *member_class_td = find_class_template_def(find_current_namespace_tok(member_class_tok));
    int stripped_type_params[16], nb_stripped_type_params = 0;
    int stripped_value_params[16], nb_stripped_value_params = 0;
    unsigned stripped_value_mask = 0;
    if (!member_class_td || !member_class_td->is_class)
    {
      int param_i;
      for (param_i = 0; param_i < nb_type_params; ++param_i)
        if (type_param_is_type[param_i])
          stripped_type_params[nb_stripped_type_params++] =
            type_param_toks[param_i];
        else {
          stripped_value_mask |= 1u << param_i;
          stripped_value_params[nb_stripped_value_params++] =
            type_param_toks[param_i];
        }
    }
    add_template_member_def(member_class_tok,
                            member_class_td && member_class_td->is_class
                              ? type_param_toks[0] : 0,
                            stripped_type_params, nb_stripped_type_params,
                            stripped_value_params, nb_stripped_value_params,
                            type_param_toks,
                            nb_stripped_type_params + nb_stripped_value_params,
                            stripped_value_mask,
                            str);
    return;
  }

  {
    int i, func_min_args = 0, func_max_args = 0, func_is_variadic = 0;
    if (!name_tok) cprime_error("unsupported template declaration");
    int is_partial_specialization =
      is_class && is_class_template_partial_specialization(str, name_tok);
    unsigned func_sig_hash = 0;
    if (!is_class && !is_alias)
      analyze_template_function_signature(str, name_tok, &func_min_args,
                                          &func_max_args,
                                          &func_is_variadic, &func_sig_hash);
    for (i = 0; i < nb_template_defs; ++i)
    {
      TemplateDef *old = template_defs[i];
      int lookup_tok = nb_namespace_stack ? make_current_namespace_tok(name_tok)
                                          : name_tok;
      if ((old->lookup_tok ? old->lookup_tok : old->name_tok) != lookup_tok)
        continue;
      if (!is_class && !old->is_class) {
        int parameter;
        if (old->nb_type_params != nb_type_params) continue;
        for (parameter = 0; parameter < nb_type_params; ++parameter) {
          TokenString *prior_type = old->value_type_strs[parameter];
          TokenString *new_type = value_type_strs[parameter];
          CType prior_parameter, new_parameter;
          if ((!prior_type) != (!new_type)) break;
          if (template_template_parameter_kinds_differ(old->value_type_strs[parameter],
                                                        value_type_strs[parameter])) break;
          /* Equal function parameters do not identify templates whose value
             parameters have different declared types. Resolve aliases before
             comparing non-dependent parameter types. */
          if (prior_type && !token_string_starts_template(prior_type)
              && template_probe_type(prior_type, 0, old->namespace_tok, &prior_parameter)
              && template_probe_type(new_type, 0, current_namespace_scope_tok(), &new_parameter)
              && !is_compatible_unqualified_types(&prior_parameter, &new_parameter))
            break;
        }
        if (parameter != nb_type_params) continue;
      }
      if (!is_class && !old->is_class
          && (old->lookup_tok ? old->lookup_tok : old->name_tok) == lookup_tok
          && old->func_max_args == func_max_args
          && old->func_is_variadic == func_is_variadic)
      {
        TokenString *normalized = tok_str_alloc();
        int at, parameter;
        for (at = 0; at < str->len; ++at) {
          int token = str->str[at];
          if (TOK_HAS_VALUE(token)) {
            tok_str_add_record(normalized, str->str, str->len, &at); continue;
          }
          for (parameter = 0; parameter < nb_type_params; ++parameter)
            if (token == type_param_toks[parameter]) break;
          tok_str_add(normalized, parameter < nb_type_params ? old->type_param_toks[parameter] : token);
        }
        if (template_function_merge_signature_hash(old->def_str, old->name_tok)
              == template_function_merge_signature_hash(normalized, name_tok)) {
          if (!friend_owner) old->friend_class_tok = 0;
          if (saw_body && !old->has_body) {
            complete_function_template_definition(old, normalized);
            tok_str_free(str); return;
          }
          if (!saw_body) {
            tok_str_free(normalized); tok_str_free(str); return;
          }
        }
        tok_str_free(normalized);
      }
      if (token_string_template_duplicate(template_defs[i], name_tok, is_class,
                                          func_min_args, func_max_args,
                                          func_is_variadic,
                                          func_sig_hash,
                                          is_partial_specialization))
      {
        if (is_class) {
          int parameter, at, part;
          if (nb_type_params != old->nb_type_params
              || variadic_param_index != old->variadic_param_index)
            cprime_error("class template redeclaration has different parameters");
          /* Redeclarations share parameter identities and accumulate defaults.
             Canonicalize both the body and dependent defaults before retaining
             either under the original declaration's parameter names. */
          for (part = -1; part < nb_type_params; ++part) {
            TokenString *tokens = part < 0 ? str : default_arg_strs[part];
            if (!tokens) continue;
            for (at = 0; at < tokens->len; ++at) {
              int token = tokens->str[at];
              if (TOK_HAS_VALUE(token)) {
                at += tok_str_value_extra_words(tokens->str, tokens->len, at);
                continue;
              }
              for (parameter = 0; parameter < nb_type_params; ++parameter)
                if (token == type_param_toks[parameter]) break;
              if (parameter < nb_type_params)
                tokens->str[at] = old->type_param_toks[parameter];
            }
          }
          old->nb_required_type_params = 0;
          for (parameter = 0; parameter < nb_type_params; ++parameter) {
            if (default_arg_strs[parameter]) {
              if (old->default_arg_strs[parameter])
                cprime_error("redefinition of default template argument");
              old->default_arg_strs[parameter] = default_arg_strs[parameter];
            }
            if (!old->default_arg_strs[parameter]
                && parameter != variadic_param_index)
              old->nb_required_type_params = parameter + 1;
            if (value_type_strs[parameter]) tok_str_free(value_type_strs[parameter]);
          }
        }
        if (!is_class && saw_body && !old->has_body)
        {
          if (!friend_owner) old->friend_class_tok = 0;
          complete_function_template_definition(old, str);
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
          if (!friend_owner) old->friend_class_tok = 0;
          tok_str_free(str);
          return;
        }
        if (is_class && !saw_body)
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
  td->friend_class_tok = !is_class && !is_alias ? friend_owner : 0;
  td->lookup_tok = nb_namespace_stack ? make_current_namespace_tok(name_tok) : name_tok;
  td->namespace_tok = nb_namespace_stack
                        ? make_namespace_tok_from_parts(namespace_stack,
                                                        nb_namespace_stack)
                        : 0;
  td->type_param_toks = cprime_malloc(nb_type_params * sizeof(int));
  memcpy(td->type_param_toks, type_param_toks, nb_type_params * sizeof(int));
  {
    int parameter;
    for (parameter = 0; parameter < nb_type_params; ++parameter)
      if (!type_param_is_type[parameter]
          && !token_string_starts_template(value_type_strs[parameter]))
        td->value_param_mask |= 1u << parameter;
  }
  memcpy(td->default_arg_strs, default_arg_strs,
         sizeof(td->default_arg_strs));
  memcpy(td->value_type_strs, value_type_strs, sizeof(td->value_type_strs));
  td->nb_type_params = nb_type_params;
  td->nb_required_type_params = nb_required_type_params;
  td->variadic_param_index = variadic_param_index;
  td->is_class = is_class;
  td->is_alias = is_alias;
  td->is_partial_specialization =
    is_class && is_class_template_partial_specialization(str, name_tok);
  td->has_body = saw_body;
  td->is_union = is_union;
  if (!is_class && !is_alias)
    analyze_template_function_signature(str, name_tok, &td->func_min_args,
                                        &td->func_max_args,
                                        &td->func_is_variadic,
                                        &td->func_sig_hash);
  prepare_saved_cpp_lambdas(str);
  td->def_str = str;
  dynarray_add(&template_defs, &nb_template_defs, td);
  index_template_def(td);

}

/* A class-scope class template has an ordinary template definition, but its
   name lives in the containing class scope.  Flatten that scope into CPC's
   token namespace in the same way as nested non-template classes, then feed
   the declaration through the regular class-template registration path. */
static int register_inline_nested_class_template(int class_tok,
                                                 TokenString *member_str)
{
  int i, angle = 0, saw_params = 0, params_done = 0;
  int nested_name_tok = 0, scoped_name_tok;
  int saved_tok, saved_nb_namespace_stack;
  int saved_namespace;
  CValue saved_tokc;
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;

  if (!class_tok || !member_str)
    return 0;
  for (i = 0; i + 1 < member_str->len; ++i)
  {
    int t = member_str->str[i];
    if (t == TOK_LT || t == '<')
    {
      ++angle;
      saw_params = 1;
    }
    else if ((t == TOK_GT || t == '>') && angle > 0)
    {
      if (--angle == 0)
        params_done = 1;
    }
    else if ((t == TOK_SAR || t == TOK_SHR) && angle > 0)
    {
      angle -= 2;
      if (angle <= 0)
      {
        angle = 0;
        params_done = 1;
      }
    }
    if (saw_params && params_done
        && (t == TOK_CLASS || t == TOK_STRUCT)
        && member_str->str[i + 1] >= TOK_UIDENT)
    {
      nested_name_tok = member_str->str[i + 1];
      break;
    }
    if (TOK_HAS_VALUE(t))
      i += tok_str_value_extra_words(member_str->str, member_str->len, i);
  }
  if (!nested_name_tok)
    return 0;

  scoped_name_tok = make_static_member_tok(class_tok, nested_name_tok);
  note_class_source_name(scoped_name_tok, nested_name_tok);
  note_cpp_record_decl(scoped_name_tok, nested_name_tok, class_tok,
                       symbol_namespace_scope_tok(class_tok));
  saved_namespace = current_namespace_scope_tok();
  enter_namespace_scope(symbol_namespace_scope_tok(class_tok));
  int cursor = 0, prior_token = -1;
  for (i = 0; i < member_str->len; ++i)
  {
    int t = member_str->str[i];
    int previous = tok_str_prev_token_index_from(member_str->str, member_str->len, i, &cursor, &prior_token);
    if (t == nested_name_tok)
      member_str->str[i] = scoped_name_tok;
    else if (t >= TOK_UIDENT && (previous < 0
        || (member_str->str[previous] != ':' && member_str->str[previous] != '.'
            && member_str->str[previous] != TOK_ARROW)))
    {
      int qualified_tok = find_current_namespace_tok(t);
      if (qualified_tok != t)
        member_str->str[i] = qualified_tok;
    }
    if (TOK_HAS_VALUE(t))
      i += tok_str_value_extra_words(member_str->str, member_str->len, i);
  }

  enter_namespace_scope(saved_namespace);
  saved_tok = tok;
  saved_tokc = tokc;
  saved_nb_namespace_stack = nb_namespace_stack;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  nb_namespace_stack = 0;
  tok_str_add(member_str, TOK_EOF);
  begin_macro(member_str, 1);
  next();
  parse_template_decl();
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  nb_namespace_stack = saved_nb_namespace_stack;
  tok = saved_tok;
  tokc = saved_tokc;
  return 1;
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

static int template_template_argument_matches(TokenString *signature, int argument)
{
  TemplateDef *actual = find_template_def(argument);
  int i, depth = 0, parameter = 0, at_start = 0, pack = 0;
  if (!actual || (!actual->is_class && !actual->is_alias)) return 0;
  for (i = 0; i < signature->len; ++i) {
    int token = signature->str[i];
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(signature->str, signature->len, i);
      continue;
    }
    if (token == TOK_LT) { ++depth; if (depth == 1) at_start = 1; continue; }
    if (token == TOK_GT || token == TOK_SAR) {
      depth -= token == TOK_SAR ? 2 : 1;
      if (depth <= 0) break;
      continue;
    }
    if (depth != 1) continue;
    if (token == TOK_DOTS) pack = 1;
    if (token == ',') { at_start = 1; continue; }
    if (at_start) {
      int is_type = token == TOK_CLASS || is_template_keyword_tok(token)
                   || (token >= TOK_UIDENT && !strcmp(get_tok_str(token, NULL), "typename")
                       && !(i + 2 < signature->len && signature->str[i + 2] == ':'));
      int index = parameter < actual->nb_type_params ? parameter : actual->variadic_param_index;
      if (index < 0 || index >= actual->nb_type_params) return 0;
      if (is_type == !!(actual->value_param_mask & (1u << index))) return 0;
      ++parameter; at_start = 0;
    }
  }
  return pack || (parameter >= actual->nb_required_type_params
                   && (parameter <= actual->nb_type_params || actual->variadic_param_index >= 0));
}

static int validate_template_value_parameter_type(TemplateDef *definition,
                                  TemplateArgList *arguments, int parameter)
{
  TokenString *source, *replay;
  CType type;
  int i, j, valid;
  if (parameter >= definition->nb_type_params)
    parameter = definition->variadic_param_index;
  if (parameter < 0) return 0;
  source = definition->value_type_strs[parameter];
  if (!source) return 1;
  if (token_string_starts_template(source)) {
    TemplateInstOwner *injected = find_template_inst_owner_entry(arguments->toks[parameter]);
    if (injected && injected->owner->is_class)
      arguments->toks[parameter] = template_def_lookup_tok(injected->owner);
    return template_template_argument_matches(source, arguments->toks[parameter]);
  }
  replay = tok_str_alloc();
  for (i = 0; i < source->len; ++i) {
    int token = source->str[i];
    if (TOK_HAS_VALUE(token)) { tok_str_add_record(replay,source->str,source->len,&i);continue; }
    for (j = 0; j < parameter && j < arguments->nb; ++j)
      if (token == definition->type_param_toks[j]) break;
    tok_str_add(replay,j < parameter && j < arguments->nb ? arguments->toks[j] : token);
  }
  valid = template_probe_type(replay, 0, definition->namespace_tok, &type);
  tok_str_free(replay);
  if (valid) {
    canonicalize_cpp_member_pointer_declarator(&type);
    CType *function_type = &type;
    if ((function_type->t & VT_BTYPE) == VT_PTR) function_type = pointed_type(function_type);
    if ((function_type->t & VT_BTYPE) == VT_FUNC) {
      int name = arguments->toks[parameter];
      Sym *selected = global_symbol_find(name);
      FreeFuncOverload *overload;
      if (selected && selected->template_address_target)
        return is_compatible_types(&selected->template_address_target->type, function_type);
      if (!selected || (selected->type.t & VT_BTYPE) != VT_FUNC
          || !is_compatible_types(&selected->type, function_type)) selected = NULL;
      for (overload = free_func_overloads; overload; overload = overload->next)
        if (overload->name_tok == name && is_compatible_types(&overload->func_type, function_type)) {
          Sym *candidate = global_symbol_find(overload->mangled_tok);
          if (selected && selected != candidate) return 0;
          selected = candidate;
        }
      if (!selected && active_member_class_tok) {
        int qualified = make_static_member_tok(active_member_class_tok, name);
        for (overload = free_func_overloads; overload; overload = overload->next) {
          if (overload->name_tok == qualified
              && is_compatible_types(&overload->func_type, function_type)) {
            Sym *candidate = global_symbol_find(overload->mangled_tok);
            if (selected && selected != candidate) return 0;
            selected = candidate;
          }
        }
      }
      if (!selected && find_function_template_def(name)) {
        Sym unresolved = {0};
        CType source_type = {VT_FUNC, &unresolved};
        unresolved.v = name;
        unresolved.f.func_unresolved_overload = 1;
        selected = resolve_function_address(&source_type, &type);
      }
      if (!selected) return 0;
      type = selected->type;
      mk_pointer(&type);
      arguments->toks[parameter] = template_intern_address(selected, &type);
    } else if (find_cpp_member_pointer_type(&type)) {
      Sym *argument = global_symbol_find(arguments->toks[parameter]);
      CppMemberPointerType *target = find_cpp_member_pointer_type(&type);
      CppMemberPointerType *source = argument ? find_cpp_member_pointer_type(&argument->type) : NULL;
      if (!argument || !argument->cpp_member_address_tok || !source
          || source->owner_tok != target->owner_tok
          || ((source->member_type.t & ~target->member_type.t) & (VT_CONSTANT | VT_VOLATILE))
          || !is_compatible_unqualified_types(&source->member_type, &target->member_type)) return 0;
      if (!is_compatible_types(&argument->type, &type)) {
        char name[128];
        int binding_tok;
        Sym *binding;
        snprintf(name, sizeof(name), "__cpc_template_member_conversion_%d_%d", argument->v, type.ref->v);
        binding_tok = tok_alloc_const(name);
        binding = global_symbol_find(binding_tok);
        if (!binding) {
          binding = global_identifier_push(binding_tok, type.t, 0);
          binding->type = type;
          binding->r = argument->r;
          binding->template_address_target = argument->template_address_target;
          binding->cpp_member_address_tok = argument->cpp_member_address_tok;
        }
        arguments->toks[parameter] = binding_tok;
      }
    } else if (is_reference_type(&type)) {
      Sym *target = global_symbol_find(arguments->toks[parameter]);
      if (target && target->template_address_target) target = target->template_address_target;
      if (!target || !(target->r & VT_SYM)
          || ((target->type.t & ~pointed_type(&type)->t) & (VT_CONSTANT | VT_VOLATILE))
          || !is_compatible_unqualified_types(&target->type, pointed_type(&type))) return 0;
      arguments->toks[parameter] = template_intern_address(target, &type);
    } else if ((type.t & VT_BTYPE) == VT_PTR) {
      Sym *argument = global_symbol_find(arguments->toks[parameter]);
      if (arguments->toks[parameter] != TOK_NULLPTR
          && (!argument || (argument->type.t & VT_BTYPE) != VT_PTR
              || !call_arg_matches_param_type(&type, &argument->type)))
        return 0;
    } else if (is_integer_btype(type.t & VT_BTYPE)) {
      Sym *argument = global_symbol_find(arguments->toks[parameter]);
      if (argument && (argument->r & VT_SYM)
          && !argument->a.integral_constexpr) return 0;
    }
  }
  return valid;
}

/* A token that names a member of a qualified or member-access expression is
   not a reference to a template parameter even when the spelling is the same.
   Unqualified substitution in a default argument otherwise rewrites the
   member name in `NumericTraits<...>::TInputImage` and makes the default
   argument unparseable. */
static int template_token_follows_scope(TokenString *tokens, int index)
{
  int previous = tok_str_prev_token_index(tokens->str, tokens->len, index);
  if (previous >= 0 && is_template_keyword_tok(tokens->str[previous]))
    previous = tok_str_prev_token_index(tokens->str, tokens->len, previous);
  if (previous < 0)
    return 0;
  if (tokens->str[previous] == ':' || tokens->str[previous] == '.')
    return 1;
  if (tokens->str[previous] == '>') {
    int before_previous = tok_str_prev_token_index(tokens->str, tokens->len, previous);
    return before_previous >= 0 && tokens->str[before_previous] == '-';
  }
  return 0;
}

/* DR 1004: inside `B<int>`, the injected class name `B` names the
   specialization as a type but the class template itself as a template-name.
   A dependent template-template default such as `T::template B` therefore
   resolves to the owner template of the supplied specialization. */
static int template_default_injected_template_arg(TemplateDef *td,
                                                  TemplateArgList *args,
                                                  int param_index)
{
  TokenString *source = td->default_arg_strs[param_index];
  int index[5];
  int count = 0, i;
  TemplateInstOwner *injected;
  int lookup;

  for (i = 0; i < source->len; ++i) {
    if (TOK_HAS_VALUE(source->str[i]))
      return 0;
    if (count < (int)(sizeof(index) / sizeof(index[0])))
      index[count] = i;
    ++count;
  }
  if (count != (int)(sizeof(index) / sizeof(index[0]))
      || source->str[index[1]] != ':' || source->str[index[2]] != ':'
      || !is_template_keyword_tok(source->str[index[3]]))
    return 0;
  for (i = 0; i < args->nb && i < param_index; ++i)
    if (source->str[index[0]] == td->type_param_toks[i])
      break;
  if (i >= args->nb || i >= param_index)
    return 0;
  injected = find_template_inst_owner_entry(args->toks[i]);
  if (!injected || !injected->owner || !injected->owner->is_class)
    return 0;
  lookup = template_def_lookup_tok(injected->owner);
  if (source->str[index[4]] != lookup
      && source->str[index[4]] != injected->owner->name_tok)
    return 0;
  return lookup;
}

static int instantiate_default_template_arg(TemplateDef *td,
                                            TemplateArgList *args,
                                            int param_index)
{
  TokenString *replay;
  CType parsed;
  int result, i, j;

  if (!td || param_index < 0 || param_index >= td->nb_type_params
      || !td->default_arg_strs[param_index])
    return 0;
  result = template_default_injected_template_arg(td, args, param_index);
  if (result)
    return result;
  /* Defaults can contain dependent expressions, not just a single type name.
     Substitute the known parameters before parsing decltype/sizeof and nested
     calls so their ordinary expression parser never sees an unbound T. */
  replay = tok_str_alloc();
  for (i = 0; i < td->default_arg_strs[param_index]->len; ++i)
  {
    TokenString *source = td->default_arg_strs[param_index];
    int t = source->str[i];
    if (TOK_HAS_VALUE(t))
    {
      tok_str_add_record(replay, source->str, source->len, &i);
      continue;
    }
    for (j = 0; j < args->nb && j < param_index; ++j)
      if (t == td->type_param_toks[j])
        break;
    tok_str_add(replay, j < args->nb && j < param_index
                        && !template_token_follows_scope(source, i)
                      ? args->toks[j] : t);
  }
  result = template_probe_type(replay, 1, td->namespace_tok, &parsed)
             ? parsed.t : 0;
  tok_str_free(replay);
  return result;
}

static void template_validate_arg_count(TemplateDef *td, TemplateArgList *args)
{
  int parameter;
  for (parameter = 0; parameter < args->nb && parameter < td->nb_type_params; ++parameter)
    if (td->value_type_strs[parameter]
        && !validate_template_value_parameter_type(td, args, parameter))
      cprime_error("template argument does not match parameter %d of '%s'",
                    parameter + 1, get_tok_str(td->name_tok, NULL));
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
  {
    int param_index = args->nb;
    int default_tok = instantiate_default_template_arg(td, args, param_index);
    if (!default_tok)
      cprime_error("template '%s' has no usable default for argument %d",
                   get_tok_str(td->name_tok, NULL), param_index + 1);
    args->toks[args->nb++] = default_tok;
  }
}

static int make_template_inst_name_tok(TemplateDef *td, TemplateArgList *args)
{
  CString name;
  int i, result;

  template_validate_arg_count(td, args);
  cstr_new(&name);
  cstr_cat(&name, get_tok_str(td->lookup_tok ? td->lookup_tok : td->name_tok, NULL), -1);
  for (i = 0; i < args->nb; ++i)
  {
    const char *argument;
    if (!args->toks[i])
      continue;
    cstr_cat(&name, "__", -1);
    argument = get_tok_str(args->toks[i], NULL);
    while (*argument)
    {
      int c = *argument++;
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '_'))
        c = '_';
      cstr_ccat(&name, c);
    }
  }
  if (!td->is_class && !td->is_alias)
  {
    char sigbuf[64];
    snprintf(sigbuf, sizeof(sigbuf), "__a%d_h%08x",
             td->func_min_args, td->func_sig_hash);
    cstr_cat(&name, sigbuf, -1);
  }
  cstr_ccat(&name, 0);
  result = tok_alloc_const(name.data);
  cstr_free(&name);
  return result;
}

static int template_type_arg_toks_match(int t1, int t2)
{
  CType c1, c2;
  Sym *s1, *s2;

  if (t1 == t2)
    return 1;
  s1 = global_symbol_find(t1);
  s2 = global_symbol_find(t2);
  if (t1 == TOK_TRUE || t1 == TOK_FALSE || t2 == TOK_TRUE || t2 == TOK_FALSE)
  {
    int known1 = t1 == TOK_TRUE || t1 == TOK_FALSE;
    int known2 = t2 == TOK_TRUE || t2 == TOK_FALSE;
    int64_t value1 = t1 == TOK_TRUE, value2 = t2 == TOK_TRUE;
    if (!known1 && s1 && !(s1->type.t & VT_TYPEDEF)
        && (s1->type.t & VT_CONSTANT)
        && is_integer_btype(s1->type.t & VT_BTYPE)
        && (s1->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST)
    { known1 = 1; value1 = s1->c; }
    if (!known2 && s2 && !(s2->type.t & VT_TYPEDEF)
        && (s2->type.t & VT_CONSTANT)
        && is_integer_btype(s2->type.t & VT_BTYPE)
        && (s2->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST)
    { known2 = 1; value2 = s2->c; }
    return known1 && known2 && value1 == value2;
  }
  if ((s1 && !(s1->type.t & VT_TYPEDEF)
       && (s1->type.t & VT_CONSTANT) && is_integer_btype(s1->type.t & VT_BTYPE)
       && (s1->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST)
      || (s2 && !(s2->type.t & VT_TYPEDEF)
          && (s2->type.t & VT_CONSTANT) && is_integer_btype(s2->type.t & VT_BTYPE)
          && (s2->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST))
    return s1 && s2 && !(s1->type.t & VT_TYPEDEF)
           && !(s2->type.t & VT_TYPEDEF) && s1->c == s2->c;
  /* Typedef spellings (i32, f32) must resolve to the same instantiation as
     their underlying types (int, float), while distinct types (ui8 vs i8)
     must stay separate.  Compare the underlying CType. */
  if (make_type_from_type_arg_tok(&c1, t1)
      && make_type_from_type_arg_tok(&c2, t2))
  {
    if ((c1.t & VT_BTYPE) != (c2.t & VT_BTYPE)
        || (c1.t & VT_UNSIGNED) != (c2.t & VT_UNSIGNED)
        || (c1.t & VT_LONG) != (c2.t & VT_LONG)
        || (c1.t & (VT_REFERENCE | VT_RVALUE_REFERENCE | VT_CONSTANT | VT_VOLATILE))
            != (c2.t & (VT_REFERENCE | VT_RVALUE_REFERENCE | VT_CONSTANT | VT_VOLATILE)))
      return 0;
    if (IS_ENUM(c1.t) || IS_ENUM_VAL(c1.t)
        || IS_ENUM(c2.t) || IS_ENUM_VAL(c2.t))
      return (IS_ENUM(c1.t) || IS_ENUM_VAL(c1.t))
             && (IS_ENUM(c2.t) || IS_ENUM_VAL(c2.t))
             && c1.ref == c2.ref;
    if ((c1.t & VT_BTYPE) == VT_STRUCT)
      return c1.ref == c2.ref;
    /* A shared pointer category does not imply a shared specialization:
       pointee types, nested qualifiers, and function signatures matter. */
    return template_types_identical(&c1, &c2);
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
    if (td->variadic_param_index >= 0)
    {
      TemplateArgList *recorded;

      if (!td->inst_arg_lists)
        continue;
      recorded = &td->inst_arg_lists[i];
      if (recorded->nb != args->nb)
        continue;
      same = 1;
      for (j = 0; j < args->nb; ++j)
        if (!template_type_arg_toks_match(recorded->toks[j],
                                          args->toks[j]))
        {
          same = 0;
          break;
        }
      if (same)
        return td->inst_name_toks[i];
      continue;
    }
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

static int template_intern_address(Sym *target, CType *type)
{
  int result;
  char name[96];
  Sym *argument;
  snprintf(name, sizeof(name), is_reference_type(type)
             ? "__cpc_template_reference_%d" : "__cpc_template_address_%d", target->v);
  result = tok_alloc_const(name);
  argument = global_symbol_find(result);
  if (!argument) {
    argument = global_identifier_push(result, type->t, 0);
    argument->type = *type;
    argument->type.ref = template_stable_pointer_ref(type);
    argument->r = VT_CONST | VT_SYM;
    argument->template_address_target = target;
  }
  return result;
}

static int template_address_argument_tok(void)
{
  int saved_nocode = nocode_wanted, result;
  nocode_wanted = 1;
  unary();
  nocode_wanted = saved_nocode;
  if (find_cpp_member_pointer_type(&vtop->type) && vtop->sym
      && vtop->sym->cpp_member_address_tok) {
    char name[128];
    Sym *argument;
    CppMemberPointerType *member = find_cpp_member_pointer_type(&vtop->type);
    snprintf(name, sizeof(name), "__cpc_template_member_address_%d_%d",
             member->owner_tok, vtop->sym->cpp_member_address_tok);
    result = tok_alloc_const(name);
    argument = global_symbol_find(result);
    if (!argument) {
      argument = global_identifier_push(result, vtop->type.t, 0);
      argument->type = vtop->type;
      argument->r = VT_CONST | VT_SYM | VT_LVAL;
      argument->template_address_target = vtop->sym;
      argument->cpp_member_address_tok = vtop->sym->cpp_member_address_tok;
    }
    vpop();
    return result;
  }
  if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) != (VT_CONST | VT_SYM)
      || !vtop->sym || vtop->c.i != 0
      || (vtop->type.t & VT_BTYPE) != VT_PTR)
    cprime_error("template address argument must name a static object or function");
  result = template_intern_address(vtop->sym, &vtop->type);
  vpop();
  return result;
}

static int template_integer_constant_arg_tok(void)
{
  char name[128], literal[64], suffix[64];
  const char *tok_text = get_tok_str(tok, &tokc);
  int i, j = 0, type = VT_LLONG, value;
  int name_tok;
  Sym *constant_sym;

  if (!tok_text)
    tok_text = "0";
  pstrcpy(literal, sizeof(literal), tok_text);
  for (i = 0; literal[i] && j + 1 < (int)sizeof(suffix); ++i)
  {
    char c = literal[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')
        || (c >= 'a' && c <= 'f') || c == 'x' || c == 'X')
      suffix[j++] = c;
    else if (c == '-')
      suffix[j++] = 'n';
    else if (c == '+')
      suffix[j++] = 'p';
    else
      suffix[j++] = '_';
  }
  suffix[j] = '\0';
  snprintf(name, sizeof(name), "__cpc_template_const_%s", suffix);
  name_tok = tok_alloc_const(name);
  if (!global_symbol_find(name_tok))
  {
    value = (int)strtoll(literal, NULL, 0);
    if (tok == TOK_CUINT || tok == TOK_CULONG || tok == TOK_CULLONG)
      type |= VT_UNSIGNED;
    constant_sym = global_identifier_push(name_tok, type | VT_CONSTANT, value);
    constant_sym->r = VT_CONST;
  }
  next();
  return name_tok;
}

static int template_integer_expression_arg_tok(int64_t value)
{
  char name[128], suffix[64];
  int name_tok;
  Sym *constant_sym;

  if (value < 0)
    snprintf(suffix, sizeof(suffix), "n%llu",
             (unsigned long long)(-(value + 1)) + 1);
  else
    snprintf(suffix, sizeof(suffix), "%llu", (unsigned long long)value);
  snprintf(name, sizeof(name), "__cpc_template_const_expr_%s", suffix);
  name_tok = tok_alloc_const(name);
  constant_sym = global_symbol_find(name_tok);
  if (!constant_sym)
  {
    constant_sym = global_identifier_push(
      name_tok, VT_LLONG | VT_CONSTANT, (int)value);
    constant_sym->r = VT_CONST;
    constant_sym->c = (int)value;
  }
  return name_tok;
}

static int template_named_cast_tok(int token)
{
  const char *name;
  if (token < TOK_UIDENT)
    return 0;
  name = get_tok_str(token, NULL);
  return !strcmp(name, "static_cast") || !strcmp(name, "const_cast")
      || !strcmp(name, "reinterpret_cast") || !strcmp(name, "dynamic_cast");
}

static int parse_template_constant_expression_arg_subst(
    TemplateDef *ctx_td, TemplateArgList *ctx_args,
    int *alias_from, int *alias_to, int nb_aliases)
{
  TokenString *expression = tok_str_alloc();
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;
  int saved_tok, paren = 0, bracket = 0, angle = 0, named_cast = 0;
  int previous = 0, qualifier_parts[16], qualifier_count = 0;
  int qualified_name = 0, qualifier_colons = 0;
  int angle_parens[64], angle_brackets[64];
  int address_argument = tok == '&', address_token = 0;
  int64_t value;
  CValue saved_tokc;

  while (tok != TOK_EOF)
  {
    int subst_tok = 0, i;
    if (!paren && !bracket && !angle
        && (tok == ',' || tok == TOK_GT || tok == '>' || tok == TOK_SAR))
      break;
    if ((tok == TOK_LT || tok == '<')
        && (named_cast
            || (previous >= TOK_UIDENT
                && (find_class_template_def_in_context(ctx_td, previous)
                    || find_class_template_def(find_current_namespace_tok(previous))
                    || find_function_template_def(find_current_namespace_tok(previous))
                    || (qualified_name && find_class_template_def(qualified_name))))
            || (angle && paren == angle_parens[angle - 1]
                          && bracket == angle_brackets[angle - 1])))
    {
      if (angle >= (int)(sizeof(angle_parens) / sizeof(angle_parens[0])))
        cprime_error("too many nested template argument casts");
      angle_parens[angle] = paren;
      angle_brackets[angle++] = bracket;
    }
    else if (angle && paren == angle_parens[angle - 1]
             && bracket == angle_brackets[angle - 1]
             && (tok == TOK_GT || tok == '>' || tok == TOK_SAR))
    {
      if (tok == TOK_SAR)
      {
        tok = TOK_GT;
        unget_tok(TOK_GT);
      }
      --angle;
    }
    named_cast = template_named_cast_tok(tok);
    if (tok == '(')
      ++paren;
    else if (tok == ')' && paren > 0)
      --paren;
    else if (tok == '[')
      ++bracket;
    else if (tok == ']' && bracket > 0)
      --bracket;
    if (ctx_td && ctx_args)
      for (i = 0; i < ctx_td->nb_type_params && i < ctx_args->nb; ++i)
        if (tok == ctx_td->type_param_toks[i])
        {
          subst_tok = ctx_args->toks[i];
          break;
        }
    if (!subst_tok)
      subst_tok = template_alias_lookup(alias_from, alias_to, nb_aliases, tok);
    if (subst_tok)
      tok_str_add(expression, subst_tok);
    else
      tok_str_add_tok(expression);
    if (tok >= TOK_UIDENT)
    {
      if (qualifier_colons != 2) qualifier_count = 0;
      if (qualifier_count < 16) qualifier_parts[qualifier_count++] = tok;
      qualified_name = qualifier_count > 1
        ? make_namespace_tok_from_parts(qualifier_parts, qualifier_count) : tok;
      qualifier_colons = 0;
    }
    else if (tok == ':')
      ++qualifier_colons;
    else
    {
      qualifier_count = qualifier_colons = 0;
      qualified_name = 0;
    }
    previous = tok;
    next();
  }
  tok_str_add(expression, TOK_EOF);

  saved_tok = tok;
  saved_tokc = tokc;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  begin_macro(expression, 1);
  next();
  if (address_argument) address_token = template_address_argument_tok();
  else value = expr_const64();
  if (tok != TOK_EOF)
    cprime_error("unexpected token '%s' in constant template argument", get_tok_str(tok, &tokc));
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  return address_argument ? address_token : template_integer_expression_arg_tok(value);
}

static int parse_template_constant_expression_arg(void)
{
  return parse_template_constant_expression_arg_subst(NULL, NULL, NULL, NULL, 0);
}

static int template_static_const_arg_tok(int member_tok)
{
  int class_tok, static_tok;
  Sym *s;

  class_tok = active_member_class_tok;
  if (!class_tok && nb_defining_class_stack > 0)
    class_tok = defining_class_stack[nb_defining_class_stack - 1];
  if (!class_tok) return 0;
  static_tok = make_static_member_tok(class_tok, member_tok);
  s = sym_find(static_tok);
  if (!s)
    s = global_symbol_find(static_tok);
  if (!s || (s->type.t & VT_TYPEDEF) || (s->r & VT_VALMASK) != VT_CONST
      || !is_integer_btype(s->type.t & VT_BTYPE))
    return 0;
  return static_tok;
}

static int template_arg_const_int64(int arg_tok, int64_t *out)
{
  Sym *s = sym_find(arg_tok);
  if (arg_tok == tok_alloc_const("true") || arg_tok == tok_alloc_const("false")) {
    *out = arg_tok == tok_alloc_const("true");
    return 1;
  }
  if (!s)
    s = global_symbol_find(arg_tok);
  if (!s || (s->r & VT_VALMASK) != VT_CONST
      || !is_integer_btype(s->type.t & VT_BTYPE))
    return 0;
  *out = IS_ENUM_VAL(s->type.t) ? s->enum_val : s->c;
  return 1;
}

static int try_eval_integer_log2_template_call(TemplateDef *td,
                                               TemplateArgList *args,
                                               int64_t *out)
{
  int64_t value, result = 0;

  if (!td || !args || args->nb != 1
      || !strstr(get_tok_str(td->name_tok, NULL), "Log2Static")
      || !template_arg_const_int64(args->toks[0], &value)
      || value <= 0)
    return 0;
  while (value > 1)
  {
    value >>= 1;
    ++result;
  }
  *out = result;
  return 1;
}

static int class_alias_layout_depth;

static int instantiate_template_declaration_type(TemplateDef *definition, TemplateArgList *arguments)
{
  TokenString *qualifiers = NULL;
  int indirect;
  int result;
  while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
         || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3) {
    if (!qualifiers) qualifiers = tok_str_alloc();
    tok_str_add_tok(qualifiers); next();
  }
  indirect = tok == '*' || tok == '&' || tok == TOK_LAND;
  if (qualifiers) restore_cpp_lifecycle_probe(qualifiers);
  class_alias_layout_depth += indirect;
  result = instantiate_template_if_needed(definition, arguments);
  class_alias_layout_depth -= indirect;
  return result;
}

static int instantiate_template_argument_type(TemplateDef *definition,
                                               TemplateArgList *arguments)
{
  int name;
  /* A template argument needs a type identity, not the argument's class
     definition. Layout/member lookup will materialize that definition on
     demand, as it already does for aliases of incomplete specializations. */
  ++class_alias_layout_depth;
  name = instantiate_template_if_needed(definition, arguments);
  --class_alias_layout_depth;
  return name;
}

static void compile_template_alias_decl_now(TokenString *alias_decl)
{
  int saved_tok = tok;
  int saved_local_scope = local_scope;
  int saved_defer_pending_member_funcs = defer_pending_member_funcs;
  int saved_nocode_wanted = nocode_wanted;
  CValue saved_tokc = tokc;
  Sym *saved_local_stack = local_stack;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;

  local_stack = NULL;
  local_scope = 0;
  defer_pending_member_funcs = 1;
  /* decl() performs one-token lookahead.  Fence the owned alias replay so it
     cannot consume and close the suspended template replay, then restore the
     exact parent cursor even when end_macro() reached the fence itself. */
  tok_str_add(alias_decl, ';');
  tok_str_add(alias_decl, TOK_EOF);
  begin_macro(alias_decl, 1);
  next();
  ++class_alias_layout_depth;
  decl(VT_CONST);
  --class_alias_layout_depth;
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
  nocode_wanted = saved_nocode_wanted;
  tok = saved_tok;
  tokc = saved_tokc;
}

/* Specialization aliases are useful before their containing class is parsed:
   an alias can itself instantiate a class whose nested type is needed while
   parsing that containing class.  Do not eagerly parse aliases that name a
   field or nested class declared in the containing class, though.  Those
   names intentionally do not exist until normal class declaration order
   reaches them (Node* and decltype(an_earlier_field) are common examples).

   At this point all template parameters in alias_decl have been substituted.
   Therefore every unqualified type-name dependency that is safe to resolve
   early is already present in the global type/template/namespace tables.
   Names following :: are members of such a resolved root and are checked by
   the real type parser when the alias is compiled. */
static int template_alias_decl_is_ready(TokenString *alias_decl,
                                        int declared_alias_tok)
{
  int i;

  if (!alias_decl)
    return 0;
  for (i = 0; i < alias_decl->len; ++i)
  {
    int t = alias_decl->str[i];
    const char *name;

    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(alias_decl->str, alias_decl->len, i);
      continue;
    }
    if (t < TOK_UIDENT || t == declared_alias_tok)
      continue;
    if (i >= 2 && alias_decl->str[i - 1] == ':'
        && alias_decl->str[i - 2] == ':')
      continue;
    name = get_tok_str(t, NULL);
    if (!strcmp(name, "typename") || !strcmp(name, "template")
        || !strcmp(name, "decltype"))
      continue;
    if (is_namespace_tok(t))
      continue;
    if (struct_find(t) || global_symbol_find(t) || find_class_template_def(t))
    {
      if (i + 2 < alias_decl->len && alias_decl->str[i + 1] == ':'
          && alias_decl->str[i + 2] == ':')
      {
        Sym *dependency = global_symbol_find(t);
        Sym *dependency_class = struct_find(t);
        if (dependency && (dependency->type.t & VT_BTYPE) == VT_STRUCT)
          dependency_class = dependency->type.ref;
        if (dependency_class && dependency_class->c < 0)
          return 0;
      }
      continue;
    }
    return 0;
  }
  return 1;
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
  if (stack == global_stack)
  {
    for (s = sym_find(tok); s; s = s->prev_tok)
      if (!sym_scope_ex(s) && (s->type.t & VT_TYPEDEF))
        return s;
    return NULL;
  }
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
  TemplateInstOwner *owner;
  const CppRecordDeclInfo *record = lookup_cpp_record_decl(inst_tok);

  if (!td || param_index < 0 || param_index >= td->nb_type_params)
    return 0;
  if (record && record->template_definition == td
      && param_index < record->template_arguments.nb)
    return record->template_arguments.toks[param_index];
  owner = find_template_inst_owner_entry(inst_tok);
  if (owner && owner->owner == td)
    return td->inst_type_toks[owner->inst_index * td->nb_type_params + param_index];
  for (i = 0; i < td->nb_inst; ++i)
    if (td->inst_name_toks[i] == inst_tok)
      return td->inst_type_toks[i * td->nb_type_params + param_index];
  return 0;
}

/* A simple template-id parameter can deduce from that specialization or a
   matching base specialization. A converting constructor is not deduction. */
static int template_class_argument_for_deduction(TemplateDef *td, int class_tok,
                                                  int parameter)
{
  ClassBaseInfo *base;
  int result = template_inst_arg_for_name(td, class_tok, parameter);
  if (result) return result;
  for (base = class_base_candidates(class_tok); base; base = base->bucket_next) {
    int candidate;
    if (base->class_tok != class_tok) continue;
    candidate = template_class_argument_for_deduction(td, base->base_tok,
                                                       parameter);
    if (candidate < 0) return -1;
    if (!candidate) continue;
    if (result && !template_type_arg_toks_match(result, candidate)) return -1;
    result = candidate;
  }
  return result;
}

/* A bound type is an identity, not a fresh unqualified lookup in the
   template definition's namespace.  Give class arguments a reserved global
   alias so replay cannot bind a caller's Item to a different namespace Item. */
static int template_bound_type_spelling(int arg_tok)
{
  Sym *class_sym = struct_find(arg_tok);
  Sym *alias;
  int alias_tok;
  char name[64];
  if (!is_cpp_translation_unit() || !class_sym)
    return arg_tok;
  snprintf(name, sizeof(name), "__cpc_bound_type_%d", arg_tok);
  alias_tok = tok_alloc_const(name);
  alias = global_symbol_find(alias_tok);
  if (!alias)
  {
    alias = global_identifier_push(alias_tok,
                                    class_sym->type.t | VT_TYPEDEF, 0);
    alias->type.ref = class_sym;
  }
  return alias_tok;
}

/* Preserve an explicitly global root when replay enters a hiding scope. */
static int template_global_type_spelling(int type_tok)
{
  Sym *source = global_symbol_find(type_tok);
  CType type;
  Sym *alias;
  CString name;
  int alias_tok;
  if (source && (source->type.t & VT_TYPEDEF))
    type = source->type;
  else
  {
    source = struct_find(type_tok);
    if (!source)
      return type_tok;
    type.t = source->type.t;
    type.ref = source;
  }
  cstr_new(&name);
  cstr_cat(&name, "__cpc_global_type_", -1);
  cstr_cat(&name, get_tok_str(type_tok, NULL), -1);
  cstr_ccat(&name, 0);
  alias_tok = tok_alloc_const(name.data);
  cstr_free(&name);
  alias = global_symbol_find(alias_tok);
  if (!alias)
  {
    alias = global_identifier_push(alias_tok, type.t | VT_TYPEDEF, 0);
    alias->type.ref = type.ref;
  }
  return alias_tok;
}

static int template_alias_name_is_qualified(TokenString *tokens, int index)
{
  int previous = tok_str_prev_token_index(tokens->str, tokens->len, index);
  if (previous >= 0 && is_template_keyword_tok(tokens->str[previous]))
    previous = tok_str_prev_token_index(tokens->str, tokens->len, previous);
  return previous >= 0 && tokens->str[previous] == ':';
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
    return template_bound_type_spelling(arg_tok);
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

static int template_add_subst_token_if_changed(TokenString *spec,
                                                TemplateDef *td,
                                                TemplateArgList *args,
                                                int class_mangled_tok,
                                                int *alias_from,
                                                int *alias_to,
                                                int nb_aliases, int t)
{
  int subst_tok = template_subst_token(td, args, class_mangled_tok,
                                       alias_from, alias_to, nb_aliases, t);
  if (subst_tok == t)
    return 0;
  tok_str_add(spec, subst_tok);
  return 1;
}

static int template_typedef_is_type_pos(TokenString *spec)
{
  int prev = spec->len > 0 ? spec->str[spec->len - 1] : 0;

  switch (prev) {
  case TOK_VOID: case TOK_CHAR: case TOK_SHORT: case TOK_INT: case TOK_LONG:
  case TOK_BOOL: case TOK_BOOL2: case TOK_FLOAT: case TOK_DOUBLE:
  case TOK_SIGNED1: case TOK_SIGNED2: case TOK_SIGNED3: case TOK_UNSIGNED:
    return 0;
  }

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

/* Return the closing angle index when tokens at `start` spell the current
   class template with its own parameters (Self<T, N, ...>).  Alias
   declarations are replayed in bulk, so they do not pass through the normal
   self-template parser that collapses this spelling to the concrete class. */
static int template_self_id_close(TokenString *str, int start, TemplateDef *td)
{
  int i, arg;

  if (!str || !td || start < 0 || start + 1 >= str->len
      || str->str[start] != td->name_tok
      || (str->str[start + 1] != TOK_LT && str->str[start + 1] != '<'))
    return -1;
  i = start + 2;
  for (arg = 0; arg < td->nb_type_params; ++arg)
  {
    if (i >= str->len || str->str[i] != td->type_param_toks[arg])
      return -1;
    ++i;
    if (arg + 1 < td->nb_type_params)
    {
      if (i >= str->len || str->str[i] != ',')
        return -1;
      ++i;
    }
  }
  if (i >= str->len || (str->str[i] != TOK_GT && str->str[i] != '>'))
    return -1;
  return i;
}

static void template_drop_self_qualifier(TokenString *out,
                                         TokenString *source, int self_index)
{
  int qualifier_start = self_index;

  while (qualifier_start >= 3
         && source->str[qualifier_start - 1] == ':'
         && source->str[qualifier_start - 2] == ':'
         && source->str[qualifier_start - 3] >= TOK_UIDENT)
    qualifier_start -= 3;
  if (qualifier_start < self_index
      && out->len >= self_index - qualifier_start)
    out->len -= self_index - qualifier_start;
}

static int template_next_token_is_scope(void)
{
  const int *p = macro_ptr;
  CValue cv;
  int t;

  if (!p)
    return 0;
  do
  {
    TOK_GET(&t, &p, &cv);
  }
  while (t == TOK_LINENUM);
  return t == ':';
}

static int template_next_token_is_angle_open(void)
{
  const int *p = macro_ptr;
  CValue cv;
  int t;

  if (!p)
    return 0;
  do
  {
    TOK_GET(&t, &p, &cv);
  }
  while (t == TOK_LINENUM);
  return t == TOK_LT || t == '<';
}

static int template_type_tok_from_ctype(CType *type)
{
  int bt = type->t &VT_BTYPE;
  int unsigned_tok;

  /* Enum and wchar_t identity is independent of their integer storage. */
  if (IS_ENUM(type->t) || IS_ENUM_VAL(type->t) || (type->t & VT_WCHAR_T))
    return template_ctype_typedef_tok(type);
  unsigned_tok = template_unsigned_scalar_type_tok(type);

  if (unsigned_tok)
    return unsigned_tok;

  if (bt == VT_PTR || bt == VT_FUNC || bt == VT_NULLPTR)
    return template_ctype_typedef_tok(type);
  if (bt == VT_VOID)
    return TOK_VOID;
  if (bt == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
    /* A typedef of an anonymous struct has no usable tag token.  Template
       deduction must recover the visible typedef spelling (for example a
       concrete class-template instantiation typedef) instead of treating
       the argument as undeducible. */
    return struct_tok ? struct_tok : template_ctype_typedef_tok(type);
  }
  if (bt == VT_DOUBLE)
  {
    if (type->t & VT_LONG)
      return template_scalar_typedef_tok("__cpc_type_long_double",
                                         VT_DOUBLE | VT_LONG);
    return TOK_DOUBLE;
  }

  if (bt == VT_LDOUBLE)
    return template_scalar_typedef_tok("__cpc_type_long_double", VT_LDOUBLE);
  if (bt == VT_FLOAT)
    return TOK_FLOAT;
  if (bt == VT_BYTE)
  {
    if (type->t & VT_DEFSIGN)
      return template_scalar_typedef_tok("__cpc_type_signed_char",
                                         VT_BYTE | VT_DEFSIGN);
    return TOK_CHAR;
  }
  if (bt == VT_SHORT)
    return TOK_SHORT;
  if (bt == VT_BOOL)
    return TOK_BOOL;
  if (bt == VT_LLONG)
    return template_scalar_typedef_tok("__cpc_type_signed_long_long",
                                       VT_LLONG | (type->t & VT_DEFSIGN));
  if (bt == VT_INT)
  {
    if (type->t & VT_LONG)
      return TOK_LONG;
    return TOK_INT;
  }
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

static int template_exact_type_tok_from_ctype(CType *type)
{
  return is_reference_type(type) || (type->t & (VT_CONSTANT | VT_VOLATILE))
           ? template_exact_ctype_typedef_tok(type)
           : template_type_tok_from_ctype(type);
}

static int finish_template_type_argument(int type_tok, int qualifiers)
{
  CType type;
  Sym *class_symbol = struct_find(type_tok);

  if (class_symbol && (class_symbol->type.t & VT_BTYPE) == VT_STRUCT)
    type_tok = class_symbol->v & ~SYM_STRUCT;

  while (tok == ':' && make_type_from_type_arg_tok(&type, type_tok)
         && (type.t & VT_BTYPE) == VT_STRUCT && type.ref)
  {
    Sym *member = parse_template_nested_typedef_ex(type.ref, 1);
    if (member->cpp_using_target && find_class_template_def(member->cpp_using_target))
      return member->cpp_using_target;
    if (!(member->type.t & VT_TYPEDEF))
    {
      if (!(member->v & SYM_FIELD) && (member->r & VT_SYM)
          && !member->a.integral_constexpr
          && (tok == ',' || tok == TOK_GT || tok == '>' || tok == TOK_SAR))
        return member->v;
      if (!(member->v & SYM_FIELD))
        unget_tok(member->v);
      return parse_template_constant_expression_arg();
    }
    type = member->type;
    type.t &= ~VT_TYPEDEF;
    type_tok = template_exact_type_tok_from_ctype(&type);
    if (!type_tok)
      type_tok = template_ctype_typedef_tok(&type);
  }

  while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
         || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3) {
    qualifiers |= tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
      ? VT_CONSTANT : VT_VOLATILE;
    next();
  }
  if (!qualifiers && tok != '*' && tok != '&' && tok != TOK_LAND
      && tok != '[' && tok != '(' && tok < TOK_UIDENT)
    return type_tok;
  if (!make_type_from_type_arg_tok(&type, type_tok))
    cprime_error("unsupported template type argument '%s'",
                 get_tok_str(type_tok, NULL));
  parse_btype_qualify(&type, qualifiers);
  {
    AttributeDef attributes = {0};
    int declarator_name = 0;
    /* A template type argument has an ordinary abstract declarator.  Use
       the same array dimensions, function types and reference collapsing
       as a type-id outside a template argument list. */
    type_decl(&type, &attributes, &declarator_name, TYPE_ABSTRACT);
  }
  return template_exact_ctype_typedef_tok(&type);
}

/* A builtin template type-id is a declaration-specifier sequence, not a
   single keyword: long long, short int and long double retain their usual
   meanings here and during dependent replay. */
#include "cprimegen_alias_templates.inc"

static int parse_builtin_template_type_argument(void)
{
  CType type;
  AttributeDef attributes;
  int argument;
  if (!(tok == TOK_VOID || tok == TOK_CHAR || tok == TOK_SHORT || tok == TOK_INT
        || tok == TOK_LONG || tok == TOK_FLOAT || tok == TOK_DOUBLE
        || tok == TOK_BOOL || tok == TOK_BOOL2 || tok == TOK_UNSIGNED
        || tok == TOK_TYPEOF1 || tok == TOK_TYPEOF2 || tok == TOK_TYPEOF3
        || tok == TOK_UNDERLYING_TYPE
        || tok == TOK_SIGNED1 || tok == TOK_SIGNED2 || tok == TOK_SIGNED3))
    return 0;
  memset(&type, 0, sizeof type);
  memset(&attributes, 0, sizeof attributes);
  if (!parse_btype(&type, &attributes, 0))
    cprime_error("invalid builtin template type argument");
  argument = template_exact_type_tok_from_ctype(&type);
  if (!argument) argument = template_exact_ctype_typedef_tok(&type);
  if (!argument) cprime_error("could not represent builtin template argument");
  return argument;
}

static int template_literal_has_expression_tail(void)
{
  TokenString *literal = tok_str_alloc();
  int expression;
  tok_str_add_tok(literal);
  next();
  expression = tok != ',' && tok != TOK_GT && tok != TOK_SAR && tok != TOK_EOF;
  restore_cpp_lifecycle_probe(literal);
  return expression;
}

static int parse_one_template_type_arg(void)
{
  int type_tok;
  int qualifiers = 0;
  int global_scope = 0;

  if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "typename"))
    next();

  if (template_named_cast_tok(tok) || tok == '(' || tok == TOK_SIZEOF
      || tok == TOK_ALIGNOF1 || tok == TOK_ALIGNOF2
      || tok == TOK_ALIGNOF3 || tok == TOK_ALIGNOF4
      || (tok >= TOK_builtin_types_compatible_p && tok <= TOK_builtin_unreachable))
    return parse_template_constant_expression_arg();

  while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
         || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
         || tok == TOK_VOLATILE3)
  {
    if (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
      qualifiers |= VT_CONSTANT;
    else
      qualifiers |= VT_VOLATILE;
    next();
  }
  if (tok == ':')
  {
    next();
    skip(':');
    global_scope = 1;
    if (tok < TOK_UIDENT)
      cprime_error("global template argument name expected");
  }
  try_expand_cpp_alias_template(global_scope);
  if ((type_tok = parse_builtin_template_type_argument()) != 0)
    return finish_template_type_argument(type_tok, qualifiers);
  if (tok == TOK_TRUE || tok == TOK_FALSE)
  {
    if (template_literal_has_expression_tail()) return parse_template_constant_expression_arg();
    type_tok = tok;
    next();
  }
  else if (tok == TOK_CINT || tok == TOK_CUINT || tok == TOK_CCHAR
           || tok == TOK_CLONG || tok == TOK_CULONG
           || tok == TOK_CLLONG || tok == TOK_CULLONG)
  {
    if (template_literal_has_expression_tail()) return parse_template_constant_expression_arg();
    type_tok = template_integer_constant_arg_tok();
  }
  else if (tok == '&')
  {
    type_tok = template_address_argument_tok();
  }
  else if (tok == TOK_SIZEOF || tok == TOK_ALIGNOF1 || tok == TOK_ALIGNOF2
           || tok == TOK_ALIGNOF3 || tok == TOK_ALIGNOF4)
  {
    type_tok = parse_template_constant_expression_arg();
  }
  else if (tok == '+' || tok == '-' || tok == '!' || tok == '~')
  {
    type_tok = parse_template_constant_expression_arg();
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
  else if (tok == TOK_DECLTYPE || tok == TOK_DECLTYPE2)
  {
    CType type;
    AttributeDef ad;
    memset(&type, 0, sizeof(type));
    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      cprime_error("unsupported decltype template argument");
    type_tok = template_exact_type_tok_from_ctype(&type);
    if (!type_tok)
      cprime_error("could not instantiate decltype template argument");
  }
  else if (tok >= TOK_UIDENT)
  {
    int name_tok = tok;
    TemplateDef *td;
    int static_const_tok = global_scope ? 0 : template_static_const_arg_tok(tok);
    if (static_const_tok)
    {
      tok = static_const_tok;
      return parse_template_constant_expression_arg();
    }
    if (!global_scope)
    {
      int nested_tok = find_current_class_nested_type_tok(tok);
      if (nested_tok != tok)
      {
        TemplateDef *nested_template = find_class_template_def(nested_tok);
        next();
        /* A nested class template of the enclosing class spelled
           unqualified in a template argument (X<int> inside Y) keeps its
           argument list and instantiates here. */
        if (nested_template && nested_template->is_class
            && (tok == TOK_LT || tok == '<'))
        {
          int nested_mangled_tok;
          TemplateArgList nested_args;
          parse_template_type_args(&nested_args);
          nested_mangled_tok = instantiate_template_argument_type(nested_template,
                                                                  &nested_args);
          compile_pending_template_specs();
          if (!struct_find(nested_mangled_tok))
            cprime_error("template class instantiation failed for '%s'",
                         get_tok_str(nested_template->name_tok, NULL));
          return finish_template_type_argument(nested_mangled_tok, qualifiers);
        }
        return finish_template_type_argument(nested_tok, qualifiers);
      }
    }
    if (is_namespace_tok(tok))
    {
      int parts[16], nb_parts = 0;
      parts[nb_parts++] = global_scope ? tok : find_current_namespace_tok(tok);
      next();
      while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
      {
        next();
        if (tok != ':')
          cprime_error("qualified template type requires '::'");
        next();
        if (tok < TOK_UIDENT)
          cprime_error("qualified template type name expected (got '%s')",
                       get_tok_str(tok, &tokc));
        parts[nb_parts++] = tok;
        next();
      }
      name_tok = find_namespace_tok_from_parts(parts, nb_parts);
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
             (AdditiveIdentity<float>). */
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
        nested_mangled_tok = instantiate_template_argument_type(td, &nested_args);
        compile_pending_template_specs();
        if (!struct_find(nested_mangled_tok))
          cprime_error("template class instantiation failed for '%s'",
                       get_tok_str(td->name_tok, NULL));
        return finish_template_type_argument(nested_mangled_tok, qualifiers);
      }
      return finish_template_type_argument(global_scope ? template_global_type_spelling(name_tok) : name_tok, qualifiers);
    }
    name_tok = global_scope ? tok : find_current_namespace_tok(tok);
    {
      Sym *value = global_scope ? global_symbol_find(name_tok) : sym_find(name_tok);
      if (!value)
        value = global_symbol_find(name_tok);
      if (value && !(value->type.t & VT_TYPEDEF)
          && is_integer_btype(value->type.t & VT_BTYPE)
          && (!(value->r & VT_SYM) || value->a.integral_constexpr))
      {
        if (global_scope) { unget_tok(':'); unget_tok(':'); }
        return parse_template_constant_expression_arg();
      }
    }
    td = find_class_template_def(name_tok);
    if (!td)
      td = find_class_template_def(tok);
    if (td && td->is_class)
    {
      int nested_mangled_tok;
      TemplateArgList nested_args;
      next();
      if (tok != TOK_LT)
        return td->lookup_tok ? td->lookup_tok : td->name_tok;
      parse_template_type_args(&nested_args);
      nested_mangled_tok = instantiate_template_argument_type(td, &nested_args);
      compile_pending_template_specs();
      if (!struct_find(nested_mangled_tok))
        cprime_error("template class instantiation failed for '%s'",
                  get_tok_str(td->name_tok, NULL));
      type_tok = nested_mangled_tok;
    }
    else
    {
      Sym *alias_sym = global_scope ? global_symbol_find(name_tok) : sym_find(name_tok);
      Sym *global_alias = global_symbol_find(name_tok);
      if (!alias_sym)
        alias_sym = global_symbol_find(name_tok);
      /* Resolve local/class-scope aliases only.  Global typedef names such
         as ui8/i8 must stay distinct template arguments (their
         specializations differ even when the underlying type is char). */
      if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
          && alias_sym != global_alias)
      {
        CType alias_type = alias_sym->type;
        alias_type.t &= ~VT_TYPEDEF;
        type_tok = template_exact_type_tok_from_ctype(&alias_type);
        if (type_tok)
        {
          next();
          return finish_template_type_argument(type_tok, qualifiers);
        }
      }
      type_tok = global_scope ? template_global_type_spelling(name_tok) : name_tok;
      next();
      /* A function template-id is an ordinary non-type argument naming the
         specialization (default_initializer<int> for a
         void init_function(const OBJECT&) parameter).  Instantiate it here so
         the argument list is consumed instead of stopping at '<'. */
      if (tok == TOK_LT || tok == '<')
      {
        TemplateDef *function_template = find_function_template_def(type_tok);
        if (!function_template)
          function_template = find_function_template_def(name_tok);
        if (function_template && !function_template->is_class
            && !function_template->is_alias)
        {
          TemplateArgList function_arguments;
          int instantiated_tok;
          parse_template_type_args(&function_arguments);
          instantiated_tok = instantiate_template_if_needed(function_template,
                                                            &function_arguments);
          compile_pending_template_specs();
          if (sym_find(instantiated_tok) || global_symbol_find(instantiated_tok)
              || struct_find(instantiated_tok))
            return finish_template_type_argument(instantiated_tok, qualifiers);
        }
      }
      if (tok == ':')
      {
        CType qualified_owner;
        if (make_type_from_type_arg_tok(&qualified_owner, type_tok)
            && (qualified_owner.t & VT_BTYPE) == VT_STRUCT)
          return finish_template_type_argument(type_tok, qualifiers);
        int parts[16], nb_parts = 0;
        parts[nb_parts++] = type_tok;
        while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
        {
          next();
          skip(':');
          if (tok < TOK_UIDENT)
            cprime_error("qualified template type name expected (got '%s')",
                         get_tok_str(tok, &tokc));
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
  return finish_template_type_argument(type_tok, qualifiers);
}

static void parse_template_type_args(TemplateArgList *args)
{
  int opening_source_line = file ? file->line_num : 0;
  args->nb = 0;
  skip(TOK_LT);
  if (tok == TOK_GT || tok == TOK_SAR) {
    if (tok == TOK_SAR) { tok = TOK_GT; unget_tok(TOK_GT); }
    next();
    return;
  }
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

static int finish_substituted_template_type_argument(int type_tok,
    TemplateDef *context, TemplateArgList *arguments,
    int *alias_from, int *alias_to, int alias_count)
{
  if (!context || !arguments || (tok != '[' && tok != '(' && tok != '*'
      && tok != '&' && tok != TOK_LAND)) return finish_template_type_argument(type_tok, 0);
  TokenString *suffix = tok_str_alloc();
  int parens = 0, brackets = 0, angles = 0;
  while (tok != TOK_EOF) {
    if (!parens && !brackets && !angles
        && (tok == ',' || tok == TOK_GT || tok == '>' || tok == TOK_SAR)) break;
    if (tok == '(') ++parens;
    else if (tok == ')') --parens;
    else if (tok == '[') ++brackets;
    else if (tok == ']') --brackets;
    else if (!parens && !brackets) {
      if (tok == TOK_LT || tok == '<') ++angles;
      else if (tok == TOK_GT || tok == '>') --angles;
      else if (tok == TOK_SAR) angles -= 2;
    }
    if (TOK_HAS_VALUE(tok)) tok_str_add_tok(suffix);
    else tok_str_add(suffix, template_subst_token(context, arguments, 0,
                              alias_from, alias_to, alias_count, tok));
    next();
  }
  restore_cpp_lifecycle_probe(suffix);
  return finish_template_type_argument(type_tok, 0);
}

static int parse_one_template_type_arg_subst_with_alias(
    TemplateDef *ctx_td, TemplateArgList *ctx_args,
    int *alias_from, int *alias_to, int nb_aliases)
{
  int type_tok;
  int global_scope = 0;

  if (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
      || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3)
  {
    int argument = template_substituted_pattern_argument(ctx_td, ctx_args);
    if (!argument) cprime_error("invalid qualified template argument");
    return argument;
  }

  if (tok >= TOK_UIDENT
      && (!strcmp(get_tok_str(tok, NULL), "typename")
          || !strcmp(get_tok_str(tok, NULL), "class")))
    next();
  if (tok == ':')
  {
    next();
    skip(':');
    global_scope = 1;
    if (tok < TOK_UIDENT)
      cprime_error("global template argument name expected");
  }
  if (template_named_cast_tok(tok) || tok == '(' || tok == TOK_SIZEOF
      || tok == TOK_ALIGNOF1 || tok == TOK_ALIGNOF2
      || tok == TOK_ALIGNOF3 || tok == TOK_ALIGNOF4
      || (tok >= TOK_builtin_types_compatible_p && tok <= TOK_builtin_unreachable))
    return parse_template_constant_expression_arg_subst(
      ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
  if (tok == '&')
    return parse_template_constant_expression_arg_subst(
      ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
  if (tok == TOK_TRUE || tok == TOK_FALSE)
  {
    if (template_literal_has_expression_tail())
      return parse_template_constant_expression_arg_subst(ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
    type_tok = tok;
    next();
    return type_tok;
  }
  if (tok == TOK_CINT || tok == TOK_CUINT || tok == TOK_CCHAR
      || tok == TOK_CLONG || tok == TOK_CULONG
      || tok == TOK_CLLONG || tok == TOK_CULLONG)
  {
    if (template_literal_has_expression_tail())
      return parse_template_constant_expression_arg_subst(ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
    return template_integer_constant_arg_tok();
  }
  if (tok == '+' || tok == '-' || tok == '!' || tok == '~')
    return parse_template_constant_expression_arg_subst(
      ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
  if ((type_tok = parse_builtin_template_type_argument()) != 0)
    return finish_substituted_template_type_argument(type_tok, ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
  if (!global_scope && ctx_td && ctx_args) {
    int parameter;
    for (parameter = 0; parameter < ctx_td->nb_type_params && parameter < ctx_args->nb; ++parameter)
      if (tok == ctx_td->type_param_toks[parameter]
          && (ctx_td->value_param_mask & (1u << parameter))
          && !(global_symbol_find(ctx_args->toks[parameter])
               && global_symbol_find(ctx_args->toks[parameter])->template_address_target))
        return parse_template_constant_expression_arg_subst(
          ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
  }
  type_tok = global_scope ? 0 : parse_template_type_param_subst(ctx_td, ctx_args);
  if (type_tok && tok == TOK_LT && find_class_template_def(type_tok)) {
    TemplateArgList nested;
    TemplateDef *definition = find_class_template_def(type_tok);
    parse_template_type_args_subst(&nested, ctx_td, ctx_args);
    type_tok = instantiate_template_if_needed(definition, &nested);
    compile_pending_template_specs_without_member_flush();
  }
  if (type_tok)
    return finish_substituted_template_type_argument(type_tok, ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
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
  if (tok == TOK_DECLTYPE || tok == TOK_DECLTYPE2)
  {
    CType type;
    AttributeDef ad;
    memset(&type, 0, sizeof(type));
    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      cprime_error("unsupported decltype template argument");
    type_tok = template_exact_type_tok_from_ctype(&type);
    if (!type_tok)
      cprime_error("could not instantiate decltype template argument");
    return type_tok;
  }
  if (tok >= TOK_UIDENT)
  {
    int name_tok = tok;
    TemplateDef *td;
    if (!global_scope && ctx_td && ctx_td->is_class && tok == ctx_td->name_tok
        && template_next_token_is_angle_open())
    {
      TemplateArgList self_args;
      int i, same = 1;
      int self_inst_tok;

      next();
      parse_template_type_args_subst(&self_args, ctx_td, ctx_args);
      if (self_args.nb != ctx_args->nb)
        same = 0;
      for (i = 0; same && i < self_args.nb; ++i)
        if (!template_type_arg_toks_match(self_args.toks[i],
                                          ctx_args->toks[i]))
          same = 0;
      if (!same)
        return instantiate_template_argument_type(ctx_td, &self_args);
      self_inst_tok = template_lookup_inst(ctx_td, ctx_args);
      return self_inst_tok ? self_inst_tok
                           : make_template_inst_name_tok(ctx_td, ctx_args);
    }
    int static_const_tok = global_scope ? 0 : template_static_const_arg_tok(tok);
    if (static_const_tok)
    {
      next();
      return static_const_tok;
    }
    int alias_tok = global_scope ? 0 : template_alias_lookup(alias_from, alias_to, nb_aliases,
                                          tok);
    if (alias_tok)
    {
      next();
      return alias_tok;
    }
    if (is_namespace_tok(tok))
    {
      int parts[16], nb_parts = 0;
      parts[nb_parts++] = global_scope ? tok : find_current_namespace_tok(tok);
      next();
      while (tok == ':' && nb_parts < (int)(sizeof(parts) / sizeof(parts[0])))
      {
        next();
        if (tok != ':')
          cprime_error("qualified template type requires '::'");
        next();
        if (tok < TOK_UIDENT)
          cprime_error("qualified template type name expected (got '%s')",
                       get_tok_str(tok, &tokc));
        parts[nb_parts++] = tok;
        next();
      }
      name_tok = find_namespace_tok_from_parts(parts, nb_parts);
      td = find_class_template_def(name_tok);
      if (tok == TOK_LT
          && (strstr(get_tok_str(name_tok, NULL), "is_floating_point")
              || strstr(get_tok_str(name_tok, NULL), "is_unsigned")
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
           (AdditiveIdentity<float>). */
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
        return global_scope ? template_global_type_spelling(name_tok) : name_tok;
    }
    else
      td = global_scope ? find_class_template_def(tok) : find_class_template_def_in_context(ctx_td, tok);
    if (td && td->is_class)
    {
      int nested_mangled_tok;
      TemplateArgList nested_args;

      if (tok != TOK_LT)
        next();
      if (tok != TOK_LT && tok != '<')
        return td->lookup_tok ? td->lookup_tok : td->name_tok;
        nested_args.nb = 0;
        skip(TOK_LT);
        if (tok != TOK_GT && tok != TOK_SAR)
        for (;;)
        {
          if (nested_args.nb >= (int)(sizeof(nested_args.toks) / sizeof(nested_args.toks[0])))
            cprime_error("too many template type arguments");
          nested_args.toks[nested_args.nb++] =
            parse_one_template_type_arg_subst_with_alias(
                ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
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
      return finish_substituted_template_type_argument(nested_mangled_tok, ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
    }
    {
      Sym *arg_sym = global_scope ? global_symbol_find(tok) : sym_find(tok);
      if (!arg_sym)
        arg_sym = global_symbol_find(tok);
      if (arg_sym && (arg_sym->type.t & VT_TYPEDEF))
      {
        int canonical_tok = global_scope ? template_global_type_spelling(tok)
                                         : template_exact_type_tok_from_ctype(&arg_sym->type);
        if (canonical_tok)
        {
          type_tok = canonical_tok;
          next();
          return type_tok;
        }
      }
    }
    type_tok = global_scope ? template_global_type_spelling(tok) : tok;
    next();
    return finish_substituted_template_type_argument(type_tok, ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
  }
  cprime_error("unsupported template type argument near '%s'", get_tok_str(tok, &tokc));
  return 0;
}

static int parse_one_template_type_arg_subst(TemplateDef *ctx_td,
                                             TemplateArgList *ctx_args)
{
  return parse_one_template_type_arg_subst_with_alias(ctx_td, ctx_args,
                                                      NULL, NULL, 0);
}

static void parse_template_type_args_subst_with_alias(
    TemplateArgList *out_args, TemplateDef *ctx_td, TemplateArgList *ctx_args,
    int *alias_from, int *alias_to, int nb_aliases)
{
  out_args->nb = 0;
  skip(TOK_LT);
  if (tok == TOK_GT || tok == TOK_SAR) {
    if (tok == TOK_SAR) { tok = TOK_GT; unget_tok(TOK_GT); }
    next();
    return;
  }
  for (;;)
  {
    if (out_args->nb >= (int)(sizeof(out_args->toks) / sizeof(out_args->toks[0])))
      cprime_error("too many template type arguments");
    out_args->toks[out_args->nb++] =
      parse_one_template_type_arg_subst_with_alias(
          ctx_td, ctx_args, alias_from, alias_to, nb_aliases);
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

static void parse_template_type_args_subst(TemplateArgList *out_args,
                                           TemplateDef *ctx_td,
                                           TemplateArgList *ctx_args)
{
  parse_template_type_args_subst_with_alias(out_args, ctx_td, ctx_args,
                                            NULL, NULL, 0);
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
    td->inst_arg_lists = cprime_realloc(td->inst_arg_lists,
        td->al_inst * sizeof(TemplateArgList));
    td->inst_name_toks = cprime_realloc(td->inst_name_toks,
        td->al_inst * sizeof(int));
    td->inst_states = cprime_realloc(td->inst_states,
        td->al_inst * sizeof(unsigned char));
  }
  for (i = 0; i < td->nb_type_params; ++i)
    td->inst_type_toks[td->nb_inst * td->nb_type_params + i] =
      i < args->nb ? args->toks[i] : 0;
  td->inst_arg_lists[td->nb_inst] = *args;
  td->inst_name_toks[td->nb_inst] = mangled_tok;
  td->inst_states[td->nb_inst] = 0;
  index_template_inst_owner(td, mangled_tok);
  td->nb_inst++;
}

static int pending_template_spec_index_for_tok(int mangled_tok)
{
  int i;

  for (i = first_pending_template_spec; i < nb_pending_template_specs;
           i = next_pending_template_spec(i + 1))
  {
    if (!pending_template_specs[i])
      continue;
    if (token_string_contains_tok(pending_template_specs[i], mangled_tok))
      return i;
  }
  return -1;
}

static int pending_template_class_definition_index(int mangled_tok)
{
  int i, j;

  for (i = first_pending_template_spec; i < nb_pending_template_specs;
           i = next_pending_template_spec(i + 1))
  {
    TokenString *ts = pending_template_specs[i];
    if (!ts)
      continue;
    for (j = 1; j + 1 < ts->len; ++j)
      if (ts->str[j] == mangled_tok
          && (ts->str[j - 1] == TOK_CLASS
              || ts->str[j - 1] == TOK_STRUCT)
          && ts->str[j + 1] != ';')
        return i;
  }
  return -1;
}

/* Parse one owned generated class while another generated declaration is
   suspended.  This is narrower than recursively draining the whole worklist:
   it is used only when C++ semantics require a complete class interface now,
   as in Template<T>::member_type.  The synthetic fence and explicit cursor
   restore keep decl() from advancing into the suspended specialization. */
static int compile_pending_template_class_definition_now(int mangled_tok)
{
  int index, saved_tok, saved_defer_pending_member_funcs;
  int saved_namespace, saved_member_class, saved_class_depth, saved_class_stack[32];
  int saved_nocode_wanted;
  CValue saved_tokc;
  TokenString *ts, *normalized, *spec_macro_parent;
  const int *spec_macro_ptr;
  jmp_buf *saved_substitution_jump;
  Sym *saved_locals, *saved_globals = NULL, *parameter_frame = NULL;
  Sym **hidden_locals = NULL;
  int saved_scope, hidden_count = 0;

  index = pending_template_class_definition_index(mangled_tok);
  if (index < 0)
    return 0;
  ts = pending_template_specs[index];
  if (!ts)
    return 0;
  /* A base or by-value field demands this layout even if one of its type
     arguments is forward-declared. Pointer-only containers remain valid in
     that case; actual incomplete fields are diagnosed by the class parser. */
  if (pending_template_spec_dependency_toks)
    pending_template_spec_dependency_toks[index] = 0;
  if (!compiling_pending_template_specs)
  {
    compile_pending_template_specs_without_member_flush();
    return struct_find(mangled_tok) && struct_find(mangled_tok)->c >= 0;
  }

  normalized = balance_generated_template_spec(ts);
  if (normalized != ts)
  {
    tok_str_free(ts);
    pending_template_specs[index] = ts = normalized;
  }
  normalized = strip_static_call_synthetic_this(ts);
  if (normalized != ts)
  {
    tok_str_free(ts);
    pending_template_specs[index] = ts = normalized;
  }

  pending_template_specs[index] = NULL;
  first_pending_template_spec = next_pending_template_spec(first_pending_template_spec);
  ++compiled_template_specs;
  saved_tok = tok;
  saved_tokc = tokc;
  saved_defer_pending_member_funcs = defer_pending_member_funcs;
  saved_nocode_wanted = nocode_wanted;
  spec_macro_parent = macro_stack;
  spec_macro_ptr = macro_ptr;
  /* A class demanded by a parameter type still has namespace lifetime.
     Keep its symbols outside the suspended function/parameter frame, as
     the ordinary specialization worklist does. */
  saved_locals = local_stack;
  saved_scope = local_scope;
  if (saved_locals) {
    hidden_locals = sym_hide_local_bindings(saved_locals, &hidden_count);
    sym_relink_local_types(hidden_locals, hidden_count);
  } else if (saved_scope) {
    Sym *frame;
    for (frame = global_stack; frame; frame = frame->prev)
      if (frame->v == SYM_FIELD && frame->sym_scope == 0) {
        saved_globals = global_stack;
        parameter_frame = frame;
        global_stack = frame->prev;
        break;
      }
  }
  local_stack = NULL;
  local_scope = 0;
  defer_pending_member_funcs = 1;
  saved_substitution_jump = cpp_substitution_jump;
  cpp_substitution_jump = NULL;
  saved_namespace = current_namespace_scope_tok();
  saved_member_class = active_member_class_tok;
  saved_class_depth = nb_defining_class_stack;
  memcpy(saved_class_stack, defining_class_stack, sizeof(saved_class_stack));
  enter_namespace_scope(pending_template_spec_namespace_toks[index]);
  active_member_class_tok = 0;
  nb_defining_class_stack = 0;

  if (ts->len > 0 && ts->str[ts->len - 1] == TOK_EOF)
  {
    ts->str[ts->len - 1] = ';';
    tok_str_add(ts, TOK_EOF);
  }
  begin_macro(ts, 1);
  next();
  while (macro_stack == ts)
  {
    if (tok == TOK_EOF || tok == ';')
    {
      end_macro();
      break;
    }
    decl(VT_CONST);
  }
  while (macro_stack && macro_stack != spec_macro_parent)
    end_macro();
  macro_ptr = spec_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
  nocode_wanted = saved_nocode_wanted;
  cpp_substitution_jump = saved_substitution_jump;
  enter_namespace_scope(saved_namespace);
  active_member_class_tok = saved_member_class;
  nb_defining_class_stack = saved_class_depth;
  memcpy(defining_class_stack, saved_class_stack, sizeof(saved_class_stack));
  if (parameter_frame) {
    parameter_frame->prev = global_stack;
    global_stack = saved_globals;
  }
  sym_relink_stack(hidden_locals, hidden_count);
  cprime_free(hidden_locals);
  local_stack = saved_locals;
  local_scope = saved_scope;
  return struct_find(mangled_tok) && struct_find(mangled_tok)->c >= 0;
}

/* A specialization requested while another generated class is being parsed
   cannot recursively drain the pending-specialization worklist: doing so
   would re-enter decl() around a suspended class declaration.  The requesting
   declaration nevertheless only needs the ordinary C++ forward type at this
   point (for example `using value_type = Other<T>`).  Publish that incomplete
   class symbol synchronously; the already-live outer worklist will compile
   the queued full definition before it finishes. */
static void publish_template_forward_type(int mangled_tok, int union_type)
{
  Sym *saved_local_stack;
  Sym *saved_global_stack = NULL, *param_frame_tail = NULL;
  int saved_local_scope;
  CType forward_type;

  if (!mangled_tok || struct_find(mangled_tok))
    return;
  saved_local_stack = local_stack;
  saved_local_scope = local_scope;
  if (!local_stack && local_scope)
  {
    Sym *frame;
    for (frame = global_stack; frame; frame = frame->prev)
      if (frame->v == SYM_FIELD && frame->sym_scope == 0)
      {
        saved_global_stack = global_stack;
        param_frame_tail = frame;
        global_stack = frame->prev;
        break;
      }
  }
  local_stack = NULL;
  local_scope = 0;
  forward_type.t = union_type ? VT_UNION : VT_STRUCT;
  forward_type.ref = NULL;
  sym_push(mangled_tok | SYM_STRUCT, &forward_type, 0, -1);
  if (param_frame_tail)
  {
    param_frame_tail->prev = global_stack;
    global_stack = saved_global_stack;
  }
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
}

#include "cprimegen_substitution.inc"

static int deduce_partial_type_argument(TemplateDef *candidate,
                                         TemplateArgList *bindings,
                                         int actual_tok)
{
  int prefix_cv = 0, wrappers[32], wrapper_cv[32], count = 0;
  int parameter_index, parameter_tok, binding_tok, i, array_dimensions = 0;
  CType actual;
  while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
         || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
         || tok == TOK_VOLATILE3)
  {
    prefix_cv |= (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
                 ? VT_CONSTANT : VT_VOLATILE;
    next();
  }
  parameter_tok = tok;
  parameter_index = template_param_index(candidate, parameter_tok);
  if (parameter_index < 0)
    return 0;
  next();
  if (parameter_index >= 0 && (candidate->value_param_mask & (1u << parameter_index))
      && tok != ',' && tok != TOK_GT && tok != TOK_SAR && tok != TOK_EOF) {
    int concrete;
    /* Arithmetic around a value parameter is a non-deduced context. Once
       that parameter is bound, compare the whole substituted expression
       rather than treating its first name as another direct deduction. */
    if (!bindings->toks[parameter_index]) return 0;
    unget_tok(parameter_tok);
    concrete = template_substituted_pattern_argument(candidate, bindings);
    return concrete && template_type_arg_toks_match(concrete, actual_tok);
  }
  /* `T const` and `const T` denote the same qualified pattern. */
  while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
         || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
         || tok == TOK_VOLATILE3)
  {
    prefix_cv |= (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
                 ? VT_CONSTANT : VT_VOLATILE;
    next();
  }
  /* A member-pointer pattern names the member type and its class separately:
     `T K::*` deduces T from the member and K from the owner, and
     `R (K::*)(T1) [cv]` does the same for a member function pointer. The
     compiler stores these as two-word member-pointer records, so the pattern
     is read textually while the argument's record supplies the types. */
  if (parameter_index >= 0 && macro_ptr
      && !(candidate->value_param_mask & (1u << parameter_index)))
  {
    const int *peek = macro_ptr;
    CValue peek_value;
    int form = 0, q;
    if (tok >= TOK_UIDENT)
    {
      int first = 0, second = 0, third = 0;
      TOK_GET(&first, &peek, &peek_value);
      while (first == TOK_LINENUM) TOK_GET(&first, &peek, &peek_value);
      TOK_GET(&second, &peek, &peek_value);
      while (second == TOK_LINENUM) TOK_GET(&second, &peek, &peek_value);
      TOK_GET(&third, &peek, &peek_value);
      while (third == TOK_LINENUM) TOK_GET(&third, &peek, &peek_value);
      if (first == ':' && second == ':' && third == '*') form = 1;
    }
    else if (tok == '(')
    {
      int look[5];
      for (q = 0; q < 5; ++q) {
        TOK_GET(&look[q], &peek, &peek_value);
        while (look[q] == TOK_LINENUM) TOK_GET(&look[q], &peek, &peek_value);
      }
      if (look[0] >= TOK_UIDENT && look[1] == ':' && look[2] == ':'
          && look[3] == '*' && look[4] == ')') form = 2;
    }
    if (form)
    {
      CType actual_type, owner_type;
      CppMemberPointerType *member;
      int owner_token, owner_index, owner_binding, member_binding = 0;
      if (prefix_cv || count
          || !make_type_from_type_arg_tok(&actual_type, actual_tok))
        return 0;
      member = find_cpp_member_pointer_type(&actual_type);
      if (!member) return 0;
      if (form == 1)
      {
        owner_token = tok;
        member_binding = template_exact_ctype_typedef_tok(&member->member_type);
        if (!member_binding)
          member_binding = template_exact_type_tok_from_ctype(&member->member_type);
        if (!member_binding) return 0;
        if (bindings->toks[parameter_index]
            && !template_type_arg_toks_match(bindings->toks[parameter_index],
                                             member_binding))
          return 0;
        bindings->toks[parameter_index] = member_binding;
        next(); next(); next(); next();
      }
      else
      {
        CType signature;
        Sym *parameter;
        int member_type_tok, trailing_cv = 0;
        if ((member->member_type.t & VT_BTYPE) != VT_FUNC) return 0;
        member_type_tok = template_exact_ctype_typedef_tok(&member->member_type);
        if (!member_type_tok
            || !make_type_from_type_arg_tok(&signature, member_type_tok)
            || (signature.t & VT_BTYPE) != VT_FUNC || !signature.ref)
          return 0;
        parameter = signature.ref->next;
        next();
        owner_token = tok;
        next();
        if (tok != ':') return 0;
        next();
        if (tok != ':') return 0;
        next();
        if (tok != '*') return 0;
        next();
        if (tok != ')') return 0;
        next();
        if (tok != '(') return 0;
        next();
        while (tok != ')' && tok != TOK_EOF)
        {
          int pack = template_param_index(candidate, tok);
          if (pack >= 0 && pack == candidate->variadic_param_index)
          {
            next();
            if (tok != TOK_DOTS) return 0;
            next();
            bindings->nb = pack;
            while (parameter)
            {
              if (bindings->nb == CPC_MAX_TEMPLATE_ARGUMENTS) return 0;
              bindings->toks[bindings->nb++] =
                template_exact_ctype_typedef_tok(&parameter->type);
              parameter = parameter->next;
            }
            break;
          }
          if (!parameter) return 0;
          if (!deduce_partial_type_argument(candidate, bindings,
                 template_exact_ctype_typedef_tok(&parameter->type))) return 0;
          parameter = parameter->next;
          if (tok != ',') break;
          next();
        }
        if (tok != ')' || parameter) return 0;
        next();
        while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
               || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
               || tok == TOK_VOLATILE3)
        {
          trailing_cv |=
            (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
            ? VT_CONSTANT : VT_VOLATILE;
          next();
        }
        if ((signature.t & (VT_CONSTANT | VT_VOLATILE)) != trailing_cv)
          return 0;
        actual_tok = template_exact_ctype_typedef_tok(&signature.ref->type);
        if (!actual_tok) return 0;
      }
      if (!make_class_type_from_tok(&owner_type, member->owner_tok)) return 0;
      owner_index = template_param_index(candidate, owner_token);
      if (owner_index >= 0)
      {
        owner_binding = template_type_tok_from_ctype(&owner_type);
        if (!owner_binding)
          owner_binding = template_exact_ctype_typedef_tok(&owner_type);
        if (!owner_binding) return 0;
        if (bindings->toks[owner_index]
            && !template_type_arg_toks_match(bindings->toks[owner_index],
                                             owner_binding))
          return 0;
        bindings->toks[owner_index] = owner_binding;
      }
      else
      {
        int owner_name_tok = get_struct_type_name_tok(&owner_type);
        if (!owner_name_tok
            || strcmp(get_tok_str(owner_name_tok, NULL),
                      get_tok_str(owner_token, NULL)))
          return 0;
      }
      if (form == 1) return 1;
    }
  }
  while (tok == '*' || tok == '&' || tok == TOK_LAND)
  {
    int qualifier = 0;
    if (count == 32)
      cprime_error("partial specialization type is too deeply nested");
    wrappers[count] = tok;
    next();
    while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
           || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
           || tok == TOK_VOLATILE3)
    {
      qualifier |= (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
                    ? VT_CONSTANT : VT_VOLATILE;
      next();
    }
    wrapper_cv[count++] = qualifier;
  }
  if (tok == '(')
  {
    CType signature;
    Sym *parameter;
    if (!make_type_from_type_arg_tok(&signature, actual_tok)
        || (signature.t & VT_BTYPE) != VT_FUNC || !signature.ref)
      return 0;
    parameter = signature.ref->next;
    next();
    while (tok != ')' && tok != TOK_EOF)
    {
      int pack = template_param_index(candidate, tok);
      if (pack >= 0 && pack == candidate->variadic_param_index)
      {
        next();
        if (tok != TOK_DOTS) return 0;
        next();
        bindings->nb = pack;
        while (parameter)
        {
          if (bindings->nb == CPC_MAX_TEMPLATE_ARGUMENTS) return 0;
          bindings->toks[bindings->nb++] = template_exact_ctype_typedef_tok(&parameter->type);
          parameter = parameter->next;
        }
        break;
      }
      if (!parameter) return 0;
      if (!deduce_partial_type_argument(candidate, bindings,
             template_exact_ctype_typedef_tok(&parameter->type))) return 0;
      parameter = parameter->next;
      if (tok != ',') break;
      next();
    }
    if (tok != ')' || parameter) return 0;
    next();
    actual_tok = template_exact_ctype_typedef_tok(&signature.ref->type);
  }
  /* Array bounds are deduced before the element's pointer/cv pattern.
     A typedef naming the array retains the same array type for deduction. */
  if (tok == '[') {
    if (!make_type_from_type_arg_tok(&actual, actual_tok)) return 0;
    while (tok == '[') {
      int bound_parameter;
      long long bound;
      if (!(actual.t & VT_ARRAY) || !actual.ref) return 0;
      bound = actual.ref->c;
      next();
      bound_parameter = template_param_index(candidate, tok);
      if (tok == ']') {
        if (bound >= 0) return 0;
      } else if (bound_parameter >= 0
                 && (candidate->value_param_mask & (1u << bound_parameter))) {
        int64_t previous_bound;
        if (bound < 0) return 0;
        if (bindings->toks[bound_parameter]
            && (!template_arg_const_int64(bindings->toks[bound_parameter], &previous_bound)
                || previous_bound != bound)) return 0;
        bindings->toks[bound_parameter] = template_integer_expression_arg_tok(bound);
        next();
      } else {
        if (bound_parameter >= 0 || bound < 0 || expr_const64() != bound) return 0;
      }
      if (tok != ']') return 0;
      next();
      actual = *pointed_type(&actual);
      ++array_dimensions;
    }
  }
  binding_tok = actual_tok;
  if (count || prefix_cv || array_dimensions)
  {
    if (!array_dimensions && !make_type_from_type_arg_tok(&actual, actual_tok))
      return 0;
    for (i = count - 1; i >= 0; --i)
    {
      if ((actual.t & VT_BTYPE) != VT_PTR || !actual.ref
          || (actual.t & (VT_CONSTANT | VT_VOLATILE)) != wrapper_cv[i])
        return 0;
      if (wrappers[i] == '*')
      {
        if (actual.t & (VT_REFERENCE | VT_ARRAY))
          return 0;
      }
      else if (!(actual.t & VT_REFERENCE)
               || ((actual.t & VT_RVALUE_REFERENCE) != 0)
                   != (wrappers[i] == TOK_LAND))
        return 0;
      actual = *pointed_type(&actual);
    }
    if (prefix_cv && ((actual.t & VT_BTYPE) == VT_FUNC
                      || is_reference_type(&actual)))
      return 0;
    if ((actual.t & prefix_cv) != prefix_cv)
      return 0;
    actual.t &= ~prefix_cv;
    binding_tok = template_exact_type_tok_from_ctype(&actual);
    if (!binding_tok)
      binding_tok = template_exact_ctype_typedef_tok(&actual);
  }
  if (bindings->toks[parameter_index]
      && !template_type_arg_toks_match(bindings->toks[parameter_index],
                                       binding_tok))
    return 0;
  bindings->toks[parameter_index] = binding_tok;
  return 1;
}

static TemplateDef *partial_pattern_class(TemplateDef *candidate, int name)
{
  if (name < TOK_UIDENT) return NULL;
  TemplateDef *definition = candidate->namespace_tok
    ? find_class_template_def(make_static_member_tok(candidate->namespace_tok, name)) : NULL;
  return definition ? definition : find_class_template_def(name);
}

static int partial_pattern_template_parameter(TemplateDef *candidate, int name)
{
  int parameter = template_param_index(candidate, name);
  return parameter >= 0 && candidate->value_type_strs[parameter]
      && token_string_starts_template(candidate->value_type_strs[parameter])
    ? parameter : -1;
}

/* Deduce nested template arguments structurally. Dependent pattern types
   must never be instantiated merely to compare a partial specialization. */
static int resolve_partial_pattern_qualifier(TemplateDef *candidate,
                                              TemplateArgList *bindings)
{
  if (tok < TOK_UIDENT) return 0;
  TokenString *prefix = tok_str_alloc(), *parent = macro_stack;
  const int *parent_ptr = macro_ptr;
  int saved_tok = tok, angles = 0, parens = 0, brackets = 0, nested = 0;
  CValue saved_value = tokc;
  CType owner;
  if (!strcmp(get_tok_str(tok, NULL), "typename")) next();
  while (tok != TOK_EOF) {
    if (!angles && !parens && !brackets
        && (tok == ':' || tok == ',' || tok == TOK_GT || tok == TOK_SAR)) break;
    if (tok == '(') ++parens;
    else if (tok == ')') --parens;
    else if (tok == '[') ++brackets;
    else if (tok == ']') --brackets;
    else if (!parens && !brackets) {
      if (tok == TOK_LT) ++angles;
      else if (tok == TOK_GT) --angles;
      else if (tok == TOK_SAR) angles -= 2;
    }
    if (TOK_HAS_VALUE(tok)) tok_str_add_tok(prefix);
    else tok_str_add(prefix, template_subst_token(candidate, bindings, 0,
                                                   NULL, NULL, 0, tok));
    next();
  }
  tok_str_add(prefix, TOK_EOF);
  if (tok == ':' && template_probe_type(prefix, 0, candidate->namespace_tok, &owner)
      && (owner.t & VT_BTYPE) == VT_STRUCT) {
    materialize_incomplete_template_type(&owner);
    next();
    if (tok == ':') {
      next();
      if (is_template_keyword_tok(tok)) next();
      if (tok >= TOK_UIDENT) {
        nested = make_static_member_tok(get_struct_type_name_tok(&owner), tok);
        if (!find_class_template_def(nested)) nested = 0;
      }
    }
  }
  tok_str_free(prefix);
  if (nested) { tok = nested; return 1; }
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr; tok = saved_tok; tokc = saved_value;
  return 0;
}

static int deduce_nested_class_pattern(TemplateDef *candidate,
                                      TemplateArgList *bindings, int actual_tok)
{
  TemplateDef *pattern = partial_pattern_class(candidate, tok), *actual_definition;
  CType actual;
  TemplateArgList actual_arguments;
  int class_tok, i, index = 0;
  const CppRecordDeclInfo *record;
  int template_parameter = partial_pattern_template_parameter(candidate, tok);
  if ((!pattern && template_parameter < 0) || !template_next_token_is_angle_open()) return 0;
  if (!make_type_from_type_arg_tok(&actual, actual_tok)
      || (actual.t & VT_BTYPE) != VT_STRUCT) return 0;
  class_tok = get_struct_type_name_tok(&actual);
  record = lookup_cpp_record_decl(class_tok);
  actual_definition = record && record->template_definition
      ? record->template_definition : find_class_template_def_for_class_tok(class_tok);
  if (!actual_definition || (pattern && actual_definition->lookup_tok != pattern->lookup_tok))
    return 0;
  if (template_parameter >= 0) {
    int binding = template_def_lookup_tok(actual_definition);
    if (bindings->toks[template_parameter]
        && !template_type_arg_toks_match(bindings->toks[template_parameter], binding)) return 0;
    bindings->toks[template_parameter] = binding;
  }
  if (record && record->template_definition)
    actual_arguments = record->template_arguments;
  else {
    for (i = 0; i < actual_definition->nb_inst; ++i)
      if (actual_definition->inst_name_toks[i] == class_tok) break;
    if (i == actual_definition->nb_inst) return 0;
    actual_arguments = actual_definition->inst_arg_lists[i];
  }
  next(); skip(TOK_LT);
  while (tok != TOK_GT && tok != TOK_SAR && tok != TOK_EOF) {
    int parameter = template_param_index(candidate, tok);
    if (index >= actual_arguments.nb) return 0;
    if (partial_pattern_template_parameter(candidate, tok) >= 0
        && template_next_token_is_angle_open()) {
      if (!deduce_nested_class_pattern(candidate, bindings, actual_arguments.toks[index]))
        return 0;
    } else if (parameter >= 0 || tok == TOK_CONST1 || tok == TOK_CONST2
        || tok == TOK_CONST3 || tok == TOK_VOLATILE1
        || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3) {
      if (!deduce_partial_type_argument(candidate, bindings, actual_arguments.toks[index]))
        return 0;
    } else if (partial_pattern_class(candidate, tok) && template_next_token_is_angle_open()) {
      if (!deduce_nested_class_pattern(candidate, bindings, actual_arguments.toks[index]))
        return 0;
    } else {
      int concrete = template_substituted_pattern_argument(candidate, bindings);
      if (!concrete || !template_type_arg_toks_match(concrete, actual_arguments.toks[index]))
        return 0;
    }
    ++index;
    if (tok != ',') break;
    next();
  }
  if (index != actual_arguments.nb) return 0;
  if (tok == TOK_SAR) { tok = TOK_GT; unget_tok(TOK_GT); }
  if (tok != TOK_GT) return 0;
  next();
  return 1;
}

static int compare_class_partial_template_order(TemplateDef *first,
                                                TemplateDef *second);

static TemplateDef *matching_partial_class_template(TemplateDef *primary,
                                                     TemplateArgList *args,
                                                     TemplateArgList *result_bindings)
{
  struct PartialMatch {
    TemplateDef *definition;
    TemplateArgList bindings;
  } *matches = NULL;
  int d, count = 0, capacity = 0, selected = -1;
  TemplateDef *result = NULL;
  TemplateDef *candidate;

  if (!primary || !primary->is_class || primary->is_partial_specialization)
    return NULL;
  for (candidate = template_def_candidates(primary->lookup_tok);
       candidate; candidate = candidate->lookup_bucket_next)
  {
    TemplateArgList candidate_bindings, *bindings = &candidate_bindings;
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
    memset(bindings, 0, sizeof(*bindings));
    bindings->nb = candidate->nb_type_params;
    {
      TokenString *replay = tok_str_alloc();
      TokenString *saved_macro_stack = macro_stack;
      const int *saved_macro_ptr = macro_ptr;
      int saved_tok = tok;
      CValue saved_tokc = tokc;
      for (i = p; i < candidate->def_str->len; ++i)
        tok_str_add_record(replay, candidate->def_str->str,
                           candidate->def_str->len, &i);
      tok_str_add(replay, TOK_EOF);
      begin_macro(replay, 1);
      next();
      while (tok != TOK_EOF && tok != TOK_GT && tok != TOK_SAR)
      {
        while (resolve_partial_pattern_qualifier(candidate, bindings)) {}
        int t = tok;
        int is_param = template_param_index(candidate, t) >= 0
                       || tok == TOK_CONST1 || tok == TOK_CONST2
                       || tok == TOK_CONST3 || tok == TOK_VOLATILE1
                       || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3;
        if (candidate->variadic_param_index >= 0
            && template_param_index(candidate, t) == candidate->variadic_param_index)
        {
          int pack = candidate->variadic_param_index;
          next();
          if (tok != TOK_DOTS) { matched = 0; break; }
          next();
          if (tok != TOK_GT && tok != TOK_SAR) { matched = 0; break; }
          bindings->nb = pack;
          while (nb < args->nb) {
            if (bindings->nb >= CPC_MAX_TEMPLATE_ARGUMENTS) { matched = 0; break; }
            bindings->toks[bindings->nb++] = args->toks[nb++];
          }
          break;
        }
        if (nb >= CPC_MAX_TEMPLATE_ARGUMENTS || nb >= args->nb)
        {
          matched = 0;
          break;
        }
        if (partial_pattern_template_parameter(candidate, t) >= 0
            && template_next_token_is_angle_open())
        {
          if (!deduce_nested_class_pattern(candidate, bindings, args->toks[nb]))
          { matched = 0; break; }
        }
        else if (is_param)
        {
          if (!deduce_partial_type_argument(candidate, bindings,
                                            args->toks[nb]))
          {
            matched = 0;
            break;
          }
        }
        else
        {
          TemplateDef *pattern_class = partial_pattern_class(candidate, t);
          if (pattern_class && template_next_token_is_angle_open())
          {
            if (!deduce_nested_class_pattern(candidate, bindings, args->toks[nb]))
            { matched = 0; break; }
            is_param = 1;
          }
          else
          {
            int pattern_tok = t;
            t = template_substituted_pattern_argument(candidate, bindings);
            if (!t || !template_type_arg_toks_match(t, args->toks[nb]))
            {
              /* A pattern argument that names a member of the enclosing class
                 (`B<C, const U*>` inside `template<class T> struct A`) is
                 dependent on that class, so the pattern keeps the member's
                 unqualified spelling while the argument carries the
                 instantiated owner token (`A__int_C`). Accept the argument
                 when it is a member of the very class that owns this partial
                 specialization and names the same member. */
              const CppRecordDeclInfo *argument_record =
                pattern_tok >= TOK_UIDENT
                  ? lookup_cpp_record_decl(args->toks[nb]) : NULL;
              const CppRecordDeclInfo *candidate_record =
                lookup_cpp_record_decl(candidate->name_tok);
              const char *member = get_tok_str(pattern_tok, NULL);
              const char *actual = get_tok_str(args->toks[nb], NULL);
              size_t member_length = strlen(member), actual_length = strlen(actual);
              if (argument_record && candidate_record
                  && argument_record->owner_tok
                  && argument_record->owner_tok == candidate_record->owner_tok
                  && actual_length > member_length
                  && actual[actual_length - member_length - 1] == '_'
                  && !strcmp(actual + actual_length - member_length, member))
                t = args->toks[nb];
            }
          }
          if (!is_param && !t) matched = 0;
        }
        if (!is_param && !template_type_arg_toks_match(t, args->toks[nb]))
          matched = 0;
        ++nb;
        if (tok != ',')
          break;
        next();
      }
      if (tok != TOK_GT && tok != TOK_SAR)
        matched = 0;
      while (macro_stack && macro_stack != saved_macro_stack)
        end_macro();
      macro_ptr = saved_macro_ptr;
      tok = saved_tok;
      tokc = saved_tokc;
    }
    if (matched && nb == args->nb)
    {
      for (i = 0; i < bindings->nb; ++i)
        if (!bindings->toks[i])
          matched = 0;
      if (matched) {
        if (count == capacity) {
          capacity = capacity ? capacity * 2 : 4;
          matches = cprime_realloc(matches, capacity * sizeof(*matches));
        }
        matches[count].definition = candidate;
        matches[count++].bindings = *bindings;
      }
    }
  }
  for (d = 0; d < count; ++d) {
    int other;
    for (other = 0; other < count; ++other)
      if (other != d
          && compare_class_partial_template_order(matches[d].definition,
                          matches[other].definition) <= 0)
        break;
    if (other == count) {
      selected = d;
      break;
    }
  }
  if (selected >= 0) {
    result = matches[selected].definition;
    *result_bindings = matches[selected].bindings;
  }
  cprime_free(matches);
  if (count && selected < 0)
    cprime_error("ambiguous partial specialization of '%s'",
                 get_tok_str(primary->lookup_tok, NULL));
  return result;
}

static int template_type_has_internal_linkage(CType *type, int depth)
{
  int bt = type->t & VT_BTYPE;
  if (depth > 64)
    return 1;
  if (bt == VT_PTR && type->ref)
    return template_type_has_internal_linkage(pointed_type(type), depth + 1);
  if (bt == VT_FUNC && type->ref)
  {
    Sym *part;
    for (part = type->ref; part; part = part->next)
      if (template_type_has_internal_linkage(&part->type, depth + 1))
        return 1;
  }
  if ((bt == VT_STRUCT || IS_ENUM(type->t) || IS_ENUM_VAL(type->t))
      && type->ref)
  {
    int name = type->ref->v & ~SYM_STRUCT;
    TemplateInstOwner *owner;
    if (type->ref->sym_scope > 0 || symbol_has_internal_namespace_linkage(name))
      return 1;
    owner = find_template_inst_owner_entry(name);
    if (owner && owner->owner->inst_arg_lists)
    {
      TemplateDef *definition = owner->owner;
      int instance, argument;
      for (instance = 0; instance < definition->nb_inst; ++instance)
        if (definition->inst_name_toks[instance] == name)
        {
          TemplateArgList *args = &definition->inst_arg_lists[instance];
          for (argument = 0; argument < args->nb; ++argument)
          {
            CType actual;
            if (make_type_from_type_arg_tok(&actual, args->toks[argument])
                && template_type_has_internal_linkage(&actual, depth + 1))
              return 1;
          }
          break;
        }
    }
  }
  return 0;
}

static int template_arguments_have_internal_linkage(TemplateArgList *arguments)
{
  int i;
  for (i = 0; i < arguments->nb; ++i)
  {
    CType type;
    if (make_type_from_type_arg_tok(&type, arguments->toks[i])
        && template_type_has_internal_linkage(&type, 0))
      return 1;
  }
  return 0;
}

static int instantiate_template_named_if_needed(TemplateDef *td,
                                                 TemplateArgList *args,
                                                 int specialization_name_tok);

static int instantiate_template_if_needed(TemplateDef *td,
                                          TemplateArgList *args)
{
  return instantiate_template_named_if_needed(td, args, 0);
}

static int instantiate_template_named_if_needed(TemplateDef *td,
                                                 TemplateArgList *args,
                                                 int specialization_name_tok)
{
  int mangled_tok;
  TokenString *spec;
  int saved_tok;
  CValue saved_tokc;
  int alias_from[64], alias_to[64], nb_aliases = 0;
  Sym *type_sym, *ident_sym;
  int type_tok;
  int function_decl_name_done = 0;
  int function_params_started = 0;
  int function_params_done = 0;
  int function_param_depth = 0;
  int function_trailing_return = 0;
  int trailing_return_paren = 0;
  int function_body_started = 0;
  int function_interface_len = 0;
  int class_body_started = 0;
  int class_member_access = 0, class_brace_depth = 0;
  int class_replay_owner[128] = {0}, pending_class_owner = 0;
  int pack_type_tok = 0;
  int pack_start = -1;
  int pack_name_tok = 0;
  int class_arg_incomplete = 0;
  int incomplete_dependency_tok = 0;
  int existing_class_inst_tok = 0;
  int preparing_class_index = -1;
  int materialize_existing_function_body = 0;
  int materialize_function_body = 0;
  TokenString definition_replay;
  TokenString *expanded_pack_body = NULL;
  TokenString *expanded_class_definition = NULL;

  if (!td)
    cprime_error("instantiate_template_if_needed called with NULL template");

  /* Defaults participate in partial-specialization matching as actual
     arguments, including detection idioms with a defaulted second type. */
  template_validate_arg_count(td, args);
  if (td->is_alias)
    return instantiate_cpp_alias_template(td, args);
  materialize_function_body = !td->is_class && td->has_body
                              && ((finalizing_template_bodies
                                   && !local_stack && !local_scope)
                                  || constexpr_function_materialization);
  if (td->is_class && args->nb > 0)
  {
    Sym *arg_sym = struct_find(args->toks[0]);
    CType arg_type;
    int defining_arg_tok = args->toks[0];
    int arg_layout_incomplete = 0;
    if (make_type_from_type_arg_tok(&arg_type, args->toks[0])
        && (arg_type.t & VT_BTYPE) == VT_STRUCT && arg_type.ref)
    {
      int arg_align;
      arg_sym = arg_type.ref;
      defining_arg_tok = get_struct_type_name_tok(&arg_type);
      arg_layout_incomplete = type_size(&arg_type, &arg_align) < 0;
    }
    class_arg_incomplete = arg_layout_incomplete
                           || (arg_sym && arg_sym->c < 0)
                           || is_currently_defining_class(defining_arg_tok);
    if (class_arg_incomplete && !is_currently_defining_class(defining_arg_tok))
      incomplete_dependency_tok = defining_arg_tok;
  }
  if (td->is_class && !td->is_partial_specialization)
  {
    TemplateArgList bindings;
    TemplateDef *partial = matching_partial_class_template(td, args,
                                                           &bindings);
    if (partial)
    {
      TemplateDef *selected = partial->expanded_partial;
      if (!selected)
      {
        /* Instantiation owners and deferred bodies retain this definition.
           A stack copy leaves those references dangling after selection. */
        selected = cprime_mallocz(sizeof(*selected));
        *selected = *partial;
        selected->partial_primary = td;
        selected->type_param_toks = cprime_malloc(partial->nb_type_params * sizeof(int));
        memcpy(selected->type_param_toks, partial->type_param_toks,
               partial->nb_type_params * sizeof(int));
        memset(selected->default_arg_strs, 0, sizeof(selected->default_arg_strs));
        selected->nb_type_params = partial->nb_type_params;
        selected->nb_required_type_params = partial->nb_required_type_params;
        selected->variadic_param_index = partial->variadic_param_index;
        selected->inst_type_toks = NULL;
        selected->inst_arg_lists = NULL;
        selected->inst_name_toks = NULL;
        selected->inst_states = NULL;
        selected->nb_inst = selected->al_inst = 0;
        partial->expanded_partial = selected;
      }
      {
        int concrete_name = make_template_inst_name_tok(td, args);
        /* The specialization's binding list describes its implementation.
           External identity retains the primary template's actual arguments. */
        note_cpp_record_template(concrete_name, td, args);
        return instantiate_template_named_if_needed(selected, &bindings, concrete_name);
      }
    }
  }

  if (td->is_class)
  {
    int spec_index;
    int i, j;
    int existing_inst;

    /* An explicit specialization may have been registered with a typedef
       spelling of the argument (e.g. AdditiveIdentity<i32> while the call
       spells AdditiveIdentity<int>).  Match by underlying type before
       building a new instance name. */
    existing_inst = template_lookup_inst(td, args);
    if (existing_inst)
    {
      existing_class_inst_tok = existing_inst;
      for (i = 0; i < td->nb_inst; ++i)
        if (td->inst_name_toks[i] == existing_inst && (td->inst_states[i] & 128))
        {
          /* Alias declarations may refer back to a specialization whose
             token replay is still being assembled. Reuse its identity and
             the aliases already published instead of rebuilding recursively. */
          publish_template_forward_type(existing_inst, td->is_union);
          return existing_inst;
        }
      if (struct_find(existing_inst)
          && (struct_find(existing_inst)->c >= 0
              || struct_find(existing_inst)->c == -2))
        return existing_inst;
      /* The matching explicit specialization may still be queued as a
         pending spec (e.g. a macro-expanded template<> declaration is
         deferred while the macro is active).  Compile pending specs so the
         struct materializes before the call proceeds. */
      if (!class_arg_incomplete
          && pending_template_spec_index_for_tok(existing_inst) >= 0)
      {
        compile_pending_template_specs_without_member_flush();
        if (struct_find(existing_inst))
          return existing_inst;
      }
    }
    mangled_tok = existing_class_inst_tok ? existing_class_inst_tok
                  : specialization_name_tok ? specialization_name_tok
                                          : make_template_inst_name_tok(td, args);
    note_cpp_record_template(mangled_tok, td, args);
    {
      const CppRecordDeclInfo *origin = lookup_cpp_record_decl(td->lookup_tok);
      note_cpp_record_decl(mangled_tok,
          origin ? origin->source_name_tok : td->name_tok,
          origin ? origin->owner_tok : 0, td->namespace_tok);
    }
    /* A recursive reference to a class currently being defined uses its
       published identity; replaying the definition would redefine that class. */
    if (struct_find(mangled_tok) && struct_find(mangled_tok)->c == -2)
      return mangled_tok;
    if (struct_find(mangled_tok) && struct_find(mangled_tok)->c >= 0)
    {
      if (!class_arg_incomplete)
        instantiate_template_members_for_class(td, args->toks[0], mangled_tok);
      return mangled_tok;
    }
    spec_index = pending_template_spec_index_for_tok(mangled_tok);
    if (spec_index >= 0)
    {
      if (!class_arg_incomplete)
      {
        compile_pending_template_specs_without_member_flush();
        if (struct_find(mangled_tok)
            && struct_find(mangled_tok)->c >= 0)
        {
          instantiate_template_members_for_class(td, args->toks[0],
                                                 mangled_tok);
          return mangled_tok;
        }
      }
      /* A queued work item owns materialization even while its dependency is
         incomplete.  Do not enqueue the same mangled class through another
         equivalent TemplateDef or a recursive use inside its own body. */
      publish_template_forward_type(mangled_tok, td->is_union);
      return mangled_tok;
    }
    if (class_alias_layout_depth && tok != ':')
    {
      /* A plain type alias needs only the specialization's identity. Its
         class body may refer to aliases declared later in the owner. */
      if (!existing_class_inst_tok) note_template_inst(td, args, mangled_tok);
      publish_template_forward_type(mangled_tok, td->is_union);
      return mangled_tok;
    }
  }

  if (!td->is_class)
  {
    int i;
    mangled_tok = template_lookup_inst(td, args);
    if (mangled_tok)
    {
      for (i = 0; i < td->nb_inst; ++i)
        if (td->inst_name_toks[i] == mangled_tok)
          break;
      if (!materialize_function_body || i == td->nb_inst
          || (td->inst_states && td->inst_states[i] >= 2))
        return mangled_tok;
      materialize_existing_function_body = 1;
      if (td->inst_states)
        td->inst_states[i] = 2;
    }
  }

  if (!materialize_existing_function_body)
    mangled_tok = existing_class_inst_tok
                    ? existing_class_inst_tok
                    : specialization_name_tok ? specialization_name_tok
                                               : make_template_inst_name_tok(td, args);
  if (!td->is_class)
  {
    int pending_index;
    if (!materialize_existing_function_body
        && (sym_find(mangled_tok) || global_symbol_find(mangled_tok)))
      return mangled_tok;
    pending_index = pending_template_spec_index_for_tok(mangled_tok);
    if (!materialize_existing_function_body && pending_index >= 0)
      return mangled_tok;
  }
  if (!materialize_existing_function_body)
  {
    note_template_inst(td, args, mangled_tok);
    if (td->is_class)
    {
      preparing_class_index = td->nb_inst - 1;
      td->inst_states[preparing_class_index] = 128;
    }
    if (!td->is_class && td->inst_states)
      td->inst_states[td->nb_inst - 1] = materialize_function_body ? 2 : 1;
  }
  if (td->is_class && !struct_find(mangled_tok))
  {
    if (compiling_pending_template_specs)
      publish_template_forward_type(mangled_tok, td->is_union);
    else
    {
      TokenString *forward = tok_str_alloc();
      tok_str_add(forward, td->is_union ? TOK_UNION : TOK_STRUCT);
      tok_str_add(forward, mangled_tok);
      tok_str_add(forward, ';');
      tok_str_add(forward, TOK_EOF);
      queue_pending_template_spec_in_namespace(forward, 0, td->namespace_tok);
      compile_pending_template_specs_without_member_flush();
    }
  }
  /* C++ permits an incomplete class as a template argument whenever the
     specialization itself does not require its layout.  This is the usual
     CRTP shape (Base<Derived> while Derived is being defined).  Materialize
     the specialization now; ordinary field layout will still reject a truly
     invalid by-value use of the incomplete argument. */
  spec = tok_str_alloc();
  if (td->friend_class_tok) tok_str_add(spec, tok_alloc_const("friend"));
  type_tok = args->toks[0];
  type_sym = struct_find(type_tok);
  ident_sym = sym_find(type_tok);
  /* C++ tag names already denote types. Injecting a same-spelled typedef
     while replaying another namespace would create a new declaration there
     and lose the concrete argument's original scope. The C template
     extension still needs a typedef for its unqualified struct spelling. */
  if (!is_cpp_translation_unit() && type_sym && !type_sym->a.is_class_tag
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
    /* Implicit function-template instantiations share one external ODR
       identity across translation units. A static specifier in the original
       declaration still gives that template internal linkage. */
    if (template_arguments_have_internal_linkage(args))
      tok_str_add(spec, TOK_STATIC);
    else
    {
      tok_str_add(spec, TOK_ATTRIBUTE1);
      tok_str_add(spec, '(');
      tok_str_add(spec, '(');
      tok_str_add(spec, TOK_WEAK1);
      tok_str_add(spec, ')');
      tok_str_add(spec, ')');
    }
    tok_str_add(spec, TOK_INLINE1);
  }
  saved_tok = tok;
  saved_tokc = tokc;
  /* TokenString also carries begin_macro()'s parent cursor.  Template return
     discovery can demand an older specialization of this same definition
     while the new one is being assembled.  A shallow per-replay frame keeps
     those nested cursors independent without copying the token payload. */
  if (td->is_class && td->variadic_param_index >= 0) {
    int pack = td->variadic_param_index;
    expanded_class_definition = expand_template_expression_packs(td->def_str,
      td->type_param_toks[pack], 0, 0, &args->toks[pack], args->nb - pack);
  }
  definition_replay = *(expanded_class_definition ? expanded_class_definition : td->def_str);
  begin_macro(&definition_replay, 3);
  next();
  while (tok != TOK_EOF)
  {
    if (!td->is_class && function_params_done
        && !expanded_pack_body && pack_name_tok && pack_start >= 0)
      expanded_pack_body = capture_template_pack_body(pack_type_tok,
          pack_name_tok, 0, &args->toks[pack_start], args->nb - pack_start);
    if (td->is_class && tok == '{')
    {
      class_body_started = 1;
      int prev = tok_str_prev_token_index(spec->str, spec->len, spec->len);
      int previous = prev >= 0 ? spec->str[prev] : 0;
      if (previous == ')' || previous == TOK_CONST1
          || previous == TOK_CONST2 || previous == TOK_CONST3)
      {
        /* A member body is instantiated when demanded. Substitute its bound
           names now, but leave nested template-ids for ordinary body parsing
           after the enclosing class interface has become complete. */
        TokenString *body;
        int bi;
        skip_or_save_block(&body);
        for (bi = 0; bi < body->len; ++bi)
        {
          int t = body->str[bi];
          int self_close;
          if (t == TOK_EOF)
            break;
          if (TOK_HAS_VALUE(t))
          {
            tok_str_add_record(spec, body->str, body->len, &bi);
            continue;
          }
          if (t == td->name_tok) {
            int previous = tok_str_prev_token_index(body->str, body->len, bi);
            if (previous >= 0 && (body->str[previous] == '.'
                                  || body->str[previous] == TOK_ARROW)) {
              tok_str_add(spec, t);
              continue;
            }
          }
          self_close = template_self_id_close(body, bi, td);
          if (self_close >= 0)
          {
            tok_str_add(spec, mangled_tok);
            bi = self_close;
            continue;
          }
          tok_str_add(spec, template_subst_token(td, args, mangled_tok,
                                                alias_from, alias_to,
                                                nb_aliases, t));
        }
        tok_str_free(body);
        continue;
      }
    }
    if (td->is_class)
    {
      if (!class_body_started && (tok == TOK_CLASS || tok == TOK_STRUCT
                                  || tok == TOK_UNION))
        class_member_access = tok == TOK_CLASS ? 2 : 0;
      if (tok == '{') {
        if (class_brace_depth + 1 >= 128) cprime_error("class replay nesting limit exceeded");
        class_replay_owner[class_brace_depth + 1] = pending_class_owner
            ? pending_class_owner : class_replay_owner[class_brace_depth];
        pending_class_owner = 0;
        ++class_brace_depth;
      }
      else if (tok == '}') --class_brace_depth;
      else if (class_brace_depth == 1
               && (tok == tok_public || tok == tok_protected || tok == tok_private))
        class_member_access = tok == tok_public ? 0 : tok == tok_protected ? 1 : 2;
    }
    if (!td->is_class && td->variadic_param_index >= 0 && !pack_type_tok)
    {
      pack_start = td->variadic_param_index;
      if (pack_start >= 0 && pack_start < td->nb_type_params)
        pack_type_tok = td->type_param_toks[pack_start];
    }
    if (!td->is_class && function_decl_name_done && !function_params_started
        && !function_params_done && tok == '(')
    {
      function_params_started = 1;
      function_param_depth = 0;
      tok_str_add(spec, tok);
      next();
      continue;
    }
    if (!td->is_class && function_params_started && !function_params_done)
    {
      if (pack_type_tok && tok == pack_type_tok)
      {
        int param_toks[64], nb_param_toks = 0;
        int depth = 0, has_pack = 0, pi, ti, found_pack_name = 0;

        /* Leading cv specifiers belong to each expanded declaration. */
        while (spec->len > 0)
        {
          int previous = tok_str_prev_token_index(spec->str, spec->len, spec->len);
          int pt = previous >= 0 ? spec->str[previous] : 0;
          if (pt != TOK_CONST1 && pt != TOK_CONST2 && pt != TOK_CONST3
              && pt != TOK_VOLATILE1 && pt != TOK_VOLATILE2 && pt != TOK_VOLATILE3)
            break;
          memmove(param_toks + 1, param_toks, nb_param_toks * sizeof(int));
          param_toks[0] = pt;
          ++nb_param_toks;
          spec->len = previous;
        }

        while (tok != TOK_EOF)
        {
          if (depth == 0 && (tok == ',' || tok == ')'))
            break;
          if (nb_param_toks < (int)(sizeof(param_toks) / sizeof(param_toks[0])))
            param_toks[nb_param_toks++] = tok;
          if (tok == TOK_DOTS)
            has_pack = 1;
          if (tok == '(' || tok == '[' || tok == TOK_LT || tok == '<')
            ++depth;
          else if ((tok == ')' || tok == ']' || tok == TOK_GT || tok == '>')
                   && depth > 0)
            --depth;
          else if (tok == TOK_SAR && depth > 0)
          {
            --depth;
            if (depth > 0)
              --depth;
          }
          next();
        }
        if (has_pack)
        {
          char pack_name[256];
          if (args->nb == pack_start)
          {
            int previous = tok_str_prev_token_index(spec->str, spec->len, spec->len);
            if (previous >= 0 && spec->str[previous] == ',')
              spec->len = previous;
            else if (tok == ',')
              next();
          }
          for (ti = nb_param_toks - 1; ti >= 0; --ti)
            if (param_toks[ti] >= TOK_UIDENT && param_toks[ti] != pack_type_tok)
            {
              pack_name_tok = param_toks[ti];
              found_pack_name = 1;
              break;
            }
          for (pi = pack_start; pi < args->nb; ++pi)
          {
            if (pi > pack_start)
              tok_str_add(spec, ',');
            for (ti = 0; ti < nb_param_toks; ++ti)
            {
              int pt = param_toks[ti];
              if (pt == TOK_DOTS)
                continue;
              if (pt == pack_type_tok)
                tok_str_add(spec, args->toks[pi]);
              else if (found_pack_name && pt == pack_name_tok)
              {
                snprintf(pack_name, sizeof(pack_name), "%s__pack%d",
                         get_tok_str(pack_name_tok, NULL), pi - pack_start);
                tok_str_add(spec, tok_alloc_const(pack_name));
              }
              else
                tok_str_add(spec, pt);
            }
          }
          continue;
        }
        for (ti = 0; ti < nb_param_toks; ++ti)
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases,
                                   param_toks[ti]);
        continue;
      }
      if (tok == '(' || tok == '[' || tok == TOK_LT || tok == '<')
        ++function_param_depth;
      else if ((tok == ')' || tok == ']' || tok == TOK_GT || tok == '>')
               && function_param_depth > 0)
        --function_param_depth;
      else if (tok == TOK_SAR && function_param_depth > 0)
      {
        --function_param_depth;
        if (function_param_depth > 0)
          --function_param_depth;
      }
      else if (tok == ')' && function_param_depth == 0)
      {
        function_params_done = 1;
        function_params_started = 0;
      }
    }
    if (!td->is_class && function_params_done && pack_type_tok && tok == TOK_SIZEOF)
    {
      next();
      if (tok == TOK_DOTS)
      {
        CValue count;
        next();
        skip('(');
        if (tok != pack_type_tok && tok != pack_name_tok)
          cprime_error("sizeof... must name a template parameter pack");
        next();
        skip(')');
        count.i = args->nb - pack_start;
        tok_str_add2(spec, TOK_CULLONG, &count);
      }
      else
        tok_str_add(spec, TOK_SIZEOF);
      continue;
    }
    if (!td->is_class && function_params_done && pack_name_tok
        && tok == pack_name_tok)
    {
      int saved_name_tok = tok;
      CValue saved_name_tokc = tokc;
      next();
      if (tok == TOK_DOTS)
      {
        int pi;
        char pack_name[256];
        if (args->nb == pack_start)
        {
          int previous = tok_str_prev_token_index(spec->str, spec->len, spec->len);
          if (previous >= 0 && spec->str[previous] == ',')
            spec->len = previous;
        }
        for (pi = pack_start; pi < args->nb; ++pi)
        {
          if (pi > pack_start)
            tok_str_add(spec, ',');
          snprintf(pack_name, sizeof(pack_name), "%s__pack%d",
                   get_tok_str(pack_name_tok, NULL), pi - pack_start);
          tok_str_add(spec, tok_alloc_const(pack_name));
        }
        next();
        continue;
      }
      if (pack_start >= 0 && args->nb > pack_start)
      {
        char pack_name[256];
        snprintf(pack_name, sizeof(pack_name), "%s__pack0",
                 get_tok_str(pack_name_tok, NULL));
        tok_str_add(spec, tok_alloc_const(pack_name));
      }
      else
        tok_str_add2(spec, saved_name_tok, &saved_name_tokc);
      continue;
    }
    if (td->variadic_param_index >= 0 && tok == TOK_DOTS)
    {
      if (!td->is_class && function_params_done && pack_name_tok)
        cprime_error("unexpanded template parameter pack expression");
      next();
      continue;
    }
    if (!td->is_class && function_params_done && !function_body_started
        && tok == TOK_ARROW)
      function_trailing_return = 1;
    if (function_trailing_return)
    {
      if (!trailing_return_paren && (tok == '{' || tok == ';'))
        function_trailing_return = 0;
      else
      {
        /* Evaluate dependent return types only when the concrete function
           parameter scope is active in post_type(), not while collecting
           its specialization's tokens. */
        if (tok == '(') ++trailing_return_paren;
        else if (tok == ')') --trailing_return_paren;
        if (TOK_HAS_VALUE(tok))
          tok_str_add_tok(spec);
        else
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, tok);
        next();
        continue;
      }
    }
    if (!td->is_class && function_params_done && !function_body_started && tok == TOK_TRY) {
      TokenString *body;
      next();
      body = capture_cpp_function_try_body(NULL, 0);
      --body->len;
      restore_cpp_lifecycle_probe(body);
      continue;
    }
    if (!td->is_class && function_params_done && !function_body_started && tok == '{')
    {
      function_interface_len = spec->len;
      function_body_started = 1;
    }
    if (!td->is_class && function_params_done
        && !materialize_function_body && tok == '{')
    {
      /* During ordinary source parsing, publish only the concrete function
         interface.  Its demanded body is replayed by the final worklist once
         no local parser stack is suspended. */
      tok_str_add(spec, ';');
      skip_or_save_block(NULL);
      continue;
    }
    if (!td->is_class && tok == td->name_tok)
    {
      tok_str_add(spec, function_decl_name_done ? tok : mangled_tok);
      function_decl_name_done = 1;
      next();
      continue;
    }
    if (!td->is_class && tok == TOK_OPERATOR)
    {
      next();
      if (get_cpp_binary_operator_method_tok(tok) == td->name_tok)
      {
        tok_str_add(spec, function_decl_name_done ? td->name_tok
                                                  : mangled_tok);
        function_decl_name_done = 1;
        next();
        continue;
      }
      tok_str_add(spec, TOK_OPERATOR);
      continue;
    }
    if (!td->is_class && template_arg_for_param(td, args, tok))
    {
      int arg_tok = template_arg_for_param(td, args, tok);
      next();
      while (tok == TOK_LINENUM)
        next();
      if (tok == ':')
      {
        int alias_tok, scoped_alias_tok;
        Sym *alias_sym;
        next();
        if (tok != ':')
          cprime_error("':' expected in dependent nested type");
        next();
        if (tok == '*') {
          /* A pointer-to-member declarator names its owning class here,
             rather than a member type to resolve during substitution. */
          tok_str_add(spec, template_bound_type_spelling(arg_tok));
          tok_str_add(spec, ':');
          tok_str_add(spec, ':');
          tok_str_add(spec, '*');
          next();
          continue;
        }
        if (tok < TOK_UIDENT)
          cprime_error("dependent nested type member expected");
        alias_tok = tok;
        scoped_alias_tok = template_member_scoped_alias_tok(arg_tok,
                                                             alias_tok);
        alias_sym = scoped_alias_tok
                    ? global_symbol_find(scoped_alias_tok) : NULL;
        if (!alias_sym && scoped_alias_tok)
          alias_sym = sym_find(scoped_alias_tok);
        if (alias_sym && (alias_sym->type.t & VT_TYPEDEF))
        {
          CType alias_type = alias_sym->type;
          int resolved_tok;
          alias_type.t &= ~VT_TYPEDEF;
          resolved_tok = template_type_tok_from_ctype(&alias_type);
          if (!resolved_tok)
            resolved_tok = get_struct_type_name_tok(&alias_type);
          if (resolved_tok)
          {
            if ((alias_type.t & VT_BTYPE) == VT_STRUCT
                && template_typedef_is_type_pos(spec))
              tok_str_add(spec, TOK_STRUCT);
            tok_str_add(spec, resolved_tok);
          }
          else
            tok_str_add(spec, scoped_alias_tok);
        }
        else
        {
          /* Static data members use the normal scoped-name lowering rather
             than the template typedef alias table. */
          tok_str_add(spec, template_bound_type_spelling(arg_tok));
          tok_str_add(spec, ':');
          tok_str_add(spec, ':');
          tok_str_add(spec, alias_tok);
        }
        next();
        continue;
      }
      tok_str_add(spec, template_bound_type_spelling(arg_tok));
      continue;
    }
    if (!td->is_class && !function_body_started && !function_decl_name_done
        && (tok == TOK_AUTO || tok == TOK_DECLTYPE || tok == TOK_DECLTYPE2))
    {
      CType return_type;
      if (infer_saved_template_body_return(td, args, &return_type)
          && template_exact_ctype_typedef_tok(&return_type))
      {
        int at, kept = 0;
        /* The deduced type includes the entire return declarator. Preserve
           declaration specifiers, remove already replayed cv qualifiers, and
           skip the original placeholder wrappers instead of applying twice. */
        for (at = 0; at < spec->len; ++at) {
          int part = spec->str[at];
          if (part == TOK_CONST1 || part == TOK_CONST2 || part == TOK_CONST3
              || part == TOK_VOLATILE1 || part == TOK_VOLATILE2 || part == TOK_VOLATILE3)
            continue;
          spec->str[kept++] = part;
          if (TOK_HAS_VALUE(part)) {
            int extra = tok_str_value_extra_words(spec->str, spec->len, at);
            while (extra--) spec->str[kept++] = spec->str[++at];
          }
        }
        spec->len = kept;
        tok_str_add(spec, template_exact_ctype_typedef_tok(&return_type));
        while (tok != td->name_tok && tok != TOK_EOF) next();
        continue;
      }
    }
    if ((!td->is_class ? !function_body_started : !class_body_started)
        && tok >= TOK_UIDENT
        && tok != td->name_tok)
    {
      int class_template_tok = tok;
      TemplateDef *class_td = td->namespace_tok
        ? find_class_template_def(make_static_member_tok(td->namespace_tok,
                                                          class_template_tok))
        : NULL;
      if (!class_td)
        class_td = find_class_template_def(class_template_tok);
      if (class_td)
      {
        next();
        if (tok == TOK_LT || tok == '<')
        {
          TemplateArgList class_args;
          parse_template_type_args_subst_with_alias(
              &class_args, td, args, alias_from, alias_to, nb_aliases);
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
            /* Lookup may find an inherited member or a nested type. Keep
               the qualified-id for semantic lookup after substitution. */
            tok_str_add(spec, class_inst_tok);
            tok_str_add(spec, ':');
            tok_str_add(spec, ':');
            tok_str_add(spec, member_tok);
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
    if (td->is_class
        && (tok == TOK_CLASS || tok == TOK_STRUCT || tok == TOK_UNION
            || tok == TOK_ENUM))
    {
      int kind_tok = tok;
      int previous = tok_str_prev_token_index(spec->str, spec->len, spec->len);
      int friend_type = previous >= 0 && spec->str[previous] >= TOK_UIDENT
          && !strcmp(get_tok_str(spec->str[previous], NULL), "friend");

      next();
      tok_str_add(spec, kind_tok);
      if (tok >= TOK_UIDENT)
      {
        int nested_name_tok = tok;
        int nested_inst_tok = template_alias_lookup(alias_from, alias_to,
                                                     nb_aliases,
                                                     nested_name_tok);

        next();
        if (nested_name_tok == td->name_tok)
        {
          nested_inst_tok = mangled_tok;
          /* The outer specialization name is already concrete.  A qualified
             elaborated use such as `friend class Pool<T>` must therefore
             discard its now-redundant source template argument list. */
          if (tok == TOK_LT || tok == '<')
          {
            int angle = 1;
            next();
            while (tok != TOK_EOF && angle > 0)
            {
              if (tok == TOK_LT)
                ++angle;
              else if (tok == TOK_SAR || tok == TOK_SHR)
                angle -= 2;
              else if (tok == TOK_GT)
                --angle;
              next();
            }
          }
        }
        /* C has one global tag namespace, so replaying an inner C++ type with
           its source spelling would make two outer specializations collide.
           Publish the nested tag under the concrete outer specialization and
           retain the mapping for constructors, friends, fields, and scoped
           lookups in the rest of this replay.  A tag followed by a declarator
           or template argument is merely a use and must not create a mapping. */
        if (!nested_inst_tok && !friend_type
            && template_param_index(td, nested_name_tok) < 0
            && (tok == '{' || tok == ';'
                || (tok == ':' && !is_namespace_tok(nested_name_tok))))
        {
          int nested_owner = class_replay_owner[class_brace_depth]
              ? class_replay_owner[class_brace_depth] : mangled_tok;
          nested_inst_tok = make_static_member_tok(nested_owner,
                                                    nested_name_tok);
          note_class_source_name(nested_inst_tok, nested_name_tok);
          note_cpp_record_decl(nested_inst_tok, nested_name_tok, nested_owner,
                               current_namespace_scope_tok());
          if (nb_aliases >= (int)(sizeof(alias_from) / sizeof(alias_from[0])))
            cprime_error("too many nested types in template '%s'",
                         get_tok_str(td->name_tok, NULL));
          alias_from[nb_aliases] = nested_name_tok;
          alias_to[nb_aliases] = nested_inst_tok;
          ++nb_aliases;
          if (kind_tok != TOK_ENUM && !struct_find(nested_inst_tok))
          {
            /* A pointer alias to a nested class is usable before the outer
               class layout is complete. Publish its tag now so a dependent
               class (e.g. a future<Node*>) can parse that alias during replay
               without demanding or inventing the nested class's layout. */
            Sym *forward;
      publish_template_forward_type(nested_inst_tok, kind_tok == TOK_UNION);
            forward = struct_find(nested_inst_tok);
            if (kind_tok == TOK_UNION)
              forward->type.t = VT_UNION;
            if (kind_tok == TOK_CLASS)
              forward->a.is_class_tag = 1;
          }
        }
        if (tok == '{' || tok == ':') pending_class_owner = nested_inst_tok ? nested_inst_tok : nested_name_tok;
        tok_str_add(spec, nested_inst_tok ? nested_inst_tok
                                         : nested_name_tok);
      }
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
      int previous = tok_str_prev_token_index(spec->str, spec->len, spec->len);
      int before_previous = previous >= 0 ? tok_str_prev_token_index(spec->str, spec->len, previous) : -1;
      if (previous >= 0 && ((spec->str[previous] == ':' && before_previous >= 0
                             && spec->str[before_previous] == ':') || spec->str[previous] == '.'
                            || spec->str[previous] == TOK_ARROW)) {
        tok_str_add_tok(spec);
        next();
        continue;
      }
      TokenString *member_str = skip_or_save_template_member_decl(1);
      CppMemberDeclContext member_context;
      /* Member-template declarations are captured before the concrete class
         parser sees them. Carry the source access label across that replay. */
      member_context.owner_tok = td->is_partial_specialization
                                   ? mangled_tok : td->name_tok;
      member_context.access = class_member_access;
      member_context.local_boundary = local_stack;
      member_context.previous = cpp_member_decl_context;
      cpp_member_decl_context = &member_context;
      if (member_str)
      {
        TokenString *bound_member = tok_str_alloc();
        int member_i;
        /* A partial specialization has its own member definitions. Bind its
           deduced outer arguments before registering the member under the
           concrete class, so another specialization cannot reuse its body. */
        for (member_i = 0; member_i < member_str->len; ++member_i)
        {
          int member_token = member_str->str[member_i];
          if (member_token == TOK_EOF)
            break;
          if (TOK_HAS_VALUE(member_token))
          {
            tok_str_add_record(bound_member, member_str->str,
                                member_str->len, &member_i);
            continue;
          }
          if (member_token == td->name_tok)
          {
            int following = tok_str_next_non_linenum_index(member_str->str,
                                      member_str->len, member_i + 1);
            if (following < member_str->len
                && (member_str->str[following] == TOK_LT
                    || member_str->str[following] == '<'))
            {
              tok_str_add(bound_member, member_token);
              continue;
            }
          }
          tok_str_add(bound_member, template_subst_token(td, args,
                       mangled_tok, alias_from, alias_to, nb_aliases,
                       member_token));
        }
        tok_str_add(bound_member, TOK_EOF);
        if (token_string_template_is_friend_decl(bound_member) || class_brace_depth > 1)
        {
          /* A template inside an ordinary nested class belongs to that
             class. Let its concrete parser register it with the right owner. */
          --bound_member->len;
          tok_str_append(spec, bound_member);
          tok_str_free(bound_member);
          tok_str_free(member_str);
        }
        else if (register_inline_nested_class_template(mangled_tok, bound_member))
          tok_str_free(member_str);
        else if (td->is_partial_specialization)
        {
          tok_str_free(member_str);
          add_template_member_def(mangled_tok, 0, NULL, 0, NULL, 0, NULL, 0, 0,
                                  bound_member);
        }
        else
        {
          tok_str_free(bound_member);
          if (td->nb_type_params > 0)
            add_template_member_def(td->name_tok, td->type_param_toks[0],
                                    NULL, 0, NULL, 0, NULL, 0, 0, member_str);
          else
            tok_str_free(member_str);
        }
      }
      cpp_member_decl_context = member_context.previous;
      continue;
    }
    if (td->is_class && tok == td->name_tok)
    {
      int self_type_pos = template_typedef_is_type_pos(spec);
      int self_prev_tok = spec->len > 0 ? spec->str[spec->len - 1] : 0;
      if (!class_body_started || self_prev_tok == TOK_STRUCT || self_prev_tok == TOK_CLASS
          || self_prev_tok == TOK_UNION)
        self_type_pos = 0;
      next();
      while (tok == TOK_LINENUM)
        next();
      if (tok == TOK_LT || tok == '<')
      {
        TemplateArgList self_args;
        TokenString *self_tokens = tok_str_alloc(), *resume_macro_stack;
        const int *resume_macro_ptr;
        CValue resume_tokc;
        int i, same_self_args = 1, self_mangled_tok, resume_tok;
        int angle = 0, paren = 0, bracket = 0, brace = 0;
        /* Substitute the entire argument expression before parsing it. A
           parameter may occur inside a cast or arithmetic expression, not
           only as a complete argument. Preserve literal token payloads. */
        for (;;)
        {
          int replacement = 0;
          if (tok == TOK_EOF)
            cprime_error("unterminated self template argument list");
          if (!paren && !bracket && !brace)
          {
            if (tok == TOK_LT || tok == '<')
              ++angle;
            else if (tok == TOK_GT || tok == '>' || tok == TOK_SAR)
            {
              if (tok == TOK_SAR)
              {
                tok = TOK_GT;
                unget_tok(TOK_GT);
              }
              --angle;
            }
          }
          if (tok == '(') ++paren;
          else if (tok == ')') --paren;
          else if (tok == '[') ++bracket;
          else if (tok == ']') --bracket;
          else if (tok == '{') ++brace;
          else if (tok == '}') --brace;
          for (i = 0; i < td->nb_type_params; ++i)
            if (tok == td->type_param_toks[i])
            {
              replacement = args->toks[i];
              break;
            }
          if (!replacement && tok >= TOK_UIDENT)
            replacement = template_alias_lookup(alias_from, alias_to, nb_aliases, tok);
          if (replacement)
            tok_str_add(self_tokens, replacement);
          else
            tok_str_add_tok(self_tokens);
          next();
          if (!angle)
            break;
        }
        tok_str_add(self_tokens, TOK_EOF);
        resume_tok = tok;
        resume_tokc = tokc;
        resume_macro_stack = macro_stack;
        resume_macro_ptr = macro_ptr;
        begin_macro(self_tokens, 1);
        next();
        parse_template_type_args(&self_args);
        while (macro_stack && macro_stack != resume_macro_stack)
          end_macro();
        macro_ptr = resume_macro_ptr;
        tok = resume_tok;
        tokc = resume_tokc;
        if (td->partial_primary || self_args.nb != args->nb)
          same_self_args = 0;
        for (i = 0; same_self_args && i < self_args.nb; ++i)
          if (!template_type_arg_toks_match(self_args.toks[i], args->toks[i]))
            same_self_args = 0;
        self_mangled_tok = same_self_args
                           ? mangled_tok
                           : instantiate_template_argument_type(
                               td->partial_primary ? td->partial_primary : td,
                               &self_args);
        if (self_type_pos && tok != '(' && tok != ':')
          tok_str_add(spec, TOK_STRUCT);
        tok_str_add(spec, self_mangled_tok);
      }
      else
      {
        /* A declarator can hide the injected class name. Keep its source
           spelling so member lookup sees the declared name. */
        if (class_body_started && !self_type_pos && tok != '(' && tok != ':'
            && self_prev_tok != TOK_STRUCT && self_prev_tok != TOK_CLASS
            && self_prev_tok != TOK_UNION) {
          tok_str_add(spec, td->name_tok);
          continue;
        }
        if (self_type_pos && tok != '(')
          tok_str_add(spec, TOK_STRUCT);
        tok_str_add(spec, mangled_tok);
      }
      continue;
    }
    if (td->is_class && tok == TOK_TYPEDEF)
    {
      int i, alias_tok = 0, new_alias_tok = 0;
      int self_alias = 0;
      int parens = 0, angles = 0, brackets = 0, pointer_alias_tok = 0;
      int previous = 0, braces = 0;
      TokenString *typedef_str = tok_str_alloc();
      TokenString *alias_decl = tok_str_alloc();

      while (tok != TOK_EOF)
      {
        tok_str_add_tok(typedef_str);
        if (tok >= TOK_UIDENT && !parens && !angles && !brackets && !braces)
          alias_tok = tok;
        if (tok >= TOK_UIDENT && parens && !angles && !brackets && !braces
            && (previous == '*' || previous == '&' || previous == TOK_LAND))
          pointer_alias_tok = tok;
        if (tok == '(') ++parens;
        else if (tok == ')') --parens;
        else if (tok == TOK_LT) ++angles;
        else if (tok == TOK_GT) --angles;
        else if ((tok == TOK_SAR || tok == TOK_SHR) && angles >= 2) angles -= 2;
        else if (tok == '[') ++brackets;
        else if (tok == ']') --brackets;
        else if (tok == '{') ++braces;
        else if (tok == '}') --braces;
        previous = tok;
        if (tok == ';' && !braces)
          break;
        next();
      }
      if (pointer_alias_tok) alias_tok = pointer_alias_tok;
      if (alias_tok)
      {
        new_alias_tok = make_joined_name_tok(mangled_tok, alias_tok, "__", "");
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
        int self_close = template_self_id_close(typedef_str, i, td);
        if (TOK_HAS_VALUE(t)) {
          int extra = tok_str_value_extra_words(typedef_str->str, typedef_str->len, i);
          int end = i + extra;
          for (; i <= end; ++i) {
            tok_str_add(spec, typedef_str->str[i]);
            tok_str_add(alias_decl, typedef_str->str[i]);
          }
          --i;
          continue;
        }
        /* The terminal name of a qualified type belongs to that qualifier,
           not to the alias being declared in the enclosing specialization. */
        if (template_alias_name_is_qualified(typedef_str, i))
        {
          tok_str_add(spec, t);
          tok_str_add(alias_decl, t);
          continue;
        }
        if (self_close >= 0)
        {
          self_alias = 1;
          template_drop_self_qualifier(spec, typedef_str, i);
          template_drop_self_qualifier(alias_decl, typedef_str, i);
          tok_str_add(spec, TOK_STRUCT);
          tok_str_add(spec, mangled_tok);
          tok_str_add(alias_decl, TOK_STRUCT);
          tok_str_add(alias_decl, mangled_tok);
          i = self_close;
          continue;
        }
        if (t == td->name_tok && i + 1 < typedef_str->len
            && (typedef_str->str[i + 1] == TOK_LT || typedef_str->str[i + 1] == '<')) {
          /* A different specialization retains the template name. Only
             the exact injected-class-name pattern above denotes self. */
          tok_str_add(spec, t);
          tok_str_add(alias_decl, t);
          continue;
        }
        if (t == td->name_tok)
          self_alias = 1;
        if (t == alias_tok && new_alias_tok)
        {
          tok_str_add(spec, new_alias_tok);
          tok_str_add(alias_decl, new_alias_tok);
        }
        else if (template_arg_for_param(td, args, t))
        {
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, t);
          template_add_subst_token(alias_decl, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, t);
        }
        else
        {
          tok_str_add(spec, template_subst_token(td, args, mangled_tok,
                                                 alias_from, alias_to,
                                                 nb_aliases, t));
          tok_str_add(alias_decl,
                      template_subst_token(td, args, mangled_tok,
                                           alias_from, alias_to,
                                           nb_aliases, t));
        }
      }
      if (new_alias_tok && !self_alias && !global_symbol_find(new_alias_tok)
          && template_alias_decl_is_ready(alias_decl, new_alias_tok))
        compile_template_alias_decl_now(alias_decl);
      else
        tok_str_free(alias_decl);
      tok_str_free(typedef_str);
    }
    else if (td->is_class && tok >= TOK_UIDENT
             && !strcmp(get_tok_str(tok, NULL), "using"))
    {
      int i, alias_tok = 0, new_alias_tok = 0;
      int choice_start = -1, choice_end = -1, choice_extra_gt = 0;
      TokenString *using_str = tok_str_alloc();
      TokenString *alias_decl = tok_str_alloc();

      tok_str_add(using_str, tok);
      next();
      if (tok >= TOK_UIDENT)
        alias_tok = tok;
      while (tok != TOK_EOF)
      {
        tok_str_add2(using_str, tok, &tokc);
        if (tok == ';')
          break;
        next();
      }
      /* A using-declaration imports a base member; it does not introduce
         an alias for its first qualifier. Preserve that qualifier while
         substituting the enclosing template's arguments. */
      if (using_str->len < 3 || using_str->str[2] != '=') {
        for (i = 0; i < using_str->len; ++i) {
          int token = using_str->str[i];
          if (TOK_HAS_VALUE(token))
            tok_str_add_record(spec, using_str->str, using_str->len, &i);
          else if (template_alias_name_is_qualified(using_str, i))
            tok_str_add(spec, token);
          else
            tok_str_add(spec, template_subst_token(td, args, mangled_tok,
                                      alias_from, alias_to, nb_aliases, token));
        }
        tok_str_free(using_str);
        tok_str_free(alias_decl);
        next();
        continue;
      }
      if (alias_tok)
      {
        new_alias_tok = make_joined_name_tok(mangled_tok, alias_tok, "__", "");
        note_template_alias_inst(mangled_tok, alias_tok, new_alias_tok);
        if (nb_aliases < 64)
        {
          alias_from[nb_aliases] = alias_tok;
          alias_to[nb_aliases] = new_alias_tok;
          nb_aliases++;
        }
      }
      for (i = 0; i < using_str->len; ++i)
      {
        int open, j, depth, comma1 = -1, comma2 = -1, close = -1;
        int collapsed_double_close = 0;
        int cond_tok;
        if (using_str->str[i] < TOK_UIDENT
            || !strstr(get_tok_str(using_str->str[i], NULL),
                       "conditional_t"))
          continue;
        open = i + 1;
        while (open < using_str->len
               && using_str->str[open] != TOK_LT
               && using_str->str[open] != '<')
          ++open;
        if (open >= using_str->len)
          continue;
        depth = 1;
        for (j = open + 1; j < using_str->len; ++j)
        {
          int t = using_str->str[j];
          if (t == TOK_LT || t == '<')
            ++depth;
          else if (t == TOK_GT || t == '>')
          {
            if (--depth == 0)
            {
              close = j;
              break;
            }
          }
          else if (t == TOK_SAR || t == TOK_SHR)
          {
            depth -= 2;
            if (depth <= 0)
            {
              close = j;
              break;
            }
          }
          else if (t == ',' && depth == 1)
          {
            if (comma1 < 0)
              comma1 = j;
            else
              comma2 = j;
          }
        }
        if (close < 0 && depth == 1 && using_str->len >= 2
            && using_str->str[using_str->len - 1] == ';'
            && (using_str->str[using_str->len - 2] == TOK_GT
                || using_str->str[using_str->len - 2] == '>'))
        {
          close = using_str->len - 2;
          collapsed_double_close = 1;
        }
        if (comma1 < 0 || comma2 < 0 || close < 0)
        {
          continue;
        }
        cond_tok = template_subst_token(td, args, mangled_tok,
                                        alias_from, alias_to, nb_aliases,
                                        using_str->str[open + 1]);
        if (cond_tok == TOK_TRUE)
        {
          choice_start = comma1 + 1;
          choice_end = comma2;
        }
        else if (cond_tok == TOK_FALSE)
        {
          choice_start = comma2 + 1;
          choice_end = close;
          choice_extra_gt = collapsed_double_close
                            || using_str->str[close] == TOK_SAR
                            || using_str->str[close] == TOK_SHR;
        }
        break;
      }
      if (choice_start >= 0)
      {
        tok_str_add(spec, using_str->str[0]);
        tok_str_add(spec, new_alias_tok);
        tok_str_add(spec, '=');
        for (i = choice_start; i < choice_end; ++i)
        {
          int t = using_str->str[i];
          if (template_alias_name_is_qualified(using_str, i))
            tok_str_add(spec, t);
          else if (template_arg_for_param(td, args, t))
            template_add_subst_token(spec, td, args, mangled_tok,
                                     alias_from, alias_to, nb_aliases, t);
          else
            tok_str_add(spec, template_subst_token(td, args, mangled_tok,
                                                   alias_from, alias_to,
                                                   nb_aliases, t));
        }
        if (choice_extra_gt)
          tok_str_add(spec, TOK_GT);
        tok_str_add(spec, ';');
      }
      else for (i = 0; i < using_str->len; ++i)
      {
        int t = using_str->str[i];
        int self_close = template_self_id_close(using_str, i, td);
        if (template_alias_name_is_qualified(using_str, i))
          tok_str_add(spec, t);
        else if (self_close >= 0)
        {
          template_drop_self_qualifier(spec, using_str, i);
          tok_str_add(spec, TOK_STRUCT);
          tok_str_add(spec, mangled_tok);
          i = self_close;
        }
        else if (t == alias_tok && new_alias_tok)
          tok_str_add(spec, i == 1 ? new_alias_tok : alias_tok);
        else if (template_arg_for_param(td, args, t))
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, t);
        else
          tok_str_add(spec, template_subst_token(td, args, mangled_tok,
                                                 alias_from, alias_to,
                                                 nb_aliases, t));
      }
      if (new_alias_tok)
      {
        int saw_equal = 0;
        tok_str_add(alias_decl, TOK_TYPEDEF);
        for (i = choice_start >= 0 ? choice_start : 0;
             i < (choice_start >= 0 ? choice_end : using_str->len); ++i)
        {
          int t = using_str->str[i];
          int self_close = template_self_id_close(using_str, i, td);
          if (self_close >= 0)
          {
            template_drop_self_qualifier(alias_decl, using_str, i);
            tok_str_add(alias_decl, TOK_STRUCT);
            tok_str_add(alias_decl, mangled_tok);
            i = self_close;
            continue;
          }
          if (choice_start < 0 && !saw_equal)
          {
            if (t == '=')
              saw_equal = 1;
            continue;
          }
          if (t == ';')
            break;
          if (template_alias_name_is_qualified(using_str, i))
          {
            tok_str_add(alias_decl, t);
            continue;
          }
          if (t == alias_tok)
          {
            tok_str_add(alias_decl, alias_tok);
            continue;
          }
          if (template_arg_for_param(td, args, t))
            template_add_subst_token(alias_decl, td, args, mangled_tok,
                                     alias_from, alias_to, nb_aliases, t);
          else
          {
            int subst = template_subst_token(td, args, mangled_tok,
                                             alias_from, alias_to,
                                             nb_aliases, t);
            if (subst == '<')
              subst = TOK_LT;
            else if (subst == '>')
              subst = TOK_GT;
            tok_str_add(alias_decl, subst);
          }
        }
        if (choice_extra_gt)
          tok_str_add(alias_decl, TOK_GT);
        tok_str_add(alias_decl, new_alias_tok);
        tok_str_add(alias_decl, ';');
      }
      if (new_alias_tok && !global_symbol_find(new_alias_tok)
          && template_alias_decl_is_ready(alias_decl, new_alias_tok))
        compile_template_alias_decl_now(alias_decl);
      else
        tok_str_free(alias_decl);
      tok_str_free(using_str);
    }
    else if (td->is_class && tok >= TOK_UIDENT)
    {
      Sym *alias_sym = NULL;
      int is_type_pos;

      /* Most identifiers in a class definition are declarator, parameter,
         field, or method names.  They cannot name a class-template unless an
         angle list follows, and a typedef only matters where a type can
         occur.  Avoid two global semantic lookups for every ordinary name in
         every specialization. */
      if (template_add_subst_token_if_changed(
            spec, td, args, mangled_tok,
            alias_from, alias_to, nb_aliases, tok))
      {
        next();
        continue;
      }
      is_type_pos = template_typedef_is_type_pos(spec);
      /* Let the declaration parser instantiate template-ids in source order.
         Earlier fields and aliases of this class are not visible while this
         lexical replay is being assembled. */
      if (is_type_pos)
        alias_sym = find_typedef_sym_in_stack(global_stack, tok);
      if (is_type_pos && !alias_sym)
      {
        Sym *visible_sym = sym_find(tok);
        if (visible_sym && (visible_sym->type.t & VT_TYPEDEF))
          alias_sym = visible_sym;
      }
      if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
          && ((alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
      {
        int struct_tok = get_struct_type_name_tok(&alias_sym->type);
        if (struct_tok && is_type_pos
            && !template_next_token_is_scope())
        {
          /* A base-specifier names a class type without an elaborated
             `struct` keyword, which would consume the derived class body. */
          if (class_brace_depth > 0) tok_str_add(spec, TOK_STRUCT);
          tok_str_add(spec, struct_tok);
        }
        else
          template_add_subst_token(spec, td, args, mangled_tok,
                                   alias_from, alias_to, nb_aliases, tok);
      }
      else
        tok_str_add(spec, tok);
    }
    else
      tok_str_add_tok(spec);
    next();
  }
  if (expanded_pack_body) {
    end_macro();
    tok_str_free(expanded_pack_body);
  }
  end_macro();
  if (expanded_class_definition) tok_str_free(expanded_class_definition);
  tok = saved_tok;
  tokc = saved_tokc;
  tok_str_add(spec, TOK_EOF);


  if (!td->is_class && compiling_pending_template_specs)
  {
    int valid;
    TokenString *interface = spec;
    /* A nested trailing return type needs the actual callee signature now.
       Keep the normal body worklist nonrecursive, and parse only this owned
       interface with the caller's parameter and expression state isolated. */
    if (materialize_function_body) {
      int at;
      interface = tok_str_alloc();
      for (at = 0; at < function_interface_len; ++at)
        tok_str_add_record(interface, spec->str, function_interface_len, &at);
      tok_str_add(interface, ';');
    } else
      interface->str[interface->len - 1] = ';';
    tok_str_add(interface, TOK_EOF);
    valid = template_probe_type(interface, 2, td->namespace_tok, NULL);
    tok_str_free(interface);
    if (!valid)
    {
      cprime_error("invalid function template interface '%s'",
                   get_tok_str(td->name_tok, NULL));
    }
    if (!materialize_function_body) return mangled_tok;
  }
  queue_pending_template_spec_in_namespace(spec,
                              td->friend_class_tok ? td->friend_class_tok :
                              incomplete_dependency_tok ? incomplete_dependency_tok :
                              td->is_class && args->nb > 0
                                && !class_arg_incomplete
                                ? args->toks[0] : 0, td->namespace_tok);
  if (preparing_class_index >= 0)
    td->inst_states[preparing_class_index] = 0;
  if (td->is_class)
  {
    if (incomplete_dependency_tok)
      publish_template_forward_type(mangled_tok, td->is_union);
    compile_pending_template_specs_without_member_flush();
    if (!class_arg_incomplete && struct_find(mangled_tok))
      instantiate_template_members_for_class(td, args->toks[0], mangled_tok);
  }
  else if (template_class_needs_member_layout(td))
    compile_pending_template_specs();
  return mangled_tok;
}

static int materialize_incomplete_template_depth;

static int materialize_incomplete_template_type(CType *type)
{
  int struct_tok;
  TemplateInstOwner *owner;
  Sym *class_sym;

  if (!type || (type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  struct_tok = get_struct_type_name_tok(type);
  class_sym = struct_tok ? struct_find(struct_tok) : NULL;
  if (!class_sym || class_sym->c >= 0)
    return class_sym != NULL;
  owner = find_template_inst_owner_entry(struct_tok);
  if (owner && owner->owner->is_class)
  {
    TemplateDef *td = owner->owner;
    TemplateArgList args;
    int attempt;
    args = td->inst_arg_lists[owner->inst_index];
    ++materialize_incomplete_template_depth;
    for (attempt = 0; attempt < 4; ++attempt)
    {
      int saved_alias_depth = class_alias_layout_depth;
      class_alias_layout_depth = 0;
      instantiate_template_if_needed(td, &args);
      class_alias_layout_depth = saved_alias_depth;
      compile_pending_template_class_definition_now(struct_tok);
      compile_pending_template_specs_without_member_flush();
      class_sym = struct_find(struct_tok);
      if (class_sym && class_sym->c >= 0)
        break;
      if (args.nb > 0)
      {
        Sym *arg_sym = struct_find(args.toks[0]);
        int pending_index = pending_template_spec_index_for_tok(struct_tok);
        /* struct_find returns the tag itself: its layout is in c/r, while
           its embedded type is not a reference back to that tag. Passing
           that embedded type to type_size dereferences a null type.ref. */
        if (arg_sym && arg_sym->c >= 0
            && pending_index >= 0
            && pending_template_spec_dependency_toks)
        {
          /* The argument layout is already usable even if its outer
             template replay is still on the defining stack.  A dependent
             iterator needed by range-for cannot remain forward-only until
             that outer replay returns, because its local storage is sized
             immediately. */
          pending_template_spec_dependency_toks[pending_index] = 0;
          compile_pending_template_specs_without_member_flush();
        }
      }
    }
    --materialize_incomplete_template_depth;
    if (!class_sym || class_sym->c < 0)
      return 0;
    type->t = (type->t & ~VT_BTYPE) | class_sym->type.t;
    type->ref = class_sym;
    return 1;
  }
  return 0;
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

/* A completed deduction depends on its definition and every bound template
   argument. Never publish a provisional result while a body is being probed. */
typedef struct TemplateBodyReturnCache
{
  TemplateDef *definition;
  int specialization_tok;
  CType return_type;
  struct TemplateBodyReturnCache *next;
} TemplateBodyReturnCache;

static TemplateBodyReturnCache *template_body_return_cache[256];

static void free_template_body_return_cache(void)
{
  int bucket;
  for (bucket = 0; bucket < 256; ++bucket)
  {
    TemplateBodyReturnCache *entry = template_body_return_cache[bucket];
    while (entry)
    {
      TemplateBodyReturnCache *next = entry->next;
      cprime_free(entry);
      entry = next;
    }
    template_body_return_cache[bucket] = NULL;
  }
}

static int infer_saved_template_body_return(TemplateDef *td, TemplateArgList *args, CType *result)
{
  TokenString *probe;
  TemplateBodyReturnCache *cached;
  int i, name = -1, open = -1, close = -1, body = -1, depth = 0, valid;
  int specialization_tok, cache_bucket, deduction_kind;
  static int recursion;
  if (!td || !td->has_body || !td->def_str || !args || !result
      || recursion >= 32)
    return 0;
  for (i = 0; i < td->def_str->len; ++i)
  {
    int t = td->def_str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(td->def_str->str, td->def_str->len, i);
      continue;
    }
    if (name < 0)
    {
      if (t == td->name_tok) name = i;
      continue;
    }
    if (open < 0)
    {
      if (t != '(') return 0;
      open = i;
      depth = 1;
      continue;
    }
    if (t == '(') ++depth;
    else if (t == ')')
    {
      --depth;
      if (depth == 0 && close < 0) close = i;
    }
    else if (t == '{' && close >= 0 && depth == 0)
    {
      body = i;
      break;
    }
  }
  if (name < 0 || open < 0 || close < 0 || body < 0) return 0;
  {
    int is_auto=0;
    for (i=0;i<name;++i) {
      if(td->def_str->str[i]==TOK_AUTO) is_auto=1;
      if(TOK_HAS_VALUE(td->def_str->str[i]))i+=tok_str_value_extra_words(td->def_str->str,td->def_str->len,i);
    }
    if(!is_auto) return 0;
    for(i=name+1;i<body;++i) {
      if(td->def_str->str[i]==TOK_ARROW) return 0;
      if(TOK_HAS_VALUE(td->def_str->str[i]))i+=tok_str_value_extra_words(td->def_str->str,td->def_str->len,i);
    }
  }

  specialization_tok = make_template_inst_name_tok(td, args);
  cache_bucket = ((unsigned)specialization_tok * 2654435761u) >> 24;
  for (cached = template_body_return_cache[cache_bucket]; cached; cached = cached->next)
    if (cached->definition == td && cached->specialization_tok == specialization_tok)
    {
      *result = cached->return_type;
      return 1;
    }

  probe=tok_str_alloc();
  deduction_kind = append_placeholder_return_pattern(probe, td->def_str, name);
  tok_str_add(probe,'(');
  {
    int begin = open + 1, at, finish;
    int pack=td->variadic_param_index;
    int pack_type=pack>=0?td->type_param_toks[pack]:0;
    int pack_name=0;
    TokenString *source=td->def_str,*raw_body=tok_str_alloc(),*expanded;
    while(begin<close) {
      int paren=0,bracket=0,brace=0,angle=0,packed=0,element,count=1;
      for(finish=begin;finish<close;++finish) {
        int t=source->str[finish];
        if(TOK_HAS_VALUE(t)){finish+=tok_str_value_extra_words(source->str,source->len,finish);continue;}
        if(!paren&&!bracket&&!brace&&!angle&&t==',')break;
        if(t=='(')++paren;else if(t==')')--paren;
        else if(t=='[')++bracket;else if(t==']')--bracket;
        else if(t=='{')++brace;else if(t=='}')--brace;
        else if(!paren&&!bracket&&!brace&&t==TOK_LT)++angle;
        else if(!paren&&!bracket&&!brace&&t==TOK_GT&&angle>0)--angle;
        else if(!paren&&!bracket&&!brace&&t==TOK_SAR&&angle>0)angle=angle>1?angle-2:0;
        if(t==TOK_DOTS && pack_type)packed=1;
      }
      if(packed) {
        count=args->nb-pack;
        for(at=begin;at<finish;++at) {
          int t=source->str[at];
          if(TOK_HAS_VALUE(t)){at+=tok_str_value_extra_words(source->str,source->len,at);continue;}
          if(t>=TOK_UIDENT&&t!=pack_type&&template_param_index(td,t)<0)pack_name=t;
        }
      }
      for(element=0;element<count;++element) {
        char spelling[256];
        if(probe->str[probe->len-1]!='(')tok_str_add(probe,',');
        for(at=begin;at<finish;++at) {
          int t=source->str[at];
          if(packed&&t==TOK_DOTS)continue;
          if(packed&&t==pack_type)tok_str_add(probe,args->toks[pack+element]);
          else if(packed&&t==pack_name) {
            snprintf(spelling,sizeof(spelling),"%s__pack%d",get_tok_str(pack_name,NULL),element);
            tok_str_add(probe,tok_alloc_const(spelling));
          }
          else if(TOK_HAS_VALUE(t))tok_str_add_record(probe,source->str,source->len,&at);
          else tok_str_add(probe,template_subst_token(td,args,0,NULL,NULL,0,t));
        }
      }
      begin=finish+1;
    }
    tok_str_add(probe,')');
    for(at=body;at<source->len;++at) {
      if(source->str[at]==TOK_EOF||!source->str[at])break;
      tok_str_add_record(raw_body,source->str,source->len,&at);
    }
    tok_str_add(raw_body,TOK_EOF);
    expanded=pack_name?expand_template_expression_packs(raw_body,pack_type,pack_name,0,
                                                       args->toks+pack,args->nb-pack):raw_body;
    for(at=0;at<expanded->len;++at) {
      int t=expanded->str[at];
      if(t==TOK_EOF)break;
      if(TOK_HAS_VALUE(t))tok_str_add_record(probe,expanded->str,expanded->len,&at);
      else tok_str_add(probe,template_subst_token(td,args,0,NULL,NULL,0,t));
    }
    tok_str_add(probe,TOK_EOF);
    if(expanded!=raw_body)tok_str_free(expanded);
    tok_str_free(raw_body);
  }
  ++recursion;
  valid = template_probe_type_ex(probe, deduction_kind == 2 ? 5 : 4,
                                  td->namespace_tok, 0, specialization_tok, result);
  --recursion;
  tok_str_free(probe);
  if (valid)
  {
    cached = cprime_malloc(sizeof(*cached));
    cached->definition = td;
    cached->specialization_tok = specialization_tok;
    cached->return_type = *result;
    cached->next = template_body_return_cache[cache_bucket];
    template_body_return_cache[cache_bucket] = cached;
  }
  return valid;
}
static int infer_template_return_struct_tok(TemplateDef *td, TemplateArgList *args)
{
  CType result;
  return infer_saved_template_body_return(td,args,&result)
      && (result.t & VT_BTYPE)==VT_STRUCT ? get_struct_type_name_tok(&result):0;
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
  ret_type->ref = s;
  return 1;
}

static void queue_demanded_template_static_data(void)
{
  TemplateStaticDataDef *definition;
  if(!finalizing_template_bodies || local_stack || local_scope)return;
  for(definition=template_static_data_defs;definition;definition=definition->next) {
    TemplateDef *owner=definition->owner;
    int instance;
    for(instance=0;instance<owner->nb_inst;++instance) {
      int class_tok=owner->inst_name_toks[instance];
      int member_tok=make_static_member_tok(class_tok,definition->member_tok);
      Sym *member=global_symbol_find(member_tok);
      int i,j;
      TokenString *replay;
      if(!member || !member->c)continue;
      for(i=0;i<definition->emitted_count;++i)if(definition->emitted[i]==class_tok)break;
      if(i<definition->emitted_count)continue;
      member->a.weak=!(member->type.t & VT_STATIC);
      update_storage(member);
      definition->emitted=cprime_realloc(definition->emitted,(definition->emitted_count+1)*sizeof(int));
      definition->emitted[definition->emitted_count++]=class_tok;
      replay=tok_str_alloc();
      if (!(member->type.t & VT_STATIC)) {
        tok_str_add(replay,TOK_ATTRIBUTE1);tok_str_add(replay,'(');tok_str_add(replay,'(');
        tok_str_add(replay,tok_alloc_const("weak"));tok_str_add(replay,')');tok_str_add(replay,')');
      }
      for(i=0;i<definition->declaration->len;++i) {
        int token=definition->declaration->str[i];
        if(i==definition->class_begin){tok_str_add(replay,class_tok);i=definition->class_end;continue;}
        if(TOK_HAS_VALUE(token)){tok_str_add_record(replay,definition->declaration->str,definition->declaration->len,&i);continue;}
        for(j=0;j<definition->parameter_count;++j)if(token==definition->parameters[j])break;
        tok_str_add(replay,j<definition->parameter_count?owner->inst_arg_lists[instance].toks[j]:token);
      }
      queue_pending_template_spec_in_namespace(replay,class_tok,definition->namespace_tok);
    }
  }
}

static void queue_demanded_function_template_bodies(void)
{
  queue_demanded_template_static_data();
  int i, j;

  if (!finalizing_template_bodies || local_stack || local_scope)
    return;

  /* A forward declaration can be instantiated during interface/type
     discovery.  Once its definition has arrived, turn every such concrete
     prototype into a body work item at the replay drain boundary, not while
     parse_template_decl() still owns the definition's source cursor. */
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (td->is_class || !td->has_body)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (!td->inst_states || td->inst_states[j] < 2)
      {
        Sym *func_sym = global_symbol_find(td->inst_name_toks[j]);
        TemplateArgList args = td->inst_arg_lists[j];
        if (!func_sym)
          func_sym = sym_find(td->inst_name_toks[j]);
        /* Interface probing (notably decltype and overload ranking) can
           create many concrete prototypes that are never called.  Inline
           symbol reachability is the demand signal: emit a body only after
           code generation has referenced that specialization. */
        if (!func_sym || !func_sym->c)
          continue;
        instantiate_template_if_needed(td, &args);
      }
  }
}

static void queue_demanded_template_member_bodies(void)
{
  int i, j;

  if (!finalizing_template_bodies || local_stack || local_scope)
    return;
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];

    if (!md || !template_member_has_body(md))
      continue;
    for (j = 0; j < md->nb_inst; ++j)
    {
      Sym *func_sym;
      int func_tok;
      int class_mangled_tok = 0;
      int pack_tok;
      int body_already_materialized;

      if (md->inst_states && md->inst_states[j] >= 2)
        continue;
      func_tok = md->inst_func_toks ? md->inst_func_toks[j] : 0;
      if (!func_tok)
        continue;
      /* Several source overloads can currently converge on the same lowered
         member token.  Once one of them owns that concrete body, demanding a
         second definition would only emit a duplicate symbol. */
      body_already_materialized = materialized_member_body_has(func_tok);
      if (body_already_materialized)
        continue;
      func_sym = global_symbol_find(func_tok);
      if (!func_sym)
        func_sym = sym_find(func_tok);
      if (!func_sym || !func_sym->c)
        continue;
      class_mangled_tok = md->inst_owner_toks[j];
      if (!class_mangled_tok)
        continue;
      pack_tok = md->inst_pack_toks ? md->inst_pack_toks[j] : 0;
      instantiate_template_member_body_if_needed(
          md, md->inst_type_toks[j], class_mangled_tok, pack_tok);
    }
  }
}

static void compile_pending_template_specs(void)
{
  LambdaExpression *saved_lambda_probe;
  Sym **saved_local_bindings = NULL;
  int nb_saved_local_bindings = 0;
  int i, made_progress, saved_tok, saved_local_scope;
  int saved_defer_pending_member_funcs;
  int saved_nb_pending_member_funcs;
  int saved_ind, saved_func_var, saved_func_vc;
  int saved_func_ind, saved_nocode_wanted;
  int saved_vstack_depth;
  SValue *saved_vstack = NULL;
  CValue saved_tokc;
  Sym *saved_local_stack, *saved_global_stack = NULL, *param_frame_tail = NULL;
  CType saved_func_vt;
  const char *saved_funcname;
  Section *saved_cur_text_section;

  /* The outer worklist has a live bound and will consume entries appended by
     the specialization it is currently compiling.  Recursive drains corrupt
     the suspended declaration/parser state and are unnecessary. */
  if (compiling_pending_template_specs)
    return;
  ++compiling_pending_template_specs;
  queue_demanded_function_template_bodies();
  if (first_pending_template_spec == nb_pending_template_specs) {
    --compiling_pending_template_specs;
    return;
  }

  saved_tok = tok;
  saved_tokc = tokc;
  saved_lambda_probe = lambda_probe;
  lambda_probe = NULL;
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
  saved_vstack_depth = (int)(vtop - vstack + 1);
  if (saved_vstack_depth > 0)
  {
    saved_vstack = cprime_malloc(saved_vstack_depth * sizeof(SValue));
    memcpy(saved_vstack, vstack, saved_vstack_depth * sizeof(SValue));
  }
  vtop = vstack - 1;
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
    /* A detached local stack is still linked through TokenSym lookup.  Hide
       those bindings while generated specializations compile: their function
       bodies restart local_scope numbering and may legitimately reuse names
       (commonly a for-loop variable named i). */
    if (saved_local_stack)
    {
      saved_local_bindings = sym_hide_local_bindings(saved_local_stack,
                                                     &nb_saved_local_bindings);
      sym_relink_local_types(saved_local_bindings, nb_saved_local_bindings);
    }
    local_stack = NULL;
    local_scope = 0;
    defer_pending_member_funcs = 1;
  }
  do
  {
    queue_demanded_function_template_bodies();
    made_progress = 0;
    for (i = first_pending_template_spec; i < nb_pending_template_specs;
           i = next_pending_template_spec(i + 1))
    {
    int dependency_tok;
    TokenString *ts = pending_template_specs[i];
    Sym *dependency_sym;
    if (!ts)
      continue;
    dependency_tok = pending_template_spec_dependency_toks
                       ? pending_template_spec_dependency_toks[i] : 0;
    dependency_sym = dependency_tok ? struct_find(dependency_tok) : NULL;
    if (dependency_tok
        && ((dependency_sym && dependency_sym->c < 0)
            || is_currently_defining_class(dependency_tok)))
      continue;
    ++compiled_template_specs;
    made_progress = 1;
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
    /* Mark the work item consumed before re-entering decl().  Resolving the
       generated declaration can recursively flush this queue; leaving the
       token visible would replay the same item and redeclare its prototype. */
    pending_template_specs[i] = NULL;
    first_pending_template_spec = next_pending_template_spec(first_pending_template_spec);
    {
      int replay_namespace_tok = current_namespace_scope_tok();
      int replay_member_class_tok = active_member_class_tok;
      int replay_class_depth = nb_defining_class_stack;
      int replay_class_stack[32];
      jmp_buf *replay_substitution_jump = cpp_substitution_jump;
      TokenString *spec_macro_parent = macro_stack;
      const int *spec_macro_ptr = macro_ptr;
      /* decl() performs one-token lookahead after a declaration.  Keep that
         lookahead inside this owned replay; otherwise a generated class that
         ends exactly at TOK_EOF can resume and parse the suspended caller
         before compile_pending_template_specs() gets a chance to restore its
         cursor.  A redundant top-level semicolon is a valid hard boundary for
         class, function, typedef, and forward-declaration work items. */
      if (ts->len > 0 && ts->str[ts->len - 1] == TOK_EOF)
      {
        ts->str[ts->len - 1] = ';';
        tok_str_add(ts, TOK_EOF);
      }
      enter_namespace_scope(pending_template_spec_namespace_toks[i]);
      active_member_class_tok = 0;
      memcpy(replay_class_stack, defining_class_stack, sizeof(replay_class_stack));
      nb_defining_class_stack = 0;
      cpp_substitution_jump = NULL;
      begin_macro(ts, 1);
      next();
      if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "friend"))
        active_member_class_tok = dependency_tok;
      /* A generated specialization can start with one or more supporting
         declarations (for example typedefs for struct-valued arguments).
         Parse the complete owned replay instead of returning after its first
         declaration and accidentally resuming the caller's token stream. */
      while (macro_stack == ts)
      {
        if (tok == TOK_EOF)
        {
          end_macro();
          break;
        }
        if (tok == ';')
        {
          /* This is the synthetic fence appended above.  Do not ask decl()
             to parse an empty declaration (it intentionally leaves ';'
             untouched), and do not next() across the fence into the
             suspended caller. */
          end_macro();
          break;
        }
        decl(VT_CONST);
      }
      /* decl() can consume the replay through TOK_EOF and close it. */
      while (macro_stack && macro_stack != spec_macro_parent)
        end_macro();
      macro_ptr = spec_macro_ptr;
      cpp_substitution_jump = replay_substitution_jump;
      enter_namespace_scope(replay_namespace_tok);
      active_member_class_tok = replay_member_class_tok;
      memcpy(defining_class_stack, replay_class_stack, sizeof(replay_class_stack));
      nb_defining_class_stack = replay_class_depth;
    }
    /* end_macro() owns the replay buffer and frees it.  Clear the slot so
       later scans of pending_template_specs cannot dereference the dangling
       TokenString (it may already have been reused by a newer allocation). */
    defer_pending_member_funcs = saved_defer_pending_member_funcs;
    }
  }
  while (made_progress);
  if (!suppress_template_member_flush
      && (saved_local_stack || saved_local_scope)
      && saved_nb_pending_member_funcs != nb_pending_member_funcs)
    compile_pending_member_funcs(saved_nb_pending_member_funcs);
  if (!suppress_template_member_flush
      && !saved_local_stack && !saved_local_scope
      && !finalizing_template_bodies
      && saved_nb_pending_member_funcs != nb_pending_member_funcs)
    compile_pending_lifecycle_member_funcs(saved_nb_pending_member_funcs);
  if (param_frame_tail)
  {
    param_frame_tail->prev = global_stack;
    global_stack = saved_global_stack;
  }
  if (saved_local_stack)
  {
    sym_unlink_local_types(saved_local_bindings, nb_saved_local_bindings);
    sym_relink_stack(saved_local_bindings, nb_saved_local_bindings);
  }
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  if (saved_vstack_depth > 0)
  {
    memcpy(vstack, saved_vstack, saved_vstack_depth * sizeof(SValue));
    cprime_free(saved_vstack);
  }
  vtop = vstack + saved_vstack_depth - 1;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
  ind = saved_ind;
  func_vt = saved_func_vt;
  func_var = saved_func_var;
  func_vc = saved_func_vc;
  func_ind = saved_func_ind;
  funcname = saved_funcname;
  nocode_wanted = saved_nocode_wanted;
  cur_text_section = saved_cur_text_section;
  lambda_probe = saved_lambda_probe;
  tok = saved_tok;
  tokc = saved_tokc;
  --compiling_pending_template_specs;
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

/* Match the declarator around a directly named type parameter before
   choosing its argument type. Qualifiers written in the pattern belong to
   the parameter, while qualifiers supplied by the argument remain part of
   T for pointer and reference deduction. */
static int deduce_direct_template_type(TokenString *definition, int start,
                                       int parameter, int end, CType *argument)
{
  CType deduced = *argument;
  int i, pointers = 0, reference = 0, qualifiers = 0, forwarding;
  int member_type_deduced = 0;
  int argument_is_lvalue = !(deduced.t & VT_RVALUE_REFERENCE);
  if (deduced.t == VT_BRACED_LIST)
    return 0;
  if (parameter + 3 < end && definition->str[parameter + 1] == ':'
      && definition->str[parameter + 2] == ':' && definition->str[parameter + 3] == '*') {
    CppMemberPointerType *member = find_cpp_member_pointer_type(&deduced);
    CType owner;
    if (!member || !make_class_type_from_tok(&owner, member->owner_tok)) return 0;
    return template_type_tok_from_ctype(&owner);
  }
  /* In T C::*, T denotes the member type rather than the compiler's
     internal two-word member-pointer representation. */
   for (i = parameter + 1; i + 3 < end; ++i) {
     if (definition->str[i] >= TOK_UIDENT
         && definition->str[i + 1] == ':' && definition->str[i + 2] == ':'
         && definition->str[i + 3] == '*') {
       CppMemberPointerType *member = find_cpp_member_pointer_type(&deduced);
       int returns_member_function_value = 0;
       if (!member) return 0;
       /* `R (C::*name)(...)` spells the deduced parameter as the member
          function's *result* type, not the stored function type. The
          parenthesized member-pointer group immediately after the parameter
          tells the two spellings apart: `T C::*` keeps the member type. */
       if ((member->member_type.t & VT_BTYPE) == VT_FUNC
           && member->member_type.ref
           && definition->str[parameter + 1] == '(' && i == parameter + 2)
         returns_member_function_value = 1;
       deduced = returns_member_function_value
                   ? member->member_type.ref->type : member->member_type;
       member_type_deduced = 1;
       break;
     }
   }
  for (i = start; i < end; ++i)
  {
    int t = definition->str[i];
    if (t == TOK_LINENUM) { ++i; continue; }
    if (i == parameter) continue;
    if (i > parameter && t == '*') { ++pointers; continue; }
    if (i > parameter && (t == '&' || t == TOK_LAND))
    { reference = t; break; }
    if (t == TOK_CONST1 || t == TOK_CONST2 || t == TOK_CONST3)
    { if (!pointers) qualifiers |= VT_CONSTANT; continue; }
    if (t == TOK_VOLATILE1 || t == TOK_VOLATILE2 || t == TOK_VOLATILE3)
    { if (!pointers) qualifiers |= VT_VOLATILE; continue; }
    if (i > parameter) break;
  }
  if (is_reference_type(&deduced))
    deduced = *pointed_type(&deduced);
  deduced.t &= ~VT_RVALUE_REFERENCE;
  if (!pointers && !reference)
  {
    /* A member type deduced through `T C::*` keeps its own type, including
       its cv-qualifiers: only a by-value parameter decays a function or
       array type to a pointer and drops top-level cv. */
    if (!member_type_deduced)
    {
      if ((deduced.t & VT_BTYPE) == VT_FUNC)
        mk_pointer(&deduced);
      else {
        deduced.t &= ~VT_ARRAY;
        if (IS_BT_ARRAY(deduced.t)) deduced.t &= ~VT_STRUCT_MASK;
      }
      deduced.t &= ~(VT_CONSTANT | VT_VOLATILE);
    }
  }
  forwarding = reference == TOK_LAND && !pointers && !qualifiers;
  while (pointers--)
  {
    if ((deduced.t & VT_BTYPE) != VT_PTR || !deduced.ref)
      return 0;
    deduced = *pointed_type(&deduced);
  }
  deduced.t &= ~qualifiers;
  if (qualifiers && (deduced.t & VT_ARRAY)) {
    CType *element = &deduced;
    /* Array qualification belongs to its element type. Deduction of
       const T& from a const array must remove that const from T too. */
    while ((element->t & VT_ARRAY) && element->ref) {
      Sym *original = element->ref;
      element->ref = sym_push(SYM_FIELD, &original->type, 0, original->c);
      element = &element->ref->type;
    }
    element->t &= ~qualifiers;
  }
  /* For a forwarding reference, an lvalue deduces T as a reference type.
     Preserve its cv qualifiers and let normal reference collapsing apply
     when the concrete T is substituted into T&&. */
  if (forwarding && argument_is_lvalue)
    mk_reference(&deduced);
  return template_exact_type_tok_from_ctype(&deduced);
}

static int template_template_base_for_deduction(int class_tok, TokenString *signature)
{
  TemplateInstOwner *instance = find_template_inst_owner_entry(class_tok);
  ClassBaseInfo *base;
  int result = 0;
  if (instance && instance->owner->is_class
      && template_template_argument_matches(signature, template_def_lookup_tok(instance->owner)))
    return class_tok;
  for (base = class_base_candidates(class_tok); base; base = base->bucket_next) {
    int candidate;
    if (base->class_tok != class_tok) continue;
    candidate = template_template_base_for_deduction(base->base_tok, signature);
    if (candidate < 0 || (candidate && result && candidate != result)) return -1;
    if (candidate) result = candidate;
  }
  return result;
}

static int infer_template_args_from_call_pattern(TemplateDef *td,
                                         CType *arg_types, int arg_count,
                                         TemplateArgList *args,
                                         TemplateArgList *explicit_args)
{
  int i, j, arg_index, type_tok;
  int pack_param_index = td->variadic_param_index >= 0
                         ? td->variadic_param_index
                         : -1;
  int pack_arg_start = pack_param_index >= 0 ? td->func_min_args : -1;
  int pack_pattern_start = -1, pack_pattern_parameter = -1;
  int pack_pattern_end = -1;
  int array_bindings[16] = {0};

  /* A non-template overload set can deduce a function-pointer parameter when
     its matching signatures agree. Template overload sets remain non-deduced. */
  for (i = 0; i < arg_count; ++i) {
    CType source = arg_types[i];
    if ((source.t & VT_BTYPE) == VT_PTR) source = *pointed_type(&source);
    if ((source.t & VT_BTYPE) == VT_FUNC && source.ref
        && source.ref->f.func_unresolved_overload
        && !find_function_template_def(source.ref->v)) {
      FreeFuncOverload *overload;
      TemplateArgList selected;
      CType saved = arg_types[i];
      int matched = 0;
      for (overload = free_func_candidates(source.ref->v); overload; overload = overload->name_next) {
        TemplateArgList trial;
        if (overload->name_tok != source.ref->v) continue;
        arg_types[i] = overload->func_type;
        if ((saved.t & VT_BTYPE) == VT_PTR) mk_pointer(&arg_types[i]);
        if (!infer_template_args_from_call_pattern(td, arg_types, arg_count, &trial, explicit_args))
          continue;
        if (matched) {
          if (trial.nb != selected.nb) { arg_types[i] = saved; return 0; }
          for (j = 0; j < trial.nb; ++j)
            if (!template_type_arg_toks_match(trial.toks[j], selected.toks[j])) {
              arg_types[i] = saved; return 0;
            }
        }
        selected = trial;
        matched = 1;
      }
      arg_types[i] = saved;
      if (matched) *args = selected;
      return matched;
    }
  }

  args->nb = td->nb_required_type_params;
  if (pack_param_index >= 0 && pack_arg_start < arg_count
      && args->nb + (arg_count - pack_arg_start) <= CPC_MAX_TEMPLATE_ARGUMENTS)
    args->nb += arg_count - pack_arg_start;
  if (explicit_args && explicit_args->nb > args->nb)
    args->nb = explicit_args->nb;
  for (i = 0; i < args->nb; ++i)
    args->toks[i] = explicit_args && i < explicit_args->nb
                      ? explicit_args->toks[i] : 0;
  if (td->def_str)
  {
    int name_index = -1, paren_index = -1, depth = 0, param_start = -1;

    for (i = 0; i < td->def_str->len; ++i)
    {
      if (td->def_str->str[i] == td->name_tok)
        name_index = i;
      else if (td->def_str->str[i] == TOK_OPERATOR
               && i + 2 < td->def_str->len
               && ((td->name_tok == tok_alloc_const("operator()")
                    && td->def_str->str[i + 1] == '(' && td->def_str->str[i + 2] == ')')
                   || (td->name_tok == tok_alloc_const("operator[]")
                       && td->def_str->str[i + 1] == '[' && td->def_str->str[i + 2] == ']')))
      {
        i += 2;
        name_index = i;
      }
      else if (td->def_str->str[i] == TOK_OPERATOR
               && i + 1 < td->def_str->len
               && get_cpp_binary_operator_method_tok(td->def_str->str[i + 1])
                  == td->name_tok)
        name_index = i + 1;
      if (name_index >= 0 && td->def_str->str[i] == '(')
      {
        paren_index = i;
        break;
      }
    }
    if (paren_index >= 0)
    {
      for (j = 0; j < td->nb_type_params; ++j)
      {
        int value;
        if (explicit_args && j < explicit_args->nb) continue;
        value = deduce_function_array_argument(td->def_str, paren_index + 1,
                         td->type_param_toks[j],
                         (td->value_param_mask >> j) & 1,
                         arg_types, arg_count);
        if (value < 0) return 0;
        if (value) args->toks[j] = array_bindings[j] = value;
      }
      arg_index = 0;
      param_start = paren_index + 1;
      for (i = param_start; i < td->def_str->len && arg_index < arg_count; ++i)
      {
        int t = td->def_str->str[i];
        int top_level = depth == 0;
        if (t == TOK_LT || t == '<' || t == '(' || t == '[')
          ++depth;
        else if ((t == TOK_GT || t == '>' || t == ')' || t == ']') && depth > 0)
          --depth;
        if (top_level && (t == ',' || t == ')'))
        {
          int end = i;
          int non_deduced = arg_types[arg_index].t == VT_BRACED_LIST;
          int nesting = 0, scan;
          {
            CType argument = arg_types[arg_index];
            if ((argument.t & VT_BTYPE) == VT_PTR) argument = *pointed_type(&argument);
            if ((argument.t & VT_BTYPE) == VT_FUNC && argument.ref
                && argument.ref->f.func_unresolved_overload) non_deduced = 1;
          }
          /* The nested-name-specifier of a qualified dependent type is a
             non-deduced context (e.g. iterator_traits<I>::difference_type).
             I is supplied by other parameters, then this parameter converts
             to the substituted type during overload resolution. */
          for (scan = param_start; scan + 2 < end; ++scan)
          {
            int part = td->def_str->str[scan];
            if (!nesting && part == '=') break;
            if (part == TOK_LT || part == '<' || part == '(') ++nesting;
            else if ((part == TOK_GT || part == '>' || part == ')') && nesting) --nesting;
            if (!nesting && td->def_str->str[scan + 1] == ':'
                && td->def_str->str[scan + 2] == ':'
                && !(scan + 3 < end && td->def_str->str[scan + 3] == '*')
                && (part == TOK_GT || part == '>'
                    || template_param_index(td, part) >= 0))
              non_deduced = 1;
          }
          type_tok = template_type_tok_from_ctype(&arg_types[arg_index]);
          for (j = param_start; !non_deduced && j < end; ++j)
          {
            TemplateDef *class_td = NULL;
            int param_idx, inst_arg;
            int param_depth = 0, k;

            for (k = param_start; k < j; ++k)
            {
              int kt = td->def_str->str[k];
              if (kt == TOK_LT || kt == '<' || kt == '(' || kt == '[')
                ++param_depth;
              else if ((kt == TOK_GT || kt == '>' || kt == ')' || kt == ']')
                       && param_depth > 0)
                --param_depth;
            }

            if (!param_depth && td->def_str->str[j] == '=') break;

            param_idx = template_param_index(td, td->def_str->str[j]);
            if (explicit_args && param_idx >= 0 && param_idx < explicit_args->nb)
              continue;
            if (param_idx >= 0 && param_depth > 0
                && !(td->value_param_mask & (1u << param_idx))) {
             CType actual = arg_types[arg_index];
             int opening = -1, cursor, nesting = 0, segment, position = 0;
             Sym *parameter;
             CppMemberPointerType *member = NULL;
             if (is_reference_type(&actual)) actual = *pointed_type(&actual);
             if ((actual.t & VT_BTYPE) == VT_PTR) actual = *pointed_type(&actual);
             /* `R (C::*name)(...)` names the member's class inside the
                parenthesized member-pointer group, so the owner parameter is
                read from the argument's record while the member function's
                parameter list supplies any further deduced parameters. */
             member = find_cpp_member_pointer_type(&actual);
             if (member && (member->member_type.t & VT_BTYPE) == VT_FUNC) {
               if (j + 3 < end && td->def_str->str[j + 1] == ':'
                   && td->def_str->str[j + 2] == ':' && td->def_str->str[j + 3] == '*') {
                 CType owner_type;
                 if (!make_class_type_from_tok(&owner_type, member->owner_tok)) return 0;
                 type_tok = template_type_tok_from_ctype(&owner_type);
                 if (!type_tok) return 0;
                 if (args->toks[param_idx]
                     && !template_type_arg_toks_match(args->toks[param_idx], type_tok))
                   return 0;
                 args->toks[param_idx] = type_tok;
                 continue;
               }
               actual = member->member_type;
             }
             if ((actual.t & VT_BTYPE) == VT_FUNC && actual.ref) {
                for (cursor = param_start; cursor < j; ++cursor) {
                  int token = td->def_str->str[cursor];
                  if (token == '(' && !nesting) opening = cursor;
                  if (token == '(') ++nesting;
                  else if (token == ')') --nesting;
                }
                if (opening >= 0 && nesting == 1) {
                  segment = opening + 1;
                  nesting = 0;
                  for (cursor = segment; cursor < j; ++cursor) {
                    int token = td->def_str->str[cursor];
                    if (token == '(' || token == '[' || token == TOK_LT) ++nesting;
                    else if (token == ')' || token == ']' || token == TOK_GT) --nesting;
                    else if (token == ',' && !nesting) { ++position; segment = cursor + 1; }
                  }
                  parameter = actual.ref->next;
                  while (parameter && position--) parameter = parameter->next;
                  if (!parameter) return 0;
                  cursor = j + 1;
                  while (cursor < end && td->def_str->str[cursor] != ','
                         && td->def_str->str[cursor] != ')') ++cursor;
                  type_tok = deduce_direct_template_type(td->def_str, segment,
                                                         j, cursor, &parameter->type);
                  if (!type_tok || (args->toks[param_idx]
                      && !template_type_arg_toks_match(args->toks[param_idx], type_tok))) return 0;
                  args->toks[param_idx] = type_tok;
                  continue;
                }
              }
            }
            if (param_idx >= 0 && type_tok && param_depth == 0)
            {
              if (td->value_param_mask & (1u << param_idx)) continue;
              if (token_string_starts_template(td->value_type_strs[param_idx])
                  && j + 1 < end && td->def_str->str[j + 1] == TOK_LT) {
                CType actual = arg_types[arg_index];
                TemplateInstOwner *instance;
                if (is_reference_type(&actual)) actual = *pointed_type(&actual);
                if ((actual.t & VT_BTYPE) == VT_PTR) actual = *pointed_type(&actual);
                int matched_class = template_template_base_for_deduction(
                  get_struct_type_name_tok(&actual), td->value_type_strs[param_idx]);
                if (matched_class <= 0) return 0;
                instance = find_template_inst_owner_entry(matched_class);
                if (!instance || !instance->owner->is_class) return 0;
                class_td = instance->owner;
                args->toks[param_idx] = template_def_lookup_tok(class_td);
              } else {
              if (param_idx == pack_param_index)
              {
                pack_pattern_start = param_start;
                pack_pattern_parameter = j;
                pack_pattern_end = end;
              }
              type_tok = (arg_types[arg_index].t & VT_ARRAY) && array_bindings[param_idx]
                ? array_bindings[param_idx]
                : deduce_direct_template_type(td->def_str, param_start,
                            j, end, &arg_types[arg_index]);
              if (!type_tok)
                return 0;
              if (args->toks[param_idx]
                  && !template_type_arg_toks_match(args->toks[param_idx], type_tok))
                return 0;
              args->toks[param_idx] = type_tok;
              continue;
              }
            }
            if (!class_td) class_td = template_pattern_class_definition(td, td->def_str,
                                                          param_start, j);
            if (j + 3 >= end
                || !class_td || !class_td->is_class
                || (td->def_str->str[j + 1] != TOK_LT
                    && td->def_str->str[j + 1] != '<'))
              continue;
            if (!param_depth
                && (template_param_index(td, td->def_str->str[j + 2]) < 0
                    || (td->def_str->str[j + 3] != ','
                        && td->def_str->str[j + 3] != TOK_GT))) {
              CType actual = arg_types[arg_index];
              TokenString *pattern = tok_str_alloc(), macro;
              TokenString *parent = macro_stack;
              const int *parent_ptr = macro_ptr;
              int saved_token = tok, cursor, matched;
              CValue saved_value = tokc;
              if (is_reference_type(&actual)) actual = *pointed_type(&actual);
              actual.t &= ~VT_RVALUE_REFERENCE;
              tok_str_add(pattern, class_td->lookup_tok);
              for (cursor = j + 1; cursor < end; ++cursor)
                tok_str_add_record(pattern, td->def_str->str, td->def_str->len, &cursor);
              tok_str_add(pattern, TOK_EOF);
              macro = *pattern; begin_macro(&macro, 0); next();
              matched = deduce_nested_class_pattern(td, args,
                            template_exact_ctype_typedef_tok(&actual));
              while (macro_stack && macro_stack != parent) end_macro();
              macro_ptr = parent_ptr; tok = saved_token; tokc = saved_value;
              tok_str_free(pattern);
              if (!matched) return 0;
              break;
            }
            param_idx = template_param_index(td, td->def_str->str[j + 2]);
            if (param_idx < 0 || !type_tok
                || (explicit_args && param_idx < explicit_args->nb))
              continue;
            {
              CType class_argument = arg_types[arg_index];
              int class_inst_tok;

              if (is_reference_type(&class_argument))
                class_argument = *pointed_type(&class_argument);
              class_inst_tok = get_struct_type_name_tok(&class_argument);

              /* Match the outer pointer in Container<T>* before looking up
                 the concrete Container specialization.  Otherwise the
                 fallback below records the entire pointer as T and replays
                 a signature such as Container<Container<int>*>*. */
              if ((class_argument.t & VT_BTYPE) == VT_PTR)
              {
                CType pointee = *pointed_type(&class_argument);
                int pointee_tok = get_struct_type_name_tok(&pointee);
                if (pointee_tok)
                  class_inst_tok = pointee_tok;
              }
              /* Each direct parameter in a class-template argument list has
                 its own binding, rather than reusing the first class argument. */
              {
                int position = 0, cursor = j + 2;
                while (cursor < end) {
                  int parameter = template_param_index(td, td->def_str->str[cursor]);
                  int following = cursor + 1 < end ? td->def_str->str[cursor + 1] : 0;
                  int argument;
                  if (parameter < 0 || (following != ',' && following != TOK_GT)) break;
                  argument = template_class_argument_for_deduction(class_td,
                                      class_inst_tok, position);
                  if (argument <= 0) return 0;
                  if (args->toks[parameter]
                      && !template_type_arg_toks_match(args->toks[parameter], argument))
                    return 0;
                  args->toks[parameter] = argument;
                  ++position;
                  if (following == TOK_GT) break;
                  cursor += 2;
                }
              }
              inst_arg = param_depth == 0
                ? template_class_argument_for_deduction(class_td,
                                                         class_inst_tok, 0)
                : template_inst_arg_for_name(class_td, class_inst_tok, 0);
            }
            if (param_depth == 0 && inst_arg <= 0) return 0;
            if (inst_arg && args->toks[param_idx]
                && !template_type_arg_toks_match(args->toks[param_idx], inst_arg))
              return 0;
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
  /* Every pack element is deduced against the declared parameter pattern,
     including its pointers, cv qualifiers, and forwarding references. */
  if (pack_param_index >= 0)
    for (i = pack_arg_start; i < arg_count && i < CPC_MAX_CALL_ARGUMENTS; ++i)
    {
      type_tok = pack_pattern_parameter >= 0
        ? deduce_direct_template_type(td->def_str, pack_pattern_start,
              pack_pattern_parameter, pack_pattern_end, &arg_types[i])
        : template_type_tok_from_ctype(&arg_types[i]);
      if (type_tok && pack_param_index + (i - pack_arg_start) < args->nb
          && (!explicit_args || pack_param_index + (i - pack_arg_start) >= explicit_args->nb))
        args->toks[pack_param_index + (i - pack_arg_start)] = type_tok;
    }
  /* A dependent zero-argument helper call inside a delayed specialization
     carries no value arguments from which to deduce its type.  Its explicit
     <T> can already have been consumed by decltype/auto-return probing before
     the owned body is replayed.  Propagate the active function
     specialization's concrete arguments, which is the dependent context the
     source spelling refers to. */
  if (arg_count == 0 && funcname && *funcname)
  {
    for (i = 0; i < args->nb; ++i) if (!args->toks[i]) break;
    if (i < args->nb) {
      TemplateInstOwner *current = find_template_inst_owner_entry(tok_alloc_const(funcname));
      if (current && !current->owner->is_class && current->owner->inst_arg_lists) {
        TemplateArgList *current_args = &current->owner->inst_arg_lists[current->inst_index];
        for (; i < args->nb && i < current_args->nb; ++i)
          if (!args->toks[i]) args->toks[i] = current_args->toks[i];
      }
    }
  }

  for (i = 0; i < args->nb; ++i)
  {
    if (args->toks[i])
      continue;
    if (td->value_param_mask & (1u << i))
      return 0;
    if (td->variadic_param_index == i)
      arg_index = td->func_min_args < arg_count ? td->func_min_args : arg_count - 1;
    else
      arg_index = i < arg_count ? i : arg_count - 1;
    if (arg_index < 0)
      return 0;
    if (arg_types[arg_index].t == VT_BRACED_LIST)
      return 0;
    type_tok = template_type_tok_from_ctype(&arg_types[arg_index]);
    if (!type_tok)
      return 0;
    args->toks[i] = type_tok;
  }
  if (td->variadic_param_index < 0)
    while (args->nb < td->nb_type_params) {
      int argument = instantiate_default_template_arg(td, args, args->nb);
      if (!argument) return 0;
      args->toks[args->nb++] = argument;
    }
  for (i = 0; i < args->nb; ++i)
    if (!validate_template_value_parameter_type(td, args, i)) return 0;
  return 1;
}

static int infer_template_args_from_call_with_explicit(TemplateDef *td,
                                         CType *arg_types, int arg_count,
                                         TemplateArgList *args,
                                         TemplateArgList *explicit_args)
{
  TokenString *pattern = cpp_expand_alias_deduction_patterns(td);
  TemplateDef expanded;
  int result;
  if (pattern) {
    expanded = *td;
    expanded.def_str = pattern;
    td = &expanded;
  }
  result = infer_template_args_from_call_pattern(td, arg_types, arg_count,
                                                   args, explicit_args);
  if (pattern) tok_str_free(pattern);
  return result;
}

static int infer_template_args_from_call(TemplateDef *td, CType *arg_types,
                                         int arg_count, TemplateArgList *args)
{
  return infer_template_args_from_call_with_explicit(td, arg_types, arg_count,
                                                     args, NULL);
}

static int template_deduced_inst_for_call(TemplateDef *definition,
                                         CType *arguments, int argument_count)
{
  TemplateArgList bindings;
  if (!infer_template_args_from_call(definition, arguments, argument_count, &bindings))
    return 0;
  return template_lookup_inst(definition, &bindings);
}

/* Compare instantiated parameter types before committing to one template
   body. Parameter declaration replay uses the same reference and pointer
   types as concrete overload resolution, preserving every cv level. */
static int template_call_signature_rank_bound(TemplateDef *td, CType *arguments,
                                        int argument_count,
                                        TemplateArgList *explicit_bindings, int compare_categories)
{
  TemplateArgList bindings;
  TokenString *parameters;
  CType function;
  Sym *parameter;
  int i, begin = -1, end = -1, depth = 0, rank = 0;
  static int active;
  if (active || !td->def_str) return -2;
  for (i = 0; i + 1 < td->def_str->len; ++i)
    if (td->def_str->str[i] == td->name_tok && td->def_str->str[i + 1] == '(')
    { begin = i + 2; break; }
    else if (td->def_str->str[i] == TOK_OPERATOR
             && i + 2 < td->def_str->len
             && get_cpp_binary_operator_method_tok(td->def_str->str[i + 1])
                  == td->name_tok
             && td->def_str->str[i + 2] == '(')
    { begin = i + 3; break; }
  if (begin < 0) return -2;
  for (i = begin; i < td->def_str->len; ++i) {
    int token = td->def_str->str[i];
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(td->def_str->str, td->def_str->len, i);
      continue;
    }
    if (token == '(') ++depth;
    else if (token == ')') {
      if (!depth) { end = i; break; }
      --depth;
    }
  }
  if (end < 0) return -2;
  active = 1;
  if (explicit_bindings)
    bindings = *explicit_bindings;
  else if (!infer_template_args_from_call(td, arguments, argument_count, &bindings)) {
    active = 0;
    return -1;
  }
  parameters = tok_str_alloc();
  for (i = begin; i < end; ++i) {
    int token = td->def_str->str[i];
    if (TOK_HAS_VALUE(token))
      tok_str_add_record(parameters, td->def_str->str, td->def_str->len, &i);
    else if (td->variadic_param_index >= 0
             && token == td->type_param_toks[td->variadic_param_index])
      tok_str_add(parameters, token);
    else
      tok_str_add(parameters, template_subst_token(td, &bindings, 0,
                                                  NULL, NULL, 0, token));
  }
  tok_str_add(parameters, TOK_EOF);
  if (td->variadic_param_index >= 0) {
    int pack = td->variadic_param_index;
    TokenString *expanded = expand_template_expression_packs(parameters,
      td->type_param_toks[pack], 0, 0, &bindings.toks[pack], bindings.nb - pack);
    tok_str_free(parameters);
    parameters = expanded;
  }
  /* Parameter replay always runs under a substitution probe: forming a
     substituted parameter type can fail (for example when instantiating a
     class template whose defaulted argument is ill-formed for this binding),
     and inside overload or partial-ordering deduction that has to drop the
     candidate instead of aborting the translation unit. */
  {
    TokenString *signature = tok_str_alloc();
    int valid;
    tok_str_add(signature, TOK_VOID);
    tok_str_add(signature, '(');
    tok_str_append_without_eof(signature, parameters);
    tok_str_add(signature, ')');
    tok_str_add(signature, TOK_EOF);
    valid = template_probe_type_ex(signature, 0, td->namespace_tok,
                                   td->member_class_tok, 0, &function);
    tok_str_free(signature);
    if (!valid) { tok_str_free(parameters); active = 0; return -1; }
  }
  tok_str_free(parameters);
  /* Member deduction also serves explicit specialization declarations,
     whose parameter types do not carry expression value categories. */
  if (compare_categories == 2) { active = 0; return 0; }
  parameter = function.ref->next;
  for (i = 0; i < argument_count; ++i) {
    int conversion;
    if (!parameter) {
      if (function.ref->f.func_type == FUNC_ELLIPSIS) { rank += 16; continue; }
      rank = -1; break;
    }
    conversion = call_arg_match_rank(&parameter->type, &arguments[i]);
    if (conversion < 0) { rank = -1; break; }
    if (compare_categories && conversion <= 2 && is_reference_type(&parameter->type)) {
      CType source = arguments[i];
      source.t &= ~(VT_RVALUE_REFERENCE | VT_NULLPTR_TYPE);
      if (is_reference_type(&source)) source = *pointed_type(&source);
      if (is_compatible_unqualified_types(pointed_type(&parameter->type), &source))
        conversion = 1;
    }
    rank += conversion;
    parameter = parameter->next;
  }
  active = 0;
  return rank;
}

static TemplateDef *find_explicit_function_template_for_call(int name,
                                      CType *arguments, int argument_count,
                                      TemplateArgList *explicit_arguments,
                                      TemplateArgList *selected_bindings)
{
  TemplateDef *definition, *best = NULL;
  int best_rank = 0;
  for (definition = template_def_candidates(name); definition;
       definition = definition->lookup_bucket_next) {
    TemplateArgList bindings;
    int rank;
    if (definition->is_class || definition->is_alias
        || template_def_lookup_tok(definition) != name
        || argument_count < definition->func_min_args
        || (!definition->func_is_variadic
            && argument_count > definition->func_max_args)
        || (definition->variadic_param_index < 0
            && explicit_arguments->nb > definition->nb_type_params)) continue;
    if (!infer_template_args_from_call_with_explicit(definition, arguments,
                   argument_count, &bindings, explicit_arguments)) continue;
    if (!template_return_substitution_matches_bound(definition, &bindings))
      continue;
    rank = template_call_signature_rank_bound(definition, arguments,
                                                argument_count, &bindings, 0);
    if (rank == -1) continue;
    if (rank == -2) rank = argument_count;
    if (!best || rank < best_rank) {
      best = definition;
      best_rank = rank;
      *selected_bindings = bindings;
    }
  }
  return best;
}

#include "cprimegen_template_ordering.inc"

/* Select an unresolved function name from its target signature. */
static Sym *resolve_function_address(CType *source, CType *target)
{
  CType *signature = target;
  CType arguments[32];
  TemplateArgList bindings;
  TemplateDef *definition;
  Sym *parameter, *function;
  int count = 0, token;
  if ((source->t & VT_BTYPE) == VT_PTR) source = pointed_type(source);
  if ((source->t & VT_BTYPE) != VT_FUNC || !source->ref
      || !source->ref->f.func_unresolved_overload) return NULL;
  if ((signature->t & VT_BTYPE) == VT_PTR) signature = pointed_type(signature);
  if ((signature->t & VT_BTYPE) != VT_FUNC || !signature->ref) return NULL;
  if (source->ref->template_address_arguments) {
    TemplateArgList explicit_args;
    TemplateDef *selected = NULL;
    Sym *match = NULL;
    if (!get_explicit_member_arguments(source->ref->template_address_arguments, &explicit_args)) return NULL;
    for (parameter = signature->ref->next; parameter; parameter = parameter->next) {
      if (count == 32) return NULL;
      arguments[count++] = parameter->type;
    }
    for (definition = template_def_candidates(source->ref->v); definition;
         definition = definition->lookup_bucket_next) {
      if (definition->is_class || definition->is_alias
          || template_def_lookup_tok(definition) != source->ref->v
          || !infer_template_args_from_call_with_explicit(definition, arguments, count,
                                                          &bindings, &explicit_args)) continue;
      token = instantiate_template_if_needed(definition, &bindings);
      compile_pending_template_specs_without_member_flush();
      function = global_symbol_find(token);
      if (!function || !is_compatible_types(&function->type, signature)) continue;
      if (selected) {
        int order = compare_free_function_template_order(definition, selected);
        if (!order) return NULL;
        if (order < 0) continue;
      }
      selected = definition;
      match = function;
    }
    return match;
  }
  {
    FreeFuncOverload *overload;
    Sym *match = NULL;
    for (overload = free_func_candidates(source->ref->v); overload; overload = overload->name_next) {
      Sym *candidate;
      if (overload->name_tok != source->ref->v
          || find_template_inst_owner_entry(overload->mangled_tok)
          || !is_compatible_types(&overload->func_type, signature)) continue;
      candidate = find_global_symbol(overload->mangled_tok);
      if (!candidate) continue;
      if (match && match != candidate) return NULL;
      match = candidate;
    }
    if (match) return match;
  }
  for (parameter = signature->ref->next; parameter; parameter = parameter->next) {
    if (count == 32) return NULL;
    arguments[count++] = parameter->type;
  }
  definition = find_function_template_for_call(source->ref->v, arguments, count, &bindings);
  if (!definition) return NULL;
  token = instantiate_template_if_needed(definition, &bindings);
  compile_pending_template_specs_without_member_flush();
  function = global_symbol_find(token);
  if (!function || !is_compatible_types(&function->type, signature)) return NULL;
  return function;
}
