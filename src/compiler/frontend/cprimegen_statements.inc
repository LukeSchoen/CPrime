static void try_call_scope_cleanup(Sym *stop)
{
  Sym *cls = cur_scope->cl.s;
  for (; cls != stop; cls = cls->next)
  {
    Sym *fs = cls->cleanup_func;
    Sym *vs = cls->cleanup_sym;
    int temporary_skip;
    SValue *saved_top = vtop;
    if (!fs || !vs || cls->a.cleanup_exception_only)
      continue;
    if (cpp_cleanup_is_elided_return_object(cls))
      continue;
    temporary_skip = cpp_temp_guard_cleanup(cls);
    cpp_eh_set_cleanup_state(cls->next);
    if (vs && (vs->type.t & VT_BTYPE) == VT_STRUCT)
    {
      int struct_tok = get_struct_type_name_tok(&vs->type);
      if (struct_tok
          && class_or_inst_has_member_template_name(struct_tok,
                                                    TOK_DESTRUCTOR1))
        instantiate_template_member_for_call(&vs->type, TOK_DESTRUCTOR1,
                                             NULL, 0);
    }
    save_lvalues();
    vpushsym(&fs->type, fs);
    vset(&vs->type, vs->r, vs->c);
    vtop->sym = vs;
    mk_pointer(&vtop->type);
    gaddrof();
    gfunc_call(1);
    drop_leaked_call_target(fs);
    if (vtop > saved_top && (vtop->type.t & VT_BTYPE) == VT_VOID)
      vpop();
    gsym(temporary_skip);
  }
  cpp_eh_restore_cleanup_state();
}


static void try_call_cleanup_goto(Sym *cleanupstate)
{
  Sym *oc, *cc;
  int ocd, ccd;

  if (!cur_scope->cl.s)
    return;

  // search NCA of both cleanup chains given parents and initial depth
  ocd = cleanupstate ? cleanupstate->v & ~SYM_FIELD : 0;
  for (ccd = cur_scope->cl.n, oc = cleanupstate; ocd > ccd; --ocd, oc = oc->next)
    ;
  for (cc = cur_scope->cl.s; ccd > ocd; --ccd, cc = cc->next)
    ;
  for (; cc != oc; cc = cc->next, oc = oc->next, --ccd)
    ;

  try_call_scope_cleanup(cc);
}

// Call 'Func' For Each __Attribute__((Cleanup(Func)))
static void block_cleanup(struct scope *o)
{
  int jmp = 0;
  Sym *g, **pg;
  for (pg = &pending_gotos; (g = *pg) && g->c > o->cl.n;)
  {
    if (g->cleanup_label->r & LABEL_FORWARD)
    {
      Sym *pcl = g->next;
      if (!jmp)
        jmp = gjmp(0);
      gsym(pcl->jnext);
      try_call_scope_cleanup(o->cl.s);
      pcl->jnext = gjmp(0);
      if (!o->cl.n)
        goto remove_pending;
      g->c = o->cl.n;
      pg = &g->prev;
    }
    else
    {
remove_pending:
      *pg = g->prev;
      sym_free(g);
    }
  }
  gsym(jmp);
  try_call_scope_cleanup(o->cl.s);
}

// -------------------------------------------------------------------------
// VLA

static void vla_restore(int loc)
{
  if (loc)
    gen_vla_sp_restore(loc);
}

static void vla_leave(struct scope *o)
{
  struct scope *c = cur_scope, *v = NULL;
  for (; c != o && c; c = c->prev)
    if (c->vla.num)
      v = c;
  if (v)
    vla_restore(v->vla.locorig);
}

// -------------------------------------------------------------------------
// Local Scopes

static void new_scope(struct scope *o)
{
  // Copy And Link Previous Scope
  *o = *cur_scope;
  o->prev = cur_scope;
  cur_scope = o;
  cur_scope->vla.num = 0;

  // Record Local Declaration Stack Position
  o->lstk = local_stack;
  o->llstk = local_label_stack;
  o->stack_loc = loc;
  o->stack_min = loc;
  o->using_namespace_count = nb_using_namespace_toks;
  ++local_scope;
}

static void prev_scope(struct scope *o, int is_expr)
{
  int keep_local_member_types = 0;

  vla_leave(o->prev);

  if (o->cl.s != o->prev->cl.s)
    block_cleanup(o->prev);

  if (debug_modes)
    cprime_debug_end_scope(o->lstk, !is_expr);

  // Pop Locally Defined Labels
  label_pop(&local_label_stack, o->llstk, is_expr);

  /* In the is_expr case (a statement expression is finished here),
     vtop might refer to symbols on the local_stack.  Either via the
     type or via vtop->sym.  We can't pop those nor any that in turn
     might be referred to.  To make it easier we don't roll back
     any symbols in that case; some upper level call to block() will
     do that.  We do have to remove such symbols from the lookup
     tables, though.  sym_pop will do that.  */

  /* A local class's lowered member helpers are emitted after the enclosing
     function body is complete.  Keep this scope's symbol storage alive until
     then, while still unlinking its names from ordinary lookup. */
  keep_local_member_types = nb_pending_local_members != 0;

  // Pop Locally Defined Symbols
  sym_pop(&local_stack, o->lstk, is_expr || keep_local_member_types);
  if (loc < o->stack_min)
    o->stack_min = loc;
  if (o->prev && o->stack_min < o->prev->stack_min)
    o->prev->stack_min = o->stack_min;
  if (!is_expr) loc = o->stack_loc;
  /* Stack slots below this point can be reused by the next sibling scope.
     Cached spills carry allocator indices as well as offsets, so retire the
     scope's cache as a unit before later locals occupy the same storage. */
  if (!is_expr)
    nb_temp_local_vars = 0;
  cur_scope = o->prev;
  nb_using_namespace_toks = o->using_namespace_count;
  --local_scope;
}

// Leave A Scope Via Break/Continue(/Goto)
static void leave_scope(struct scope *o)
{
  if (!o)
    return;
  try_call_scope_cleanup(o->cl.s);
  vla_leave(o);
}

/* short versiona for scopes with 'if/do/while/switch' which can
   declare only types (of struct/union/enum) */
static void new_scope_s(struct scope *o)
{
  o->lstk = local_stack;
  o->using_namespace_count = nb_using_namespace_toks;
  ++local_scope;
}

static void prev_scope_s(struct scope *o)
{
  sym_pop(&local_stack, o->lstk, 0);
  nb_using_namespace_toks = o->using_namespace_count;
  --local_scope;
}

// -------------------------------------------------------------------------
// call block from 'for do while' loops

static void lblock(int *bsym, int *csym)
{
  struct scope *lo = loop_scope, *co = cur_scope;
  int *b = co->bsym, *c = co->csym;
  if (csym)
  {
    co->csym = csym;
    loop_scope = co;
  }
  co->bsym = bsym;
  block(0);
  co->bsym = b;
  if (csym)
  {
    co->csym = c;
    loop_scope = lo;
  }
}

static void skip_or_save_balanced_tokens(TokenString *dst)
{
  int level = 0;
  do
  {
    int t = tok;
    if (t == TOK_EOF)
      cprime_error("unexpected end of file");
    tok_str_add_tok(dst);
    next();
    if (t == '{' || t == '(' || t == '[')
      level++;
    else if (t == '}' || t == ')' || t == ']')
      level--;
  }
  while (level > 0);
}

static void skip_or_save_statement_tokens(TokenString *dst, int nested)
{
statement_again:
  while (tok == TOK_LINENUM)
  {
    tok_str_add_tok(dst);
    next();
  }

  if (tok == '{')
  {
    skip_or_save_balanced_tokens(dst);
    return;
  }

  if (tok == TOK_IF)
  {
    tok_str_add_tok(dst);
    next();
    if (is_cpp_translation_unit() && tok == tok_constexpr) {
      tok_str_add_tok(dst);
      next();
    }
    while (tok == TOK_LINENUM)
    {
      tok_str_add_tok(dst);
      next();
    }
    if (tok == '(')
      skip_or_save_balanced_tokens(dst);
    else
      cprime_error("'(' expected");
    skip_or_save_statement_tokens(dst, 1);
    while (tok == TOK_LINENUM)
    {
      tok_str_add_tok(dst);
      next();
    }
    if (tok == TOK_ELSE)
    {
      tok_str_add_tok(dst);
      next();
      nested = 1;
      goto statement_again;
    }
    return;
  }

  if (tok == TOK_FOR || tok == TOK_WHILE || tok == TOK_SWITCH)
  {
    tok_str_add_tok(dst);
    next();
    if (tok != '(') cprime_error("'(' expected");
    skip_or_save_balanced_tokens(dst);
    skip_or_save_statement_tokens(dst, 1);
    return;
  }
  if (tok == TOK_DO)
  {
    tok_str_add_tok(dst);
    next();
    skip_or_save_statement_tokens(dst, 1);
    if (tok != TOK_WHILE) cprime_error("'while' expected");
    tok_str_add_tok(dst);
    next();
    if (tok != '(') cprime_error("'(' expected");
    skip_or_save_balanced_tokens(dst);
    if (nested && tok == ';') { tok_str_add_tok(dst); next(); }
    return;
  }
  while (tok != TOK_EOF && tok != ';' && tok != '}')
  {
    if (tok == '(' || tok == '[' || tok == '{')
      skip_or_save_balanced_tokens(dst);
    else { tok_str_add_tok(dst); next(); }
  }
  if (nested && tok == ';')
  {
    tok_str_add_tok(dst);
    next();
  }
}

static void skip_or_save_statement(TokenString **str)
{
  if (str)
    *str = tok_str_alloc();
  skip_or_save_statement_tokens(*str, 0);
  if (str)
    tok_str_add(*str, TOK_EOF);
}

static int for_header_starts_range_declaration(void)
{
  TokenString *replay = tok_str_alloc();
  int paren = 0, bracket = 0, brace = 0;
  int saw_initializer = 0;
  int is_range = 0;

  while (tok != TOK_EOF)
  {
    int at_top = paren == 0 && bracket == 0 && brace == 0;
    if (at_top && (tok == ';' || tok == ')'))
      break;
    if (at_top && tok == '=')
      saw_initializer = 1;
    if (at_top && tok == ':' && !saw_initializer)
    {
      tok_str_add(replay, tok);
      next();
      if (tok == ':')
      {
        tok_str_add(replay, tok);
        next();
        continue;
      }
      is_range = 1;
      break;
    }
    if (tok == '(')
      ++paren;
    else if (tok == ')' && paren > 0)
      --paren;
    else if (tok == '[')
      ++bracket;
    else if (tok == ']' && bracket > 0)
      --bracket;
    else if (tok == '{')
      ++brace;
    else if (tok == '}' && brace > 0)
      --brace;
    tok_str_add_tok(replay);
    next();
  }
  restore_cpp_lifecycle_probe(replay);
  return is_range;
}

static int range_has_member_name(CType *type, int name)
{
  int offset = 0;
  return type_has_member_func_name(type, name)
      || find_field_try(type, name, &offset)
      || find_static_member_try(type, name, NULL)
      || find_inherited_class_alias(get_struct_type_name_tok(type), name);
}

static TokenString *lower_range_for(void)
{
  char tmp_name[64];
  int var_tok, range_tok, index_tok, iterator_tok, end_tok;
  int body_first_tok;
  int body_had_semicolon = 0;
  int is_array_range, array_count;
  int is_iterator_range = 0;
  int adl_begin = 0, adl_end = 0;
  int has_range_type = 0;
  Sym *range_sym;
  CType range_type;
  TokenString *body, *lowered, *var_decl, *range_expr;

  var_decl = tok_str_alloc();
  if (!for_header_starts_range_declaration())
  {
    tok_str_free(var_decl);
    return 0;
  }
  while (tok == TOK_LINENUM)
    next();
  while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
         || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2
         || tok == TOK_VOLATILE3)
  {
    tok_str_add(var_decl, tok);
    next();
  }
  if (tok == TOK_AUTO)
  {
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    while (tok == '*' || tok == '&' || tok == TOK_LAND
           || tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
           || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3)
    {
      tok_str_add(var_decl, tok);
      next();
      while (tok == TOK_LINENUM)
        next();
    }
    if (tok < TOK_UIDENT)
      cprime_error("range-for variable");
    var_tok = tok;
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
  }
  else
  {
    CType declared_type;
    AttributeDef attributes;
    memset(&declared_type, 0, sizeof declared_type);
    memset(&attributes, 0, sizeof attributes);
    if (!parse_btype(&declared_type, &attributes, 0))
      cprime_error("range-for variable type");
    type_decl(&declared_type, &attributes, &var_tok, TYPE_DIRECT);
    if (var_tok < TOK_UIDENT)
      cprime_error("range-for variable");
    if (!add_ctype_declarator_tokens(var_decl, &declared_type, var_tok))
      cprime_error("unsupported range-for variable declarator");
  }
  skip(':');
  while (tok == TOK_LINENUM)
    next();
  if (tok == ')' || tok == TOK_EOF)
    cprime_error("range-for range");
  range_expr = tok_str_alloc();
  {
    int paren = 0, bracket = 0, angle = 0;
    while (tok != TOK_EOF)
    {
      if (!paren && !bracket && !angle && tok == ')')
        break;
      tok_str_add2(range_expr, tok, &tokc);
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
      next();
    }
  }
  tok_str_add(range_expr, TOK_EOF);
  range_tok = range_expr->len == 2 ? range_expr->str[0] : 0;
  range_sym = sym_find(range_tok);
  if (!range_sym) range_sym = find_lambda_static_symbol(range_tok);
  if (range_sym) range_sym = lambda_capture_symbol(range_sym);
  is_array_range = range_sym && (range_sym->type.t & VT_ARRAY);
  array_count = is_array_range ? range_sym->type.ref->c : 0;
  if (range_sym)
  {
    range_type = range_sym->type;
    has_range_type = 1;
  }
  else
  {
    infer_expr_type_from_tokens(range_expr, &range_type);
    has_range_type = 1;
  }
  /* A named range is commonly a reference parameter (`const C &range`).
     Member discovery belongs to the referred-to class, not to CPC's lowered
     pointer representation of the reference. */
  if (has_range_type && is_reference_type(&range_type))
  {
    decay_reference_type(&range_type);
    range_type = *pointed_type(&range_type);
  }
  if (has_range_type && (range_type.t & VT_ARRAY) && range_type.ref)
  {
    is_array_range = 1;
    array_count = range_type.ref->c;
  }
  if (has_range_type && (range_type.t & VT_BTYPE) == VT_STRUCT)
    materialize_incomplete_template_type(&range_type);
  if (!is_array_range && has_range_type
      && (range_type.t & VT_BTYPE) == VT_STRUCT)
  {
    is_iterator_range = range_has_member_name(&range_type,
                                     tok_alloc_const("begin"))
                        && range_has_member_name(&range_type,
                                     tok_alloc_const("end"));
  }
  if (!is_array_range && !is_iterator_range && has_range_type)
  {
    CType argument = range_type;
    Sym *begin_function, *end_function;
    /* The named forwarding-reference range is an lvalue. Range-for's free
       functions are found only through associated types and namespaces. */
    mk_reference(&argument);
    begin_function = cpp_resolve_associated_function(tok_alloc_const("begin"),
                        0, &argument, 1, NULL, 0);
    end_function = cpp_resolve_associated_function(tok_alloc_const("end"),
                        0, &argument, 1, NULL, 0);
    if (begin_function && end_function) {
      adl_begin = begin_function->v;
      adl_end = end_function->v;
      is_iterator_range = 1;
    }
  }
  skip(')');

  body_first_tok = tok;
  skip_or_save_statement(&body);
  if (body_first_tok != '{' && tok == ';')
  {
    skip(';');
    body_had_semicolon = 1;
  }

  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_i_%d",
           range_for_temp_counter++);
  index_tok = tok_alloc_const(tmp_name);
  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_%d",
           range_for_temp_counter++);
  range_tok = tok_alloc_const(tmp_name);
  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_it_%d",
           range_for_temp_counter++);
  iterator_tok = tok_alloc_const(tmp_name);
  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_end_%d",
           range_for_temp_counter++);
  end_tok = tok_alloc_const(tmp_name);

  lowered = tok_str_alloc();
  tok_str_add(lowered, '{');
  /* The range expression is evaluated once. The hidden forwarding reference
     also extends a temporary range's lifetime through the entire loop. */
  tok_str_add(lowered, TOK_AUTO);
  tok_str_add(lowered, TOK_LAND);
  tok_str_add(lowered, range_tok);
  tok_str_add(lowered, '=');
  tok_str_append_without_eof(lowered, range_expr);
  tok_str_add(lowered, ';');
  if (is_iterator_range)
  {
    tok_str_add(lowered, TOK_AUTO);
    tok_str_add(lowered, iterator_tok);
    tok_str_add(lowered, '=');
    if (adl_begin) tok_str_add(lowered, adl_begin);
    else {
      tok_str_add(lowered, range_tok);
      tok_str_add(lowered, '.');
      tok_str_add(lowered, tok_alloc_const("begin"));
    }
    tok_str_add(lowered, '(');
    if (adl_begin) tok_str_add(lowered, range_tok);
    tok_str_add(lowered, ')');
    tok_str_add(lowered, ';');
    tok_str_add(lowered, TOK_AUTO);
    tok_str_add(lowered, end_tok);
    tok_str_add(lowered, '=');
    if (adl_end) tok_str_add(lowered, adl_end);
    else {
      tok_str_add(lowered, range_tok);
      tok_str_add(lowered, '.');
      tok_str_add(lowered, tok_alloc_const("end"));
    }
    tok_str_add(lowered, '(');
    if (adl_end) tok_str_add(lowered, range_tok);
    tok_str_add(lowered, ')');
    tok_str_add(lowered, ';');
  }
  else
  {
    tok_str_add(lowered, TOK_INT);
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, '=');
    tok_str_add_cint(lowered, 0);
    tok_str_add(lowered, ';');
  }
  tok_str_add(lowered, TOK_FOR);
  tok_str_add(lowered, '(');
  tok_str_add(lowered, ';');
  if (is_iterator_range)
  {
    tok_str_add(lowered, iterator_tok);
    tok_str_add(lowered, TOK_NE);
    tok_str_add(lowered, end_tok);
  }
  else
  {
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, TOK_NE);
    if (is_array_range)
      tok_str_add_cint(lowered, array_count);
    else
    {
      tok_str_add(lowered, range_tok);
      tok_str_add(lowered, '.');
      tok_str_add(lowered, tok_alloc_const("Size"));
      tok_str_add(lowered, '(');
      tok_str_add(lowered, ')');
    }
  }
  tok_str_add(lowered, ';');
  if (is_iterator_range)
  {
    tok_str_add(lowered, TOK_INC);
    tok_str_add(lowered, iterator_tok);
  }
  else
  {
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, '=');
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, '+');
    tok_str_add_cint(lowered, 1);
  }
  tok_str_add(lowered, ')');
  tok_str_add(lowered, '{');
  /* Keep the user's declaration: a value loop variable owns a copy, while
     a reference loop variable binds to the actual iterator result. */
  tok_str_append_without_eof(lowered, var_decl);
  tok_str_add(lowered, '=');
  if (is_iterator_range)
  {
    tok_str_add(lowered, '*');
    tok_str_add(lowered, iterator_tok);
  }
  else
  {
    tok_str_add(lowered, range_tok);
    tok_str_add(lowered, '[');
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, ']');
  }
  tok_str_add(lowered, ';');
  tok_str_append_without_eof(lowered, body);
  if (body_first_tok != '{' && body_had_semicolon)
    tok_str_add(lowered, ';');
  tok_str_add(lowered, '}');
  tok_str_add(lowered, '}');
  tok_str_add2(lowered, tok, &tokc);
  tok_str_add(lowered, 0);

  return lowered;
}

static int try_parse_range_for(void)
{
  TokenString *lowered = lower_range_for();
  if (!lowered) return 0;

  begin_macro(lowered, 1);
  next();
  block(STMT_COMPOUND);
  if (tok == 0)
    next();
  return 1;
}

// c2y if/switch declaration
static void gexpr_decl(int for_condition)
{
  if (tok >= TOK_UIDENT)
  {
    int first_tok = tok;
    CValue first_tokc = tokc;
    Sym *type_sym = struct_find(first_tok);
    TokenString *replay = tok_str_alloc();

    if (!type_sym)
    {
      Sym *alias_sym = sym_find(first_tok);
      if (!alias_sym)
        alias_sym = global_symbol_find(first_tok);
      if (alias_sym && (alias_sym->type.t & VT_TYPEDEF)
          && ((alias_sym->type.t & VT_BTYPE) == VT_STRUCT))
        type_sym = alias_sym->type.ref;
    }
    tok_str_add2(replay, first_tok, &first_tokc);
    next();
    if (type_sym && tok == '(')
    {
      restore_cpp_lifecycle_probe(replay);
      gexpr();
      return;
    }
    restore_cpp_lifecycle_probe(replay);
  }
  if (tok >= TOK_UIDENT && find_class_template_def(find_current_namespace_tok(tok))) {
    TokenString *replay = tok_str_alloc();
    int depth = 0, expression;
    tok_str_add_tok(replay); next();
    if (tok == TOK_LT) {
      do {
        if (tok == TOK_LT) ++depth;
        else if (tok == TOK_GT) --depth;
        else if (tok == TOK_SAR) depth -= 2;
        tok_str_add_tok(replay); next();
      } while (depth > 0 && tok != TOK_EOF);
    }
    expression = tok == ':' || tok == '(' || tok == '{';
    restore_cpp_lifecycle_probe(replay);
    if (expression) { gexpr(); return; }
  }
  int v = decl_context(VT_JMP, 1);
  if (v > 1 && (tok != ';' || for_condition))
  {
    Sym *s = sym_find(v);
    vset(&s->type, s->r, (s->r &VT_SYM) ? 0 : s->c);
    vtop->sym = s;
    maybe_indir_reference();
    if (integral_constant_expression_wanted && s->a.integral_constexpr
        && (s->type.t & VT_CONSTANT)
        && is_integer_btype(s->type.t & VT_BTYPE)) {
      vtop->r = VT_CONST;
      vtop->c.i = s->const_value;
      vtop->sym = NULL;
    }
  }
  else
  {
    if (v)
    {
      skip(';');
      gexpr_decl(1);
      return;
    }
    gexpr();
  }
}

static void gexpr_selection(void)
{
  int temporary_mark = cpp_temp_mark();
  if (is_cpp_translation_unit() && tok == ';')
    next();
  else
  {
    gexpr_decl(0);
    if (!is_cpp_translation_unit() || tok != ';')
      return;
    /* An expression init-statement finishes before the condition starts.
       Its temporaries die here; declarations remain in the selection scope. */
    vpop();
    cpp_temp_end(temporary_mark);
    next();
  }
  gexpr_decl(1);
}

/* Inspect the complete header without evaluating an initializer as part of
   the constant condition. Nested lambdas and braced expressions can contain
   semicolons of their own. Replay preserves the original token values. */
static int selection_has_initializer(void)
{
  TokenString *header = tok_str_alloc();
  int depth = 0, found = 0;
  while (tok != TOK_EOF && (depth || tok != ')')) {
    if (!depth && tok == ';') found = 1;
    if (tok == '(' || tok == '[' || tok == '{') ++depth;
    else if (tok == ')' || tok == ']' || tok == '}') --depth;
    tok_str_add_tok(header);
    next();
  }
  if (tok == TOK_EOF) cprime_error("unterminated selection condition");
  restore_cpp_lifecycle_probe(header);
  return found;
}

static void block(int flags)
{
  int a, b, c, d, e, t;
  int temporary_mark = cpp_temp_mark();
  struct scope o;
  Sym *s;
  CType ref_ret_type;

again:
  if (is_cpp_translation_unit() && !(flags & STMT_COMPOUND) && tok != '{')
  {
    struct scope declaration_scope;
    new_scope(&declaration_scope);
    if (decl(VT_JMP))
    {
      skip(';');
      prev_scope(&declaration_scope, 0);
      return;
    }
    prev_scope(&declaration_scope, 0);
  }
  t = tok;
  /* If the token carries a value, next() might destroy it. Only with
     invalid code such as f(){"123"4;} */
  if (TOK_HAS_VALUE(t))
    goto expr;
  next();

  if (debug_modes)
    cprime_tcov_check_line (cprime_state, 0), cprime_tcov_block_begin (cprime_state);

  if (t == TOK_INIT_CLAUSE)
  {
    gexpr();
    vpop();
    skip(';');
    /* The containing declaration closes explicit initializer temporaries.
       Omitted array elements still use ordinary statement boundaries. */
  }
  else if (t == CPP_TOK_USING
      && tok == TOK_NAMESPACE)
  {
    unget_tok(tok);
    tok = t;
    try_parse_using_namespace_directive();
  }
  else if (t == CPP_TOK_USING)
  {
    int alias_tok, dummy_v = 0;
    CType alias_type;
    AttributeDef alias_ad;
    Sym *alias_sym;

    if (tok < TOK_UIDENT)
      expect("using alias name");
    alias_tok = tok;
    next();
    skip('=');
    memset(&alias_ad, 0, sizeof alias_ad);
    if (!parse_btype(&alias_type, &alias_ad, 0))
      expect("using alias type");
    type_decl(&alias_type, &alias_ad, &dummy_v, TYPE_ABSTRACT);
    alias_type.t |= VT_TYPEDEF;
    alias_sym = sym_push(alias_tok, &alias_type, 0, 0);
    alias_sym->a = alias_ad.a;
    if ((alias_type.t & VT_BTYPE) == VT_FUNC)
      merge_funcattr(&alias_sym->type.ref->f, &alias_ad.f);
    skip(';');
  }
  else if (t == TOK_NAMESPACE && is_cpp_translation_unit())
  {
    /* Block-scope namespace alias: namespace NAME = target;.  The alias
       binding is block-local, so it shadows (and later unshadows) any
       enclosing alias or namespace with the same name. */
    int ns_tok, target, global = 0, parts[17];
    Sym *binding;
    CType alias_type;

    if (tok < TOK_UIDENT)
      cprime_error("namespace name");
    ns_tok = tok;
    next();
    if (tok != '=')
      cprime_error("namespace alias definition expected");
    next();
    if (tok == ':') { next(); skip(':'); global = 1; }
    if (tok < TOK_UIDENT)
      cprime_error("namespace alias target expected");
    target = global ? make_namespace_tok_from_parts(&tok, 1)
                    : find_namespace_qualifier_tok(tok);
    next();
    while (tok == ':') {
      next();
      skip(':');
      if (tok < TOK_UIDENT)
        cprime_error("qualified namespace alias target expected");
      parts[0] = target;
      parts[1] = tok;
      target = make_namespace_tok_from_parts(parts, 2);
      next();
    }
    skip(';');
    alias_type.t = VT_VOID | VT_TYPEDEF;
    alias_type.ref = NULL;
    binding = sym_push(ns_tok, &alias_type, 0, 0);
    note_cpp_using_binding(binding, target);
  }
  else if (t == TOK_TRY)
  {
    cpp_eh_parse_try();
  }
  else if (t == TOK_CO_RETURN)
  {
    TokenString *operand = NULL;
    TokenString *replay = tok_str_alloc();
    int returns_value = tok != ';';
    int move_operand = 0;
    CType move_type;

    Sym *promise = cpp_coroutine_promise_symbol();
    if (returns_value)
      skip_or_save_block(&operand);
    if (operand) {
      int first = 0, last = operand->len;
      Sym *source;
      while (last > first && (operand->str[last - 1] == TOK_EOF
                              || operand->str[last - 1] == 0)) --last;
      while (first + 1 < last && operand->str[first] == TOK_LINENUM) first += 2;
      while (last - first >= 3 && operand->str[first] == '('
             && operand->str[last - 1] == ')') { ++first; --last; }
      source = last - first == 1 ? sym_find(operand->str[first]) : NULL;
      if (source && !(source->type.t & (VT_STATIC | VT_EXTERN))
          && ((source->r & VT_VALMASK) == VT_LOCAL
              || (source->r & VT_VALMASK) == VT_LLOCAL)
          && (!is_reference_type(&source->type)
              || (source->type.t & VT_RVALUE_REFERENCE))) {
        int method = tok_alloc_const("return_value");
        move_type = source->type;
        if (is_reference_type(&move_type)) move_type = *pointed_type(&move_type);
        move_type.t |= VT_RVALUE_REFERENCE;
        instantiate_template_member_for_call(&promise->type, method, &move_type, 1);
        /* Implicit move first tries the rvalue overload set. If no candidate
           is viable, retain the original lvalue expression for the retry. */
        move_operand = resolve_member_func_by_arg_types(&promise->type, method,
                                                         &move_type, 1) != NULL;
      }
    }
    tok_str_add(replay, TOK_CPC_COROUTINE_PROMISE);
    tok_str_add(replay, '.');
    tok_str_add(replay, tok_alloc_const(returns_value
                                          ? "return_value" : "return_void"));
    tok_str_add(replay, '(');
    if (move_operand) {
      move_type.t &= ~VT_RVALUE_REFERENCE;
      tok_str_add(replay, tok_alloc_const("static_cast"));
      tok_str_add(replay, TOK_LT);
      add_ctype_tokens(replay, &move_type);
      tok_str_add(replay, TOK_LAND);
      tok_str_add(replay, TOK_GT);
      tok_str_add(replay, '(');
    }
    if (returns_value)
      tok_str_append_without_eof(replay, operand);
    if (move_operand) tok_str_add(replay, ')');
    tok_str_add(replay, ')');
    tok_str_add_tok(replay);
    tok_str_add(replay, 0);
    if (operand)
      tok_str_free(operand);
    begin_macro(replay, 1);
    next();
    gexpr();
    vpop();
    skip(';');
    cpp_temp_end(temporary_mark);
  }
  else if (t == TOK_CXX_EH_CONSTRUCTED)
    cpp_eh_constructed_member();
  else if (t == TOK_CXX_BASE_SUBOBJECT)
    /* Marks the placement construction that follows as a base subobject. */
    cpp_pending_base_subobject_construction = 1;
  else if (t == TOK_CXX_CTOR_BODY)
    cpp_eh_constructor_body();
  else if (t == TOK_IF)
  {
    int end_chain = 0;
if_chain:
    new_scope(&o);
    if (is_cpp_translation_unit() && tok == tok_constexpr) {
      int condition, discarded, arm;
      ConstexprTemporaryObject *evaluation_mark;
      CType boolean_type;
      next();
      skip('(');
      if (selection_has_initializer()) {
        int initializer_mark = cpp_temp_mark();
        if (tok != ';' && !decl(VT_JMP)) {
          gexpr();
          vpop();
        }
        skip(';');
        cpp_temp_end(initializer_mark);
      }
      ++integral_constant_expression_wanted;
      evaluation_mark = constexpr_temporary_objects;
      ++constexpr_temporary_expression_depth;
      gexpr_decl(1);
      try_call_cpp_bool_conversion_operator();
      --integral_constant_expression_wanted;
      boolean_type.t = VT_BOOL;
      boolean_type.ref = NULL;
      gen_cast(&boolean_type);
      if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
        cprime_error("if constexpr condition is not a constant expression");
      condition = vtop->c.i != 0;
      vpop();
      --constexpr_temporary_expression_depth;
      release_constexpr_temporary_objects(evaluation_mark, 1);
      skip(')');
      for (arm = 0; arm < 2; ++arm) {
        discarded = arm ? condition : !condition;
        if (discarded && cpp_template_replay_bound) {
          TokenString *unused = tok_str_alloc();
          skip_or_save_statement_tokens(unused, 1);
          tok_str_free(unused);
        } else {
          int saved_nocode = nocode_wanted;
          int saved_discarded_owner = cpp_discarded_return_owner;
          LambdaExpression *deduction = lambda_probe;
          int saved_discarded = deduction ? deduction->discarded_statement : 0;
          if (discarded) {
            if (deduction) deduction->discarded_statement = 1;
            else if (funcname && *funcname)
              cpp_discarded_return_owner = tok_alloc_const(funcname);
          }
          if (discarded) ++nocode_wanted;
          block(0);
          cpp_discarded_return_owner = saved_discarded_owner;
          if (deduction) deduction->discarded_statement = saved_discarded;
          if (discarded) nocode_wanted = saved_nocode;
        }
        if (arm || tok != TOK_ELSE) break;
        next();
      }
      prev_scope(&o, 0);
      gsym(end_chain);
      return;
    }
    skip('(');
    cpp_range_condition_begin();
    gexpr_selection();
    cpp_temp_end_condition(temporary_mark);
    cpp_range_condition_push();
    a = gvtst(1, 0);
    skip(')');
    block(0);
    cpp_range_condition_pop();
    if (tok == TOK_ELSE)
    {
      end_chain = gjmp(end_chain);
      gsym(a);
      next();
      /* An else-if with no condition-scope declarations needs no suspended
         parser frame. Thread its exit into the same jump list. Preserve the
         recursive path when a declaration must remain visible in the else. */
      if (tok == TOK_IF && local_stack == o.lstk
          && nb_using_namespace_toks == o.using_namespace_count)
      {
        prev_scope(&o, 0);
        next();
        if (debug_modes)
          cprime_tcov_check_line(cprime_state, 0), cprime_tcov_block_begin(cprime_state);
        goto if_chain;
      }
      block(0);
    }
    else
      gsym(a);
    gsym(end_chain);
    prev_scope(&o, 0);

  }
  else if (t == TOK_WHILE)
  {
    new_scope(&o);
    d = gind();
    skip('(');
    gexpr_decl(0);
    cpp_temp_end_condition(temporary_mark);
    a = gvtst(1, 0);
    skip(')');
    b = 0;
    lblock(&a, &b);
    gsym(b);
    try_call_scope_cleanup(o.prev->cl.s);
    gjmp_addr(d);
    gsym(a);
    prev_scope(&o, 0);

  }
  else if (t == '{')
  {
    if (flags & STMT_COMPOUND) flags &= ~STMT_EXPR;
    if (debug_modes)
      cprime_debug_stabn(cprime_state, 0xc0, ind - func_ind);
    new_scope(&o);

    // Handle Local Labels Declarations
    cpp_eh_enter_function_body();
    while (tok == TOK_LABEL)
    {
      do
      {
        next();
        if (tok < TOK_UIDENT)
          expect("label identifier");
        label_push(&local_label_stack, tok, LABEL_DECLARED);
        next();
      }
      while (tok == ',');
      skip(';');
    }

    while (tok != '}')
    {
      if (flags & STMT_EXPR) { vpop(); push_void_value(); }
      decl(VT_LOCAL);
      if (tok != '}')
        block(flags | STMT_COMPOUND);
    }

    prev_scope(&o, flags &STMT_EXPR);
    if ((flags & STMT_EXPR) && is_cpp_translation_unit()
        && (vtop->type.t & VT_BTYPE) == VT_STRUCT)
      cpp_register_class_temporary(vtop);
    if (debug_modes)
      cprime_debug_stabn(cprime_state, 0xe0, ind - func_ind);
    if (local_scope)
      next();
    else if (!nocode_wanted)
      check_func_return();

  }
  else if (t == TOK_RETURN)
  {
    CType aggregate_return_register_type;
    int aggregate_return_registers = 0;
    if (lambda_probe)
    {
      lambda_note_return();
      return;
    }
    if (cpp_discarded_return_owner && funcname
        && cpp_discarded_return_owner == tok_alloc_const(funcname)) {
      Sym *function = global_symbol_find(cpp_discarded_return_owner);
      if (!function) function = sym_find(cpp_discarded_return_owner);
      if (function && function->type.ref
          && function->type.ref->f.func_cpp_deduced_return) {
        if (tok != ';') { gexpr(); vpop(); }
        skip(';');
        cpp_temp_end(temporary_mark);
        return;
      }
    }
    b = (func_vt.t &VT_BTYPE) != VT_VOID;
    a = 0;
    e = 0;
    if (tok >= TOK_UIDENT)
    {
      int result_tok = tok;
      int is_defining_record = 0;
      int defining_i;
      for (defining_i = 0; defining_i < nb_defining_class_stack; ++defining_i)
        if (defining_class_stack[defining_i] == result_tok)
        {
          is_defining_record = 1;
          break;
        }
      if (!is_defining_record)
      {
        Sym *record = struct_find(result_tok);
        if (!record)
        {
          Sym *binding = sym_find(result_tok);
          if (binding && (binding->type.t & VT_TYPEDEF)
              && (binding->type.t & VT_BTYPE) == VT_STRUCT)
            record = binding->type.ref;
        }
        if (record && record->c < 0)
          is_defining_record = 1;
      }
      next();
      if (tok == '{' && is_defining_record)
      {
        int depth = 1;
        next();
        while (tok != TOK_EOF && depth > 0)
        {
          if (tok == '{')
            ++depth;
          else if (tok == '}')
            --depth;
          next();
        }
        /* The enclosing record is still incomplete here.  Its synthetic
           return value has no observable code in this frontend pass. */
        tok = ';';
      }
      else
        unget_tok(result_tok);
    }
    if (tok != ';')
    {
      if (tok == '{' && (func_vt.t & VT_BTYPE) != VT_STRUCT
          && !is_reference_type(&func_vt))
      {
        CType result = func_vt;
        if ((result.t & VT_BTYPE) == VT_VOID)
          cprime_error("void function cannot return an initializer list");
        next();
        if (tok == '}') vset(&result, VT_CONST, 0);
        else {
          expr_eq();
          check_scalar_list_initialization(&result, vtop);
          gen_assign_cast(&result);
          if (tok == ',') next();
        }
        skip('}');
      }
      else if (tok == '{' && (func_vt.t & VT_BTYPE) == VT_STRUCT
          && func_vt.ref && !func_vt.ref->a.cpp_user_constructor)
      {
        CppConstructionDestination construction, *saved_destination;
        CType register_type;
        int align, regsize;
        saved_destination = cpp_construction_destination;
        construction.type = func_vt;
        construction.storage = VT_LLOCAL | VT_LVAL;
        construction.address = func_vc;
        construction.available = cpp_current_return_regs(&func_vt,
                                  &register_type, &align, &regsize) == 0;
        construction.conditional_marker = 0;
        construction.extend_backing_array_lifetime = 0;
        cpp_construction_destination = &construction;
        materialize_braced_temporary(&func_vt);
        cpp_construction_destination = saved_destination;
      }
      else if (tok == '{' && (func_vt.t & VT_BTYPE) == VT_STRUCT)
      {
        int depth = 1;
        int struct_tok = get_struct_type_name_tok(&func_vt);
        int saved_tok;
        CValue saved_tokc;
        int use_initializer_list_ctor;
        TokenString *return_expr;

        if (!struct_tok)
          cprime_error("braced return requires a named class or struct");
        use_initializer_list_ctor =
            class_has_initializer_list_constructor_candidate(&func_vt);
        return_expr = tok_str_alloc();
        tok_str_add(return_expr, struct_tok);
        tok_str_add(return_expr, '(');
        if (use_initializer_list_ctor)
          tok_str_add(return_expr, '{');
        next();
        while (tok != TOK_EOF && depth > 0)
        {
          if (tok == '{')
            ++depth;
          else if (tok == '}' && --depth == 0)
          {
            if (return_expr->len > 0
                && return_expr->str[return_expr->len - 1] == ',')
              --return_expr->len;
            next();
            break;
          }
          tok_str_add_tok(return_expr);
          next();
        }
        if (use_initializer_list_ctor)
          tok_str_add(return_expr, '}');
        tok_str_add(return_expr, ')');
        tok_str_add(return_expr, TOK_EOF);
        saved_tok = tok;
        saved_tokc = tokc;
        begin_macro(return_expr, 1);
        next();
        if (!cpp_parse_class_return_expression()) gexpr();
        /* end_macro() owns and frees return_expr. */
        end_macro();
        tok = saved_tok;
        tokc = saved_tokc;
      }
      else if (!cpp_parse_class_return_expression())
        gexpr();
      if (b)
      {
        if (is_reference_type(&func_vt))
        {
          CType ret_ptr = func_vt;
          decay_reference_type(&ret_ptr);
          if (is_reference_type(&vtop->type))
            decay_reference_type(&vtop->type);
          else {
            if ((vtop->type.t & VT_BTYPE) != VT_FUNC
                && !(vtop->type.t & (VT_ARRAY | VT_VLA)))
              test_lvalue();
            mk_pointer(&vtop->type);
            gaddrof();
          }
          /* A reference result denotes an object; its ABI adjustment is
             unconditional, unlike a nullable pointer result. */
          if (!adjust_derived_address_to_base(&ret_ptr, 0))
            gen_assign_cast(&ret_ptr);
          ref_ret_type = ret_ptr;
          e = 1;
        }
        else
        {
          int converting_ctor = 0;
          if ((func_vt.t & VT_BTYPE) == VT_STRUCT
              && !is_compatible_types(&func_vt, &vtop->type))
          {
            CType arg_type = vtop->type;
            if ((arg_type.t & VT_ARRAY) && arg_type.ref)
            {
              arg_type = *pointed_type(&arg_type);
              mk_pointer(&arg_type);
            }
            /* As for call arguments, an integer constant zero is a null
               pointer constant and can select a converting constructor
               taking a pointer. Keep the marker on the live value while the
               constructor is selected and constructed. */
            if (is_null_pointer(vtop)
                && (vtop->type.t & VT_BTYPE) != VT_PTR)
            {
              arg_type.t |= VT_NULLPTR_TYPE;
              vtop->type.t |= VT_NULLPTR_TYPE;
            }
            if (resolve_member_func_by_arg_types(&func_vt, TOK_CONSTRUCTOR1,
                                                 &arg_type, 1))
              converting_ctor = 1;
            else if (class_has_single_arg_constructor_for(&func_vt,
                                                          &arg_type))
              converting_ctor = 1;
            else
              vtop->type.t &= ~VT_NULLPTR_TYPE;
          }
          if (!converting_ctor)
          {
            CType result_initialization_type = func_vt;
            result_initialization_type.t &= ~VT_CONSTANT;
            gen_assign_cast(&result_initialization_type);
          }
        }
      }
      else
      {
        if (vtop->type.t != VT_VOID)
          cprime_warning("void function returns a value");
        vtop--;
      }
    }
    else if (b)
    {
      cprime_warning("'return' with no value");
      b = 0;
    }
    /* A scalar return expression can still be an lvalue into a local object
       (for example, `return values.front()`).  Materialize it before ending
       the scope: local destructors below are allowed to release that storage,
       and ordinary call spilling keeps the staged value alive until
       gfunc_return assigns the ABI result register. */
    if (b && (func_vt.t & VT_BTYPE) != VT_STRUCT)
      gv(RC_TYPE(func_vt.t));
    if (b && (func_vt.t & VT_BTYPE) == VT_STRUCT)
    {
      if (cpp_elide_named_local_return(&func_vt))
      {
        /* The operand names the returned object, so no copy or move
           constructor runs; gfunc_return stores the object into the result
           slot and the cleanup below skips its destructor. */
        gfunc_return(&func_vt);
        a = 1;
      }
      else if (!(a = try_gfunc_return_copy_construct(&func_vt)))
      {
        /* No constructor runs, but the trivial copy still lands in the
           caller's result object. */
        cpp_return_result_in_caller_slot = 1;
        gfunc_return(&func_vt);
        a = 1;
      }
      {
        int return_align, return_regsize, n;
        aggregate_return_registers = cpp_current_return_regs(&func_vt,
          &aggregate_return_register_type, &return_align, &return_regsize);
        if (!aggregate_return_registers && cpp_return_result_in_caller_slot)
        {
          /* The result object now lives in the caller's storage, which this
             frame still owns until the return completes: a cleanup raised
             later in this statement must destroy it before the caller can. */
          if (cpp_nrvo_return_operand)
            /* A named return object's declaration already registered that
               cleanup.  Deferring it below keeps it off the ordinary path
               while leaving it in every exception state. */
            cpp_elided_return_result_active = 1;
          else
            cpp_eh_register_result_object_cleanup(&func_vt, func_vc);
        }
        /* Keep packed results live on the value stack while temporary and
           local destructors run. Ordinary call spilling then preserves all
           return registers, including for records with defaulted dtors. */
        for (n = aggregate_return_registers; --n >= 0;)
        {
          int r, rc = RC_RET(aggregate_return_register_type.t) << n;
          for (r = 0; r < NB_REGS; ++r)
            if (reg_classes[r] & rc)
              break;
          vset(&aggregate_return_register_type, r, 0);
        }
      }
    }
    cpp_temp_end(temporary_mark);
    if (cpp_elided_return_active || cpp_elided_return_result_active)
      cpp_eh_defer_return_object_cleanups();
    leave_scope(root_scope);
    cpp_elided_return_active = 0;
    cpp_return_result_in_caller_slot = 0;
    cpp_nrvo_return_operand = 0;
    cpp_elided_return_result_active = 0;
    cpp_eh_restore_deferred_cleanups();
    if (aggregate_return_registers > 0)
    {
      int n;
      for (n = 0; n < aggregate_return_registers; ++n)
      {
        gv(RC_RET(aggregate_return_register_type.t) << n);
        vtop--;
      }
    }
#if defined(CPRIME_TARGET_PE) && defined(CPRIME_TARGET_X86_64)
    else if (b && a && (func_vt.t & VT_BTYPE) == VT_STRUCT)
    {
      /* Win64 also returns the indirect result address in RAX. Emit this
         after cleanups, which are allowed to clobber that register. */
      CType result_pointer = func_vt;
      mk_pointer(&result_pointer);
      vset(&result_pointer, VT_LOCAL | VT_LVAL, func_vc);
      gv(RC_RET(result_pointer.t));
      vtop--;
    }
#endif
    if (b && !a)
      gfunc_return(e ? &ref_ret_type : &func_vt);
    skip(';');
    // Jump Unless Last Stmt In Top-Level Block
    if (tok != '}' || local_scope != 1)
      rsym = gjmp(rsym);
    if (debug_modes)
      cprime_tcov_block_end (cprime_state, -1);
    CODE_OFF();

  }
  else if (t == TOK_BREAK)
  {
    // Compute Jump
    if (!cur_scope->bsym)
      cprime_error("cannot break");
    if (cur_switch && cur_scope->bsym == cur_switch->bsym)
      leave_scope(cur_switch->scope);
    else
      leave_scope(loop_scope);
    *cur_scope->bsym = gjmp(*cur_scope->bsym);
    skip(';');

  }
  else if (t == TOK_CONTINUE)
  {
    // Compute Jump
    if (!cur_scope->csym)
      cprime_error("cannot continue");
    leave_scope(loop_scope);
    *cur_scope->csym = gjmp(*cur_scope->csym);
    skip(';');

  }
  else if (t == TOK_FOR)
  {
    struct scope condition_scope;
    new_scope(&o);

    skip('(');
    if (try_parse_range_for())
    {
      prev_scope(&o, 0);
      return;
    }
    if (tok != ';')
    {
      // C99 For-Loop Init Decl?
      if (!decl(VT_JMP))
      {
        // No, Regular For-Loop Init Expr
        gexpr();
        vpop();
        cpp_temp_end(temporary_mark);
      }
    }
    skip(';');
    new_scope(&condition_scope);
    a = b = 0;
    c = d = gind();
    if (tok != ';')
    {
      gexpr_decl(1);
      cpp_temp_end_condition(temporary_mark);
      a = gvtst(1, 0);
    }
    skip(';');
    e = gjmp(0);
    d = gind();
    if (tok != ')') {
      gexpr();
      vpop();
      cpp_temp_end(temporary_mark);
    }
    try_call_scope_cleanup(condition_scope.prev->cl.s);
    gjmp_addr(c);
    gsym(e);
    skip(')');
    lblock(&a, &b);
    gjmp_addr(d);
    gsym_addr(b, d);
    gsym(a);
    prev_scope(&condition_scope, 0);
    prev_scope(&o, 0);

  }
  else if (t == TOK_DO)
  {
    new_scope_s(&o);
    a = b = 0;
    d = gind();
    lblock(&a, &b);
    gsym(b);
    skip(TOK_WHILE);
    skip('(');
    gexpr();
    cpp_temp_end_condition(temporary_mark);
    c = gvtst(0, 0);
    skip(')');
    skip(';');
    gsym_addr(c, d);
    gsym(a);
    prev_scope_s(&o);

  }
  else if (t == TOK_SWITCH)
  {
    struct switch_t *sw;

    sw = cprime_mallocz(sizeof *sw);
    sw->bsym = &a;
    sw->scope = &o;
    sw->prev = cur_switch;
    sw->nocode_wanted = nocode_wanted;
    cur_switch = sw;

    new_scope(&o);
    skip('(');
    gexpr_selection();
    cpp_temp_end(temporary_mark);
    if ((vtop->type.t & VT_BTYPE) == VT_STRUCT)
      try_call_cpp_conversion_operator(&int_type, 0);
    if (!is_integer_btype(vtop->type.t & VT_BTYPE))
      cprime_error("switch value not an integer");
    skip(')');
    sw->sv = *vtop--; // Save Switch Value
    a = 0;
    b = gjmp(0); // Jump To First Case
    lblock(&a, NULL);
    a = gjmp(a); // Add Implicit Break
    // Case Lookup
    gsym(b);
    if (sw->nocode_wanted)
      goto skip_switch;
    case_sort(sw);
    sw->bsym = NULL; // Marker For 32Bit:Gen_Opl()
    vpushv(&sw->sv);
    gv(RC_INT);
    d = gcase(sw->p, sw->n, 0);
    vpop();
    if (sw->def_sym)
      gsym_addr(d, sw->def_sym);
    else
      gsym(d);
skip_switch:
    // Break Label
    gsym(a);
    prev_scope(&o, 0);
    end_switch();

  }
  else if (t == TOK_CASE)
  {
    struct case_t *cr;
    if (!cur_switch)
      expect("switch");
    cr = cprime_malloc(sizeof(struct case_t));
    dynarray_add(&cur_switch->p, &cur_switch->n, cr);
    t = cur_switch->sv.type.t;
    cr->v1 = cr->v2 = value64(expr_const64(), t);
    if (tok == TOK_DOTS && non_iso)
    {
      next();
      cr->v2 = value64(expr_const64(), t);
      if (case_cmp(cr->v2, cr->v1) < 0)
        cprime_warning("empty case range");
    }
    // Case And Default Are Unreachable From A Switch Under Nocode_Wanted
    if (!cur_switch->nocode_wanted)
      cr->ind = gind();
    cr->line = file->line_num;
    skip(':');
    goto block_after_label;

  }
  else if (t == TOK_DEFAULT)
  {
    if (!cur_switch)
      expect("switch");
    if (cur_switch->def_sym)
      cprime_error("too many 'default'");
    cur_switch->def_sym = cur_switch->nocode_wanted ? -1 : gind();
    skip(':');
    goto block_after_label;

  }
  else if (t == TOK_GOTO)
  {
    vla_restore(cur_scope->vla.locorig);
    if (tok == '*' && non_iso)
    {
      // Computed Goto
      next();
      gexpr();
      if ((vtop->type.t & VT_BTYPE) != VT_PTR)
        expect("pointer");
      ggoto();

    }
    else if (tok >= TOK_UIDENT)
    {
      s = label_find(tok);
      // put forward definition if needed
      if (!s)
        s = label_push(&global_label_stack, tok, LABEL_FORWARD);
      else if (s->r == LABEL_DECLARED)
        s->r = LABEL_FORWARD;

      if (s->r & LABEL_FORWARD)
      {
        // Start New Goto Chain For Cleanups, Linked Via Label->Next
        if (cur_scope->cl.s && !nocode_wanted)
        {
          sym_push2(&pending_gotos, SYM_FIELD, 0, cur_scope->cl.n);
          pending_gotos->cleanup_label = s;
          s = sym_push2(&s->next, SYM_FIELD, 0, 0);
          pending_gotos->next = s;
        }
        s->jnext = gjmp(s->jnext);
      }
      else
      {
        try_call_cleanup_goto(s->cleanupstate);
        gjmp_addr(s->jind);
      }
      next();

    }
    else
      expect("label identifier");
    skip(';');

  }
  else if (t == TOK_ASM1 || t == TOK_ASM2 || t == TOK_ASM3)
    asm_instr();

  else
  {
    if (tok == ':' && t >= TOK_UIDENT)
    {
      TokenString *scope_probe = tok_str_alloc();
      tok_str_add(scope_probe, tok);
      next();
      if (tok == ':')
      {
        tok_str_add2(scope_probe, tok, &tokc);
        next();
        if (tok != ':')
        {
          /* Keep the token that followed the `::`; the lookahead already
             consumed it, and the replay must resume before it. */
          tok_str_add_tok(scope_probe);
          tok_str_add(scope_probe, 0);
          begin_macro(scope_probe, 1);
          tok = t;
          goto expr;
        }
        /* `X: ::stmt` (a label whose name is also a class) keeps the two
           remaining colons as the `::` of the labelled statement, which
           begins after the label. */
        tok_str_free(scope_probe);
        unget_tok(':');
      }
      else
        tok_str_free(scope_probe);
      // Label Case
      s = label_find(t);
      if (s)
      {
        if (s->r == LABEL_DEFINED)
          cprime_error("duplicate label '%s'", get_tok_str(s->v, NULL));
        s->r = LABEL_DEFINED;
        if (s->next)
        {
          Sym *pcl; // Pending Cleanup Goto
          for (pcl = s->next; pcl; pcl = pcl->prev)
            gsym(pcl->jnext);
          sym_pop(&s->next, NULL, 0);
        }
        else
          gsym(s->jnext);
      }
      else
        s = label_push(&global_label_stack, t, LABEL_DEFINED);
      s->jind = gind();
      s->cleanupstate = cur_scope->cl.s;

block_after_label:
      // Accept attributes after labels (e.g. 'unused')
      parse_attribute(NULL);

      if (debug_modes)
        cprime_tcov_reset_ind(cprime_state);
      vla_restore(cur_scope->vla.loc);

      if (tok != '}')
      {
        if (0 == (flags & STMT_COMPOUND))
          goto again;
        // C23: insert implicit null-statement whithin compound statement
      }
      else
      {
        // We Accept This, But It Is A Mistake
        cprime_warning_c(warn_all)("deprecated use of label at end of compound statement");
      }
    }
    else
    {
      // Expression Case
      if (t != ';')
      {
        unget_tok(t);
expr:
        if (flags & STMT_EXPR)
        {
          vpop();
          gexpr();
        }
        else
        {
          gexpr();
          vpop();
        }
        skip(';');
        if ((flags & STMT_EXPR) && tok == '}' && is_cpp_translation_unit()
            && (vtop->type.t & VT_BTYPE) == VT_STRUCT) {
          if (!(vtop->r & VT_CXX_PRVALUE)) {
            CType result = vtop->type;
            result.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_RVALUE_REFERENCE);
            if (!try_materialize_constructor_conversion_mode(&result, 1))
              cprime_error("statement expression result cannot be copied");
          }
          cpp_adopt_class_temporary(vtop);
        }
        cpp_temp_end(temporary_mark);
      }
    }
  }

  if (debug_modes)
    cprime_tcov_check_line (cprime_state, 0), cprime_tcov_block_end (cprime_state, 0);
}

/* This skips over a stream of tokens containing balanced {} and ()
   pairs, stopping at outer ',' ';' and '}' (or matching '}' if we started
   with a '{').  If STR then allocates and stores the skipped tokens
   in *STR.  This doesn't check if () and {} are nested correctly,
   i.e. "({)}" is accepted.  */
static int saved_member_template_name(int name)
{
  int i;
  for (i = 0; i < nb_template_member_defs; ++i)
    if (template_member_defs[i]
        && template_member_def_method_tok(template_member_defs[i]) == name) return 1;
  return 0;
}

static void skip_or_save_block_mode(TokenString **str, int allow_eof, int logical_operator)
{
  int braces = tok == '{';
  int level = 0;
  int angle = 0;
  int prev_sig = 0;
  int prev_prev_sig = 0;
  int qualified_name = 0;
  int entity_name = 0;
  int colon_owner = 0;
  int qualified_member_owner = 0;
  if (str)
    *str = tok_str_alloc();

  while (1)
  {
    int t = tok;
    if (logical_operator && level == 0 && angle == 0
        && (t == TOK_LOR || (logical_operator == TOK_LAND && t == TOK_LAND)
            || t == '?' || t == '=' || TOK_ASSIGN(t))) break;
    if (level == 0 && angle == 0
        && (t == ','
            || t == ';'
            || t == '}'
            || t == ')'
            || t == ']'))
      break;
    if (t == TOK_EOF)
    {
      if ((str && !allow_eof) || level > 0)
        cprime_error("unexpected end of file");
      else
        break;
    }
    if (str)
      tok_str_add_tok(*str);
    next();
    if ((t == TOK_LT || t == '<')
        && (angle > 0
            || (prev_sig >= TOK_UIDENT
                && (find_template_def(prev_sig)
                    || find_template_variable(prev_sig)
                    /* A qualified member template id (`Impl::member<...>`)
                       opens an argument list in ordinary call arguments too;
                       the `::` before the name makes the '<' unambiguous. */
                    || ((logical_operator || prev_prev_sig == ':')
                        && saved_member_template_name(prev_sig))
                    /* A nested class template is registered under its
                       interned member token when its owner is instantiated,
                       so the bare name does not identify it.  Resolve it
                       through the class named before the `::`, falling back
                       to the owner's saved body before that instantiation. */
                    || (prev_prev_sig == ':' && qualified_member_owner
                        && (find_class_template_def(make_static_member_tok(
                              qualified_member_owner, prev_sig))
                            || class_template_body_declares_nested_class_template(
                                 qualified_member_owner, prev_sig)))
                    || find_template_def(find_current_namespace_tok(prev_sig))
                    || (qualified_name && find_template_def(qualified_name))
                    || is_template_keyword_tok(prev_prev_sig)))))
      ++angle;
    else if ((t == TOK_SAR || t == TOK_SHR) && angle > 0)
    {
      --angle;
      if (angle > 0)
        --angle;
    }
    else if (t == TOK_GT && angle > 0)
      --angle;
    else if (t == '{' || t == '(' || t == '[' || is_cpp_lambda_marker(t))
      level++;
    else if (t == '}' || t == ')' || t == ']')
    {
      level--;
      if (level == 0 && braces && t == '}')
        break;
    }
    if (t != TOK_LINENUM && !TOK_HAS_VALUE(t))
    {
      /* Track the qualified names spelled at this level.  Tokens inside a
         template argument list are skipped so `Outer<int>::Inner<...>` keeps
         Outer as the owner instead of the last argument type. */
      if (angle == 0 && t >= TOK_UIDENT) {
        if (prev_sig == ':' && prev_prev_sig == ':' && qualified_name) {
          int parts[2] = { qualified_name, t };
          qualified_member_owner = colon_owner;
          qualified_name = find_namespace_tok_from_parts(parts, 2);
        } else {
          qualified_member_owner = 0;
          qualified_name = find_current_namespace_tok(t);
        }
        entity_name = t;
      }
      else if (angle == 0 && t == ':' && prev_sig != ':')
        colon_owner = entity_name;
      prev_prev_sig = prev_sig;
      prev_sig = t;
    }
  }
  if (str)
    tok_str_add(*str, TOK_EOF);
}

static void skip_or_save_block_ex(TokenString **str, int allow_eof)
{
  skip_or_save_block_mode(str, allow_eof, 0);
}

static void skip_or_save_block(TokenString **str)
{
  skip_or_save_block_ex(str, 0);
}

#define EXPR_CONST 1
#define EXPR_ANY   2

static void parse_init_elem(int expr_type)
{
  int saved_global_expr;
  switch (expr_type)
  {
  case EXPR_CONST:
    // Compound Literals Must Be Allocated Globally In This Case
    saved_global_expr = global_expr;
    global_expr = 1;
    expr_const1();
    global_expr = saved_global_expr;
    /* NOTE: symbols are accepted, as well as lvalue for anon symbols
    (compound literals).  */
    if (((vtop->r & (VT_VALMASK | VT_LVAL)) != VT_CONST
         && ((vtop->r & (VT_SYM | VT_LVAL)) != (VT_SYM | VT_LVAL)
             || vtop->sym->v < SYM_FIRST_ANOM))
#ifdef CPRIME_TARGET_PE
        || ((vtop->r & VT_SYM) && vtop->sym->a.dllimport)
#endif
       )
      cprime_error("initializer element is not constant");
    break;
  case EXPR_ANY:
    expr_eq();
    break;
  }
}

#if 1
static void init_assert(init_params *p, int offset)
{
  if (p->sec ? !NODATA_WANTED && offset > p->sec->data_offset
      : !nocode_wanted && offset > p->local_offset)
    cprime_internal_error("initializer overflow");
}
#else
#define init_assert(sec, offset)
#endif

// Put Zeros For Variable Based Init
static void init_putz(init_params *p, unsigned long c, int size)
{
  init_assert(p, c + size);
  if (p->constant_object_target && size > 0) {
    unsigned bucket = constexpr_local_bucket(p->constant_object_target);
    ConstexprLocalField *entry = cprime_mallocz(sizeof(*entry));
    entry->object = p->constant_object_target;
    entry->offset = (int)c - p->constant_object_offset;
    entry->zero_size = size;
    entry->next = constexpr_local_fields[bucket];
    constexpr_local_fields[bucket] = entry;
  }
  if (p->sec)
  {
    // nothing to do because globals are already set to zero
  }
  else
  {
    vpush_helper_func(TOK_memset);
    if (p->indirect_base)
    {
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, p->indirect_base);
      if (c) { vpushi(c); gen_op('+'); }
    }
    else
      vseti(VT_LOCAL, c);
    vpushi(0);
    vpushs(size);
#if defined CPRIME_TARGET_ARM && defined CPRIME_ARM_EABI
    vswap();  // Using __Aeabi_Memset(Void*, Size_T, Int)
#endif
    gfunc_call(3);
  }
}

/* delete relocations for specified range c ... c + size. Unfortunatly
   in very special cases, relocations may occur unordered */
