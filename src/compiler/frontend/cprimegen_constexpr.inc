/* Keep constexpr bodies independently of emitted inline bodies.
   Evaluation uses the ordinary expression parser, preserving its constant
   checks and the function's declaration scope. */
typedef struct ConstexprFunction {
  int token, namespace_tok, owner_tok;
  int parameter_count;
  int *parameter_names;
  TokenString expression;
  struct ConstexprFunction *next;
} ConstexprFunction;
static ConstexprFunction *constexpr_functions;
static int constexpr_evaluation_depth;

static int evaluate_builtin_function_call(Sym *function, SValue *arguments,
                                          int count, SValue *result)
{
  const char *name;
  SValue *argument;
  ObjSym *symbol;
  Section *section;
  const unsigned char *bytes;
  int length, available;
  if (!function || !function->type.ref || count != 1)
    return 0;
  name = function->asm_label
           ? get_tok_str(function->asm_label, NULL)
           : get_tok_str(function->v, NULL);
  if (!name || strcmp(name, "strlen"))
    return 0;
  argument = arguments;
  if (!argument || !argument->sym
      || (argument->r & (VT_VALMASK | VT_LVAL | VT_SYM)) != (VT_CONST | VT_SYM)
      || !argument->sym->string_literal || !argument->sym->c)
    return 0;
  symbol = elfsym(argument->sym);
  if (!symbol || symbol->st_shndx <= 0
      || symbol->st_shndx >= cprime_state->nb_sections)
    return 0;
  section = cprime_state->sections[symbol->st_shndx];
  if (!section || !section->data
      || symbol->st_value >= section->data_offset)
    return 0;
  bytes = section->data + symbol->st_value;
  available = (int)(section->data_offset - symbol->st_value);
  for (length = 0; length < available; ++length)
    if (!bytes[length])
      break;
  if (length == available)
    return 0;
  result->type = function->type.ref->type;
  result->r = VT_CONST;
  result->c.i = length;
  return 1;
}

static void note_constexpr_function(Sym *function, TokenString *body)
{
  int i, first, end;
  ConstexprFunction *entry;
  TokenString *expression;
  Sym *parameter;
  if (!function || !function->type.ref || !body) return;
  first = tok_str_next_non_linenum_index(body->str, body->len, 0);
  if (first >= body->len || body->str[first] != '{') return;
  end = body->len;
  for (entry = constexpr_functions; entry; entry = entry->next)
    if (entry->token == function->v) return;
  expression = tok_str_alloc();
  for (i = first; i < end; ++i) tok_str_add_record(expression, body->str, body->len, &i);
  if (!expression->len || expression->str[expression->len - 1] != TOK_EOF)
    tok_str_add(expression, TOK_EOF);
  entry = cprime_mallocz(sizeof(*entry));
  entry->token = function->v;
  entry->namespace_tok = current_namespace_scope_tok();
  entry->owner_tok = member_function_class_tok(function->v);
  for (parameter = function->type.ref->next; parameter; parameter = parameter->next)
    ++entry->parameter_count;
  if (entry->parameter_count) {
    entry->parameter_names = cprime_malloc(entry->parameter_count * sizeof(int));
    for (parameter = function->type.ref->next, i = 0; parameter; parameter = parameter->next)
      entry->parameter_names[i++] = parameter->v & ~SYM_FIELD;
  }
  entry->expression.len = expression->len;
  entry->expression.str = cprime_malloc(expression->len * sizeof(int));
  memcpy(entry->expression.str, expression->str, expression->len * sizeof(int));
  tok_str_free(expression);
  entry->next = constexpr_functions;
  constexpr_functions = entry;
}

static int evaluate_constexpr_function(Sym *function, SValue *arguments, int count, SValue *result)
{
  ConstexprFunction *entry;
  TokenString replay, *parent;
  const int *parent_ptr;
  int saved_tok, saved_namespace, saved_owner, valid, saved_nocode;
  CValue saved_tokc;
  Sym *saved_locals;
  Sym **bindings = NULL;
  int saved_scope, binding_count = 0, argument;
  SValue *saved_top = vtop;
  for (argument = 0; argument < count; ++argument)
    if ((arguments[argument].r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST
        && !((arguments[argument].type.t & VT_BTYPE) == VT_PTR
             && (arguments[argument].r & VT_VALMASK) == VT_CONST
             && (arguments[argument].r & VT_SYM)
             && !(arguments[argument].r & VT_NONCONST)))
      return 0;
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
  if (!entry || entry->parameter_count != count) return 0;
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
  sym_push2(&local_stack, SYM_FIELD, 0, 0);
  local_scope = 1;
  for (argument = 0; argument < count; ++argument) {
    Sym *binding = sym_push(entry->parameter_names[argument], &arguments[argument].type,
                            VT_CONST, arguments[argument].c.i);
    binding->type.t |= VT_CONSTANT;
    if (is_float(arguments[argument].type.t)) {
      binding->a.static_float_constant = 1;
      binding->const_float_value = arguments[argument].c;
    } else {
      binding->a.integral_constexpr = 1;
      binding->const_value = arguments[argument].c.i;
    }
  }
  nocode_wanted |= CONST_WANTED_BIT;
  enter_namespace_scope(entry->namespace_tok);
  active_member_class_tok = entry->owner_tok;
  replay = entry->expression;
  begin_macro(&replay, 3);
  next();
  skip('{');
  valid = 0;
  while (tok != TOK_RETURN) {
    CType local_type;
    AttributeDef attributes = {0};
    Sym *binding;
    int name = 0;
    if (!parse_btype(&local_type, &attributes, 0)) goto evaluated;
    type_decl(&local_type, &attributes, &name, TYPE_DIRECT);
    if (!name || tok != '=') goto evaluated;
    next();
    if ((local_type.t & VT_BTYPE) == VT_STRUCT || (local_type.t & VT_ARRAY)) {
      if (tok != '{' || type_requires_class_construction(&local_type)
          || resolve_autodtor_func(&local_type)) goto evaluated;
      next();
      if (tok != '}') goto evaluated;
      next();
      /* A typed local is sufficient in unevaluated/value-ignored operands.
         Reading its stored value is not folded until object evaluation is supported. */
      sym_push(name, &local_type, VT_LOCAL | VT_LVAL, 0);
    } else {
      expr_eq();
      if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
        goto evaluated;
      gen_assign_cast(&local_type);
      local_type.t |= VT_CONSTANT;
      binding = sym_push(name, &local_type, VT_CONST, 0);
      if (is_float(local_type.t)) {
        binding->a.static_float_constant = 1;
        binding->const_float_value = vtop->c;
      } else {
        binding->a.integral_constexpr = 1;
        binding->const_value = vtop->c.i;
      }
      vpop();
    }
    if (tok != ';') goto evaluated;
    next();
  }
  next();
  expr_eq();
  valid = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_CONST;
  if (valid) {
    gen_cast(&function->type.ref->type);
    *result = *vtop;
  }
evaluated:
  vtop = saved_top;
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  tok = saved_tok; tokc = saved_tokc;
  enter_namespace_scope(saved_namespace);
  active_member_class_tok = saved_owner;
  sym_pop(&local_stack, NULL, 0);
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
    cprime_free(entry->parameter_names);
    cprime_free(entry);
  }
  constexpr_evaluation_depth = 0;
}
