/* A class-prvalue conditional constructs exactly one result object. Share
   its destination between branches, after the condition has been evaluated. */
static int cpp_is_conditional_marker(int token)
{
  return token >= TOK_UIDENT && token < SYM_FIRST_ANOM
      && !strncmp(get_tok_str(token, NULL), "__cpc_conditional_result_", 25);
}

static int cpp_prepare_conditional_destination(TokenString *expression)
{
  static unsigned sequence;
  const int *after = cpp_saved_conditional_at(expression, NULL, NULL);
  int index;
  char name[80];
  if (!after) return 0;
  index = (int)(after - expression->str) - 1;
  if (!cpp_is_conditional_marker(expression->str[index])) {
    snprintf(name, sizeof(name), "__cpc_conditional_result_%u", ++sequence);
    expression->str[index] = tok_alloc_const(name);
  }
  return expression->str[index];
}
static TokenString *cpp_token_slice(TokenString *source, int first, int end)
{
  TokenString *result = tok_str_alloc();
  int i;
  for (i = first; i < end; ++i)
    if (TOK_HAS_VALUE(source->str[i]))
      tok_str_add_record(result, source->str, source->len, &i);
    else tok_str_add(result, source->str[i]);
  tok_str_add(result, TOK_EOF);
  return result;
}

static int cpp_saved_arm_is_throw(TokenString *expression)
{
  int first = tok_str_next_non_linenum_index(expression->str, expression->len, 0);
  while (first < expression->len && expression->str[first] == '(')
    first = tok_str_next_non_linenum_index(expression->str, expression->len, first + 1);
  return first < expression->len && expression->str[first] == TOK_THROW;
}

static int cpp_type_may_name_class(CType *type)
{
  int depth = 0;
  while (type && ++depth < 64) {
    int bt = type->t & VT_BTYPE;
    if (bt == VT_STRUCT) return 1;
    if ((bt != VT_PTR && bt != VT_FUNC) || !type->ref) return 0;
    type = &type->ref->type;
  }
  return depth >= 64;
}

/* If every named operand and function result is scalar, overload resolution
   cannot turn this arm into a class prvalue. Leave that common case on the
   ordinary single-pass conditional path. Unknown names stay conservative. */
static int cpp_arm_may_produce_class(TokenString *arm)
{
  int i;
  for (i = 0; i < arm->len; ++i) {
    int token = arm->str[i];
    Sym *symbol;
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(arm->str, arm->len, i);
      continue;
    }
    if (token == TOK_THROW) return 1;
    if (token < TOK_UIDENT || token == TOK_EOF) continue;
    symbol = sym_find(token);
    if (!symbol) symbol = global_symbol_find(token);
    if (!symbol || cpp_type_may_name_class(&symbol->type)
        || has_free_func_overload(token)) return 1;
  }
  return 0;
}

static const int *cpp_saved_conditional_at(TokenString *expression,
                                           TokenString **yes, TokenString **no)
{
  int first, end, i, level = 0, nested = 0, question = -1;
  if (!expression || !expression->str) return NULL;
  first = tok_str_next_non_linenum_index(expression->str, expression->len, 0);
  end = tok_str_prev_token_index(expression->str, expression->len, expression->len);
  if (end >= 0 && expression->str[end] == TOK_EOF)
    end = tok_str_prev_token_index(expression->str, expression->len, end);
  while (first < end && expression->str[first] == '('
         && saved_initializer_close(expression, first, '(', ')') == end) {
    first = tok_str_next_non_linenum_index(expression->str, expression->len, first + 1);
    end = tok_str_prev_token_index(expression->str, expression->len, end);
  }
  for (i = first; i <= end; ++i) {
    int token = expression->str[i];
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(expression->str, expression->len, i);
      continue;
    }
    if (token == '(' || token == '[' || token == '{' || is_cpp_lambda_marker(token)) ++level;
    else if (token == ')' || token == ']' || token == '}') --level;
    else if (!level && (token == '?' || cpp_is_conditional_marker(token))) {
      if (question < 0) question = i;
      else ++nested;
    } else if (!level && token == ':') {
      int next = tok_str_next_non_linenum_index(expression->str, expression->len, i + 1);
      if (next <= end && expression->str[next] == ':') { i = next; continue; }
      if (nested) --nested;
      else if (question >= 0) {
        if (yes) *yes = cpp_token_slice(expression, question + 1, i);
        if (no) *no = cpp_token_slice(expression, i + 1, end + 1);
        return expression->str + question + 1;
      }
    }
  }
  return NULL;
}

static TokenString *cpp_save_conditional_arm(int first_arm)
{
  TokenString *arm = tok_str_alloc();
  int level = 0, nested = 0, angle = 0, previous = 0, before_previous = 0;
  while (tok != TOK_EOF) {
    int token = tok;
    if (!level && !angle) {
      if (token == ':') {
        next();
        if (tok == ':') {
          tok_str_add(arm, ':'); tok_str_add(arm, ':'); next();
          previous = ':';
          continue;
        }
        unget_tok(tok); tok = ':';
        if (!nested) break;
      }
      if (!nested && (!first_arm && (token == ',' || token == ';'
          || token == ')' || token == ']' || token == '}'))) break;
      if (token == '?' || cpp_is_conditional_marker(token)) ++nested;
      else if (token == ':' && nested) --nested;
    }
    tok_str_add_tok(arm); next();
    if ((token == TOK_LT || token == '<')
        && (angle || (previous >= TOK_UIDENT
            && (find_template_def(previous) || is_template_keyword_tok(before_previous))))) ++angle;
    else if ((token == TOK_SAR || token == TOK_SHR) && angle) angle = angle > 1 ? angle - 2 : 0;
    else if (token == TOK_GT && angle) --angle;
    else if (token == '(' || token == '[' || token == '{' || is_cpp_lambda_marker(token)) ++level;
    else if (token == ')' || token == ']' || token == '}') --level;
    if (token != TOK_LINENUM && !TOK_HAS_VALUE(token)) {
      before_previous = previous; previous = token;
    }
  }
  tok_str_add(arm, TOK_EOF);
  return arm;
}

static int cpp_probe_conditional_arm(TokenString *arm, CType *type)
{
  TokenString macro = *arm, *parent = macro_stack;
  const int *parent_ptr = macro_ptr;
  int saved_tok = tok, saved_nocode = nocode_wanted, result;
  int saved_loc = loc, saved_temp_count = nb_temp_local_vars;
  int value_count = (int)(vtop - vstack + 1);
  SValue saved_values[VSTACK_SIZE];
  struct temp_local_variable saved_temps[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
  CValue saved_tokc = tokc;
  SValue *saved_vtop = vtop;
  memcpy(saved_values, vstack, value_count * sizeof(SValue));
  memcpy(saved_temps, arr_temp_local_vars, sizeof(saved_temps));
  nocode_wanted++;
  begin_macro(&macro, 0); next(); gexpr();
  *type = vtop->type;
  result = (vtop->r & VT_THROW) ? 2
      : ((vtop->r & VT_CXX_PRVALUE) && (type->t & VT_BTYPE) == VT_STRUCT);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  vtop = saved_vtop; nocode_wanted = saved_nocode;
  memcpy(vstack, saved_values, value_count * sizeof(SValue));
  memcpy(arr_temp_local_vars, saved_temps, sizeof(saved_temps));
  loc = saved_loc; nb_temp_local_vars = saved_temp_count;
  tok = saved_tok; tokc = saved_tokc;
  return result;
}

static void cpp_emit_conditional_arm(TokenString *arm, CType *type,
                                      int storage, int address)
{
  CppConstructionDestination destination, *saved = cpp_construction_destination;
  TokenString macro = *arm, *parent = macro_stack;
  const int *parent_ptr = macro_ptr;
  int saved_tok = tok;
  int saved_nocode = nocode_wanted;
  CValue saved_tokc = tokc;
  destination.type = *type; destination.storage = storage;
  destination.address = address; destination.available = 1;
  destination.conditional_marker = cpp_prepare_conditional_destination(arm);
  cpp_construction_destination = &destination;
  begin_macro(&macro, 0); next(); gexpr();
  if (!nocode_wanted && destination.available && !(vtop->r & VT_THROW))
    cprime_internal_error("conditional prvalue did not consume construction destination");
  vpop();
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  cpp_construction_destination = saved;
  nocode_wanted = saved_nocode;
  tok = saved_tok; tokc = saved_tokc;
}

static int cpp_try_conditional_prvalue(void)
{
  TokenString *yes, *no, *replay;
  CType type, other;
  int question = tok;
  int address = 0, storage = VT_LOCAL | VT_LVAL, direct = 0;
  int size, align, r2 = VT_CONST, false_jump = 0, end_jump = 0, constant;
  int first_kind, second_kind;
  if (!is_cpp_translation_unit() || nocode_wanted) return 0;
  next();
  if (tok == ':') { unget_tok(tok); tok = question; return 0; }
  yes = cpp_save_conditional_arm(1);
  if (tok != ':') cprime_error("':' expected in conditional expression");
  if (!cpp_arm_may_produce_class(yes)) {
    replay = tok_str_alloc(); tok_str_add(replay, question);
    tok_str_append_without_eof(replay, yes); tok_str_free(yes);
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  next(); no = cpp_save_conditional_arm(0);
  first_kind = cpp_probe_conditional_arm(yes, &type);
  second_kind = first_kind ? cpp_probe_conditional_arm(no, &other) : 0;
  if (first_kind == 2 && second_kind == 1) type = other;
  if (!first_kind || !second_kind || (first_kind == 2 && second_kind == 2)
      || (first_kind == 1 && second_kind == 1
          && !is_compatible_unqualified_types(&type, &other))
      || (first_kind != 2 && !saved_initializer_is_direct_constructor(yes, &type))
      || (second_kind != 2 && !saved_initializer_is_direct_constructor(no, &type))) {
    replay = tok_str_alloc(); tok_str_add(replay, question);
    tok_str_append_without_eof(replay, yes); tok_str_add(replay, ':');
    tok_str_append_without_eof(replay, no);
    tok_str_free(yes); tok_str_free(no);
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  type.t &= ~(VT_STORAGE | VT_CONSTANT | VT_VOLATILE | VT_RVALUE_REFERENCE);
  if (cpp_construction_destination
      && cpp_construction_destination->conditional_marker == question) {
    cpp_construction_destination->conditional_marker = 0;
    direct = take_cpp_construction_destination(&type, &address, &storage);
  }
  constant = condition_3way();
  if (constant < 0) { save_regs(1); false_jump = gvtst(1, 0); }
  else vpop();
  size = type_size(&type, &align);
  if (!direct) address = get_temp_local_var(size, align, &r2);
  vset(&type, storage, address); vtop->r2 = r2;
  if (constant == 0) ++nocode_wanted;
  cpp_emit_conditional_arm(yes, &type, storage, address);
  if (constant == 0) --nocode_wanted;
  if (constant < 0) { end_jump = gjmp(0); gsym(false_jump); }
  if (constant == 1) ++nocode_wanted;
  cpp_emit_conditional_arm(no, &type, storage, address);
  if (constant == 1) --nocode_wanted;
  gsym(end_jump);
  if (!direct) cpp_register_class_temporary(vtop);
  tok_str_free(yes); tok_str_free(no);
  return 1;
}
