/* Alias templates substitute a type-id and publish a typedef. They do not
   declare a function or introduce a distinct class identity. */
static TemplateDef *find_cpp_alias_template(int name)
{
  TemplateDef *definition;
  for (definition = template_def_candidates(name); definition;
       definition = definition->lookup_bucket_next)
    if (definition->is_alias && template_def_lookup_tok(definition) == name)
      return definition;
  return NULL;
}

static TemplateDef *find_cpp_alias_template_in_scope(int name, int global_scope)
{
  TemplateDef *definition;
  int depth, i, parts[17];
  if (global_scope) return find_cpp_alias_template(name);
  for (i = 0; i < nb_namespace_stack; ++i) parts[i] = namespace_stack[i];
  for (depth = nb_namespace_stack; depth >= 0; --depth) {
    parts[depth] = name;
    definition = find_cpp_alias_template(depth
      ? make_namespace_tok_from_parts(parts, depth + 1) : name);
    if (definition) return definition;
  }
  for (i = nb_using_namespace_toks - 1; i >= 0; --i) {
    definition = find_cpp_alias_template(make_static_member_tok(using_namespace_toks[i], name));
    if (definition) return definition;
  }
  return NULL;
}

static int instantiate_cpp_alias_template(TemplateDef *definition,
                                           TemplateArgList *arguments)
{
  TokenString *substituted, *pattern;
  CType type;
  Sym *symbol, *saved_locals;
  int name = template_lookup_inst(definition, arguments);
  int i, rhs = 0, index, saved_scope;
  if (name) {
    symbol = global_symbol_find(name);
    if (symbol && (symbol->type.t & VT_TYPEDEF)) return name;
    for (index = 0; index < definition->nb_inst; ++index)
      if (definition->inst_name_toks[index] == name) break;
    if (index < definition->nb_inst && definition->inst_states[index] == 1)
      cprime_error("recursive alias template '%s'", get_tok_str(definition->name_tok, NULL));
  } else {
    name = make_template_inst_name_tok(definition, arguments);
    note_template_inst(definition, arguments, name);
    index = definition->nb_inst - 1;
  }
  pattern = tok_str_alloc();
  for (i = 0; i < definition->def_str->len; ++i) {
    int token = definition->def_str->str[i];
    if (TOK_HAS_VALUE(token)) {
      if (rhs) tok_str_add_record(pattern, definition->def_str->str,
                                 definition->def_str->len, &i);
      else i += tok_str_value_extra_words(definition->def_str->str,
                                          definition->def_str->len, i);
      continue;
    }
    if (!rhs) { if (token == '=') rhs = 1; continue; }
    if (token == ';' || token == TOK_EOF) break;
    tok_str_add(pattern, token);
  }
  tok_str_add(pattern, TOK_EOF);
  if (definition->variadic_param_index >= 0) {
    int pack = definition->variadic_param_index;
    TokenString *expanded = expand_template_expression_packs(pattern,
      definition->type_param_toks[pack], 0, 0,
      arguments->toks + pack, arguments->nb - pack);
    tok_str_free(pattern);
    pattern = expanded;
  }
  substituted = tok_str_alloc();
  for (i = 0; i < pattern->len; ++i) {
    int token = pattern->str[i];
    if (token == TOK_EOF) break;
    if (TOK_HAS_VALUE(token))
      tok_str_add_record(substituted, pattern->str, pattern->len, &i);
    else
      tok_str_add(substituted, template_subst_token(definition, arguments, name,
                                                    NULL, NULL, 0, token));
  }
  tok_str_add(substituted, TOK_EOF);
  tok_str_free(pattern);
  definition->inst_states[index] = 1;
  i = template_probe_type(substituted, 0, definition->namespace_tok, &type);
  tok_str_free(substituted);
  definition->inst_states[index] = 0;
  if (!i)
    cprime_error("invalid type in alias template '%s'", get_tok_str(definition->name_tok, NULL));
  saved_locals = local_stack; saved_scope = local_scope;
  local_stack = NULL; local_scope = 0;
  type.t |= VT_TYPEDEF;
  symbol = sym_push(name, &type, 0, 0);
  local_stack = saved_locals; local_scope = saved_scope;
  definition->inst_states[index] = 2;
  return name;
}

static int try_expand_cpp_alias_template(int global_scope)
{
  TemplateDef *definition;
  TokenString *replay;
  TemplateArgList arguments;
  Sym *visible;
  int parts[17], count = 0, name, alias_name;
  if (!is_cpp_translation_unit() || tok < TOK_UIDENT) return 0;
  visible = global_scope ? global_symbol_find(tok) : sym_find(tok);
  if (visible && visible->sym_scope && !(visible->type.t & VT_TYPEDEF)) return 0;
  definition = find_cpp_alias_template_in_scope(tok, global_scope);
  if (!definition && !is_namespace_tok(tok)) return 0;
  replay = tok_str_alloc();
  for (;;) {
    if (count == 17) cprime_error("namespace nesting too deep");
    parts[count++] = tok;
    tok_str_add_tok(replay);
    next();
    if (definition) break;
    if (tok != ':') goto restore;
    tok_str_add_tok(replay); next();
    if (tok != ':') goto restore;
    tok_str_add_tok(replay); next();
    if (tok < TOK_UIDENT) goto restore;
    parts[count] = tok;
    name = find_namespace_tok_from_parts(parts, count + 1);
    definition = find_cpp_alias_template(name);
  }
  if (tok != TOK_LT && tok != '<') goto restore;
  tok_str_free(replay);
  parse_template_type_args(&arguments);
  template_validate_arg_count(definition, &arguments);
  alias_name = instantiate_cpp_alias_template(definition, &arguments);
  unget_tok(tok);
  tok = alias_name;
  return 1;
restore:
  restore_cpp_lifecycle_probe(replay);
  return 0;
}

static void cpp_alias_append_pattern(TokenString *destination,
                                      TokenString *source, int start, int end)
{
  int i;
  for (i = start; i < end && source->str[i] != TOK_EOF; ++i)
    tok_str_add_record(destination, source->str, source->len, &i);
}

/* Deduction uses an alias's defining type-id. Expand the saved parameter
   pattern before deduction, retaining unbound template parameters as tokens.
   Materializing an alias with invented concrete types would incorrectly
   evaluate dependent traits and non-deduced contexts. */
static TokenString *cpp_expand_alias_deduction_patterns(TemplateDef *context)
{
  TokenString *source = context->def_str, *owned = NULL;
  int i, expansions = 0;
  for (i = 0; i < source->len; ++i) {
    TemplateDef *alias = NULL;
    TokenString *arguments[16] = {0}, *replacement, *result;
    int name = source->str[i], start = i, open, at, count = 0;
    int angle = 1, paren = 0, bracket = 0, extra_close = 0;
    int j, rhs = 0;
    if (TOK_HAS_VALUE(name)) {
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    if (name == '{' || name == TOK_EOF) break;
    if (name < TOK_UIDENT) continue;
    open = tok_str_next_non_linenum_index(source->str, source->len, i + 1);
    if (open >= source->len || source->str[open] != TOK_LT) continue;
    while (start >= 3 && source->str[start - 1] == ':'
           && source->str[start - 2] == ':'
           && source->str[start - 3] >= TOK_UIDENT) start -= 3;
    if (start != i) {
      int parts[17], n = 0;
      for (j = start; j <= i && n < 17; j += 3) parts[n++] = source->str[j];
      name = make_namespace_tok_from_parts(parts, n);
    }
    if (context->namespace_tok)
      alias = find_cpp_alias_template(make_static_member_tok(context->namespace_tok, name));
    if (!alias) alias = find_cpp_alias_template(name);
    if (!alias) continue;
    if (++expansions > 64) cprime_error("recursive alias in template parameter type");
    arguments[0] = tok_str_alloc();
    for (at = open + 1; at < source->len; ++at) {
      int token = source->str[at];
      if (token == TOK_EOF) break;
      if (TOK_HAS_VALUE(token)) {
        tok_str_add_record(arguments[count], source->str, source->len, &at);
        continue;
      }
      if (token == '(') ++paren;
      else if (token == ')') --paren;
      else if (token == '[') ++bracket;
      else if (token == ']') --bracket;
      if (!paren && !bracket) {
        if (token == TOK_LT) ++angle;
        if (token == TOK_GT || token == TOK_SAR) {
          int closes = token == TOK_SAR ? 2 : 1;
          while (closes--) {
            if (!--angle) { extra_close = closes; break; }
            tok_str_add(arguments[count], TOK_GT);
          }
          if (!angle) break;
          continue;
        }
        if (token == ',' && angle == 1) {
          if (++count == 16) cprime_error("too many alias template arguments");
          arguments[count] = tok_str_alloc();
          continue;
        }
      }
      tok_str_add(arguments[count], token);
    }
    if (arguments[0]->len || count) ++count;
    else { tok_str_free(arguments[0]); arguments[0] = NULL; }
    if (angle || count < alias->nb_required_type_params
        || (alias->variadic_param_index < 0 && count > alias->nb_type_params)) {
      for (j = 0; j < count; ++j) tok_str_free(arguments[j]);
      continue;
    }
    while (count < alias->nb_type_params && alias->variadic_param_index != count) {
      TokenString *defaults = alias->default_arg_strs[count];
      if (!defaults) break;
      arguments[count] = tok_str_alloc();
      for (j = 0; j < defaults->len; ++j) {
        int parameter = template_param_index(alias, defaults->str[j]);
        if (parameter >= 0 && parameter < count)
          cpp_alias_append_pattern(arguments[count], arguments[parameter], 0,
                                      arguments[parameter]->len);
        else if (defaults->str[j] != TOK_EOF)
          tok_str_add_record(arguments[count], defaults->str, defaults->len, &j);
      }
      ++count;
    }
    replacement = tok_str_alloc();
    for (j = 0; j < alias->def_str->len; ++j) {
      int token = alias->def_str->str[j], parameter, argument;
      if (TOK_HAS_VALUE(token)) {
        if (rhs) tok_str_add_record(replacement, alias->def_str->str, alias->def_str->len, &j);
        else j += tok_str_value_extra_words(alias->def_str->str, alias->def_str->len, j);
        continue;
      }
      if (!rhs) { if (token == '=') rhs = 1; continue; }
      if (token == ';' || token == TOK_EOF) break;
      parameter = template_param_index(alias, token);
      if (parameter == alias->variadic_param_index && parameter >= 0
          && j + 1 < alias->def_str->len && alias->def_str->str[j + 1] == TOK_DOTS) {
        for (argument = parameter; argument < count; ++argument) {
          if (argument != parameter) tok_str_add(replacement, ',');
          cpp_alias_append_pattern(replacement, arguments[argument], 0, arguments[argument]->len);
        }
        ++j;
      } else if (parameter >= 0 && parameter < count)
        cpp_alias_append_pattern(replacement, arguments[parameter], 0, arguments[parameter]->len);
      else {
        if (token >= TOK_UIDENT && alias->namespace_tok
            && !(j && alias->def_str->str[j - 1] == ':')) {
          int qualified = make_static_member_tok(alias->namespace_tok, token);
          if (find_class_template_def(qualified) || find_cpp_alias_template(qualified)
              || global_symbol_find(qualified)) token = qualified;
        }
        tok_str_add(replacement, token);
      }
    }
    result = tok_str_alloc();
    cpp_alias_append_pattern(result, source, 0, start);
    tok_str_append(result, replacement);
    while (extra_close--) tok_str_add(result, TOK_GT);
    cpp_alias_append_pattern(result, source, at + 1, source->len);
    tok_str_add(result, TOK_EOF);
    tok_str_free(replacement);
    for (j = 0; j < count; ++j) tok_str_free(arguments[j]);
    if (owned) tok_str_free(owned);
    source = owned = result;
    i = start - 1;
  }
  return owned;
}
