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
static int token_string_has_variadic_pack(TokenString *str);

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
  if (!td->is_class && td->variadic_param_index >= 0)
    return 0;
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
  int function_params_started = 0;
  int function_params_done = 0;
  int function_param_depth = 0;
  int pack_type_tok = 0;
  int pack_start = -1;
  int pack_name_tok = 0;

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
      if (pack_start >= 0 && args->nb == pack_start + 1)
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
  if (pack_param_index >= 0)
    for (i = pack_arg_start; i < arg_count && i < 16; ++i)
    {
      type_tok = template_type_tok_from_ctype(&arg_types[i]);
      if (type_tok && pack_param_index + (i - pack_arg_start) < args->nb)
        args->toks[pack_param_index + (i - pack_arg_start)] = type_tok;
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
