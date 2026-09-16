/* Keep constexpr bodies independently of emitted inline bodies.
   Evaluation uses the ordinary expression parser, preserving its constant
   checks and the function's declaration scope. */
typedef struct ConstexprFunction {
  int token, namespace_tok, owner_tok;
  int parameter_count;
  int *parameter_names;
  int *parameter_object_qualifiers;
  TokenString expression;
  struct ConstexprFunction *next;
} ConstexprFunction;
static ConstexprFunction *constexpr_functions;
static int constexpr_evaluation_depth;
static unsigned constexpr_evaluation_steps;
/* Discarding a call's value must not discard its evaluation failure. Nested
   evaluations save this state so an unsuccessful callee can fall back normally. */
static int constexpr_evaluation_invalid;
static void invalidate_constexpr_runtime_call(void)
{
  if (getenv("CPC_TRACE"))
    fprintf(stderr, "CPCDBG invalidate tok=%s depth=%d\n",
            TOK_HAS_VALUE(tok) ? "(value)" : get_tok_str(tok, NULL),
            constexpr_evaluation_depth);
  if (constexpr_evaluation_depth && constexpr_pointer_evaluation_active())
    constexpr_evaluation_invalid = 1;
}
/* A return value is initialized outside the callee's lexical scopes, then
   transferred to the caller after validating its stored references. */
static ConstexprTemporaryObject *constexpr_result_object;
static ConstexprTemporaryObject *constexpr_pending_result_objects;

static int constexpr_expression_value_valid(SValue *value)
{
  int storage = value->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST);
  return storage == VT_CONST || is_active_constexpr_temporary(value->sym)
      || (storage == (VT_CONST | VT_SYM)
          && (value->type.t & VT_BTYPE) == VT_PTR);
}

static int constexpr_result_fields_valid(SValue *object, CType *type, int offset,
                                        ConstexprTemporaryObject *mark)
{
  Sym *field;
  if (type->t & VT_ARRAY) {
    int i, align, size = type_size(&type->ref->type, &align);
    for (i = 0; i < type->ref->c; ++i)
      if (!constexpr_result_fields_valid(object, &type->ref->type, offset + i * size, mark)) return 0;
    return 1;
  }
  if ((type->t & VT_BTYPE) == VT_STRUCT) {
    if (!type->ref || IS_UNION(type->t)) return 0;
    for (field = type->ref->next; field; field = field->next)
      if ((field->v & SYM_FIELD) && !(field->type.t & VT_STATIC)
          && !constexpr_result_fields_valid(object, &field->type, offset + field->c, mark)) return 0;
    return 1;
  }
  {
    Sym scalar = {0}, *relocation;
    CValue value;
    ConstexprPointerBounds bounds;
    ConstexprTemporaryObject *temporary;
    scalar.type = *type;
    if (!constexpr_aggregate_field_value(object, &scalar, offset, &value, &relocation, &bounds)) return 0;
    for (temporary = constexpr_temporary_objects; temporary != mark; temporary = temporary->next)
      if (relocation == &temporary->object && relocation != object->sym) return 0;
  }
  return 1;
}

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
    entry->parameter_object_qualifiers = cprime_malloc(entry->parameter_count * sizeof(int));
    for (parameter = function->type.ref->next, i = 0; parameter; parameter = parameter->next)
    {
      entry->parameter_object_qualifiers[i] = parameter->parameter_object_qualifiers;
      entry->parameter_names[i++] = parameter->v & ~SYM_FIELD;
    }
  }
  entry->expression.len = expression->len;
  entry->expression.str = cprime_malloc(expression->len * sizeof(int));
  memcpy(entry->expression.str, expression->str, expression->len * sizeof(int));
  tok_str_free(expression);
  entry->next = constexpr_functions;
  constexpr_functions = entry;
  if (getenv("CPC_TRACE")) {
    int di;
    fprintf(stderr, "CPCDBG note_body %s params=%d owner=%d:", get_tok_str(function->v, NULL),
            entry->parameter_count, entry->owner_tok);
    for (di = 0; di < entry->expression.len && di < 48; ++di) {
      int token = entry->expression.str[di];
      if (TOK_HAS_VALUE(token)) {
        fprintf(stderr, " %lld", (long long)entry->expression.str[di + 1]);
        di += tok_str_value_extra_words(entry->expression.str, entry->expression.len, di);
      } else
        fprintf(stderr, " %s", get_tok_str(token, NULL));
    }
    fprintf(stderr, "\n");
  }
}

static void deduce_cpp_auto_initializer_type(CType *type, TokenString *initializer)
{
  CType inferred;
  int reference = type->t & (VT_REFERENCE | VT_RVALUE_REFERENCE);
  int cv = (reference && type->ref ? type->ref->type.t : type->t)
           & (VT_CONSTANT | VT_VOLATILE);
  int rvalue;
  infer_saved_arg_type(initializer, &inferred);
  rvalue = (inferred.t & VT_RVALUE_REFERENCE) != 0;
  if (is_reference_type(&inferred)) {
    decay_reference_type(&inferred);
    inferred = *pointed_type(&inferred);
  }
  inferred.t &= ~(VT_STORAGE | VT_RVALUE_REFERENCE);
  if (!reference) inferred.t &= ~(VT_CONSTANT | VT_VOLATILE);
  inferred.t |= cv;
  *type = inferred;
  if (reference) {
    mk_reference(type);
    if ((reference & VT_RVALUE_REFERENCE) && rvalue)
      type->t |= VT_RVALUE_REFERENCE;
  }
}

/* Conditions leave their value on the stack and their delimiter unconsumed.
   Ordinary local statements consume their semicolon and discard the value. */
static int evaluate_constexpr_local_statement(int condition)
{
    CType local_type;
    AttributeDef attributes = {0};
    Sym *binding;
    int name = 0, initializer_close = 0, copy_initializer;
    int declaration = parse_btype(&local_type, &attributes, 0);
    int auto_type = last_decl_was_auto;
    if (declaration && !auto_type && !(local_type.t & VT_STORAGE)
        && cpp_local_type_starts_expression(&local_type, condition != 0)) {
      unget_tok(template_exact_ctype_typedef_tok(&local_type));
      declaration = 0;
    }
    if (!declaration) {
      gexpr();
      if (condition) return tok == condition ? 0 : -1;
      if (tok != ';') return -1;
      constexpr_read_local_value(vtop);
      if (!constexpr_expression_value_valid(vtop))
        return -1;
      vpop();
      next();
      return 0;
    }
    CType base_type = local_type;
    AttributeDef base_attributes = attributes;
    for (;;) {
    local_type = base_type;
    attributes = base_attributes;
    name = initializer_close = 0;
    type_decl(&local_type, &attributes, &name, TYPE_DIRECT | TYPE_LOCAL_CTOR_INIT);
    if (!name) return -1;
    /* The point of declaration precedes the initializer. Keep an unreadable
       local binding until initialization has produced its value/object. */
    binding = sym_push(name, &local_type, VT_LOCAL | VT_LVAL, 0);
    if (auto_type) {
      TokenString *initializer = NULL;
      if (tok != '=') return -1;
      next();
      skip_or_save_block(&initializer);
      deduce_cpp_auto_initializer_type(&local_type, initializer);
      binding->type = local_type;
      --initializer->len;
      restore_cpp_lifecycle_probe(initializer);
      unget_tok('=');
    }
    copy_initializer = tok == '=';
    if (copy_initializer) next();
    else if (tok != '(' && tok != '{')
      return -1;
    if ((local_type.t & VT_BTYPE) != VT_STRUCT && !(local_type.t & VT_ARRAY)
        && ((!copy_initializer && tok == '(') || tok == '{')) {
      initializer_close = tok == '(' ? ')' : '}';
      next();
    }
    if ((local_type.t & VT_BTYPE) == VT_STRUCT || (local_type.t & VT_ARRAY)) {
      if (copy_initializer && tok != '{' && !(local_type.t & VT_ARRAY)) {
        TokenString *expression = NULL, *list;
        CType source_type;
        skip_or_save_block(&expression);
        infer_saved_arg_type(expression, &source_type);
        if (is_reference_type(&source_type)) source_type = *pointed_type(&source_type);
        if (!is_compatible_unqualified_types(&local_type, &source_type)
            && !find_cpp_member_pointer_type(&local_type)) {
          tok_str_free(expression);
          return -1;
        }
        /* Same-type aggregate list copying already preserves object fields,
           relocations and initialization state. Reuse it for copy syntax. */
        list = tok_str_alloc();
        tok_str_add(list, '{');
        tok_str_append_without_eof(list, expression);
        tok_str_add(list, '}');
        tok_str_free(expression);
        restore_cpp_lifecycle_probe(list);
      }
      if (type_requires_class_construction(&local_type)) {
        /* A class-type local is built by its own constructor, which the
           evaluation runs against an evaluation-owned object. */
        if (tok != '{' && tok != '(') return -1;
        if (!constexpr_evaluated_object_type(&local_type)) {
          if (getenv("CPC_TRACE")) fprintf(stderr, "CPCDBG local bail object_type\n");
          return -1;
        }
      } else if (tok != '{' || resolve_autodtor_func(&local_type)) return -1;
      TokenString *initializer = NULL;
      int object_align;
      if (!type_requires_class_construction(&local_type)
          && !constexpr_aggregate_temporary_type(&local_type)) return -1;
      if (type_size(&local_type, &object_align) < 0) {
        init_params sizing = {0};
        if (local_type.t & VT_ARRAY) sizing.flex_array_ref = local_type.ref;
        skip_or_save_block(&initializer);
        unget_tok(0);
        begin_macro(initializer, 1);
        next();
        decl_initializer(&sizing, &local_type, 0, DIF_FIRST | DIF_SIZE_ONLY);
        macro_ptr = initializer->str;
        next();
      }
      ++constexpr_temporary_expression_depth;
      materialize_braced_temporary_mode(&local_type, 0, binding);
      --constexpr_temporary_expression_depth;
      if (!is_active_constexpr_temporary(vtop->sym)) {
        if (getenv("CPC_TRACE")) fprintf(stderr, "CPCDBG local bail active r=%x sym=%p\n",
                                          (unsigned)vtop->r, (void *)vtop->sym);
        return -1;
      }
      /* A local binding refers to the evaluation-owned object. It must not
         acquire static storage or outlive this function evaluation. */
      {
        CType reference = local_type;
        Sym *object = vtop->sym;
        long long offset = vtop->c.i;
        mk_pointer(&reference);
        reference.t |= VT_REFERENCE;
        binding->type = reference;
        binding->r = VT_CONST;
        binding->constexpr_address_target = object;
        binding->const_value = offset;
        vpop();
      }
      if (initializer) { end_macro(); next(); }
    } else if (is_reference_type(&local_type)) {
      CType address_type = local_type;
      int storage;
      ++suppress_integral_constexpr_fold;
      expr_eq();
      --suppress_integral_constexpr_fold;
      if (initializer_close) {
        if (initializer_close == '}' && tok == ',') next();
        skip(initializer_close);
      }
      check_reference_binding_category(&local_type);
      storage = vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST);
      /* Array expressions carry their address without the scalar lvalue bit. */
      if (vtop->type.t & VT_ARRAY) storage |= VT_LVAL;
      if (storage != (VT_CONST | VT_SYM | VT_LVAL)
          && !(storage == (VT_LOCAL | VT_LVAL)
               && is_active_constexpr_temporary(vtop->sym))) return -1;
      mk_pointer(&vtop->type);
      gaddrof();
      address_type.t &= ~(VT_REFERENCE | VT_RVALUE_REFERENCE);
      gen_assign_cast(&address_type);
      binding->type = local_type;
      binding->r = VT_CONST;
      binding->c = vtop->c.i;
      binding->type.t |= VT_CONSTANT;
      binding->constexpr_address_target = vtop->sym;
      binding->constexpr_bounds = vtop->constexpr_bounds;
      binding->const_value = vtop->c.i;
      vpop();
    } else {
      int object_size, object_align;
      ConstexprTemporaryObject *object;
      ConstexprLocalField *field;
      CType reference = local_type;
      object_size = type_size(&local_type, &object_align);
      if (object_size <= 0 || (local_type.t & VT_VOLATILE)) return -1;
      object = cprime_mallocz(sizeof(*object));
      loc = (loc - object_size) & -object_align;
      object->object.type = local_type;
      object->object.r = VT_LOCAL | VT_LVAL;
      object->object.c = loc;
      object->next = constexpr_temporary_objects;
      constexpr_temporary_objects = object;
      mk_pointer(&reference);
      reference.t |= VT_REFERENCE;
      binding->type = reference;
      binding->r = VT_CONST;
      binding->constexpr_address_target = &object->object;
      binding->const_value = loc;
     if (initializer_close == '}' && tok == '}')
        vset(&local_type, VT_CONST, 0);
      else
        expr_eq();
      if (initializer_close) {
        if (initializer_close == '}') {
          check_scalar_list_initialization(&local_type, vtop);
          if (tok == ',') next();
        }
        skip(initializer_close);
      }
      constexpr_read_local_value(vtop);
      if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST
          && !((local_type.t & VT_BTYPE) == VT_PTR
               && (vtop->type.t & VT_BTYPE) == VT_PTR
               && ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST))
                     == (VT_CONST | VT_SYM)
                   || ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST))
                       == VT_LOCAL && is_active_constexpr_temporary(vtop->sym)))))
        return -1;
      gen_assign_cast(&local_type);
      field = cprime_mallocz(sizeof(*field));
      field->object = &object->object;
      field->type = local_type;
      field->value = vtop->c;
      field->relocation = ((vtop->r & VT_SYM) || is_active_constexpr_temporary(vtop->sym)) ? vtop->sym : NULL;
      field->pointer_bounds = vtop->constexpr_bounds;
      field->next = constexpr_local_fields[constexpr_local_bucket(&object->object)];
      constexpr_local_fields[constexpr_local_bucket(&object->object)] = field;
      vpop();
    }
    if (condition || tok != ',') break;
    next();
    }
    if (condition) {
      TokenString *read;
      if (tok != condition) return -1;
      read = tok_str_alloc();
      tok_str_add(read, name);
      restore_cpp_lifecycle_probe(read);
      unary();
      return 0;
    }
    if (tok != ';') return -1;
    next();
  return 0;
}

/* Statement results: -1 cannot evaluate, 0 fallthrough, 1 returned,
   2 break, 3 continue. Enclosing loops consume their own transfers. */
static int evaluate_constexpr_statement(Sym *function, SValue *result)
{
  int valid;
  if (++constexpr_evaluation_steps > 262144)
    cprime_error("constexpr evaluation statement limit exceeded");
  if (tok == TOK_WHILE || tok == TOK_DO || tok == TOK_FOR) {
    TokenString *condition = tok_str_alloc(), *body = tok_str_alloc();
    TokenString *increment = NULL;
    Sym *scope_mark = local_stack;
    ConstexprTemporaryObject *object_mark = constexpr_temporary_objects, *object;
    TokenString *parent;
    const int *parent_ptr;
    int saved_tok, status = 0, repeat;
    int body_first = tok == TOK_DO;
    int do_loop = body_first;
    int for_loop = tok == TOK_FOR;
    CValue saved_tokc;
    next();
    if (for_loop) {
      TokenString *lowered;
      skip('(');
      lowered = lower_range_for();
      if (lowered) {
        saved_tok = tok; saved_tokc = tokc;
        parent = macro_stack; parent_ptr = macro_ptr;
        begin_macro(lowered, 1); next();
        status = evaluate_constexpr_statement(function, result);
        while (macro_stack && macro_stack != parent) end_macro();
        macro_ptr = parent_ptr;
        tok = saved_tok; tokc = saved_tokc;
        tok_str_free(condition); tok_str_free(body);
        return status;
      }
      ++local_scope;
      if (evaluate_constexpr_statement(function, result) != 0) {
        tok_str_free(condition); tok_str_free(body);
        sym_pop(&local_stack, scope_mark, 0);
        --local_scope;
        return -1;
      }
      if (tok == ';') {
        tok_str_add(condition, TOK_CINT); tok_str_add(condition, 1);
        tok_str_add(condition, ';'); next();
      }
      else skip_or_save_statement_tokens(condition, 1);
      increment = tok_str_alloc();
      while (tok != ')' && tok != TOK_EOF) {
        if (tok == '(' || tok == '[' || tok == '{') skip_or_save_balanced_tokens(increment);
        else { tok_str_add_tok(increment); next(); }
      }
      skip(')');
      tok_str_add(increment, TOK_EOF);
    } else if (!body_first) {
      if (tok != '(') expect("(");
      skip_or_save_balanced_tokens(condition);
    }
    tok_str_add(body, '{');
    skip_or_save_statement_tokens(body, 1);
    tok_str_add(body, '}'); tok_str_add(body, TOK_EOF);
    if (body_first) {
      skip(TOK_WHILE);
      if (tok != '(') expect("(");
      skip_or_save_balanced_tokens(condition);
      skip(';');
    }
    tok_str_add(condition, TOK_EOF);
    saved_tok = tok; saved_tokc = tokc;
    parent = macro_stack; parent_ptr = macro_ptr;
    for (;;) {
      Sym *iteration_scope = local_stack;
      ConstexprTemporaryObject *iteration_objects = constexpr_temporary_objects;
      int stop = 0;
      ++local_scope;
      if (!body_first) {
        begin_macro(condition, 3); next();
        if (do_loop) gexpr();
        else {
          if (!for_loop) skip('(');
          if (evaluate_constexpr_local_statement(for_loop ? ';' : ')') != 0) {
            status = -1;
            stop = 1;
            goto loop_iteration_finished;
          }
        }
        constexpr_read_local_value(vtop);
        gen_cast_s(VT_BOOL);
        repeat = 0;
        if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
          status = -1;
        else repeat = vtop->c.i != 0;
        vpop();
        while (macro_stack && macro_stack != parent) end_macro();
        macro_ptr = parent_ptr;
        if (status || !repeat) { stop = 1; goto loop_iteration_finished; }
        }
      body_first = 0;
      begin_macro(body, 3); next();
      status = evaluate_constexpr_statement(function, result);
      while (macro_stack && macro_stack != parent) end_macro();
      macro_ptr = parent_ptr;
      if (status == 2) { status = 0; stop = 1; goto loop_iteration_finished; }
      if (status == 3) status = 0;
      if (status) { stop = 1; goto loop_iteration_finished; }
      if (increment) {
        begin_macro(increment, 3); next();
        if (tok != TOK_EOF) {
          gexpr();
          constexpr_read_local_value(vtop);
          if (!constexpr_expression_value_valid(vtop)) status = -1;
          vpop();
        }
        while (macro_stack && macro_stack != parent) end_macro();
        macro_ptr = parent_ptr;
        if (status) stop = 1;
      }
loop_iteration_finished:
      while (macro_stack && macro_stack != parent) end_macro();
      macro_ptr = parent_ptr;
      for (object = constexpr_temporary_objects; object != iteration_objects; object = object->next) {
        if (status == 1 && result->sym == &object->object) status = -1;
        object->expired = 1;
        free_constexpr_local_fields(&object->object);
      }
      sym_pop(&local_stack, iteration_scope, 0);
      --local_scope;
      if (stop) break;
    }
    tok = saved_tok; tokc = saved_tokc;
    tok_str_free(condition); tok_str_free(body);
    if (increment) tok_str_free(increment);
    if (for_loop) {
      for (object = constexpr_temporary_objects; object != object_mark; object = object->next) {
        if (status == 1 && result->sym == &object->object) status = -1;
        object->expired = 1;
        free_constexpr_local_fields(&object->object);
      }
      sym_pop(&local_stack, scope_mark, 0);
      --local_scope;
    }
    return status;
  }
  if (tok == '{') {
    Sym *scope_mark = local_stack;
    ConstexprTemporaryObject *object_mark = constexpr_temporary_objects, *object;
    int status = 0;
    ++local_scope;
    next();
    while (tok != '}' && tok != TOK_EOF && status == 0)
      status = evaluate_constexpr_statement(function, result);
    if (!status) skip('}');
    for (object = constexpr_temporary_objects; object != object_mark; object = object->next) {
      if (status == 1 && result->sym == &object->object) status = -1;
      object->expired = 1;
      free_constexpr_local_fields(&object->object);
    }
    sym_pop(&local_stack, scope_mark, 0);
    --local_scope;
    return status;
  }
  if (tok == TOK_IF) {
    TokenString *yes = tok_str_alloc(), *no = tok_str_alloc(), *selected;
    TokenString *header = tok_str_alloc();
    Sym *scope_mark = local_stack;
    ConstexprTemporaryObject *object_mark = constexpr_temporary_objects, *object;
    TokenString *parent;
    const int *parent_ptr;
    int selected_true, saved_tok, status = 0;
    int has_initializer = 0;
    CValue saved_tokc;
    tok_str_add(yes, '{'); tok_str_add(no, '{');
    next();
    skip('(');
    while (tok != ')' && tok != TOK_EOF) {
      if (tok == '(' || tok == '[' || tok == '{') skip_or_save_balanced_tokens(header);
      else {
        if (tok == ';') has_initializer = 1;
        tok_str_add_tok(header); next();
      }
    }
    tok_str_add_tok(header);
    skip(')');
    restore_cpp_lifecycle_probe(header);
    ++local_scope;
    if (has_initializer && evaluate_constexpr_statement(function, result) != 0) {
      status = -1;
      goto if_finished;
    }
    if (evaluate_constexpr_local_statement(')') != 0) {
      status = -1;
      goto if_finished;
    }
    constexpr_read_local_value(vtop);
    gen_cast_s(VT_BOOL);
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST) {
      status = -1;
      vpop();
      goto if_finished;
    }
    selected_true = vtop->c.i != 0;
    vpop();
    skip(')');
    skip_or_save_statement_tokens(yes, 1);
    if (tok == TOK_ELSE) { next(); skip_or_save_statement_tokens(no, 1); }
    tok_str_add(yes, '}'); tok_str_add(no, '}');
    tok_str_add(yes, TOK_EOF); tok_str_add(no, TOK_EOF);
    selected = selected_true ? yes : no;
    saved_tok = tok; saved_tokc = tokc;
    parent = macro_stack; parent_ptr = macro_ptr;
    begin_macro(selected, 3); next();
    if (tok != TOK_EOF) status = evaluate_constexpr_statement(function, result);
    while (macro_stack && macro_stack != parent) end_macro();
    macro_ptr = parent_ptr; tok = saved_tok; tokc = saved_tokc;
if_finished:
    for (object = constexpr_temporary_objects; object != object_mark; object = object->next) {
      if (status == 1 && result->sym == &object->object) status = -1;
      object->expired = 1;
      free_constexpr_local_fields(&object->object);
    }
    sym_pop(&local_stack, scope_mark, 0);
    --local_scope;
    tok_str_free(yes); tok_str_free(no);
    return status;
  }
  if (tok == ';') { next(); return 0; }
  /* A constructor's vptr-establishment marker has no value of its own; the
     evaluated object carries its class identity already. */
  if (tok == TOK_CXX_CTOR_BODY) { next(); skip(';'); return 0; }
  /* A constructed-subobject marker only records lifetime for generated code;
     the evaluation-owned object already holds the subobject's values. */
  if (tok == TOK_CXX_EH_CONSTRUCTED) {
    int depth = 0;
    next();
    if (tok == '(') {
      do {
        if (tok == '(') ++depth;
        else if (tok == ')') --depth;
        next();
      } while (tok != TOK_EOF && depth > 0);
    }
    if (tok == ';') next();
    return 0;
  }
  if (tok == TOK_CXX_BASE_SUBOBJECT) { next(); return 0; }
  if (tok == TOK_BREAK || tok == TOK_CONTINUE) {
    int status = tok == TOK_BREAK ? 2 : 3;
    next(); skip(';'); return status;
  }
  if (tok == TOK_RETURN) {
  next();
  if (tok == ';') {
    if ((function->type.ref->type.t & VT_BTYPE) != VT_VOID) return -1;
    memset(result, 0, sizeof(*result));
    result->type = function->type.ref->type;
    result->r = VT_CONST;
    next();
    return 1;
  }
  if (is_reference_type(&function->type.ref->type)) ++suppress_integral_constexpr_fold;
  if (constexpr_result_object) ++constexpr_temporary_expression_depth;
  gexpr();
  if (constexpr_result_object) --constexpr_temporary_expression_depth;
  if (is_reference_type(&function->type.ref->type)) --suppress_integral_constexpr_fold;
  else constexpr_read_local_value(vtop);
  if (constexpr_result_object
      && (function->type.ref->type.t & VT_BTYPE) == VT_STRUCT) {
    CType *target = &function->type.ref->type;
    init_params initialization = {0};
    int align, size = type_size(target, &align);
    if (!is_compatible_unqualified_types(target, &vtop->type)
        || !cpp_type_trivially_copyable(target)) return -1;
    initialization.constant_object_target = &constexpr_result_object->object;
    initialization.constant_object_offset = constexpr_result_object->object.c;
    initialization.local_offset = constexpr_result_object->object.c + size;
    initialization.require_constant_values = 1;
    initialization.constexpr_initializer = 1;
    init_putv(&initialization, target, constexpr_result_object->object.c);
    vset(target, VT_LOCAL | VT_LVAL | VT_CXX_PRVALUE, constexpr_result_object->object.c);
    vtop->sym = &constexpr_result_object->object;
    *result = *vtop;
    vpop();
    skip(';');
    return 1;
  }
  valid = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_CONST;
  if (is_reference_type(&function->type.ref->type)) {
    CType target = function->type.ref->type;
    int storage = vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST);
    valid = storage == (VT_CONST | VT_SYM | VT_LVAL)
        || (storage == (VT_LOCAL | VT_LVAL) && is_active_constexpr_temporary(vtop->sym));
    if (valid) {
      check_reference_binding_category(&target);
      mk_pointer(&vtop->type);
      gaddrof();
      target.t &= ~(VT_REFERENCE | VT_RVALUE_REFERENCE);
      gen_assign_cast(&target);
    }
  }
  if ((function->type.ref->type.t & VT_BTYPE) == VT_PTR
      && !is_reference_type(&function->type.ref->type)
      && (vtop->type.t & VT_BTYPE) == VT_PTR
      && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == (VT_CONST | VT_SYM)
      && vtop->sym && !is_active_constexpr_temporary(vtop->sym))
    valid = 1;
  if ((function->type.ref->type.t & VT_BTYPE) == VT_PTR
      && !is_reference_type(&function->type.ref->type)
      && (vtop->type.t & VT_BTYPE) == VT_PTR
      && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_LOCAL
      && is_active_constexpr_temporary(vtop->sym))
    valid = 1;
  if (valid) {
    gen_cast(&function->type.ref->type);
    *result = *vtop;
  }
    if (getenv("CPC_TRACE"))
      fprintf(stderr, "CPCDBG ret %s valid=%d r=%x type=%x sym=%p\n",
              get_tok_str(function->v, NULL), valid,
              (unsigned)vtop->r, (unsigned)vtop->type.t, (void *)vtop->sym);
    if (!valid) return -1;
    vpop();
    skip(';');
    return 1;
  }
  return evaluate_constexpr_local_statement(0);
}

static int evaluate_constexpr_function(Sym *function, SValue *arguments, int count, SValue *result)
{
  ConstexprFunction *entry;
  TokenString replay, *parent;
  const int *parent_ptr;
  int saved_tok, saved_namespace, saved_owner, valid, saved_nocode;
  int saved_invalid;
  CValue saved_tokc;
  Sym *saved_locals;
  Sym *parameter;
  Sym **bindings = NULL;
  int saved_scope, binding_count = 0, argument, saved_loc, saved_temp_count;
  struct temp_local_variable saved_temps[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
  SValue *saved_top = vtop;
  ConstexprTemporaryObject *temporary_mark = constexpr_temporary_objects;
  ConstexprTemporaryObject *saved_result_object, *returned_object = NULL;
  for (argument = 0; argument < count; ++argument)
    if ((arguments[argument].r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST
        && !((arguments[argument].type.t & VT_BTYPE) == VT_PTR
             && (arguments[argument].r & VT_VALMASK) == VT_CONST
             && (arguments[argument].r & VT_SYM)
             && !(arguments[argument].r & VT_NONCONST))
        && !((arguments[argument].type.t & VT_BTYPE) == VT_PTR
             && (arguments[argument].r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_LOCAL
             && is_active_constexpr_temporary(arguments[argument].sym))
        && !((arguments[argument].type.t & VT_BTYPE) == VT_STRUCT
             && (((arguments[argument].r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == (VT_LOCAL | VT_LVAL)
                  && is_active_constexpr_temporary(arguments[argument].sym))
                 || ((arguments[argument].r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == (VT_CONST | VT_SYM | VT_LVAL)
                     && arguments[argument].sym && arguments[argument].sym->cpp_constexpr_object))
            && constexpr_aggregate_temporary_type(&arguments[argument].type)))
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
  if (!entry || entry->parameter_count != count) {
    if (getenv("CPC_TRACE")) fprintf(stderr, "CPCDBG eval_nofunc %s count=%d\n", get_tok_str(function->v, NULL), count);
    return 0;
  }
  if (!constexpr_evaluation_depth) constexpr_evaluation_steps = 0;
  if (++constexpr_evaluation_depth > 128) cprime_error("constexpr evaluation depth exceeded");
  saved_invalid = constexpr_evaluation_invalid;
  constexpr_evaluation_invalid = 0;
  saved_tok = tok; saved_tokc = tokc;
  parent = macro_stack; parent_ptr = macro_ptr;
  saved_namespace = current_namespace_scope_tok();
  saved_owner = active_member_class_tok;
  saved_nocode = nocode_wanted;
  saved_loc = loc;
  saved_temp_count = nb_temp_local_vars;
  memcpy(saved_temps, arr_temp_local_vars, sizeof(saved_temps));
  saved_result_object = constexpr_result_object;
  constexpr_result_object = NULL;
  if ((function->type.ref->type.t & VT_BTYPE) == VT_STRUCT
      && constexpr_aggregate_temporary_type(&function->type.ref->type)) {
    int align, size = type_size(&function->type.ref->type, &align);
    returned_object = cprime_mallocz(sizeof(*returned_object));
    returned_object->pending_result_next = constexpr_pending_result_objects;
    constexpr_pending_result_objects = returned_object;
    returned_object->object.type = function->type.ref->type;
    returned_object->object.r = VT_LOCAL | VT_LVAL;
    loc = (loc - size) & -align;
    returned_object->object.c = loc;
    constexpr_result_object = returned_object;
  }
  saved_locals = local_stack;
  saved_scope = local_scope;
  if (saved_locals) bindings = sym_hide_local_bindings(saved_locals, &binding_count);
  local_stack = NULL;
  sym_push2(&local_stack, SYM_FIELD, 0, 0);
  local_scope = 1;
  parameter = function->type.ref->next;
  for (argument = 0; argument < count; ++argument, parameter = parameter->next) {
    Sym *binding = sym_push(entry->parameter_names[argument], &parameter->type,
                            VT_CONST, arguments[argument].c.i);
    if ((parameter->type.t & VT_BTYPE) == VT_STRUCT
        && !is_reference_type(&parameter->type)) {
      CType reference = parameter->type;
      ConstexprTemporaryObject *object = cprime_mallocz(sizeof(*object));
      init_params initialization = {0};
      int align, size = type_size(&parameter->type, &align);
      loc = (loc - size) & -align;
      object->object.type = parameter->type;
      object->object.type.t |= entry->parameter_object_qualifiers[argument];
      object->object.r = VT_LOCAL | VT_LVAL;
      object->object.c = loc;
      object->next = constexpr_temporary_objects;
      constexpr_temporary_objects = object;
      initialization.constant_object_target = &object->object;
      initialization.constant_object_offset = loc;
      initialization.local_offset = loc + size;
      initialization.require_constant_values = 1;
      initialization.constexpr_initializer = 1;
      vpushv(&arguments[argument]);
      init_putv(&initialization, &parameter->type, loc);
      reference.t |= entry->parameter_object_qualifiers[argument];
      mk_pointer(&reference);
      reference.t |= VT_REFERENCE;
      binding->type = reference;
      binding->constexpr_address_target = &object->object;
      binding->const_value = loc;
      continue;
    }
    if (!is_reference_type(&parameter->type)
        /* The implicit this expression is a pointer prvalue, not a writable
           parameter object. Keep its receiver binding representation. */
        && !(entry->owner_tok
             && (entry->parameter_names[argument] == CPP_TOK_THIS
                 || !strncmp(get_tok_str(entry->parameter_names[argument], NULL),
                             "__cprime_this_", 14)))
        && !(parameter->type.t & VT_ARRAY)
        && (parameter->type.t & VT_BTYPE) != VT_STRUCT) {
      CType reference = parameter->type;
      reference.t |= entry->parameter_object_qualifiers[argument];
      ConstexprTemporaryObject *object = cprime_mallocz(sizeof(*object));
      ConstexprLocalField *field = cprime_mallocz(sizeof(*field));
      int align, size = type_size(&parameter->type, &align);
      loc = (loc - size) & -align;
      object->object.type = parameter->type;
      object->object.type.t |= entry->parameter_object_qualifiers[argument];
      object->object.r = VT_LOCAL | VT_LVAL;
      object->object.c = loc;
      object->next = constexpr_temporary_objects;
      constexpr_temporary_objects = object;
      mk_pointer(&reference);
      reference.t |= VT_REFERENCE;
      binding->type = reference;
      binding->constexpr_address_target = &object->object;
      binding->const_value = loc;
      field->object = &object->object;
      field->type = parameter->type;
      field->value = arguments[argument].c;
      field->relocation = ((arguments[argument].r & VT_SYM)
                           || is_active_constexpr_temporary(arguments[argument].sym))
                            ? arguments[argument].sym : NULL;
      field->pointer_bounds = arguments[argument].constexpr_bounds;
      field->next = constexpr_local_fields[constexpr_local_bucket(&object->object)];
      constexpr_local_fields[constexpr_local_bucket(&object->object)] = field;
      continue;
    }
    binding->type.t |= VT_CONSTANT;
    binding->constexpr_bounds = arguments[argument].constexpr_bounds;
    if ((arguments[argument].r & VT_SYM)
        || is_active_constexpr_temporary(arguments[argument].sym))
      binding->constexpr_address_target = arguments[argument].sym;
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
  valid = evaluate_constexpr_statement(function, result);
  if (getenv("CPC_TRACE")) fprintf(stderr, "CPCDBG eval_done %s valid=%d invalid=%d\n",
                                   get_tok_str(function->v, NULL), valid,
                                   constexpr_evaluation_invalid);
  if (valid == 0 && (function->type.ref->type.t & VT_BTYPE) == VT_VOID) {
    memset(result, 0, sizeof(*result));
    result->type = function->type.ref->type;
    result->r = VT_CONST;
    valid = 1;
  }
  valid = valid == 1;
  if (constexpr_evaluation_invalid) valid = 0;
  constexpr_evaluation_invalid = saved_invalid;
  if (valid) {
    ConstexprTemporaryObject *temporary;
    for (temporary = constexpr_temporary_objects; temporary != temporary_mark;
         temporary = temporary->next)
      if (result->sym == &temporary->object) valid = 0;
  }
  if (valid && returned_object) {
    returned_object->next = constexpr_temporary_objects;
    constexpr_temporary_objects = returned_object;
    valid = constexpr_result_fields_valid(result, &result->type, 0, temporary_mark);
    constexpr_temporary_objects = returned_object->next;
  }
  release_constexpr_temporary_objects(temporary_mark, 1);
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
  loc = saved_loc;
  nb_temp_local_vars = saved_temp_count;
  memcpy(arr_temp_local_vars, saved_temps, sizeof(saved_temps));
  constexpr_result_object = saved_result_object;
  if (returned_object) {
    constexpr_pending_result_objects = returned_object->pending_result_next;
    if (valid) {
      int align, size = type_size(&returned_object->object.type, &align);
      int temporary_slot;
      returned_object->object.c = get_temp_local_var(size, align, &temporary_slot);
      result->r2 = (unsigned short)temporary_slot;
      result->c.i = returned_object->object.c;
      returned_object->next = constexpr_temporary_objects;
      constexpr_temporary_objects = returned_object;
    } else {
      free_constexpr_local_fields(&returned_object->object);
      cprime_free(returned_object);
    }
  }
  --constexpr_evaluation_depth;
  return valid;
}

static void free_constexpr_functions(void)
{
  while (constexpr_pending_result_objects) {
    ConstexprTemporaryObject *object = constexpr_pending_result_objects;
    constexpr_pending_result_objects = object->pending_result_next;
    free_constexpr_local_fields(&object->object);
    cprime_free(object);
  }
  constexpr_result_object = NULL;
  while (constexpr_functions) {
    ConstexprFunction *entry = constexpr_functions;
    constexpr_functions = entry->next;
    cprime_free(entry->expression.str);
    cprime_free(entry->parameter_names);
    cprime_free(entry->parameter_object_qualifiers);
    cprime_free(entry);
  }
  constexpr_evaluation_depth = 0;
  constexpr_evaluation_steps = 0;
  constexpr_evaluation_invalid = 0;
}
