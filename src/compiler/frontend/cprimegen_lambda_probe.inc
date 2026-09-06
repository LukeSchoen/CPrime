/* Parse the complete body for deduction. This uses the normal statement
   parser, so local declarations and shadowing determine capture use. */
static void probe_lambda_body(LambdaExpression *lambda, TokenString *parameters,
                              TokenString *body)
{
  struct scope isolated = {0};
  struct scope *saved_current = cur_scope, *saved_root = root_scope;
  struct scope *saved_loop = loop_scope;
  struct switch_t *saved_switch = cur_switch;
  struct temp_local_variable saved_temps[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
  Sym *saved_locals, *saved_cleanups = all_cleanups, *saved_gotos = pending_gotos;
  Sym *saved_labels = local_label_stack;
  SValue saved_values[VSTACK_SIZE];
  int saved_scope, i, saved_tok = tok, saved_loc = loc;
  int saved_nocode = nocode_wanted, saved_temp_count = nb_temp_local_vars;
  int saved_unevaluated_depth = cpp_unevaluated_expression_depth;
  int saved_defer = defer_pending_member_funcs;
  int saved_type_owner = local_type_owner_tok;
  int saved_type_index = local_type_declaration_index;
  int value_count = (int)(vtop - vstack + 1);
  char saved_debug = debug_modes;
  CValue saved_tokc = tokc;
  TokenString macro = *body, *parent = macro_stack;
  const int *parent_ptr = macro_ptr;
  LambdaExpression *saved_probe = lambda_probe;

  memcpy(saved_values, vstack, value_count * sizeof(SValue));
  memcpy(saved_temps, arr_temp_local_vars, sizeof(saved_temps));
  vtop = vstack - 1;
  nocode_wanted = 1;
  cpp_unevaluated_expression_depth = 0;
  defer_pending_member_funcs = 1;
  debug_modes = 0;
  nb_temp_local_vars = 0;
  cur_scope = root_scope = &isolated;
  cur_switch = NULL;
  loop_scope = NULL;
  all_cleanups = pending_gotos = local_label_stack = NULL;
  push_saved_param_scope(parameters, &saved_locals, &saved_scope);
  /* Keep the parent symbols visible through the token table, but give the
     probe its own allocation stack and parameter scope. */
  for (i = 0; i < lambda->count; ++i)
  {
    CType visible = lambda->captures[i].type;
    if (!lambda->captures[i].by_reference && !lambda->mutable_call)
      visible.t |= VT_CONSTANT;
    sym_push(lambda->captures[i].name, &visible, VT_LOCAL | VT_LVAL, 0);
  }
  lambda->parent_probe = saved_probe;
  if (lambda->body_owner_tok)
  {
    local_type_owner_tok = lambda->body_owner_tok;
    local_type_declaration_index = 0;
  }
  lambda_probe = lambda;
  begin_macro(&macro, 0);
  next();
  block(STMT_COMPOUND);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  lambda_probe = saved_probe;
  local_type_owner_tok = saved_type_owner;
  local_type_declaration_index = saved_type_index;
  sym_pop(&local_stack, NULL, 0);
  sym_pop(&all_cleanups, NULL, 0);
  sym_pop(&pending_gotos, NULL, 0);
  label_pop(&local_label_stack, NULL, 0);
  local_stack = saved_locals;
  local_scope = saved_scope;
  all_cleanups = saved_cleanups;
  pending_gotos = saved_gotos;
  local_label_stack = saved_labels;
  cur_scope = saved_current;
  root_scope = saved_root;
  loop_scope = saved_loop;
  cur_switch = saved_switch;
  loc = saved_loc;
  nocode_wanted = saved_nocode;
  cpp_unevaluated_expression_depth = saved_unevaluated_depth;
  nb_temp_local_vars = saved_temp_count;
  defer_pending_member_funcs = saved_defer;
  debug_modes = saved_debug;
  memcpy(arr_temp_local_vars, saved_temps, sizeof(saved_temps));
  memcpy(vstack, saved_values, value_count * sizeof(SValue));
  vtop = vstack + value_count - 1;
  tok = saved_tok;
  tokc = saved_tokc;
}
