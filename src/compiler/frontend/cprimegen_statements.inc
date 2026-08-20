static void try_call_scope_cleanup(Sym *stop)
{
  Sym *cls = cur_scope->cl.s;
  for (; cls != stop; cls = cls->next)
  {
    Sym *fs = cls->cleanup_func;
    Sym *vs = cls->cleanup_sym;
    if (!fs || !vs)
      continue;
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
    if (vtop >= vstack && (vtop->type.t & VT_BTYPE) == VT_VOID)
      vpop();
  }
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
  ++local_scope;
}

static void prev_scope(struct scope *o, int is_expr)
{
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

  // Pop Locally Defined Symbols
  sym_pop(&local_stack, o->lstk, is_expr);
  cur_scope = o->prev;
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
  ++local_scope;
}

static void prev_scope_s(struct scope *o)
{
  sym_pop(&local_stack, o->lstk, 0);
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

static void append_range_for_body(TokenString *dst, TokenString *body,
                                  int var_tok, int ptr_tok)
{
  const int *p = body->str;
  const int *end = body->str + body->len;
  CValue cv;
  int t;

  while (p < end)
  {
    TOK_GET(&t, &p, &cv);
    if (t == TOK_EOF || t == 0)
      break;
    if ((t & ~SYM_FIELD) == (var_tok & ~SYM_FIELD))
    {
      tok_str_add(dst, '(');
      tok_str_add(dst, '*');
      tok_str_add(dst, ptr_tok);
      tok_str_add(dst, ')');
    }
    else
    {
      tok_str_add2(dst, t, &cv);
    }
  }
}

static int try_parse_range_for(void)
{
  char tmp_name[64];
  int var_tok, range_tok, index_tok, ptr_tok, body_first_tok;
  int is_array_range, array_count;
  int has_range_elem_type = 0;
  int has_range_type = 0;
  Sym *range_sym;
  CType range_type, range_elem_type;
  TokenString *body, *lowered, *var_decl, *range_expr;

  var_decl = tok_str_alloc();
  while (tok == TOK_LINENUM)
    next();
  if (tok == TOK_AUTO)
  {
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    if (tok == '&' || tok == TOK_LAND)
    {
      tok_str_add(var_decl, '&');
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
  else if (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
  {
    CType range_type;
    AttributeDef range_ad;

    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    memset(&range_type, 0, sizeof range_type);
    memset(&range_ad, 0, sizeof range_ad);
    if (!parse_btype(&range_type, &range_ad, 0))
      cprime_error("range-for variable type");
    if (!add_ctype_tokens(var_decl, &range_type))
      cprime_error("range-for variable type");
    while (tok == TOK_LINENUM)
      next();
    if (tok != '&')
      cprime_error("range-for reference");
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    if (tok < TOK_UIDENT)
      cprime_error("range-for variable");
    var_tok = tok;
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
  }
  else if (tok >= TOK_UIDENT && struct_find(tok))
  {
    int class_tok = tok;
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
    if (tok == ':')
    {
      int nested_tok;
      next();
      skip(':');
      while (tok == TOK_LINENUM)
        next();
      if (tok < TOK_UIDENT)
        cprime_error("range-for nested type");
      nested_tok = make_static_member_tok(class_tok, tok);
      if (!struct_find(nested_tok))
        nested_tok = tok;
      var_decl->len = 0;
      tok_str_add(var_decl, nested_tok);
      next();
      while (tok == TOK_LINENUM)
        next();
    }
    if (tok != '&')
      cprime_error("range-for reference");
    tok_str_add(var_decl, tok);
    next();
    while (tok == TOK_LINENUM)
      next();
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
    tok_str_free(var_decl);
    return 0;
  }
  skip(':');
  while (tok == TOK_LINENUM)
    next();
  if (tok < TOK_UIDENT)
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
  if (!is_array_range && has_range_type
      && (range_type.t & VT_BTYPE) == VT_STRUCT)
  {
    CType index_type = int_type;
    Sym *index_func =
      resolve_member_func_by_arg_types(&range_type,
                                       tok_alloc_const("operator[]"),
                                       &index_type, 1);
    if (index_func && (index_func->type.t & VT_BTYPE) == VT_FUNC
        && index_func->type.ref)
    {
      range_elem_type = index_func->type.ref->type;
      if (is_reference_type(&range_elem_type))
      {
        decay_reference_type(&range_elem_type);
        range_elem_type = *pointed_type(&range_elem_type);
      }
      has_range_elem_type = 1;
    }
  }
  skip(')');

  body_first_tok = tok;
  skip_or_save_block(&body);
  if (body_first_tok != '{')
    skip(';');

  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_i_%d",
           range_for_temp_counter++);
  index_tok = tok_alloc_const(tmp_name);
  snprintf(tmp_name, sizeof(tmp_name), "__cprime_range_p_%d",
           range_for_temp_counter++);
  ptr_tok = tok_alloc_const(tmp_name);

  lowered = tok_str_alloc();
  tok_str_add(lowered, '{');
  tok_str_add(lowered, TOK_INT);
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, ';');
  tok_str_add(lowered, TOK_FOR);
  tok_str_add(lowered, '(');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, '=');
  tok_str_add_cint(lowered, 0);
  tok_str_add(lowered, ';');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, TOK_NE);
  if (is_array_range)
    tok_str_add_cint(lowered, array_count);
  else
  {
    tok_str_append_without_eof(lowered, range_expr);
    tok_str_add(lowered, '.');
    tok_str_add(lowered, tok_alloc_const("Size"));
    tok_str_add(lowered, '(');
    tok_str_add(lowered, ')');
  }
  tok_str_add(lowered, ';');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, '=');
  tok_str_add(lowered, index_tok);
  tok_str_add(lowered, '+');
  tok_str_add_cint(lowered, 1);
  tok_str_add(lowered, ')');
  tok_str_add(lowered, '{');
  if (is_array_range)
  {
    tok_str_append_without_eof(lowered, var_decl);
    tok_str_add(lowered, '=');
    tok_str_add(lowered, range_tok);
    tok_str_add(lowered, '[');
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, ']');
    tok_str_add(lowered, ';');
    tok_str_append_without_eof(lowered, body);
  }
  else
  {
    if (has_range_elem_type)
    {
      if (!add_ctype_tokens(lowered, &range_elem_type))
        tok_str_add(lowered, TOK_AUTO);
    }
    else
      tok_str_add(lowered, TOK_AUTO);
    tok_str_add(lowered, '*');
    tok_str_add(lowered, ptr_tok);
    tok_str_add(lowered, '=');
    tok_str_add(lowered, '&');
    tok_str_append_without_eof(lowered, range_expr);
    tok_str_add(lowered, '[');
    tok_str_add(lowered, index_tok);
    tok_str_add(lowered, ']');
    tok_str_add(lowered, ';');
    append_range_for_body(lowered, body, var_tok, ptr_tok);
  }
  if (body_first_tok != '{')
    tok_str_add(lowered, ';');
  tok_str_add(lowered, '}');
  tok_str_add(lowered, '}');
  tok_str_add2(lowered, tok, &tokc);
  tok_str_add(lowered, 0);

  begin_macro(lowered, 1);
  next();
  block(STMT_COMPOUND);
  if (tok == 0)
    next();
  return 1;
}

// c2y if/switch declaration
static void gexpr_decl(void)
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
        alias_sym = sym_find2(global_stack, first_tok);
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
  if ((tok >= TOK_UIDENT && is_namespace_tok(tok))
      || (tok >= TOK_UIDENT
          && find_class_template_def(find_current_namespace_tok(tok))))
  {
    gexpr();
    return;
  }
  int v = decl(VT_JMP);
  if (v > 1 && tok != ';')
  {
    Sym *s = sym_find(v);
    vset(&s->type, s->r, (s->r &VT_SYM) ? 0 : s->c);
    vtop->sym = s;
  }
  else
  {
    if (v)
      skip(';');
    gexpr();
  }
}

static void block(int flags)
{
  int a, b, c, d, e, t;
  struct scope o;
  Sym *s;
  CType ref_ret_type;

again:
  t = tok;
  /* If the token carries a value, next() might destroy it. Only with
     invalid code such as f(){"123"4;} */
  if (TOK_HAS_VALUE(t))
    goto expr;
  next();

  if (debug_modes)
    cprime_tcov_check_line (cprime_state, 0), cprime_tcov_block_begin (cprime_state);

  if (t >= TOK_UIDENT && !strcmp(get_tok_str(t, NULL), "using"))
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
  else if (t == TOK_IF)
  {
    new_scope_s(&o);
    skip('(');
    gexpr_decl();
    a = gvtst(1, 0);
    skip(')');
    block(0);
    if (tok == TOK_ELSE)
    {
      d = gjmp(0);
      gsym(a);
      next();
      block(0);
      gsym(d); // patch else jmp
    }
    else
      gsym(a);
    prev_scope_s(&o);

  }
  else if (t == TOK_WHILE)
  {
    new_scope_s(&o);
    d = gind();
    skip('(');
    gexpr();
    a = gvtst(1, 0);
    skip(')');
    b = 0;
    lblock(&a, &b);
    gjmp_addr(d);
    gsym_addr(b, d);
    gsym(a);
    prev_scope_s(&o);

  }
  else if (t == '{')
  {
    if (debug_modes)
      cprime_debug_stabn(cprime_state, 0xc0, ind - func_ind);
    new_scope(&o);

    // Handle Local Labels Declarations
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
      decl(VT_LOCAL);
      if (tok != '}')
        block(flags | STMT_COMPOUND);
    }

    prev_scope(&o, flags &STMT_EXPR);
    if (debug_modes)
      cprime_debug_stabn(cprime_state, 0xe0, ind - func_ind);
    if (local_scope)
      next();
    else if (!nocode_wanted)
      check_func_return();

  }
  else if (t == TOK_RETURN)
  {
    b = (func_vt.t &VT_BTYPE) != VT_VOID;
    a = 0;
    e = 0;
    if (tok != ';')
    {
      if (tok == '{' && (func_vt.t & VT_BTYPE) == VT_STRUCT)
      {
        int depth = 1;
        int struct_tok = get_struct_type_name_tok(&func_vt);
        int saved_tok;
        CValue saved_tokc;
        TokenString *return_expr;

        if (!struct_tok)
          cprime_error("braced return requires a named class or struct");
        return_expr = tok_str_alloc();
        tok_str_add(return_expr, struct_tok);
        tok_str_add(return_expr, '(');
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
        tok_str_add(return_expr, ')');
        tok_str_add(return_expr, TOK_EOF);
        saved_tok = tok;
        saved_tokc = tokc;
        begin_macro(return_expr, 1);
        next();
        gexpr();
        /* end_macro() owns and frees return_expr. */
        end_macro();
        tok = saved_tok;
        tokc = saved_tokc;
      }
      else
        gexpr();
      if (b)
      {
        if (is_reference_type(&func_vt))
        {
          CType ret_ptr = func_vt;
          decay_reference_type(&ret_ptr);
          test_lvalue();
          mk_pointer(&vtop->type);
          gaddrof();
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
            if (resolve_member_func_by_arg_types(&func_vt, TOK_CONSTRUCTOR1,
                                                 &arg_type, 1))
              converting_ctor = 1;
          }
          if (!converting_ctor)
            gen_assign_cast(&func_vt);
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
    if (b && (func_vt.t & VT_BTYPE) == VT_STRUCT)
    {
      a = try_gfunc_return_copy_construct(&func_vt);
      if (!a)
      {
        gfunc_return(&func_vt);
        a = 1;
      }
    }
    leave_scope(root_scope);
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
      }
    }
    skip(';');
    a = b = 0;
    c = d = gind();
    if (tok != ';')
    {
      gexpr();
      a = gvtst(1, 0);
    }
    skip(';');
    if (tok != ')')
    {
      e = gjmp(0);
      d = gind();
      gexpr();
      vpop();
      gjmp_addr(c);
      gsym(e);
    }
    skip(')');
    lblock(&a, &b);
    gjmp_addr(d);
    gsym_addr(b, d);
    gsym(a);
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
    sw->scope = cur_scope;
    sw->prev = cur_switch;
    sw->nocode_wanted = nocode_wanted;
    cur_switch = sw;

    new_scope_s(&o);
    skip('(');
    gexpr_decl();
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
    prev_scope_s(&o);
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
        tok_str_add(scope_probe, 0);
        begin_macro(scope_probe, 1);
        tok = t;
        goto expr;
      }
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
static void skip_or_save_block(TokenString **str)
{
  int braces = tok == '{';
  int level = 0;
  if (str)
    *str = tok_str_alloc();

  while (1)
  {
    int t = tok;
    if (level == 0
        && (t == ','
            || t == ';'
            || t == '}'
            || t == ')'
            || t == ']'))
      break;
    if (t == TOK_EOF)
    {
      if (str || level > 0)
        cprime_error("unexpected end of file");
      else
        break;
    }
    if (str)
      tok_str_add_tok(*str);
    next();
    if (t == '{' || t == '(' || t == '[')
      level++;
    else if (t == '}' || t == ')' || t == ']')
    {
      level--;
      if (level == 0 && braces && t == '}')
        break;
    }
  }
  if (str)
    tok_str_add(*str, TOK_EOF);
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
  if (p->sec)
  {
    // nothing to do because globals are already set to zero
  }
  else
  {
    vpush_helper_func(TOK_memset);
    vseti(VT_LOCAL, c);
    vpushi(0);
    vpushs(size);
#if defined CPRIME_TARGET_ARM && defined CPRIME_ARM_EABI
    vswap();  // Using __Aeabi_Memset(Void*, Size_T, Int)
#endif
    gfunc_call(3);
  }
}

#define DIF_FIRST     1
#define DIF_SIZE_ONLY 2
#define DIF_HAVE_ELEM 4
#define DIF_CLEAR     8

/* delete relocations for specified range c ... c + size. Unfortunatly
   in very special cases, relocations may occur unordered */
