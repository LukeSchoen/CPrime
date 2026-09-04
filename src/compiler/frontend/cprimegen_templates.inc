static int template_typedef_is_type_pos(TokenString *spec);
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
      pointer_pattern = 1;
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
        return 0;
      }
    }
  }
  if (pointer_pattern)
    return (arg_value_type.t & VT_BTYPE) == VT_PTR;
  return 1;
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

static TemplateDef *find_function_template_for_call(int name_tok,
                                                    CType *arg_types,
                                                    int arg_count)
{
  TemplateDef *td;
  TemplateDef *variadic = NULL;
  TemplateDef *best_defaulted = NULL;
  TemplateDef *exact_unpreferred = NULL;
  int exact_specificity = -1;
  int best_min = 0;

  for (td = template_def_candidates(name_tok); td;
       td = td->lookup_bucket_next)
  {
    if (td->is_class)
      continue;
    if ((td->lookup_tok ? td->lookup_tok : td->name_tok) != name_tok)
      continue;
    if (!td->func_is_variadic && td->func_min_args == arg_count)
    {
      int first_matches =
        template_def_first_param_matches_call(td, arg_types, arg_count);
      if (first_matches)
      {
        int specificity = template_def_first_param_class_tok(td) ? 2
                          : template_def_first_param_pointer_pattern(td) ? 1
                          : 0;
        if (!exact_unpreferred || specificity > exact_specificity)
        {
          exact_unpreferred = td;
          exact_specificity = specificity;
        }
      }
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
  if (exact_unpreferred)
    return exact_unpreferred;
  if (best_defaulted)
    return best_defaulted;
  return variadic;
}

static TemplateDef *find_function_template_def(int name_tok)
{
  TemplateDef *td;

  for (td = template_def_candidates(name_tok); td;
       td = td->lookup_bucket_next)
  {
    if (td->is_class)
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
    if (td->is_class)
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
  *sig_hash = 2166136261u;
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
  }
  if (i >= n)
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
    if (t == TOK_LT)
    {
      angle++;
      if (!skip_default)
      {
        *sig_hash ^= (unsigned)t;
        *sig_hash *= 16777619u;
      }
    }
    else if ((t == TOK_GT || t == TOK_SAR) && angle > 0)
    {
      angle--;
      if (!skip_default)
      {
        *sig_hash ^= (unsigned)t;
        *sig_hash *= 16777619u;
      }
    }
    else if (angle == 0)
    {
      if (t == '(')
      {
        paren++;
        if (!skip_default)
        {
          *sig_hash ^= (unsigned)t;
          *sig_hash *= 16777619u;
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
          *sig_hash ^= (unsigned)t;
          *sig_hash *= 16777619u;
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
        *sig_hash ^= (unsigned)t;
        *sig_hash *= 16777619u;
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
          *sig_hash ^= (unsigned)t;
          *sig_hash *= 16777619u;
        }
      }
      else if (!skip_default)
      {
        *sig_hash ^= (unsigned)t;
        *sig_hash *= 16777619u;
      }
    }
    else if (!skip_default)
    {
      *sig_hash ^= (unsigned)t;
      *sig_hash *= 16777619u;
    }
    if (t != TOK_LINENUM)
      in_param = 1;
  }
}

static unsigned template_function_merge_signature_hash(TokenString *str,
                                                       int name_tok)
{
  unsigned hash = 2166136261u;
  int i, n = str->len, paren = 0, angle = 0, bracket = 0;
  int skip_default = 0;

  for (i = 0; i + 1 < n; ++i)
    if (str->str[i] == name_tok && str->str[i + 1] == '(')
    {
      i += 2;
      break;
    }
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
            || str->str[next] == '='))
      continue;

    if (!skip_default)
    {
      hash ^= (unsigned)t;
      hash *= 16777619u;
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

static void canonicalize_template_member_signature_namespace_types(
    TokenString *str)
{
  int i;

  if (!str || !nb_namespace_stack)
    return;
  for (i = 0; i < str->len && str->str[i] != '{'; ++i)
  {
    int t = str->str[i];
    int qualified_tok;
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      continue;
    }
    if (t < TOK_UIDENT)
      continue;
    qualified_tok = find_current_namespace_tok(t);
    if (qualified_tok != t
        && (struct_find(qualified_tok)
            || find_class_template_def(qualified_tok)))
      str->str[i] = qualified_tok;
  }
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
                && i + 5 < n && str->str[i + 5] == '(')))
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
        canonicalize_template_member_signature_namespace_types(str);
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
static int token_string_has_variadic_pack(TokenString *str);
static int template_member_is_call_specialized(TemplateMemberDef *md);

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
  int is_member_template = template_member_is_call_specialized(md);

  for (i = 0; i < md->nb_inst; ++i)
    if (md->inst_type_toks[i] == type_tok
        && (!(is_variadic || is_member_template)
            || (md->inst_pack_toks && md->inst_pack_toks[i] == pack_tok)))
      return md->inst_func_toks && md->inst_func_toks[i]
             ? md->inst_func_toks[i] : 1;
  return 0;
}

static int template_member_lookup_inst_index(TemplateMemberDef *md,
                                             int type_tok, int pack_tok)
{
  int i;
  int is_variadic = token_string_has_variadic_pack(md->def_str);
  int is_member_template = template_member_is_call_specialized(md);

  for (i = 0; i < md->nb_inst; ++i)
    if (md->inst_type_toks[i] == type_tok
        && (!(is_variadic || is_member_template)
            || (md->inst_pack_toks && md->inst_pack_toks[i] == pack_tok)))
      return i;
  return -1;
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
    md->inst_func_toks = cprime_realloc(md->inst_func_toks,
                                     md->al_inst *sizeof(int));
    md->inst_states = cprime_realloc(md->inst_states,
                                     md->al_inst * sizeof(unsigned char));
    md->inst_call_arg_types = cprime_realloc(
      md->inst_call_arg_types, md->al_inst * 16 * sizeof(CType));
    md->inst_call_arg_counts = cprime_realloc(
      md->inst_call_arg_counts, md->al_inst * sizeof(unsigned char));
  }
  md->inst_type_toks[md->nb_inst++] = type_tok;
  md->inst_pack_toks[md->nb_inst - 1] = pack_tok;
  md->inst_func_toks[md->nb_inst - 1] = 0;
  md->inst_states[md->nb_inst - 1] = 0;
  md->inst_call_arg_counts[md->nb_inst - 1] =
    nb_template_member_call_arg_types < 16
      ? nb_template_member_call_arg_types : 16;
  if (md->inst_call_arg_counts[md->nb_inst - 1])
    memcpy(&md->inst_call_arg_types[(md->nb_inst - 1) * 16],
           template_member_call_arg_types,
           md->inst_call_arg_counts[md->nb_inst - 1] * sizeof(CType));
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
    if (value_param_tok && tok == value_param_tok)
      tok_str_add_cint(str, 1);
    else
      tok_str_add_tok(str);
  }
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
  static int template_keyword_tok;
  if (!template_keyword_tok)
    template_keyword_tok = tok_alloc_const("template");
  return t == template_keyword_tok;
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
    if (str->str[i] >= TOK_UIDENT
        && !strcmp(get_tok_str(str->str[i], NULL), "friend"))
      return 1;
  return 0;
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
    else if (tok >= TOK_UIDENT
             && !strcmp(get_tok_str(tok, NULL), "noexcept"))
    {
      next();
      if (tok == '(')
      {
        int paren = 1;
        next();
        while (tok != TOK_EOF && paren > 0)
        {
          if (tok == '(')
            ++paren;
          else if (tok == ')')
            --paren;
          next();
        }
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

static int type_has_member_func_name(CType *type, int member_tok)
{
  Sym *func_sym, *field;
  MemberFuncOverload *o;
  ClassBaseInfo *info;
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
  if (func_sym && !IS_ASM_SYM(func_sym))
    return 1;
  for (info = class_base_infos; info; info = info->next)
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
          return destructor_tok == md->class_tok
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
          return t == md->class_tok ? TOK_CONSTRUCTOR1 : t;
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
static void publish_template_member_proto(TokenString *proto)
{
  int saved_tok = tok;
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
    sym_pop(&local_stack, NULL, 1);
    sym_relink_local_types(saved_local_stack);
  }
  local_stack = NULL;
  local_scope = 0;
  defer_pending_member_funcs = 1;
  begin_macro(proto, 1);
  next();
  decl(VT_CONST);
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  if (saved_local_stack)
  {
    sym_unlink_local_types(saved_local_stack);
    sym_relink_stack(saved_local_stack);
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
  tok = saved_tok;
  tokc = saved_tokc;
}

static int template_member_method_end_index(TemplateMemberDef *md, int index,
                                            int method_tok);

static int template_member_def_param_count(TemplateMemberDef *md,
                                           int method_tok)
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

static int template_member_def_first_param_is_array_ref(TemplateMemberDef *md,
                                                        int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0;
    int saw_ref = 0, saw_array = 0;

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

static int template_member_def_first_param_type_tok(TemplateMemberDef *md,
                                                    int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0;

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

static int template_member_def_is_const_qualified(TemplateMemberDef *md,
                                                  int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i < n; ++i)
  {
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
      == (method_tok == TOK_CONSTRUCTOR1 ? md->class_tok : method_tok))
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

static int template_member_type_param_tok(TemplateMemberDef *md)
{
  int i = 0, angle = 0, expect_name = 0;

  while (i + 1 < md->def_str->len && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= md->def_str->len || !is_template_keyword_tok(md->def_str->str[i]))
    return 0;
  ++i;
  if (i >= md->def_str->len
      || (md->def_str->str[i] != TOK_LT && md->def_str->str[i] != '<'))
    return 0;
  angle = 1;
  for (++i; i < md->def_str->len && angle > 0; ++i)
  {
    int t = md->def_str->str[i];
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
static int template_member_is_call_specialized(TemplateMemberDef *md)
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
  if (getenv("CPC_TRACE_SHALLOW")
      && token_string_contains_tok(raw, tok_alloc_const("clVector2")))
    fprintf(stderr, "CPC_SHALLOW param-match func=%s class=%s method=%s rawlen=%d\n",
            funcname ? funcname : "<none>",
            get_tok_str(class_mangled_tok, NULL),
            get_tok_str(method_tok, NULL), raw->len);
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
    type_decl(&param_type, &ad, &v, TYPE_DIRECT | TYPE_PARAM);
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
    return is_compatible_unqualified_types(&param_value_type,
                                           &arg_value_type);
  }
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
  int i, n;

  if (!md || !md->def_str)
    return -1;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    int j, angle = 0, paren = 0, bracket = 0;

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
      {
        char name[512];
        int k;
        snprintf(name, sizeof(name), "__cpc_member_pack");
        for (k = param_index; k < explicit_arg_count; ++k)
        {
          pstrcat(name, sizeof(name), "_");
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
                                                int method_tok)
{
  int i, n, j, paren = 0, angle = 0, bracket = 0;
  int param_index = 0;

  if (!md || !md->def_str)
    return -1;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
    j = template_member_method_end_index(md, i, method_tok);
    if (!j)
      continue;
    while (j + 1 < n && md->def_str->str[j] == TOK_LINENUM)
      j += 2;
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
        ++param_index;
      else if (t == TOK_DOTS && !paren && !angle && !bracket)
        return param_index;
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
  char name[512];
  Sym *arg;
  int min_args, max_args, is_variadic;
  unsigned sig_hash;

  snprintf(name, sizeof(name), "%s_%s",
           get_tok_str(struct_tok, NULL), get_tok_str(method_tok, NULL));
  if (func_type && (func_type->t & VT_CONSTANT))
    pstrcat(name, sizeof(name), "_const");
  if (def_str)
  {
    char suffix[24];
    analyze_template_function_signature(def_str, method_tok,
                                        &min_args, &max_args,
                                        &is_variadic, &sig_hash);
    snprintf(suffix, sizeof suffix, "_h%08x", sig_hash);
    pstrcat(name, sizeof(name), suffix);
  }
  if (member_type_arg_tok > 0)
  {
    CType member_arg_type;
    pstrcat(name, sizeof(name), "__cpc_template_type_");
    if (make_type_from_type_arg_tok(&member_arg_type,
                                    member_type_arg_tok))
    {
      if (!append_type_mangle(name, sizeof(name), &member_arg_type))
        cprime_error("unsupported member-template type argument");
    }
    else
      pstrcat(name, sizeof(name), get_tok_str(member_type_arg_tok, NULL));
  }
  if (func_type && func_type->ref)
    for (arg = func_type->ref->next; arg; arg = arg->next)
      if (!append_type_mangle(name, sizeof(name), &arg->type))
        cprime_error("unsupported member-template parameter type");
  return tok_alloc_const(name);
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
    else
      return 0;
  }
  return saw_auto;
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
  return i < str->len && str->str[i] == TOK_AUTO;
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

static int template_member_def_is_static_signature(TemplateMemberDef *md)
{
  int j, scan_limit;

  if (!md || !md->def_str)
    return 0;
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
      return 1;
  return 0;
}

static int class_has_static_member_template_name(int class_tok, int member_tok)
{
  TemplateDef *td = find_class_template_def_for_class_tok(class_tok);
  int i;

  if (!td)
    return 0;
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (md->class_tok == td->name_tok
        && template_member_def_method_tok(md) == member_tok
        && template_member_def_is_static_signature(md))
      return 1;
  }
  return 0;
}

static int instantiate_static_member_template_for_replay(int class_mangled_tok,
                                                         int member_tok,
                                                         int class_type_tok,
                                                         int member_type_tok);

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

static int count_token_call_args(TokenString *str, int open_paren_index)
{
  int i, depth = 0, saw_arg = 0, count = 0;

  if (!str || open_paren_index < 0 || open_paren_index >= str->len
      || str->str[open_paren_index] != '(')
    return -1;
  for (i = open_paren_index + 1; i < str->len; ++i)
  {
    int t = str->str[i];
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

static int template_member_returns_call_tok(TemplateMemberDef *md, int call_tok)
{
  int i;

  if (!md || !md->def_str || !call_tok)
    return 0;
  for (i = 0; i + 2 < md->def_str->len; ++i)
  {
    int ri;
    if (md->def_str->str[i] != TOK_RETURN)
      continue;
    ri = i + 1;
    while (ri < md->def_str->len && md->def_str->str[ri] == TOK_LINENUM)
      ri += 2;
    if (ri + 1 < md->def_str->len && md->def_str->str[ri] == call_tok
        && md->def_str->str[ri + 1] == '(')
      return 1;
  }
  return 0;
}

static int template_member_return_clcreatevector_inst_tok(TemplateMemberDef *md,
                                                          int elem_tok)
{
  int i, call_tok = tok_alloc_const("clCreateVector");

  if (!md || !md->def_str || !elem_tok)
    return 0;
  for (i = 0; i + 2 < md->def_str->len; ++i)
  {
    int ri, arity, vector_tok = 0;
    if (md->def_str->str[i] != TOK_RETURN)
      continue;
    ri = i + 1;
    while (ri < md->def_str->len && md->def_str->str[ri] == TOK_LINENUM)
      ri += 2;
    if (ri + 1 >= md->def_str->len || md->def_str->str[ri] != call_tok
        || md->def_str->str[ri + 1] != '(')
      continue;
    arity = count_token_call_args(md->def_str, ri + 1);
    if (arity == 2)
      vector_tok = tok_alloc_const("clVector2");
    else if (arity == 3)
      vector_tok = tok_alloc_const("clVector3");
    else if (arity == 4)
      vector_tok = tok_alloc_const("clVector4");
    if (vector_tok)
      return class_template_inst_tok_for_member_arg(vector_tok, elem_tok);
  }
  return 0;
}

static void adjust_template_member_auto_return_for_call(CType *type,
                                                        int method_tok,
                                                        CType *arg_types,
                                                        int explicit_arg_count,
                                                        Sym *func_sym)
{
  int struct_tok, type_tok = 0, member_type_arg_tok = 0, ret_struct_tok;
  int i, j;
  CType ret_type;
  TemplateDef *td = NULL;
  TemplateMemberDef *matched_md = NULL;
  MemberFuncOverload *o;

  if (!type || !arg_types || !func_sym || explicit_arg_count != 1
      || (func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    return;
  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return;
  if (profile_scans_enabled)
    ++profile_template_calls;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *candidate = template_defs[i];
    if (!candidate->is_class)
      continue;
    for (j = 0; j < candidate->nb_inst; ++j)
      if (candidate->inst_name_toks[j] == struct_tok)
      {
        td = candidate;
        type_tok = candidate->inst_type_toks[j * candidate->nb_type_params];
        break;
      }
    if (td)
      break;
  }
  if (!td || !type_tok)
    return;
  if (arg_types)
  {
    member_type_arg_tok = first_template_arg_tok_from_inst_type(arg_types);
    if (!member_type_arg_tok)
      member_type_arg_tok = template_type_tok_from_ctype(arg_types);
  }
  if (!member_type_arg_tok)
    member_type_arg_tok = type_tok;
  if (member_type_arg_tok != type_tok)
  {
    CType inferred_arg_type, class_arg_type;
    if (make_type_from_type_arg_tok(&inferred_arg_type, member_type_arg_tok)
        && make_type_from_type_arg_tok(&class_arg_type, type_tok)
        && is_compatible_unqualified_types(&inferred_arg_type,
                                           &class_arg_type))
      member_type_arg_tok = type_tok;
  }
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *md = template_member_defs[i];
    if (md->class_tok == td->name_tok
        && template_member_def_method_tok(md) == method_tok
        && !template_member_def_is_scoped(md)
        && token_string_is_auto_type_token_prefix(md->def_str)
        && template_member_returns_call_tok(md,
                                            tok_alloc_const("clCreateVector"))
        && template_member_first_param_matches_arg(
             md, method_tok, struct_tok, type_tok, member_type_arg_tok,
             &arg_types[0]))
    {
      matched_md = md;
      break;
    }
  }
  if (!matched_md)
    return;
  ret_struct_tok =
    template_member_return_clcreatevector_inst_tok(matched_md,
                                                  promoted_template_element_tok(
                                                      type_tok,
                                                      member_type_arg_tok));
  if (!ret_struct_tok
      || !template_return_ctype_from_struct_tok(&ret_type, ret_struct_tok))
    return;
  func_sym->type.ref->type = ret_type;
  for (o = member_func_overloads; o; o = o->next)
    if (o->mangled_tok == func_sym->v && o->func_type.ref)
      o->func_type.ref->type = ret_type;
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
  TemplateInstOwner *owner;
  TemplateDef *td = find_template_def(class_tok);

  if (td && td->is_class)
    return td;
  owner = find_template_inst_owner_entry(class_tok);
  if (owner && owner->owner->is_class)
    return owner->owner;
  /* Token identity is the common case, but replay can intern an equivalent
     spelling in a distinct namespace/token context.  Retain the old spelling
     fallback for that uncommon case. */
  for (i = 0; i < nb_template_defs; ++i)
  {
    td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (!strcmp(get_tok_str(td->inst_name_toks[j], NULL),
                  get_tok_str(class_tok, NULL)))
        return td;
  }
  return NULL;
}

static int is_class_template_instantiation_tok(int class_tok)
{
  int i, j;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == class_tok
          || !strcmp(get_tok_str(td->inst_name_toks[j], NULL),
                     get_tok_str(class_tok, NULL)))
        return 1;
  }
  return 0;
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
  int i, j;

  if (class_has_member_template_name(class_tok, member_tok))
    return 1;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (profile_scans_enabled)
      ++profile_template_def_scans;
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

/* Prefer the concrete interface assembled by the class-template prepass when
   inferring the type of a member call.  Explicit-return members do not need
   (and often cannot use) body probing: their bodies can reference parameters
   that are deliberately absent from an unevaluated return-expression probe. */
static int template_member_materialized_return_type(int class_mangled_tok,
                                                    int method_tok,
                                                    int explicit_arg_count,
                                                    CType *ret_type)
{
  MemberFuncOverload *o;

  if (!ret_type)
    return 0;
  for (o = member_func_candidates(class_mangled_tok, method_tok);
       o; o = o->bucket_next)
  {
    CType candidate;
    if (o->struct_tok != class_mangled_tok || o->method_tok != method_tok
        || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count
        || !o->func_type.ref)
      continue;
    candidate = o->func_type.ref->type;
    while ((candidate.t & VT_BTYPE) == VT_FUNC && candidate.ref)
      candidate = candidate.ref->type;
    if ((candidate.t & VT_BTYPE) == VT_VOID)
      continue;
    if (is_reference_type(&candidate))
      candidate = *pointed_type(&candidate);
    candidate.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_RVALUE_REFERENCE);
    *ret_type = candidate;
    return 1;
  }
  return 0;
}

static int template_return_probe_has_free_callable(int name_tok)
{
  int qualified_tok = find_current_namespace_tok(name_tok);
  FreeFuncOverload *o;

  if (find_function_template_def(name_tok)
      || (qualified_tok != name_tok
          && find_function_template_def(qualified_tok)))
    return 1;
  for (o = free_func_overloads; o; o = o->next)
    if (o->name_tok == name_tok || o->name_tok == qualified_tok)
      return 1;
  return 0;
}

static int template_member_return_functional_class_inst_tok(
    TemplateMemberDef *md, int class_mangled_tok, int start, int end)
{
  TemplateDef *return_td;
  TemplateArgList args;
  int i, t, angle = 0, arg_tok = 0, complex_arg = 0;

  if (!md || !md->def_str || start < 0 || start >= end)
    return 0;
  i = start;
  while (i < end && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= end || md->def_str->str[i] < TOK_UIDENT)
    return 0;
  return_td = find_class_template_def(md->def_str->str[i]);
  if (!return_td || !return_td->is_class)
    return 0;
  ++i;
  while (i < end && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= end
      || (md->def_str->str[i] != TOK_LT && md->def_str->str[i] != '<'))
    return 0;

  args.nb = 0;
  angle = 1;
  for (++i; i < end && angle > 0; ++i)
  {
    t = md->def_str->str[i];
    if (t == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (t == TOK_LT || t == '<')
    {
      ++angle;
      complex_arg = 1;
      continue;
    }
    if (t == TOK_GT || t == '>')
    {
      if (--angle == 0)
      {
        if (!arg_tok || complex_arg || args.nb >= 16)
          return 0;
        args.toks[args.nb++] = arg_tok;
        ++i;
        break;
      }
      complex_arg = 1;
      continue;
    }
    if (t == TOK_SAR)
      return 0;
    if (angle == 1 && t == ',')
    {
      if (!arg_tok || complex_arg || args.nb >= 16)
        return 0;
      args.toks[args.nb++] = arg_tok;
      arg_tok = 0;
      complex_arg = 0;
      continue;
    }
    if (angle != 1 || arg_tok)
    {
      complex_arg = 1;
      continue;
    }
    arg_tok = template_member_class_arg_subst_tok(md, class_mangled_tok, t);
    if (!arg_tok)
      arg_tok = t;
  }
  if (angle != 0 || args.nb == 0)
    return 0;
  while (i < end && md->def_str->str[i] == TOK_LINENUM)
    i += 2;
  if (i >= end || md->def_str->str[i] != '(')
    return 0;
  t = template_lookup_inst(return_td, &args);
  if (!t)
  {
    t = make_template_inst_name_tok(return_td, &args);
    note_template_inst(return_td, &args, t);
  }
  if (!struct_find(t))
  {
    TokenString *forward = tok_str_alloc();
    tok_str_add(forward, TOK_STRUCT);
    tok_str_add(forward, t);
    tok_str_add(forward, ';');
    tok_str_add(forward, TOK_EOF);
    queue_pending_template_spec(forward, 0);
    compile_pending_template_specs_without_member_flush();
  }
  return struct_find(t) ? t : 0;
}

/* Infer the return type of an auto-return out-of-class template member from
   its first return expression.  The expression is replayed in a
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
  {
    return 1;
  }

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

  {
    int direct_class_tok = template_member_return_functional_class_inst_tok(
      md, class_mangled_tok, start, end);
    if (direct_class_tok
        && template_return_ctype_from_struct_tok(ret_type, direct_class_tok))
    {
      template_member_note_cached_ret(md, class_mangled_tok, ret_type);
      return 1;
    }
  }

  expr = tok_str_alloc();
  for (i = start; i < end; ++i)
  {
    int t = md->def_str->str[i];
    int class_arg_tok;
    int ni2 = i + 1;
    if (TOK_HAS_VALUE(t))
    {
      tok_str_add_record(expr, md->def_str->str, md->def_str->len, &i);
      continue;
    }
    class_arg_tok = template_member_class_arg_subst_tok(md,
                                                        class_mangled_tok,
                                                        t);
    if (class_arg_tok)
      t = class_arg_tok;
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
        int call_arg_count = count_token_call_args(md->def_str, ni2);
        memset(&called_ret, 0, sizeof(called_ret));
        if ((template_member_materialized_return_type(
               class_mangled_tok, t, call_arg_count, &called_ret)
             || infer_template_member_auto_return_type_inner(
                  called_member, class_mangled_tok, &called_ret, depth + 1))
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
    if (t == this_tok)
    {
      tok_str_add(expr, probe_this_tok);
      continue;
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
        probe_sym = global_symbol_find(t);
      if (!probe_sym && !struct_find(t)
          && !find_class_template_def(find_current_namespace_tok(t))
          && !(ni < md->def_str->len && md->def_str->str[ni] == '('
               && template_return_probe_has_free_callable(t)))
      {
        if (getenv("CPC_TRACE_AUTO_RETURN"))
          fprintf(stderr,
                  "CPC_AUTO_RETURN_UNRESOLVED class=%s method=%s tok=%s\n",
                  get_tok_str(class_mangled_tok, NULL),
                  get_tok_str(template_member_def_method_tok(md), NULL),
                  get_tok_str(t, NULL));
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
  if (getenv("CPC_TRACE_AUTO_RETURN")
      && strstr(get_tok_str(class_mangled_tok, NULL),
                getenv("CPC_TRACE_AUTO_RETURN")))
  {
    int pi;
    fprintf(stderr, "CPC_AUTO_RETURN_PROBE class=%s method=%s toks=",
            get_tok_str(class_mangled_tok, NULL),
            get_tok_str(template_member_def_method_tok(md), NULL));
    for (pi = 0; pi < probe->len && pi < 100; ++pi)
    {
      if (TOK_HAS_VALUE(probe->str[pi]))
      {
        fprintf(stderr, "%s<value>", pi ? " " : "");
        pi += tok_str_value_extra_words(probe->str, probe->len, pi);
        continue;
      }
      fprintf(stderr, "%s%s", pi ? " " : "",
              get_tok_str(probe->str[pi], NULL));
    }
    fprintf(stderr, "\n");
  }
  if (getenv("CPC_TRACE_MATRIX")
      && strstr(get_tok_str(class_mangled_tok, NULL), "clMatrix4x4"))
  {
    int pi;
    fprintf(stderr, "CPC_MATRIX_AUTO_PROBE method=%s toks=",
            get_tok_str(template_member_def_method_tok(md), NULL));
    for (pi = 0; pi < probe->len && pi < 80; ++pi)
    {
      if (TOK_HAS_VALUE(probe->str[pi]))
      {
        fprintf(stderr, "%s<value>", pi ? " " : "");
        pi += tok_str_value_extra_words(probe->str, probe->len, pi);
        continue;
      }
      fprintf(stderr, "%s%s", pi ? " " : "",
              get_tok_str(probe->str[pi], NULL));
    }
    fprintf(stderr, "\n");
  }
  infer_expr_type_from_tokens(probe, &inferred);
  if (getenv("CPC_TRACE_AUTO_RETURN")
      && strstr(get_tok_str(class_mangled_tok, NULL),
                getenv("CPC_TRACE_AUTO_RETURN")))
    fprintf(stderr, "CPC_AUTO_RETURN_RESULT class=%s method=%s t=%04x struct=%s\n",
            get_tok_str(class_mangled_tok, NULL),
            get_tok_str(template_member_def_method_tok(md), NULL),
            inferred.t, get_tok_str(get_struct_type_name_tok(&inferred), NULL));
  if (getenv("CPC_TRACE_MATRIX")
      && strstr(get_tok_str(class_mangled_tok, NULL), "clMatrix4x4"))
    fprintf(stderr, "CPC_MATRIX_AUTO_INFER method=%s inferred=%04x/%s depth=%d vdepth=%d\n",
            get_tok_str(template_member_def_method_tok(md), NULL),
            inferred.t, get_tok_str(get_struct_type_name_tok(&inferred), NULL),
            depth, (int)(vtop - vstack));
  while (vtop > saved_vtop)
    vpop();
  tok_str_free(expr);
  tok_str_free(probe);
  if ((inferred.t & VT_BTYPE) == VT_VOID)
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

static void instantiate_template_member_if_needed(TemplateMemberDef *md,
                                                  int type_tok,
                                                  int class_mangled_tok,
                                                  int member_type_arg_tok)
{
  int saved_tok, method_tok, member_func_tok, paren, i, cv_qualifiers;
  int member_decl_exists = 0;
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
  TokenString *clean_ret = NULL;
  PendingMemberFunc *pm;
  int value_param_tok = 0;
  int member_type_param_tok = 0;
  int auto_return_struct_tok = 0;
  CType inferred_auto_ret;
  int has_inferred_auto_ret = 0;
  int is_static_member = 0;
  int is_member_template_replay = 0;
  int orig_pack_tok = member_type_arg_tok;
  int pack_is_empty = member_type_arg_tok == -1;
  int pack_name_tok = 0;
  int member_type_param_count = 0;
  int pack_arg_start = -1;
  int cached_index = -1;
  int cached_inst = 0;
  CType replay_call_arg_types[16];
  int replay_nb_call_arg_types = nb_template_member_call_arg_types < 16
                                   ? nb_template_member_call_arg_types : 16;
  int saved_nb_extra_params;
  int saved_extra_param_toks[15];
  int saved_extra_arg_toks[15];
  int materialize_body = requested_template_member_body == md
                         && !materializing_template_interface;

  if (replay_nb_call_arg_types)
    memcpy(replay_call_arg_types, template_member_call_arg_types,
           replay_nb_call_arg_types * sizeof(CType));

  if (md->nested_class_tok)
    return;

  method_tok = template_member_def_method_tok(md);
  if (token_string_has_variadic_pack(md->def_str))
    pack_arg_start = template_member_def_pack_param_index(md, method_tok);

  cached_index = template_member_lookup_inst_index(md, type_tok, orig_pack_tok);
  cached_inst = template_member_lookup_inst(md, type_tok, orig_pack_tok);
  /* Interface discovery may be separated from body materialization by the
     template worklist.  Preserve the complete call type vector on the cache
     entry so a later body replay expands the same pack, rather than falling
     back to its first element. */
  if (materialize_body && cached_index >= 0 && md->inst_call_arg_counts)
  {
    replay_nb_call_arg_types = md->inst_call_arg_counts[cached_index];
    if (replay_nb_call_arg_types)
      memcpy(replay_call_arg_types,
             &md->inst_call_arg_types[cached_index * 16],
             replay_nb_call_arg_types * sizeof(CType));
  }
  if (pack_arg_start >= 0 && pack_arg_start < replay_nb_call_arg_types)
    member_type_arg_tok = template_type_tok_from_ctype(
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
  if (getenv("CPC_TRACE_MEMBER_REPLAY")
      && strstr(get_tok_str(class_mangled_tok, NULL),
                getenv("CPC_TRACE_MEMBER_REPLAY")))
    fprintf(stderr, "CPC_MEMBER_STAGE enter class=%s method=%s type=%s pack=%s\n",
            get_tok_str(class_mangled_tok, NULL),
            get_tok_str(method_tok, NULL),
            get_tok_str(type_tok, NULL), get_tok_str(orig_pack_tok, NULL));
  if (getenv("CPC_TRACE_CLBITREF")
      && class_mangled_tok >= TOK_UIDENT
      && strstr(get_tok_str(class_mangled_tok, NULL), "clBitRef"))
    fprintf(stderr, "CPC_CLBITREF_ENTER class=%s type=%s pack=%s method=%s instantiating=%d lookup=%d\n",
            get_tok_str(class_mangled_tok, NULL),
            type_tok ? get_tok_str(type_tok, NULL) : "<none>",
            orig_pack_tok ? get_tok_str(orig_pack_tok, NULL) : "<none>",
            get_tok_str(method_tok, NULL),
            md->instantiating,
            cached_inst);
  if (cached_inst
      && (!materialize_body
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

  /* Mark before replaying the saved definition.  Constructor initializer
     parsing and overload resolution can ask for the same member again while
     this instantiation is still being built; without an in-progress mark, the
     compiler recursively replays the same template body until timeout. */
  if (!template_member_lookup_inst(md, type_tok, orig_pack_tok))
    template_member_note_inst(md, type_tok, orig_pack_tok);
  md->instantiating = 1;

  saved_nb_extra_params = nb_template_member_extra_params;
  memcpy(saved_extra_param_toks, template_member_extra_param_toks,
         sizeof(saved_extra_param_toks));
  memcpy(saved_extra_arg_toks, template_member_extra_arg_toks,
         sizeof(saved_extra_arg_toks));
  nb_template_member_extra_params = 0;

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
        int inferred_arg_tok = 0;
        if (member_type_param_count < replay_nb_call_arg_types)
        {
          int call_arg_index = member_type_param_count;
          if (pack_arg_start >= 0 && member_type_param_count == 0)
            call_arg_index = pack_arg_start;
          inferred_arg_tok = first_template_arg_tok_from_inst_type(
            &replay_call_arg_types[call_arg_index]);
          if (!inferred_arg_tok)
            inferred_arg_tok = template_type_tok_from_ctype(
              &replay_call_arg_types[call_arg_index]);
        }
        if (member_type_param_count == 0)
          member_type_param_tok = tok;
        else if (nb_template_member_extra_params < 15)
        {
          int extra = nb_template_member_extra_params++;
          template_member_extra_param_toks[extra] = tok;
          template_member_extra_arg_toks[extra] = inferred_arg_tok
                                                        ? inferred_arg_tok
                                                        : member_type_arg_tok;
        }
        ++member_type_param_count;
        expect_type_param_name = 0;
      }
      else if (angle == 1 && tok == ',')
      {
        saw_type_param = 0;
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

  for (;;)
  {
    if (tok == TOK_EOF)
      cprime_error("unsupported template member declaration");
    if (tok == tok_explicit
        || (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "explicit")))
    {
      next();
      continue;
    }
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
    if (tok == TOK_OPERATOR && ret_str->len > 0)
    {
      method_tok = parse_cpp_operator_method_tok();
      method_already_parsed = 1;
      break;
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
                     && class_has_static_member_func(class_mangled_tok,
                                                     method_tok);

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
  {
    const char *method_name = get_tok_str(method_tok, NULL);
    int return_struct_tok = 0;
    int promoted_tok = promoted_template_element_tok(type_tok,
                                                      member_type_arg_tok);
    int vector_return_struct_tok =
      template_member_return_clcreatevector_inst_tok(md, promoted_tok);
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
    else if (vector_return_struct_tok
        || !strcmp(method_name, "operator+")
        || !strcmp(method_name, "operator-")
        || !strcmp(method_name, "operator*")
        || !strcmp(method_name, "operator/")
        || !strcmp(method_name, "operator>>")
        || !strcmp(method_name, "operator<<")
        || !strcmp(method_name, "operator"))
    {
      auto_return_struct_tok = vector_return_struct_tok;
      if (!auto_return_struct_tok)
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
      int return_call_open_index = -1;
      int ri;
      for (i = 0; i + 1 < md->def_str->len; ++i)
        if (md->def_str->str[i] == TOK_RETURN)
        {
          ri = i + 1;
          while (ri < md->def_str->len
                 && md->def_str->str[ri] == TOK_LINENUM)
            ri += 2;
          if (ri < md->def_str->len && md->def_str->str[ri] >= TOK_UIDENT)
          {
            return_call_tok = md->def_str->str[ri];
            if (ri + 1 < md->def_str->len
                && md->def_str->str[ri + 1] == '(')
              return_call_open_index = ri + 1;
          }
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
      if (!auto_return_struct_tok && return_call_tok
          && return_call_open_index >= 0)
      {
        TemplateDef *return_td = find_function_template_def_for_arg_count(
          return_call_tok,
          count_token_call_args(md->def_str, return_call_open_index), 0);
        if (return_td)
        {
          TemplateArgList return_args;
          template_arg_list_one(&return_args, type_tok);
          auto_return_struct_tok =
            infer_template_return_struct_tok(return_td, &return_args);
          if (auto_return_struct_tok)
          {
            tok_str_add(sig, TOK_STRUCT);
            tok_str_add(sig, auto_return_struct_tok);
          }
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
            int concrete_tok = template_type_tok_from_ctype(
              &replay_call_arg_types[pack_call_index]);
            int pi;
            tok_str_add(params_str, ',');
            for (pi = pattern_start; pi < pattern_end; ++pi)
            {
              int pt = params_str->str[pi];
              tok_str_add(params_str,
                          pt == member_type_arg_tok && concrete_tok
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
  cv_qualifiers = skip_member_func_cv_qualifiers();
  is_member_template_replay = member_type_param_tok
                              && member_type_param_tok != md->type_param_tok;
  typed_params = tok_str_alloc();
  if (params_str->len > 0)
  {
    int start = params_str->str[0] == ',' ? 1 : 0;
    for (i = start; i < params_str->len; ++i)
      tok_str_add(typed_params, params_str->str[i]);
  }
  tok_str_add(typed_params, TOK_EOF);

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
    if (getenv("CPC_TRACE_MEMBER_REPLAY")
        && strstr(get_tok_str(class_mangled_tok, NULL),
                  getenv("CPC_TRACE_MEMBER_REPLAY")))
      fprintf(stderr, "CPC_MEMBER_STAGE init class=%s method=%s\n",
              get_tok_str(class_mangled_tok, NULL),
              get_tok_str(method_tok, NULL));
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
      /* Initializer argument types are resolved while the prefix is lowered.
         Give that probe the constructor's concrete parameter scope, just as
         the ordinary out-of-class member path does. */
      push_saved_param_scope(typed_params, &saved_param_locals,
                             &saved_param_scope);
      init_prefix = parse_substituted_template_constructor_initializers(
        md, class_mangled_tok, type_tok, member_type_param_tok,
        member_type_arg_tok, value_param_tok, &struct_type);
      pop_saved_param_scope(saved_param_locals, saved_param_scope);
    }
    if (getenv("CPC_TRACE_MEMBER_REPLAY")
        && strstr(get_tok_str(class_mangled_tok, NULL),
                  getenv("CPC_TRACE_MEMBER_REPLAY")))
      fprintf(stderr, "CPC_MEMBER_STAGE init-done class=%s method=%s\n",
              get_tok_str(class_mangled_tok, NULL),
              get_tok_str(method_tok, NULL));
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
    class_sym->type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    struct_type.t = class_sym->type.t & ~(VT_CONSTANT | VT_VOLATILE);
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
    if (getenv("CPC_TRACE_MEMBER_REPLAY")
        && strstr(get_tok_str(class_mangled_tok, NULL),
                  getenv("CPC_TRACE_MEMBER_REPLAY")))
      fprintf(stderr,
              "CPC_MEMBER_REPLAY class=%s method=%s member=%s argc=%d\n",
              get_tok_str(class_mangled_tok, NULL),
              get_tok_str(method_tok, NULL),
              get_tok_str(member_func_tok, NULL),
              member_func_explicit_arg_count(&lowered_type));
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
      declare_static_member_func(&struct_type, method_tok,
                                 &static_type);
      member_func_tok = make_static_member_func_tok_for_type(static_base_tok,
                                                             &static_type);
    }
    else if (is_member_template_replay)
    {
      member_func_tok = make_member_template_func_tok_for_type(class_mangled_tok,
                                                               method_tok,
                                                               &replay_func_type,
                                                               md->def_str,
                                                               member_type_arg_tok);
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
    if (auto_return_struct_tok || is_member_template_replay)
      last_instantiated_member_func_tok = member_func_tok;
    if (getenv("CPC_TRACE_MEMBER_REPLAY")
        && class_mangled_tok >= TOK_UIDENT
        && strstr(get_tok_str(class_mangled_tok, NULL),
                  getenv("CPC_TRACE_MEMBER_REPLAY")))
      fprintf(stderr, "CPC_MEMBER_REPLAY class=%s method=%s member=%s member_tpl=%d mtparam=%s mdparam=%s argc=%d\n",
              get_tok_str(class_mangled_tok, NULL),
              get_tok_str(method_tok, NULL),
              get_tok_str(member_func_tok, NULL),
              is_member_template_replay,
              member_type_param_tok ? get_tok_str(member_type_param_tok, NULL) : "<none>",
              md->type_param_tok ? get_tok_str(md->type_param_tok, NULL) : "<none>",
              member_func_explicit_arg_count(&replay_func_type));
    {
      MemberFuncOverload *o;
      Sym *interface_sym = sym_find(member_func_tok);
      if (!interface_sym)
        interface_sym = global_symbol_find(member_func_tok);
      if (interface_sym && interface_sym->type.ref)
        interface_sym->type.ref->type = replay_ret_type;
      for (o = member_func_overloads; o; o = o->next)
        if (o->mangled_tok == member_func_tok && o->func_type.ref)
          o->func_type.ref->type = replay_ret_type;
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
      for (i = 0; i < nb_pending_template_specs; ++i)
      {
        if (!pending_template_specs[i])
          continue;
        if (token_string_contains_tok(pending_template_specs[i], member_func_tok))
          break;
      }
      if (i < nb_pending_template_specs)
        tok_str_free(proto);
      else if (compiling_pending_template_specs)
        publish_template_member_proto(proto);
      else
        queue_pending_template_spec(proto, class_mangled_tok);
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
        int replay_static_func_tok = 0;
        /* Static members have no this pointer and cannot be reached
           through this->: qualify nested static member calls (e.g.
           Translation() calling Identity(), or a member template like
           CreateMatrix<T>(...)) as Class::Member. */
        if (tok == '('
            && class_has_static_member_template_name(class_mangled_tok,
                                                     member_tok))
        {
          int explicit_member_type_tok = member_type_arg_tok
                                     ? member_type_arg_tok : type_tok;
          replay_static_func_tok =
            instantiate_static_member_template_for_replay(
                class_mangled_tok, member_tok, type_tok,
                explicit_member_type_tok);
        }
        if (replay_static_func_tok)
          tok_str_add(body_def, replay_static_func_tok);
        else
        {
          tok_str_add(body_def, class_mangled_tok);
          tok_str_add(body_def, ':');
          tok_str_add(body_def, ':');
          tok_str_add(body_def, member_tok);
        }
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
      int self_type_pos = template_typedef_is_type_pos(body_def);
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
      if (self_type_pos && tok != '(' && tok != '{')
        tok_str_add(body_def, TOK_STRUCT);
      tok_str_add(body_def, class_mangled_tok);
      continue;
    }
    if (tok == TOK_DOTS && member_type_arg_tok)
    {
      if (pack_name_tok && pack_arg_start >= 0)
      {
        int pack_call_index;
        char expanded_name[256];
        for (pack_call_index = pack_arg_start + 1;
             pack_call_index < replay_nb_call_arg_types;
             ++pack_call_index)
        {
          tok_str_add(body_def, ',');
          snprintf(expanded_name, sizeof(expanded_name), "%s__pack%d",
                   get_tok_str(pack_name_tok, NULL),
                   pack_call_index - pack_arg_start);
          tok_str_add(body_def, tok_alloc_const(expanded_name));
        }
      }
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
    tok_str_add_template_member_subst_scoped(body_def, md, class_mangled_tok,
                                             md->type_param_tok, type_tok,
                                             member_type_param_tok,
                                             member_type_arg_tok,
                                             value_param_tok);
    if (init_prefix && tok == '{')
      tok_str_append(body_def, init_prefix);
    next();
  }
  tok_str_add(body_def, TOK_EOF);

  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
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
      if (getenv("CPC_TRACE_CLBITREF")
          && class_mangled_tok >= TOK_UIDENT
          && strstr(get_tok_str(class_mangled_tok, NULL), "clBitRef"))
        fprintf(stderr, "CPC_CLBITREF_REGISTER class=%s member=%s bt=%x\n",
                get_tok_str(class_mangled_tok, NULL),
                get_tok_str(member_func_tok, NULL), bt);
      note_member_func_overload(class_mangled_tok, method_tok,
                                member_func_tok, &lowered_type);
    }
  }
  if (!materialize_body)
    tok_str_free(body_def);
  else if (pending_member_func_has_body_tok(member_func_tok))
    tok_str_free(body_def);
  else
  {
    pm = cprime_mallocz(sizeof(*pm));
    pm->str = body_def;
    pm->func_tok = member_func_tok;
    pm->struct_tok = class_mangled_tok;
    pm->is_template_member = 1;
    pm->is_lifecycle_member = is_lifecycle_member_tok(method_tok);
    pm->is_static_member = is_static_member;
    dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
  }
  for (i = 0; i < md->nb_inst; ++i)
    if (md->inst_type_toks[i] == type_tok
        && (!(token_string_has_variadic_pack(md->def_str)
              || template_member_is_call_specialized(md))
            || md->inst_pack_toks[i] == orig_pack_tok))
    {
      md->inst_func_toks[i] = member_func_tok;
      if (md->inst_states)
      {
        md->inst_states[i] = materialize_body ? 2 : 1;
        if (materialize_body)
          materialized_member_body_note(member_func_tok);
      }
      break;
    }
  md->instantiating = 0;
  nb_template_member_extra_params = saved_nb_extra_params;
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
  if (compiling_pending_member_funcs && !draining_template_member_body_requests)
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

static void instantiate_template_member_body_for_func_tok(CType *type,
                                                          int method_tok,
                                                          int func_tok)
{
  TemplateDef *td;
  TemplateMemberDef *md;
  int struct_tok = get_struct_type_name_tok(type);
  int type_tok = 0;
  int saved_pending = nb_pending_member_funcs;
  int i, j;

  if (!struct_tok || !func_tok)
    return;
  td = find_class_template_def_for_class_tok(struct_tok);
  if (!td)
    return;
  for (i = 0; i < td->nb_inst; ++i)
    if (td->inst_name_toks[i] == struct_tok)
    {
      type_tok = td->inst_type_toks[i * td->nb_type_params];
      break;
    }
  if (!type_tok)
    return;

  for (md = template_member_candidates(td->name_tok, method_tok);
       md; md = md->bucket_next)
  {
    if (md->class_tok != td->name_tok
        || template_member_def_method_tok(md) != method_tok
        || !template_member_def_is_scoped(md)
        || !token_string_has_top_level_body(md->def_str))
      continue;
    for (j = 0; j < md->nb_inst; ++j)
      if (md->inst_type_toks[j] == type_tok && md->inst_func_toks
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

static int template_member_def_first_param_is_rvalue_ref(
  TemplateMemberDef *md, int method_tok)
{
  int i, n;

  if (!md || !md->def_str)
    return 0;
  n = md->def_str->len;
  for (i = 0; i + 1 < n; ++i)
  {
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
        && token_string_has_top_level_body(md->def_str)
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
        if (md->inst_type_toks[j] == type_tok && md->inst_func_toks
            && md->inst_func_toks[j])
          result_tok = md->inst_func_toks[j];
    }
  return result_tok;
}

static void instantiate_template_members_for_class(TemplateDef *td, int type_tok,
    int class_mangled_tok)
{
  int i, inst_index = -1;
  Sym *arg_sym;

  if (!td->is_class)
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
        && template_member_def_is_scoped(md)
        && !template_member_is_call_specialized(md)
        && !token_string_is_auto_type_token_prefix(md->def_str))
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
  int struct_tok, i, j;

  if (arg_index < 0)
    return 0;
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
    if (!td->is_class || arg_index >= td->nb_type_params)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == struct_tok)
        return td->inst_type_toks[j * td->nb_type_params + arg_index];
  }
  return 0;
}

static int first_template_arg_tok_from_inst_type(CType *type)
{
  return template_arg_tok_from_inst_type(type, 0);
}

static int first_template_arg_tok_from_inst_tok(int inst_tok)
{
  int i, j;

  if (!inst_tok)
    return 0;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class || td->nb_type_params <= 0)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == inst_tok)
        return td->inst_type_toks[j * td->nb_type_params];
  }
  return 0;
}

static int is_vector_template_tok(int tok)
{
  return tok == tok_alloc_const("clVector2")
         || tok == tok_alloc_const("clVector3")
         || tok == tok_alloc_const("clVector4");
}

static int template_name_returns_first_vector_shape(int tok)
{
  const char *name = tok >= TOK_UIDENT ? get_tok_str(tok, NULL) : "";
  return !strcmp(name, "clMin")
         || !strcmp(name, "clMax")
         || !strcmp(name, "clClamp")
         || !strcmp(name, "clAbs")
         || !strcmp(name, "clCeil")
         || !strcmp(name, "clFloor")
         || !strcmp(name, "clRound")
         || !strcmp(name, "clLerp")
         || !strcmp(name, "clBiLerp");
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
        if (getenv("CPC_TRACE_STATIC_MEMBER")
            && (strstr(get_tok_str(class_mangled_tok, NULL), "Box2")
                || strstr(get_tok_str(class_mangled_tok, NULL), "clAABB2")))
          fprintf(stderr, "CPC_STATIC_MEMBER class=%s member=%s md_class=%s md_method=%s has_static=%d\n",
                  get_tok_str(class_mangled_tok, NULL),
                  get_tok_str(member_tok, NULL),
                  get_tok_str(md->class_tok, NULL),
                  get_tok_str(template_member_def_method_tok(md), NULL),
                  class_has_static_member_func(class_mangled_tok, member_tok));
        if (md->class_tok == td->name_tok
            && template_member_def_method_tok(md) == member_tok
            && class_has_static_member_func(class_mangled_tok, member_tok)
            && token_string_has_top_level_body(md->def_str))
        {
          instantiate_template_member_body_if_needed(
              md, td->inst_type_toks[j * td->nb_type_params],
              class_mangled_tok, 0);
        }
      }
      return;
    }
  }
}

static int instantiate_static_member_template_for_replay(int class_mangled_tok,
                                                         int member_tok,
                                                         int class_type_tok,
                                                         int member_type_tok)
{
  int i, j, k, result_tok = 0;
  int saved_last_instantiated_member_func_tok;

  if (!class_mangled_tok || !member_tok || !class_type_tok
      || member_type_tok <= 0)
    return 0;
  saved_last_instantiated_member_func_tok = last_instantiated_member_func_tok;
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
        TemplateMemberDef *smd = template_member_defs[k];
        if (smd->class_tok != td->name_tok
            || template_member_def_method_tok(smd) != member_tok
            || !template_member_def_is_static_signature(smd)
            || !token_string_has_top_level_body(smd->def_str))
          continue;
        last_instantiated_member_func_tok = 0;
        instantiate_template_member_body_if_needed(smd, class_type_tok,
                                                   class_mangled_tok,
                                                   member_type_tok);
        /* Publish every same-name overload before the suspended body is
           parsed.  Calls such as CreateMatrix(...) cannot select between a
           scalar and vector member-template overload until their argument
           expressions are typed; stopping after declaration order's first
           match leaves the resolver with an incomplete candidate set. */
        if (!result_tok)
          result_tok = last_instantiated_member_func_tok;
      }
      break;
    }
  }
  last_instantiated_member_func_tok = saved_last_instantiated_member_func_tok;
  return result_tok;
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
  CType saved_call_arg_types[16];

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return;
  saved_nb_call_arg_types = nb_template_member_call_arg_types;
  memcpy(saved_call_arg_types, template_member_call_arg_types,
         sizeof(saved_call_arg_types));
  nb_template_member_call_arg_types = 0;
  if (arg_types && explicit_arg_count > 0)
  {
    nb_template_member_call_arg_types = explicit_arg_count < 16
                                          ? explicit_arg_count : 16;
    memcpy(template_member_call_arg_types, arg_types,
           nb_template_member_call_arg_types * sizeof(CType));
  }
  if (getenv("CPC_TRACE_CLBITREF")
      && struct_tok >= TOK_UIDENT
      && strstr(get_tok_str(struct_tok, NULL), "clBitRef")
      && method_tok == tok_alloc_const("operator="))
    fprintf(stderr, "CPC_CLBITREF_FOR_CALL struct=%s argc=%d arg0_t=%04x\n",
            get_tok_str(struct_tok, NULL), explicit_arg_count,
            arg_types ? arg_types[0].t : 0);
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
            && token_string_has_top_level_body(md->def_str))
        {
          instantiate_template_member_body_if_needed(md, type_tok, struct_tok,
                                                     scalar_arg_tok);
        }
      }
      }
    }
    {
      /* Only instantiate the overloads whose declared signatures actually
         accept the call's argument types.  Arity alone is not enough:
         e.g. Erase(c--) (i64) must not instantiate Erase(clList<i64>&). */
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
            if (member_func_exact_arg_types(os, arg_types,
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
          if (param_count == explicit_arg_count
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
            && !token_string_has_top_level_body(md->def_str))
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
            if (!overload_ok && param_count != explicit_arg_count
                && !token_string_has_variadic_pack(md->def_str))
              continue;
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
          int first_decl_kind =
            template_member_def_first_param_decl_kind(md, method_tok);
          int arg_is_ptr = (arg_types[0].t & VT_BTYPE) == VT_PTR;
          if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
              && method_tok >= TOK_UIDENT
              && strstr(get_tok_str(method_tok, NULL), "operator[]"))
          {
            int ptok = template_member_def_first_param_type_tok(md, method_tok);
            fprintf(stderr, "CPC_MEMBER_INDEX first_kind=%d ptok=%s pstruct=%s arg=%s\n",
                    first_kind,
                    ptok ? get_tok_str(ptok, NULL) : "<none>",
                    get_tok_str(concrete_type_tok_struct_tok(ptok), NULL),
                    get_tok_str(get_struct_type_name_tok(&arg_types[0]), NULL));
          }
          if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
              && method_tok == TOK_CONSTRUCTOR1
              && arg_is_struct)
          {
            int ptok = template_member_def_first_param_type_tok(md, method_tok);
            fprintf(stderr, "CPC_MEMBER_CTOR class=%s first_kind=%d ptok=%s pstruct=%s arg=%s\n",
                    get_tok_str(struct_tok, NULL), first_kind,
                    ptok ? get_tok_str(ptok, NULL) : "<none>",
                    get_tok_str(concrete_type_tok_struct_tok(ptok), NULL),
                    get_tok_str(get_struct_type_name_tok(&arg_types[0]), NULL));
          }
          /* Only guard against instantiating an explicitly class-typed
             member for a scalar call argument (e.g. Erase(c--) must not
             instantiate Erase(clList<i64>&)).  Generic template parameters
             and scalar signatures stay untouched. */
          if (first_kind == 1 && !arg_is_struct)
            continue;
          if (method_tok == TOK_CONSTRUCTOR1 && arg_is_struct
              && first_kind == 0)
            continue;
          if (prefer_class_first_param && arg_is_struct && first_kind != 1)
            continue;
          {
            int param_struct_tok =
              concrete_type_tok_struct_tok(
                  template_member_def_first_param_type_tok(md, method_tok));
            int arg_struct_tok = get_struct_type_name_tok(&arg_types[0]);
            if (param_struct_tok && !arg_is_struct)
              continue;
            if (param_struct_tok && arg_struct_tok
                && param_struct_tok != arg_struct_tok)
              continue;
          }
          if (template_member_is_call_specialized(md)
              && first_decl_kind == 1 && !arg_is_ptr)
            continue;
          if (template_member_is_call_specialized(md)
              && first_decl_kind == 2 && arg_is_ptr)
            continue;
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
            member_type_arg_tok = first_template_arg_tok_from_inst_type(&arg_types[0]);
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
          if (getenv("CPC_TRACE_MATRIX")
              && strstr(get_tok_str(struct_tok, NULL), "clMatrix4x4")
              && !strcmp(method_name, "operator*"))
            fprintf(stderr,
                    "CPC_MATRIX_CALL arg_t=%04x arg_s=%s member_arg=%s first_kind=%d scoped=%d len=%d\n",
                    arg_types[0].t,
                    get_tok_str(get_struct_type_name_tok(&arg_types[0]), NULL),
                    get_tok_str(member_type_arg_tok, NULL), first_kind,
                    template_member_def_is_scoped(md), md->def_str->len);
          if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
              && method_tok >= TOK_UIDENT
              && strstr(get_tok_str(method_tok, NULL), "Read"))
            fprintf(stderr, "CPC_MEMBER_TEMPLATE method=%s first_kind=%d arg0=%s inferred=%s\n",
                    get_tok_str(method_tok, NULL), first_kind,
                    get_tok_str(template_type_tok_from_ctype(&arg_types[0]), NULL),
                    member_type_arg_tok ? get_tok_str(member_type_arg_tok, NULL) : "<none>");
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
          if (!explicit_member_template_arg_tok
              && !token_string_has_variadic_pack(md->def_str))
          {
            int first_matches = template_member_first_param_matches_arg(
              md, method_tok, struct_tok, type_tok,
              member_type_arg_tok, &arg_types[0]);
            if (getenv("CPC_TRACE_MATRIX")
                && strstr(get_tok_str(struct_tok, NULL), "clMatrix4x4")
                && !strcmp(method_name, "operator*"))
              fprintf(stderr, "CPC_MATRIX_MATCH len=%d result=%d\n",
                      md->def_str->len, first_matches);
            if (!first_matches)
              continue;
          }
        }
        if (explicit_arg_count >= 0 || is_lifecycle_member_tok(method_tok))
        {
          instantiate_template_member_body_if_needed(md, type_tok, struct_tok,
                                                     member_type_arg_tok);
        }
        else
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
    if (selected_member_func_tok)
    {
      Sym *inst_sym = sym_find(selected_member_func_tok);
      if (!inst_sym)
        inst_sym = global_symbol_find(selected_member_func_tok);
      if (inst_sym)
      {
        if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
            && method_tok >= TOK_UIDENT
            && strstr(get_tok_str(method_tok, NULL), "Read"))
        {
          Sym *a = inst_sym->type.ref ? inst_sym->type.ref->next : NULL;
          if (a) a = a->next;
          fprintf(stderr, "CPC_NONCLASS_REGISTER %s t=%04x arg_t=%04x\n",
                  get_tok_str(selected_member_func_tok, NULL),
                  inst_sym->type.t,
                  a ? a->type.t : 0);
        }
        note_member_func_overload(struct_tok, method_tok,
                                  selected_member_func_tok,
                                  &inst_sym->type);
      }
    }
    if (saved_nb_pending_member_funcs != nb_pending_member_funcs)
      compile_pending_member_funcs(saved_nb_pending_member_funcs);
    nb_template_member_call_arg_types = saved_nb_call_arg_types;
    memcpy(template_member_call_arg_types, saved_call_arg_types,
           sizeof(saved_call_arg_types));
    tok = entry_tok;
    tokc = entry_tokc;
    if (macro_stack == entry_macro_stack)
      macro_ptr = entry_macro_ptr;
    return;
  }
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
      if (member_func_exact_arg_types(os, arg_types, explicit_arg_count))
      {
        has_preexisting_exact_match = 1;
        break;
      }
    }
  for (j = 0; j < nb_template_member_defs; ++j)
  {
    TemplateMemberDef *md = template_member_defs[j];
    int member_type_arg_tok = explicit_member_template_arg_tok;
    int param_count;
    int interface_only_for_concrete_void_pointer = 0;
    if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
        && method_tok >= TOK_UIDENT
        && strstr(get_tok_str(method_tok, NULL), "Read")
        && template_member_def_method_tok(md) == method_tok)
      fprintf(stderr, "CPC_NONCLASS_MEMBER_TEMPLATE candidate class=%s recv=%s scoped=%d params=%d first_decl=%d\n",
              get_tok_str(md->class_tok, NULL), get_tok_str(struct_tok, NULL),
              template_member_def_is_scoped(md),
              template_member_def_param_count(md, method_tok),
              template_member_def_first_param_decl_kind(md, method_tok));
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
        && !token_string_has_top_level_body(md->def_str))
      continue;
    param_count = template_member_def_param_count(md, method_tok);
    if (explicit_arg_count >= 0
        && param_count >= 0
        && param_count != explicit_arg_count
        && !token_string_has_variadic_pack(md->def_str))
      continue;
    if (arg_types && explicit_arg_count > 0
        && template_member_is_call_specialized(md))
    {
      int first_decl_kind =
        template_member_def_first_param_decl_kind(md, method_tok);
      int arg_is_ptr = (arg_types[0].t & VT_BTYPE) == VT_PTR;
      if (first_decl_kind == 1 && !arg_is_ptr)
        continue;
      if (first_decl_kind == 2 && arg_is_ptr)
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
    if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
        && method_tok >= TOK_UIDENT
        && strstr(get_tok_str(method_tok, NULL), "Read"))
      fprintf(stderr, "CPC_NONCLASS_INSTANTIATE method=%s arg=%s\n",
              get_tok_str(method_tok, NULL),
              member_type_arg_tok ? get_tok_str(member_type_arg_tok, NULL) : "<none>");
    if (interface_only_for_concrete_void_pointer)
    {
      materializing_template_interface++;
      instantiate_template_member_if_needed(md, 0, struct_tok,
                                            member_type_arg_tok);
      materializing_template_interface--;
    }
    else
      instantiate_template_member_body_if_needed(md, 0, struct_tok,
                                                 member_type_arg_tok);
  }
  }
  selected_member_func_tok = last_instantiated_member_func_tok;
  if (nb_pending_template_specs != saved_nb_pending_template_specs)
    compile_pending_template_specs_without_member_flush();
  last_instantiated_member_func_tok = selected_member_func_tok;
  if (selected_member_func_tok)
  {
    Sym *inst_sym = sym_find(selected_member_func_tok);
    if (!inst_sym)
      inst_sym = global_symbol_find(selected_member_func_tok);
    if (inst_sym)
      note_member_func_overload(struct_tok, method_tok,
                                selected_member_func_tok,
                                &inst_sym->type);
  }
  if (saved_nb_pending_member_funcs != nb_pending_member_funcs)
    compile_pending_member_funcs(saved_nb_pending_member_funcs);
  nb_template_member_call_arg_types = saved_nb_call_arg_types;
  memcpy(template_member_call_arg_types, saved_call_arg_types,
         sizeof(saved_call_arg_types));
  tok = entry_tok;
  tokc = entry_tokc;
  if (macro_stack == entry_macro_stack)
    macro_ptr = entry_macro_ptr;
}

static void add_template_member_def(int class_tok, int type_param_tok,
                                     int *stripped_member_type_param_toks,
                                     int nb_stripped_member_type_params,
                                     int *stripped_member_value_param_toks,
                                     int nb_stripped_member_value_params,
                                     TokenString *str)
{
  int i;
  TemplateDef *td;
  TemplateMemberDef *md;

  md = cprime_mallocz(sizeof(*md));
  md->class_tok = class_tok;
  md->lookup_class_tok = find_current_namespace_tok(class_tok);
  md->type_param_tok = type_param_tok;
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
  }
  md->method_tok = template_member_def_method_tok(md);
  md->method_tok_known = 1;
  if (getenv("CPC_DUMP_MEMBER_TEMPLATE"))
  {
    int method_tok = template_member_def_method_tok(md);
    if ((method_tok >= TOK_UIDENT && strstr(get_tok_str(method_tok, NULL), "Read"))
        || method_tok == TOK_CONSTRUCTOR1
        || (class_tok >= TOK_UIDENT && strstr(get_tok_str(class_tok, NULL), "clVector4")))
      fprintf(stderr, "CPC_ADD_MEMBER_TEMPLATE class=%s type_param=%s method=%s scoped=%d len=%d\n",
              get_tok_str(class_tok, NULL),
              type_param_tok ? get_tok_str(type_param_tok, NULL) : "<none>",
              method_tok ? get_tok_str(method_tok, NULL) : "<none>",
              template_member_def_is_scoped(md),
              str ? str->len : 0);
  }
  dynarray_add(&template_member_defs, &nb_template_member_defs, md);
  index_template_member(md);


  td = find_template_def(class_tok);
  if (!td || !td->is_class)
    return;
  for (i = 0; i < td->nb_inst; ++i)
  {
    int method_tok = template_member_def_method_tok(md);
    if (template_member_def_is_scoped(md)
        && !template_member_is_call_specialized(md)
        && !token_string_is_auto_type_token_prefix(md->def_str))
    {
      materializing_template_interface++;
      instantiate_template_member_if_needed(md,
                                            td->inst_type_toks[i * td->nb_type_params],
                                            td->inst_name_toks[i], 0);
      materializing_template_interface--;
    }
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

static void complete_function_template_definition(TemplateDef *td,
                                                  TokenString *definition)
{
  tok_str_free(td->def_str);
  td->def_str = definition;
  td->has_body = 1;
}

static void parse_template_decl(void)
{
  int type_param_toks[16], nb_type_params = 0;
  int type_param_is_type[16] = { 0 };
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
      queue_pending_template_spec(str, args.nb > 0 ? args.toks[0] : 0);
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
    type_param_is_type[param_index] = 1;
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
      type_param_is_type[param_index] = 1;
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
    if (!name_tok && prev_tok == TOK_OPERATOR)
    {
      int operator_name_tok = get_cpp_binary_operator_method_tok(tok);
      if (operator_name_tok)
        name_tok = operator_name_tok;
    }
    if (!saw_body && brace == 0 && tok == '(' && prev_ident && !name_tok)
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
      && nb_required_type_params < nb_type_params
      && strncmp(get_tok_str(name_tok, NULL), "operator", 8))
  {
    tok_str_free(str);
    return;
  }

  member_class_tok = find_template_member_class_in_str(str, type_param_toks[0]);
  if (member_class_tok)
  {
    TemplateDef *member_class_td = find_class_template_def(member_class_tok);
    int stripped_type_params[16], nb_stripped_type_params = 0;
    int stripped_value_params[16], nb_stripped_value_params = 0;
    if (!member_class_td || !member_class_td->is_class)
    {
      int param_i;
      for (param_i = 0; param_i < nb_type_params; ++param_i)
        if (type_param_is_type[param_i])
          stripped_type_params[nb_stripped_type_params++] =
            type_param_toks[param_i];
        else
          stripped_value_params[nb_stripped_value_params++] =
            type_param_toks[param_i];
    }
    add_template_member_def(member_class_tok,
                            member_class_td && member_class_td->is_class
                              ? type_param_toks[0] : 0,
                            stripped_type_params, nb_stripped_type_params,
                            stripped_value_params, nb_stripped_value_params,
                            str);
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
          && old->func_max_args == func_max_args
          && old->func_is_variadic == func_is_variadic
          && (old->func_sig_hash == func_sig_hash
              || template_function_merge_signature_hash(old->def_str,
                                                         old->name_tok)
                   == template_function_merge_signature_hash(str,
                                                              name_tok)))
      {
        complete_function_template_definition(old, str);
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
  index_template_def(td);

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
    if (!td->is_class && td->variadic_param_index >= 0)
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

static int parse_template_constant_expression_arg(void)
{
  TokenString *expression = tok_str_alloc();
  TokenString *saved_macro_stack;
  const int *saved_macro_ptr;
  int saved_tok, paren = 0, bracket = 0;
  int64_t value;
  CValue saved_tokc;

  while (tok != TOK_EOF)
  {
    if (!paren && !bracket
        && (tok == ',' || tok == TOK_GT || tok == '>'))
      break;
    if (tok == '(')
      ++paren;
    else if (tok == ')' && paren > 0)
      --paren;
    else if (tok == '[')
      ++bracket;
    else if (tok == ']' && bracket > 0)
      --bracket;
    tok_str_add_tok(expression);
    next();
  }
  tok_str_add(expression, TOK_EOF);

  saved_tok = tok;
  saved_tokc = tokc;
  saved_macro_stack = macro_stack;
  saved_macro_ptr = macro_ptr;
  begin_macro(expression, 1);
  next();
  value = expr_const64();
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  return template_integer_expression_arg_tok(value);
}

static int template_static_const_arg_tok(int member_tok)
{
  int class_tok, static_tok;
  Sym *s;

  if (nb_defining_class_stack <= 0)
    return 0;
  class_tok = defining_class_stack[nb_defining_class_stack - 1];
  static_tok = make_static_member_tok(class_tok, member_tok);
  s = sym_find(static_tok);
  if (!s)
    s = global_symbol_find(static_tok);
  if (!s || (s->r & VT_VALMASK) != VT_CONST
      || !is_integer_btype(s->type.t & VT_BTYPE))
    return 0;
  return static_tok;
}

static int template_arg_const_int64(int arg_tok, int64_t *out)
{
  Sym *s = sym_find(arg_tok);
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

static void compile_template_alias_decl_now(TokenString *alias_decl)
{
  int saved_tok = tok;
  int saved_local_scope = local_scope;
  int saved_defer_pending_member_funcs = defer_pending_member_funcs;
  int saved_nocode_wanted = nocode_wanted;
  CValue saved_tokc = tokc;
  Sym *saved_local_stack = local_stack;

  local_stack = NULL;
  local_scope = 0;
  defer_pending_member_funcs = 1;
  tok_str_add(alias_decl, TOK_EOF);
  begin_macro(alias_decl, 1);
  next();
  decl(VT_CONST);
  end_macro();
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
  nocode_wanted = saved_nocode_wanted;
  tok = saved_tok;
  tokc = saved_tokc;
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
  int unsigned_tok = template_unsigned_scalar_type_tok(type);

  if (unsigned_tok)
    return unsigned_tok;

  if (bt == VT_PTR)
    return template_ctype_typedef_tok(type);
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
    return TOK_DOUBLE;
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

static int finish_template_type_argument(int type_tok, int qualifiers)
{
  CType type;

  if (!qualifiers && tok != '*')
    return type_tok;
  if (!make_type_from_type_arg_tok(&type, type_tok))
    cprime_error("unsupported template type argument '%s'",
                 get_tok_str(type_tok, NULL));
  type.t |= qualifiers;
  while (tok == '*')
  {
    next();
    mk_pointer(&type);
    while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
           || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
           || tok == TOK_VOLATILE3)
    {
      if (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
        type.t |= VT_CONSTANT;
      else
        type.t |= VT_VOLATILE;
      next();
    }
  }
  return template_ctype_typedef_tok(&type);
}

static int parse_one_template_type_arg(void)
{
  int type_tok;
  int qualifiers = 0;

  if (getenv("CPC_TRACE_TEMPLATE_ARG"))
    fprintf(stderr, "CPC_TEMPLATE_ARG tok=%s code=%d\n",
            get_tok_str(tok, &tokc), tok);
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
  if (tok == TOK_CHAR || tok == TOK_SHORT || tok == TOK_INT
      || tok == TOK_LONG || tok == TOK_FLOAT
      || tok == TOK_DOUBLE || tok == TOK_BOOL || tok == TOK_BOOL2
      || tok == TOK_TRUE || tok == TOK_FALSE)
  {
    type_tok = tok;
    next();
  }
  else if (tok == TOK_CINT || tok == TOK_CUINT || tok == TOK_CCHAR
           || tok == TOK_CLONG || tok == TOK_CULONG
           || tok == TOK_CLLONG || tok == TOK_CULLONG)
  {
    type_tok = template_integer_constant_arg_tok();
  }
  else if (tok == TOK_SIZEOF)
  {
    type_tok = parse_template_constant_expression_arg();
  }
  else if (tok == TOK_UNSIGNED || tok == TOK_SIGNED1
           || tok == TOK_SIGNED2 || tok == TOK_SIGNED3)
  {
    CType type;
    AttributeDef ad;
    memset(&type, 0, sizeof(type));
    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      cprime_error("unsupported template type argument");
    type_tok = template_type_tok_from_ctype(&type);
    if (!type_tok)
      cprime_error("could not instantiate scalar template argument");
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
    int static_const_tok = template_static_const_arg_tok(tok);
    if (static_const_tok)
    {
      next();
      return static_const_tok;
    }
    {
      int nested_tok = find_current_class_nested_type_tok(tok);
      if (nested_tok != tok)
      {
        next();
        return finish_template_type_argument(nested_tok, qualifiers);
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
        return finish_template_type_argument(nested_mangled_tok, qualifiers);
      }
      return finish_template_type_argument(name_tok, qualifiers);
    }
    name_tok = find_current_namespace_tok(tok);
    td = find_class_template_def(name_tok);
    if (!td)
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
      Sym *alias_sym = sym_find(name_tok);
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
        type_tok = template_type_tok_from_ctype(&alias_type);
        if (type_tok)
        {
          next();
          return finish_template_type_argument(type_tok, qualifiers);
        }
      }
      type_tok = name_tok;
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
  return finish_template_type_argument(type_tok, qualifiers);
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

static int parse_one_template_type_arg_subst_with_alias(
    TemplateDef *ctx_td, TemplateArgList *ctx_args,
    int *alias_from, int *alias_to, int nb_aliases)
{
  int type_tok;

  if (getenv("CPC_TRACE_TEMPLATE_ARG"))
    fprintf(stderr, "CPC_TEMPLATE_ARG_SUBST tok=%s code=%d\n",
            get_tok_str(tok, &tokc), tok);
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
  if (tok == TOK_CINT || tok == TOK_CUINT || tok == TOK_CCHAR
      || tok == TOK_CLONG || tok == TOK_CULONG
      || tok == TOK_CLLONG || tok == TOK_CULLONG)
    return template_integer_constant_arg_tok();
  if (tok == TOK_UNSIGNED || tok == TOK_SIGNED1
      || tok == TOK_SIGNED2 || tok == TOK_SIGNED3)
  {
    CType type;
    AttributeDef ad;
    memset(&type, 0, sizeof(type));
    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      cprime_error("unsupported template type argument");
    type_tok = template_type_tok_from_ctype(&type);
    if (!type_tok)
      cprime_error("could not instantiate scalar template argument");
    return type_tok;
  }
  if (tok == TOK_CHAR || tok == TOK_SHORT || tok == TOK_INT
      || tok == TOK_LONG || tok == TOK_FLOAT || tok == TOK_DOUBLE)
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
    int static_const_tok = template_static_const_arg_tok(tok);
    if (static_const_tok)
    {
      next();
      return static_const_tok;
    }
    int alias_tok = template_alias_lookup(alias_from, alias_to, nb_aliases,
                                          tok);
    if (alias_tok)
    {
      next();
      return alias_tok;
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
        arg_sym = global_symbol_find(tok);
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
    td->inst_type_toks[td->nb_inst * td->nb_type_params + i] = args->toks[i];
  td->inst_arg_lists[td->nb_inst] = *args;
  td->inst_name_toks[td->nb_inst] = mangled_tok;
  td->inst_states[td->nb_inst] = 0;
  index_template_inst_owner(td, mangled_tok);
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

static int instantiate_template_if_needed(TemplateDef *td,
                                          TemplateArgList *args)
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
  int pack_type_tok = 0;
  int pack_start = -1;
  int pack_name_tok = 0;
  int class_arg_incomplete = 0;
  int existing_class_inst_tok = 0;
  int materialize_existing_function_body = 0;
  int materialize_function_body = 0;
  TokenString definition_replay;

  if (!td)
    cprime_error("instantiate_template_if_needed called with NULL template");
  materialize_function_body = !td->is_class && td->has_body
                              && finalizing_template_bodies
                              && !local_stack && !local_scope;
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
  }
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
      selected.inst_arg_lists = NULL;
      selected.inst_name_toks = NULL;
      selected.inst_states = NULL;
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
      existing_class_inst_tok = existing_inst;
      if (struct_find(existing_inst) && struct_find(existing_inst)->c >= 0)
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
    mangled_tok = make_template_inst_name_tok(td, args);
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
      else if (!struct_find(mangled_tok))
      {
        TokenString *forward = tok_str_alloc();
        tok_str_add(forward, TOK_STRUCT);
        tok_str_add(forward, mangled_tok);
        tok_str_add(forward, ';');
        tok_str_add(forward, TOK_EOF);
        queue_pending_template_spec(forward, 0);
        compile_pending_template_specs_without_member_flush();
      }
      /* A queued work item owns materialization even while its dependency is
         incomplete.  Do not enqueue the same mangled class through another
         equivalent TemplateDef or a recursive use inside its own body. */
      if (class_arg_incomplete)
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
    if (!td->is_class && td->inst_states)
      td->inst_states[td->nb_inst - 1] = materialize_function_body ? 2 : 1;
  }
  /* An incomplete class argument can only support a forward declaration.
     Building and queueing the fully substituted class here used to parse the
     specialization once while its dependency was incomplete, then rebuild
     it from scratch when the dependency became complete.  Record the
     canonical instance now, but defer token substitution until layout is
     actually possible. */
  if (td->is_class && class_arg_incomplete)
  {
    if (!struct_find(mangled_tok))
    {
      TokenString *forward = tok_str_alloc();
      tok_str_add(forward, TOK_STRUCT);
      tok_str_add(forward, mangled_tok);
      tok_str_add(forward, ';');
      tok_str_add(forward, TOK_EOF);
      queue_pending_template_spec(forward, 0);
      compile_pending_template_specs_without_member_flush();
    }
    return mangled_tok;
  }
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
  /* TokenString also carries begin_macro()'s parent cursor.  Template return
     discovery can demand an older specialization of this same definition
     while the new one is being assembled.  A shallow per-replay frame keeps
     those nested cursors independent without copying the token payload. */
  definition_replay = *td->def_str;
  begin_macro(&definition_replay, 3);
  next();
  while (tok != TOK_EOF)
  {
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
      /* For a forwarding expansion such as
         clForward(Args, args)..., the first pack name was rewritten inside
         the call above.  CPC does not clone arbitrary postfix expressions,
         so append the remaining concrete arguments at the expansion point.
         This preserves the complete constructor/call argument list while
         the first element retains the explicit forwarding operation. */
      if (!td->is_class && function_params_done && pack_name_tok
          && pack_start >= 0 && args->nb > pack_start + 1)
      {
        int pi;
        char pack_name[256];
        for (pi = pack_start + 1; pi < args->nb; ++pi)
        {
          tok_str_add(spec, ',');
          snprintf(pack_name, sizeof(pack_name), "%s__pack%d",
                   get_tok_str(pack_name_tok, NULL), pi - pack_start);
          tok_str_add(spec, tok_alloc_const(pack_name));
        }
      }
      next();
      continue;
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
          tok_str_add(spec, arg_tok);
          tok_str_add(spec, ':');
          tok_str_add(spec, ':');
          tok_str_add(spec, alias_tok);
        }
        next();
        continue;
      }
      tok_str_add(spec, arg_tok);
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
    if (td->is_class && tok >= TOK_UIDENT
        && template_next_token_is_scope() && is_namespace_tok(tok))
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
          int inst_tok;
          TemplateArgList class_args;

          next();
          class_args.nb = 0;
          for (;;)
          {
            if (class_args.nb >= (int)(sizeof(class_args.toks) / sizeof(class_args.toks[0])))
              cprime_error("too many template type arguments");
            class_args.toks[class_args.nb++] =
              parse_one_template_type_arg_subst_with_alias(
                  td, args, alias_from, alias_to, nb_aliases);
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
    if (td->is_class
        && (tok == TOK_CLASS || tok == TOK_STRUCT || tok == TOK_UNION
            || tok == TOK_ENUM))
    {
      int kind_tok = tok;

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
              if (tok == TOK_LT || tok == '<')
                ++angle;
              else if (tok == TOK_GT || tok == '>')
                --angle;
              else if (tok == TOK_SAR || tok == TOK_SHR)
                angle -= 2;
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
        if (!nested_inst_tok
            && template_param_index(td, nested_name_tok) < 0
            && (tok == '{' || tok == ';'
                || (tok == ':' && !is_namespace_tok(nested_name_tok))))
        {
          nested_inst_tok = make_static_member_tok(mangled_tok,
                                                    nested_name_tok);
          if (nb_aliases >= (int)(sizeof(alias_from) / sizeof(alias_from[0])))
            cprime_error("too many nested types in template '%s'",
                         get_tok_str(td->name_tok, NULL));
          alias_from[nb_aliases] = nested_name_tok;
          alias_to[nb_aliases] = nested_inst_tok;
          ++nb_aliases;
        }
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
      TokenString *member_str = skip_or_save_template_member_decl(1);
      if (getenv("CPC_DUMP_MEMBER_TEMPLATE")
          && td->name_tok >= TOK_UIDENT
          && strstr(get_tok_str(td->name_tok, NULL), "clVector4"))
        fprintf(stderr, "CPC_SEEN_TEMPLATE_MEMBER class=%s saved=%d friend=%d\n",
                get_tok_str(td->name_tok, NULL), member_str != NULL,
                member_str ? token_string_template_is_friend_decl(member_str) : 0);
      if (member_str && token_string_template_is_friend_decl(member_str))
        tok_str_free(member_str);
      else if (member_str && td->nb_type_params > 0)
        add_template_member_def(td->name_tok, td->type_param_toks[0],
                                NULL, 0, NULL, 0, member_str);
      else if (member_str)
        tok_str_free(member_str);
      continue;
    }
    if (td->is_class && tok == td->name_tok)
    {
      int self_type_pos = template_typedef_is_type_pos(spec);
      int self_prev_tok = spec->len > 0 ? spec->str[spec->len - 1] : 0;
      if (self_prev_tok == TOK_STRUCT || self_prev_tok == TOK_CLASS
          || self_prev_tok == TOK_UNION)
        self_type_pos = 0;
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
        else if (self_type_pos && tok != '(')
          tok_str_add(spec, TOK_STRUCT);
        tok_str_add(spec, self_mangled_tok);
      }
      else
      {
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
      char alias_name[512];
      TokenString *typedef_str = tok_str_alloc();
      TokenString *alias_decl = tok_str_alloc();

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
      if (new_alias_tok && !self_alias && !global_symbol_find(new_alias_tok))
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
      char alias_name[512];
      TokenString *using_str = tok_str_alloc();
      TokenString *alias_decl = tok_str_alloc();

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
          if (getenv("CPC_TRACE_ALIAS"))
          {
            int di;
            fprintf(stderr,
                    "CPC_ALIAS_SCAN class=%s alias=%s commas=%d,%d close=%d\n",
                    get_tok_str(mangled_tok, NULL),
                    get_tok_str(alias_tok, NULL), comma1, comma2, close);
            for (di = 0; di < using_str->len; ++di)
              fprintf(stderr, "CPC_ALIAS_TOK i=%d tok=%s code=%d\n", di,
                      get_tok_str(using_str->str[di], NULL),
                      using_str->str[di]);
          }
          continue;
        }
        cond_tok = template_subst_token(td, args, mangled_tok,
                                        alias_from, alias_to, nb_aliases,
                                        using_str->str[open + 1]);
        if (getenv("CPC_TRACE_ALIAS"))
          fprintf(stderr,
                  "CPC_ALIAS_CHOICE class=%s alias=%s cond=%s commas=%d,%d close=%d\n",
                  get_tok_str(mangled_tok, NULL), get_tok_str(alias_tok, NULL),
                  get_tok_str(cond_tok, NULL), comma1, comma2, close);
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
          if (template_arg_for_param(td, args, t))
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
      if (new_alias_tok)
      {
        int saw_equal = 0;
        tok_str_add(alias_decl, TOK_TYPEDEF);
        for (i = choice_start >= 0 ? choice_start : 0;
             i < (choice_start >= 0 ? choice_end : using_str->len); ++i)
        {
          int t = using_str->str[i];
          if (choice_start < 0 && !saw_equal)
          {
            if (t == '=')
              saw_equal = 1;
            continue;
          }
          if (t == ';')
            break;
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
      if (new_alias_tok && !global_symbol_find(new_alias_tok))
        compile_template_alias_decl_now(alias_decl);
      else
        tok_str_free(alias_decl);
      tok_str_free(using_str);
    }
    else if (td->is_class && tok >= TOK_UIDENT)
    {
      TemplateDef *class_td = NULL;
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
      if (template_next_token_is_angle_open())
        class_td = find_class_template_def(tok);

      if (class_td && class_td->is_class)
      {
        int class_template_tok = tok;
        next();
        if (tok == TOK_LT || tok == '<')
        {
          TemplateArgList class_args;
          parse_template_type_args_subst_with_alias(
              &class_args, td, args, alias_from, alias_to, nb_aliases);
          tok_str_add(spec, TOK_STRUCT);
          tok_str_add(spec, instantiate_template_if_needed(class_td,
                                                           &class_args));
          continue;
        }
        tok_str_add(spec, class_template_tok);
        continue;
      }

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
          tok_str_add(spec, TOK_STRUCT);
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
  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;
  tok_str_add(spec, TOK_EOF);
  if (getenv("CPC_TRACE_TEMPLATE_SPEC")
      && td->name_tok >= TOK_UIDENT
      && strstr(get_tok_str(td->name_tok, NULL),
                getenv("CPC_TRACE_TEMPLATE_SPEC")))
  {
    int si;
    fprintf(stderr, "CPC_TEMPLATE_SPEC name=%s mangled=%s arg0=%s toks=",
            get_tok_str(td->name_tok, NULL), get_tok_str(mangled_tok, NULL),
            args->nb > 0 ? get_tok_str(args->toks[0], NULL) : "<none>");
    for (si = 0; si < spec->len && si < 220; ++si)
    {
      if (spec->str[si] == TOK_LINENUM)
      {
        ++si;
        continue;
      }
      if (TOK_HAS_VALUE(spec->str[si]))
      {
        fprintf(stderr, "%s<value>", si ? " " : "");
        si += tok_str_value_extra_words(spec->str, spec->len, si);
        continue;
      }
      fprintf(stderr, "%s%s", si ? " " : "", get_tok_str(spec->str[si], NULL));
    }
    fprintf(stderr, "\n");
  }

  queue_pending_template_spec(spec,
                              td->is_class && args->nb > 0
                                ? args->toks[0] : 0);
  if (td->is_class)
  {
    /* A specialization mentioned while its element class is still being
       declared is only a forward request.  Materializing its layout here
       would re-enter the class parser with an incomplete argument type. */
    if (!class_arg_incomplete)
    {
      compile_pending_template_specs_without_member_flush();
      if (struct_find(mangled_tok))
        instantiate_template_members_for_class(td, args->toks[0], mangled_tok);
    }
    else if (!struct_find(mangled_tok))
    {
      TokenString *forward = tok_str_alloc();
      tok_str_add(forward, TOK_STRUCT);
      tok_str_add(forward, mangled_tok);
      tok_str_add(forward, ';');
      tok_str_add(forward, TOK_EOF);
      queue_pending_template_spec(forward, 0);
      compile_pending_template_specs_without_member_flush();
    }
  }
  else if (template_class_needs_member_layout(td))
    compile_pending_template_specs();
  return mangled_tok;
}

static int materialize_incomplete_template_depth;

static int materialize_incomplete_template_type(CType *type)
{
  int struct_tok, i, j;
  Sym *class_sym;

  if (!type || (type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  struct_tok = get_struct_type_name_tok(type);
  class_sym = struct_tok ? struct_find(struct_tok) : NULL;
  if (!class_sym || class_sym->c >= 0)
    return class_sym != NULL;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == struct_tok)
      {
        TemplateArgList args;
        int k, attempt;
        args.nb = td->nb_type_params;
        for (k = 0; k < args.nb; ++k)
          args.toks[k] = td->inst_type_toks[j * td->nb_type_params + k];
        ++materialize_incomplete_template_depth;
        for (attempt = 0; attempt < 4; ++attempt)
        {
          instantiate_template_if_needed(td, &args);
          compile_pending_template_specs_without_member_flush();
          class_sym = struct_find(struct_tok);
          if (class_sym && class_sym->c >= 0)
            break;
          if (args.nb > 0)
          {
            Sym *arg_sym = struct_find(args.toks[0]);
            int arg_align = 0;
            int pending_index = pending_template_spec_index_for_tok(struct_tok);
            if (arg_sym && type_size(&arg_sym->type, &arg_align) >= 0
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
        type->t = class_sym->type.t;
        type->ref = class_sym;
        return 1;
      }
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

/* Infer `auto` from a simple member-call return expression such as

     template<class T> auto f(const Vec<T> &v) { return v.Normalized(); }

   The class interface is materialized first, so this is ordinary member
   lookup over a concrete receiver rather than a spelling/name heuristic. */
static int infer_template_return_member_call_ctype(TemplateDef *td,
                                                   TemplateArgList *args,
                                                   CType *ret_type)
{
  int i, name_index = -1, close_paren = -1, depth = 0;
  int class_template_tok, class_param_index = -1, class_inst_tok;
  int param_name_tok = 0, member_tok = 0;
  TemplateDef *class_td;
  TemplateArgList class_args;
  Sym *class_sym, *member_sym;
  MemberFuncOverload *member_overload;

  if (!td || !td->def_str || !args || !ret_type || args->nb <= 0)
    return 0;
  class_template_tok = template_def_first_param_class_tok(td);
  class_td = find_class_template_def(class_template_tok);
  if (!class_td || !class_td->is_class)
    return 0;
  for (i = 0; i + 2 < td->def_str->len; ++i)
  {
    if (td->def_str->str[i] == class_template_tok
        && (td->def_str->str[i + 1] == TOK_LT
            || td->def_str->str[i + 1] == '<'))
    {
      class_param_index = template_param_index(td, td->def_str->str[i + 2]);
      if (class_param_index >= 0)
        break;
    }
  }
  if (class_param_index < 0 || class_param_index >= args->nb)
    return 0;
  for (i = 0; i < td->def_str->len; ++i)
    if (td->def_str->str[i] == td->name_tok)
    {
      name_index = i;
      break;
    }
  if (name_index < 0)
    return 0;
  for (i = name_index + 1; i < td->def_str->len; ++i)
  {
    int t = td->def_str->str[i];
    if (t == '(')
    {
      ++depth;
      continue;
    }
    if (t == ')')
    {
      if (--depth == 0)
      {
        close_paren = i;
        break;
      }
      continue;
    }
    /* The declarator name is the last non-template identifier in the first
       parameter.  A comma ends the supported first-parameter form. */
    if (depth == 1 && t == ',')
      break;
    if (depth == 1 && t >= TOK_UIDENT
        && t != class_template_tok && template_param_index(td, t) < 0)
      param_name_tok = t;
  }
  if (close_paren < 0 || !param_name_tok)
    return 0;
  for (i = close_paren + 1; i + 4 < td->def_str->len; ++i)
  {
    int ri;
    if (td->def_str->str[i] != TOK_RETURN)
      continue;
    ri = i + 1;
    while (ri + 1 < td->def_str->len
           && td->def_str->str[ri] == TOK_LINENUM)
      ri += 2;
    if (ri + 3 >= td->def_str->len
        || td->def_str->str[ri] != param_name_tok
        || td->def_str->str[ri + 1] != '.'
        || td->def_str->str[ri + 2] < TOK_UIDENT
        || td->def_str->str[ri + 3] != '(')
      continue;
    ri += 4;
    while (ri + 1 < td->def_str->len
           && td->def_str->str[ri] == TOK_LINENUM)
      ri += 2;
    if (ri >= td->def_str->len || td->def_str->str[ri] != ')')
      continue;
    member_tok = td->def_str->str[ri - 2];
    break;
  }
  if (!member_tok)
    return 0;
  template_arg_list_one(&class_args, args->toks[class_param_index]);
  class_inst_tok = instantiate_template_if_needed(class_td, &class_args);
  compile_pending_template_specs_without_member_flush();
  class_sym = struct_find(class_inst_tok);
  if (!class_sym)
    return 0;
  /* Prefer the captured member definition.  This keeps return discovery a
     true interface prepass: replaying a member body here would re-enter
     declaration parsing while the enclosing function specialization is
     still being assembled, corrupting its symbol-frame ownership. */
  for (i = 0; i < nb_template_member_defs; ++i)
  {
    TemplateMemberDef *member_md = template_member_defs[i];
    int member_ret_struct_tok;
    if (member_md->class_tok != class_td->name_tok
        || template_member_def_method_tok(member_md) != member_tok)
      continue;
    member_ret_struct_tok = template_member_return_clcreatevector_inst_tok(
      member_md, args->toks[class_param_index]);
    if (member_ret_struct_tok
        && template_return_ctype_from_struct_tok(ret_type,
                                                 member_ret_struct_tok))
      return 1;
    if (infer_template_member_auto_return_type(member_md, class_inst_tok,
                                                ret_type))
      return 1;
  }
  member_sym = NULL;
  for (member_overload = member_func_candidates(class_inst_tok, member_tok);
       member_overload; member_overload = member_overload->bucket_next)
  {
    if (member_overload->struct_tok != class_inst_tok
        || member_overload->method_tok != member_tok
        || member_overload->min_arg_count != 0)
      continue;
    member_sym = global_symbol_find(member_overload->mangled_tok);
    if (!member_sym)
      member_sym = sym_find(member_overload->mangled_tok);
    if (member_sym && member_sym->type.ref
        && (member_sym->type.ref->type.t & VT_BTYPE) != VT_VOID)
      break;
    member_sym = NULL;
  }
  if (!member_sym || (member_sym->type.t & VT_BTYPE) != VT_FUNC
      || !member_sym->type.ref
      || (member_sym->type.ref->type.t & VT_BTYPE) == VT_VOID)
    return 0;
  *ret_type = member_sym->type.ref->type;
  return 1;
}

static int infer_template_return_scalar_ctype(TemplateDef *td,
                                              TemplateArgList *args,
                                              CType *ret_type)
{
  static int scalar_inference_depth;
  int i;
  if (!td || !td->def_str || !args || !ret_type)
    return 0;
  if (infer_template_return_member_call_ctype(td, args, ret_type)
      && (ret_type->t & VT_BTYPE) != VT_STRUCT)
    return 1;

  /* Preserve the type selected by a dependent decltype cast.  Numeric
     helpers commonly return a fixed floating type even when their template
     argument is integral:

       return (decltype(scale<T>()))(value) * scale<T>();

     Falling back to T for the outer auto return truncates that expression.
     Follow the concrete helper interface when the same zero-argument helper
     is both the decltype operand and an operand of the returned expression. */
  if (scalar_inference_depth < 16)
    for (i = 0; i + 9 < td->def_str->len; ++i)
      if (td->def_str->str[i] == TOK_RETURN)
      {
        int di, call_tok, param_index, j, saw_matching_call = 0;
        TemplateDef *return_td;
        TemplateArgList return_args;

        for (di = i + 1; di + 8 < td->def_str->len
                         && td->def_str->str[di] != ';'; ++di)
          if (td->def_str->str[di] == TOK_DECLTYPE)
            break;
        if (di + 8 >= td->def_str->len
            || td->def_str->str[di] != TOK_DECLTYPE
            || td->def_str->str[di + 1] != '('
            || td->def_str->str[di + 2] < TOK_UIDENT
            || (td->def_str->str[di + 3] != TOK_LT
                && td->def_str->str[di + 3] != '<')
            || (td->def_str->str[di + 5] != TOK_GT
                && td->def_str->str[di + 5] != '>')
            || td->def_str->str[di + 6] != '('
            || td->def_str->str[di + 7] != ')'
            || td->def_str->str[di + 8] != ')')
          continue;
        call_tok = td->def_str->str[di + 2];
        param_index = template_param_index(td, td->def_str->str[di + 4]);
        if (param_index < 0 || param_index >= args->nb)
          continue;
        for (j = di + 9; j + 4 < td->def_str->len
                         && td->def_str->str[j] != ';'; ++j)
          if (td->def_str->str[j] == call_tok
              && (td->def_str->str[j + 1] == TOK_LT
                  || td->def_str->str[j + 1] == '<')
              && td->def_str->str[j + 2] == td->def_str->str[di + 4]
              && (td->def_str->str[j + 3] == TOK_GT
                  || td->def_str->str[j + 3] == '>')
              && td->def_str->str[j + 4] == '(')
          {
            saw_matching_call = 1;
            break;
          }
        if (!saw_matching_call)
          continue;
        return_td = find_function_template_def(call_tok);
        if (!return_td)
        {
          int qualified_tok = find_current_namespace_tok(call_tok);
          if (qualified_tok != call_tok)
            return_td = find_function_template_def(qualified_tok);
        }
        if (!return_td)
          continue;
        template_arg_list_one(&return_args, args->toks[param_index]);
        ++scalar_inference_depth;
        if (infer_template_return_scalar_ctype(return_td, &return_args,
                                               ret_type))
        {
          --scalar_inference_depth;
          return 1;
        }
        --scalar_inference_depth;
      }
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
        member_sym = global_symbol_find(member_tok);
      if (member_sym && (member_sym->type.t & VT_BTYPE) == VT_FUNC
          && member_sym->type.ref
          && (member_sym->type.ref->type.t & VT_BTYPE) != VT_STRUCT)
      {
        *ret_type = member_sym->type.ref->type;
        return 1;
      }
    }

  /* A scalar accumulator is commonly returned through a local `auto`, as in
       auto sum = zero<Element>();
       for (...) sum += vector[i];
       return sum;
     The interface prepass cannot parse that dependent initializer as a full
     expression yet.  Its type is nevertheless fixed by the first template
     argument of the concrete container specialization.  Publish that type
     instead of allowing an unresolved `auto` to fall back to C's implicit
     int, which truncates every later compound assignment. */
  if (args->nb > 0)
  {
    int returned_tok = 0;
    int saw_auto_decl = 0;
    int saw_accumulate = 0;
    int elem_tok = first_template_arg_tok_from_inst_tok(args->toks[0]);

    for (i = 0; i + 1 < td->def_str->len && !returned_tok; ++i)
      if (td->def_str->str[i] == TOK_RETURN)
      {
        int j = i + 1;
        while (j < td->def_str->len
               && td->def_str->str[j] == TOK_LINENUM)
          j += 2;
        if (j < td->def_str->len && td->def_str->str[j] >= TOK_UIDENT)
          returned_tok = td->def_str->str[j];
      }
    if (returned_tok && elem_tok)
    {
      for (i = 0; i + 1 < td->def_str->len; ++i)
      {
        int j;
        if (td->def_str->str[i] == TOK_AUTO)
        {
          j = i + 1;
          while (j < td->def_str->len
                 && td->def_str->str[j] == TOK_LINENUM)
            j += 2;
          if (j < td->def_str->len
              && td->def_str->str[j] == returned_tok)
            saw_auto_decl = 1;
        }
        if (td->def_str->str[i] == returned_tok)
        {
          j = i + 1;
          while (j < td->def_str->len
                 && td->def_str->str[j] == TOK_LINENUM)
            j += 2;
          if (j < td->def_str->len
              && td->def_str->str[j] == TOK_A_ADD)
            saw_accumulate = 1;
        }
      }
      if (saw_auto_decl && saw_accumulate
          && make_type_from_type_arg_tok(ret_type, elem_tok))
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
  CType member_ret_type;

  if (!td || !td->def_str || !args)
    return 0;
  if (infer_template_return_member_call_ctype(td, args, &member_ret_type)
      && (member_ret_type.t & VT_BTYPE) == VT_STRUCT)
    return get_struct_type_name_tok(&member_ret_type);
  for (i = 0; i < td->def_str->len; ++i)
    if (td->def_str->str[i] == td->name_tok)
    {
      name_index = i;
      break;
    }
  if (name_index <= 0)
    return 0;
  if (args->nb > 0 && struct_find(args->toks[0]))
  {
    int close_paren = -1, depth = 0, param_name = 0;

    for (i = name_index + 1; i < td->def_str->len; ++i)
    {
      int t = td->def_str->str[i];
      if (t == '(')
      {
        ++depth;
        continue;
      }
      if (t == ')')
      {
        if (--depth == 0)
        {
          close_paren = i;
          break;
        }
        continue;
      }
      if (depth == 1 && t >= TOK_UIDENT
          && template_param_index(td, t) < 0)
        param_name = t;
    }
    if (close_paren > 0 && param_name)
      for (i = close_paren + 1; i + 3 < td->def_str->len; ++i)
        if (td->def_str->str[i] == TOK_RETURN)
        {
          int ri = i + 1;
          int op;
          while (ri < td->def_str->len
                 && td->def_str->str[ri] == TOK_LINENUM)
            ri += 2;
          if (ri + 2 >= td->def_str->len
              || td->def_str->str[ri] != param_name)
            continue;
          op = td->def_str->str[ri + 1];
          if ((op == '+' || op == '-' || op == '*' || op == '/')
              && td->def_str->str[ri + 2] == param_name)
            return args->toks[0];
        }
  }
  if (args->nb > 0 && template_name_returns_first_vector_shape(td->name_tok))
  {
    int first_class_tok = template_def_first_param_class_tok(td);
    TemplateDef *class_td = find_class_template_def(first_class_tok);
    if (!first_class_tok && struct_find(args->toks[0]))
    {
      if (getenv("CPC_DUMP_AUTO_RET"))
        fprintf(stderr, "CPC_AUTO_RET %s first-type -> %s\n",
                get_tok_str(td->name_tok, NULL), get_tok_str(args->toks[0], NULL));
      return args->toks[0];
    }
    if (class_td && class_td->is_class && is_vector_template_tok(first_class_tok))
    {
      TemplateArgList class_args;
      int elem_tok = first_template_arg_tok_from_inst_tok(args->toks[0]);
      int inst_tok;
      class_args.nb = 1;
      class_args.toks[0] = elem_tok ? elem_tok : args->toks[0];
      inst_tok = instantiate_template_if_needed(class_td, &class_args);
      compile_pending_template_specs_without_member_flush();
      if (getenv("CPC_DUMP_AUTO_RET"))
        fprintf(stderr, "CPC_AUTO_RET %s first-vector -> %s\n",
                get_tok_str(td->name_tok, NULL), get_tok_str(inst_tok, NULL));
      return inst_tok;
    }
  }
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
    if (getenv("CPC_DUMP_AUTO_RET"))
      fprintf(stderr, "CPC_AUTO_RET %s declared -> %s\n",
              get_tok_str(td->name_tok, NULL), get_tok_str(inst_tok, NULL));
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
    int ri;

    if (td->def_str->str[i] != TOK_RETURN)
      continue;
    ri = i + 1;
    while (ri < td->def_str->len && td->def_str->str[ri] == TOK_LINENUM)
      ri += 2;
    if (ri + 4 >= td->def_str->len)
      continue;
    class_td = find_class_template_def(td->def_str->str[ri]);
    if (!class_td || !class_td->is_class
        || (td->def_str->str[ri + 1] != TOK_LT
            && td->def_str->str[ri + 1] != '<')
        || (td->def_str->str[ri + 3] != TOK_GT
            && td->def_str->str[ri + 3] != '>')
        || td->def_str->str[ri + 4] != '(')
      continue;
    param_idx = template_param_index(td, td->def_str->str[ri + 2]);
    if (param_idx < 0 || param_idx >= args->nb)
      continue;
    class_args.nb = 1;
    class_args.toks[0] = args->toks[param_idx];
    if (is_vector_template_tok(class_td->name_tok))
    {
      int elem_tok = first_template_arg_tok_from_inst_tok(class_args.toks[0]);
      if (elem_tok)
        class_args.toks[0] = elem_tok;
    }
    inst_tok = instantiate_template_if_needed(class_td, &class_args);
    compile_pending_template_specs_without_member_flush();
    if (getenv("CPC_DUMP_AUTO_RET"))
      fprintf(stderr, "CPC_AUTO_RET %s return-class -> %s\n",
              get_tok_str(td->name_tok, NULL), get_tok_str(inst_tok, NULL));
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
        int ri = i + 1;
        TemplateDef *return_td =
          NULL;
        int struct_tok;
        while (ri < td->def_str->len && td->def_str->str[ri] == TOK_LINENUM)
          ri += 2;
        if (ri + 1 >= td->def_str->len || td->def_str->str[ri + 1] != '(')
          continue;
        return_td = find_function_template_def_for_arg_count(
              td->def_str->str[ri],
              count_template_return_call_args(td, ri + 1),
              template_def_first_param_class_tok(td));
        if (!return_td)
          continue;
        ++inference_depth;
        struct_tok = infer_template_return_struct_tok(return_td, args);
        --inference_depth;
        if (struct_tok)
        {
          if (getenv("CPC_DUMP_AUTO_RET"))
            fprintf(stderr, "CPC_AUTO_RET %s via %s -> %s\n",
                    get_tok_str(td->name_tok, NULL),
                    get_tok_str(return_td->name_tok, NULL),
                    get_tok_str(struct_tok, NULL));
          return struct_tok;
        }
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
  ret_type->ref = s;
  return 1;
}

static void queue_demanded_function_template_bodies(void)
{
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
    int method_tok;

    if (!md || !token_string_has_top_level_body(md->def_str))
      continue;
    method_tok = template_member_def_method_tok(md);
    for (j = 0; j < md->nb_inst; ++j)
    {
      MemberFuncOverload *o;
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
      for (o = member_func_overloads; o; o = o->next)
        if (o->mangled_tok == func_tok && o->method_tok == method_tok)
        {
          class_mangled_tok = o->struct_tok;
          break;
        }
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
      sym_pop(&local_stack, NULL, 1);
      sym_relink_local_types(saved_local_stack);
    }
    local_stack = NULL;
    local_scope = 0;
    defer_pending_member_funcs = 1;
  }
  do
  {
    queue_demanded_function_template_bodies();
    made_progress = 0;
    for (i = 0; i < nb_pending_template_specs; ++i)
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
    if (getenv("CPC_TRACE_TEMPLATE_COMPILE"))
    {
      int ti, has = 0;
      for (ti = 0; ti < ts->len; ++ti)
      {
        if (TOK_HAS_VALUE(ts->str[ti]))
        {
          ti += tok_str_value_extra_words(ts->str, ts->len, ti);
          continue;
        }
        if (strstr(get_tok_str(ts->str[ti], NULL),
                   getenv("CPC_TRACE_TEMPLATE_COMPILE")))
        {
          has = 1;
          break;
        }
      }
      if (has)
      {
        fprintf(stderr, "CPC_TEMPLATE_COMPILE toks=");
        for (ti = 0; ti < ts->len && ti < 100; ++ti)
        {
          if (TOK_HAS_VALUE(ts->str[ti]))
          {
            fprintf(stderr, "%s<value>", ti ? " " : "");
            ti += tok_str_value_extra_words(ts->str, ts->len, ti);
            continue;
          }
          fprintf(stderr, "%s%s", ti ? " " : "",
                  get_tok_str(ts->str[ti], NULL));
        }
        fprintf(stderr, "\n");
      }
    }
    /* Mark the work item consumed before re-entering decl().  Resolving the
       generated declaration can recursively flush this queue; leaving the
       token visible would replay the same item and redeclare its prototype. */
    pending_template_specs[i] = NULL;
    {
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
      begin_macro(ts, 1);
      next();
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
    sym_unlink_local_types(saved_local_stack);
    sym_relink_stack(saved_local_stack);
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

static int infer_template_args_from_call(TemplateDef *td, CType *arg_types,
                                         int arg_count, TemplateArgList *args)
{
  int i, j, arg_index, type_tok;
  int pack_param_index = td->variadic_param_index >= 0
                         ? td->variadic_param_index
                         : -1;
  int pack_arg_start = pack_param_index >= 0 ? td->func_min_args : -1;

  args->nb = td->nb_required_type_params;
  if (pack_param_index >= 0 && pack_arg_start < arg_count
      && args->nb + (arg_count - pack_arg_start) <= 16)
    args->nb += arg_count - pack_arg_start;
  for (i = 0; i < args->nb; ++i)
    args->toks[i] = 0;
  if (td->def_str)
  {
    int name_index = -1, paren_index = -1, depth = 0, param_start = -1;

    for (i = 0; i < td->def_str->len; ++i)
    {
      if (td->def_str->str[i] == td->name_tok)
        name_index = i;
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
          for (j = param_start; j < end; ++j)
          {
            TemplateDef *class_td;
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

            param_idx = template_param_index(td, td->def_str->str[j]);
            if (param_idx >= 0 && type_tok && param_depth == 0)
            {
              int after_param = j + 1;
              while (after_param < end
                     && td->def_str->str[after_param] == TOK_LINENUM)
                after_param += 2;
              /* Deduce T from the pointee for a parameter spelled T*.  The
                 generic fallback must retain an actual pointer for T& (as in
                 swap), so pointer decay belongs to the parameter pattern,
                 not template_type_tok_from_ctype(). */
              if (after_param < end && td->def_str->str[after_param] == '*'
                  && (arg_types[arg_index].t & VT_BTYPE) == VT_PTR)
              {
                CType pointee = *pointed_type(&arg_types[arg_index]);
                int pointee_tok = template_type_tok_from_ctype(&pointee);
                if (pointee_tok)
                  type_tok = pointee_tok;
              }
              if (args->toks[param_idx]
                  && args->toks[param_idx] != type_tok)
                return 0;
              args->toks[param_idx] = type_tok;
              continue;
            }
            class_td = find_class_template_def(td->def_str->str[j]);
            if (j + 3 >= end
                || !class_td || !class_td->is_class
                || (td->def_str->str[j + 1] != TOK_LT
                    && td->def_str->str[j + 1] != '<'))
              continue;
            param_idx = template_param_index(td, td->def_str->str[j + 2]);
            if (param_idx < 0 || !type_tok)
              continue;
            {
              int class_inst_tok = type_tok;

              /* Match the outer pointer in Container<T>* before looking up
                 the concrete Container specialization.  Otherwise the
                 fallback below records the entire pointer as T and replays
                 a signature such as Container<Container<int>*>*. */
              if ((arg_types[arg_index].t & VT_BTYPE) == VT_PTR)
              {
                CType pointee = *pointed_type(&arg_types[arg_index]);
                int pointee_tok = template_type_tok_from_ctype(&pointee);
                if (pointee_tok)
                  class_inst_tok = pointee_tok;
              }
              inst_arg = template_inst_arg_for_name(class_td,
                                                    class_inst_tok, 0);
            }
            if (inst_arg && args->toks[param_idx]
                && args->toks[param_idx] != inst_arg)
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
  /* Fill inferred pack element types from the remaining call arguments so
     variadic forwarding bodies (e.g. clConstruct's clForward(Args, args)...)
     receive a concrete type for each pack element. */
  if (pack_param_index >= 0)
    for (i = pack_arg_start; i < arg_count && i < 16; ++i)
    {
      type_tok = template_type_tok_from_ctype(&arg_types[i]);
      if (type_tok && pack_param_index + (i - pack_arg_start) < args->nb)
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
    int current_func_tok = tok_alloc_const(funcname);
    int di, ii;
    for (di = 0; di < nb_template_defs; ++di)
    {
      TemplateDef *current_td = template_defs[di];
      if (!current_td || current_td->is_class)
        continue;
      for (ii = 0; ii < current_td->nb_inst; ++ii)
        if (current_td->inst_name_toks[ii] == current_func_tok)
        {
          TemplateArgList *current_args = current_td->inst_arg_lists
            ? &current_td->inst_arg_lists[ii] : NULL;
          if (current_args)
            for (i = 0; i < args->nb && i < current_args->nb; ++i)
              if (!args->toks[i])
                args->toks[i] = current_args->toks[i];
          di = nb_template_defs;
          break;
        }
    }
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
      cprime_error("unable to infer template type argument %d for '%s' in '%s'",
                   i, get_tok_str(td->name_tok, NULL),
                   funcname ? funcname : "<global>");
    type_tok = template_type_tok_from_ctype(&arg_types[arg_index]);
    if (!type_tok)
      cprime_error("unable to infer template type argument %d for '%s' from argument %d in '%s'",
                   i, get_tok_str(td->name_tok, NULL), arg_index,
                   funcname ? funcname : "<global>");
    args->toks[i] = type_tok;
  }
  return 1;
}
