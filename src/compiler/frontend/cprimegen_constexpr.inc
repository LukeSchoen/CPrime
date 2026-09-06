/* Keep constexpr return expressions independently of emitted inline bodies.
   Evaluation uses the ordinary expression parser, preserving its constant
   checks and the function's declaration scope. */
typedef struct ConstexprFunction {
  int token, namespace_tok, owner_tok;
  TokenString expression;
  struct ConstexprFunction *next;
} ConstexprFunction;
static ConstexprFunction *constexpr_functions;
static int constexpr_evaluation_depth;

static void note_constexpr_function(Sym *function, TokenString *body)
{
  int i, first, end;
  ConstexprFunction *entry;
  TokenString *expression;
  if (!function || !function->type.ref || function->type.ref->next || !body) return;
  first = tok_str_next_non_linenum_index(body->str, body->len, 0);
  if (first >= body->len || body->str[first] != '{') return;
  first = tok_str_next_non_linenum_index(body->str, body->len, first + 1);
  if (first >= body->len || body->str[first] != TOK_RETURN) return;
  first = tok_str_next_non_linenum_index(body->str, body->len, first + 1);
  end = first;
  while (end < body->len && body->str[end] != ';') {
    if (TOK_HAS_VALUE(body->str[end])) end += tok_str_value_extra_words(body->str, body->len, end);
    ++end;
  }
  i = tok_str_next_non_linenum_index(body->str, body->len, end + 1);
  if (i >= body->len || body->str[i] != '}') return;
  for (entry = constexpr_functions; entry; entry = entry->next)
    if (entry->token == function->v) return;
  expression = tok_str_alloc();
  for (i = first; i < end; ++i) tok_str_add_record(expression, body->str, body->len, &i);
  tok_str_add(expression, TOK_EOF);
  entry = cprime_mallocz(sizeof(*entry));
  entry->token = function->v;
  entry->namespace_tok = current_namespace_scope_tok();
  entry->owner_tok = member_function_class_tok(function->v);
  entry->expression.len = expression->len;
  entry->expression.str = cprime_malloc(expression->len * sizeof(int));
  memcpy(entry->expression.str, expression->str, expression->len * sizeof(int));
  tok_str_free(expression);
  entry->next = constexpr_functions;
  constexpr_functions = entry;
}

static int evaluate_constexpr_function(Sym *function, SValue *result)
{
  ConstexprFunction *entry;
  TokenString replay, *parent;
  const int *parent_ptr;
  int saved_tok, saved_namespace, saved_owner, valid, saved_nocode;
  CValue saved_tokc;
  Sym *saved_locals;
  Sym **bindings = NULL;
  int saved_scope, binding_count = 0;
  for (entry = constexpr_functions; entry; entry = entry->next)
    if (entry->token == function->v) break;
  if (!entry && !constexpr_function_materialization)
  {
    int i, j;
    for (i = 0; i < nb_template_defs; ++i)
    {
      TemplateDef *definition = template_defs[i];
      if (definition->is_class || !definition->has_body
          || !token_string_contains_tok(definition->def_str, tok_constexpr)) continue;
      for (j = 0; j < definition->nb_inst; ++j)
        if (definition->inst_name_toks[j] == function->v)
        {
          ++constexpr_function_materialization;
          instantiate_template_if_needed(definition, &definition->inst_arg_lists[j]);
          compile_pending_template_specs_without_member_flush();
          --constexpr_function_materialization;
          break;
        }
    }
    for (entry = constexpr_functions; entry; entry = entry->next)
      if (entry->token == function->v) break;
  }
  if (!entry) return 0;
  if (++constexpr_evaluation_depth > 128) cprime_error("constexpr evaluation depth exceeded");
  saved_tok = tok; saved_tokc = tokc;
  parent = macro_stack; parent_ptr = macro_ptr;
  saved_namespace = current_namespace_scope_tok();
  saved_owner = active_member_class_tok;
  saved_nocode = nocode_wanted;
  saved_locals = local_stack;
  saved_scope = local_scope;
  if (saved_locals) bindings = sym_hide_local_bindings(saved_locals, &binding_count);
  local_stack = NULL;
  local_scope = 0;
  nocode_wanted |= CONST_WANTED_BIT;
  enter_namespace_scope(entry->namespace_tok);
  active_member_class_tok = entry->owner_tok;
  replay = entry->expression;
  begin_macro(&replay, 3);
  next();
  expr_eq();
  valid = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_CONST;
  if (valid) {
    gen_cast(&function->type.ref->type);
    *result = *vtop;
    if (getenv("CPC_TRACE_CONSTEXPR")) fprintf(stderr, "CPC_CONSTEXPR %s type=%x value=%llx\n", get_tok_str(function->v, NULL), result->type.t, result->c.i);
  }
  vpop();
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  tok = saved_tok; tokc = saved_tokc;
  enter_namespace_scope(saved_namespace);
  active_member_class_tok = saved_owner;
  local_stack = saved_locals;
  local_scope = saved_scope;
  if (bindings) sym_relink_stack(bindings, binding_count);
  nocode_wanted = saved_nocode;
  --constexpr_evaluation_depth;
  return valid;
}

static void free_constexpr_functions(void)
{
  while (constexpr_functions) {
    ConstexprFunction *entry = constexpr_functions;
    constexpr_functions = entry->next;
    cprime_free(entry->expression.str);
    cprime_free(entry);
  }
  constexpr_evaluation_depth = 0;
}
