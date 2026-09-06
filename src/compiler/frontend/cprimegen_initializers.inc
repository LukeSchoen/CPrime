static void decl_design_delrels(Section *sec, int c, int size)
{
  ObjW_Rel *rel, *rel2, *rel_end;
  if (!sec || !sec->reloc)
    return;
  rel = rel2 = (ObjW_Rel *)sec->reloc->data;
  rel_end = (ObjW_Rel *)(sec->reloc->data + sec->reloc->data_offset);
  while (rel < rel_end)
  {
    if (rel->r_offset >= c && rel->r_offset < c + size)
      sec->reloc->data_offset -= sizeof * rel;
    else
    {
      if (rel2 != rel)
        memcpy(rel2, rel, sizeof *rel);
      ++rel2;
    }
    ++rel;
  }
}

static void decl_design_flex(init_params *p, Sym *ref, int index)
{
  if (ref == p->flex_array_ref)
  {
    if (index >= ref->c)
      ref->c = index + 1;
  }
  else if (ref->c < 0 && index >= 0)
    cprime_error("flexible array has zero size in this context");
}

/* t is the array or struct type. c is the array or struct
   address. cur_field is the pointer to the current
   field, for arrays the 'c' member contains the current start
   index.  'flags' is as in decl_initializer.
   'al' contains the already initialized length of the
   current container (starting at c).  This returns the new length of that.  */
static int decl_designator(init_params *p, CType *type, unsigned long c,
                           Sym **cur_field, int flags, int al)
{
  Sym *s, *f;
  int index, index_last, align, l, nb_elems, elem_size;
  unsigned long corig = c;

  elem_size = 0;
  nb_elems = 1;

  if (flags & DIF_HAVE_ELEM)
    goto no_designator;

  if (non_iso && tok >= TOK_UIDENT)
  {
    l = tok, next();
    if (tok == ':')
    {
      /* C permits the legacy designated-initializer spelling `field: value`,
         but in C++ the same prefix can begin a qualified expression such as
         `std::move(value)`.  Do not consume the identifier as a field name
         when the colon is followed by another colon. */
      next();
      if (tok != ':')
        goto struct_field_name_read;
      unget_tok(':');
    }
    unget_tok(l);
  }

  // NOTE: we only support ranges for last designator
  while (nb_elems == 1 && (tok == '[' || tok == '.'))
  {
    if (tok == '[')
    {
      if (!(type->t & VT_ARRAY))
        expect("array type");
      next();
      index = index_last = expr_const();
      if (tok == TOK_DOTS && non_iso)
      {
        next();
        index_last = expr_const();
      }
      skip(']');
      s = type->ref;
      decl_design_flex(p, s, index_last);
      if (index < 0 || index_last >= s->c || index_last < index)
        cprime_error("index exceeds array bounds or range is empty");
      if (cur_field)
        (*cur_field)->c = index_last;
      type = pointed_type(type);
      elem_size = type_size(type, &align);
      c += index * elem_size;
      nb_elems = index_last - index + 1;
    }
    else
    {
      int cumofs;
      next();
      l = tok;
struct_field:
      next();
struct_field_name_read:
      f = find_field(type, l, &cumofs);
      if (cur_field)
        *cur_field = f;
      type = &f->type;
      c += cumofs;
    }
    cur_field = NULL;
  }
  if (!cur_field)
  {
    if (tok == '=')
      next();
    else if (!non_iso)
      expect("=");
  }
  else
  {
no_designator:
    if (type->t & VT_ARRAY)
    {
      index = (*cur_field)->c;
      s = type->ref;
      decl_design_flex(p, s, index);
      if (index >= s->c)
        cprime_error("too many initializers");
      type = pointed_type(type);
      elem_size = type_size(type, &align);
      c += index * elem_size;
    }
    else
    {
      f = *cur_field;
      // Skip bitfield padding. Also with size 32 and 64.
      while (f && (f->v & SYM_FIRST_ANOM) &&
             is_integer_btype(f->type.t & VT_BTYPE))
        *cur_field = f = f->next;
      if (!f)
        cprime_error("too many initializers");
      type = &f->type;
      c += f->c;
    }
  }

  if (!elem_size) // For Structs
    elem_size = type_size(type, &align);

  /* Using designators the same element can be initialized more
     than once.  In that case we need to delete possibly already
     existing relocations. */
  if (!(flags & DIF_SIZE_ONLY) && c - corig < al)
  {
    decl_design_delrels(p->sec, c, elem_size *nb_elems);
    flags &= ~DIF_CLEAR; // Mark Stack Dirty Too
  }

  decl_initializer(p, type, c, flags & ~DIF_FIRST);

  if (!(flags & DIF_SIZE_ONLY) && nb_elems > 1)
  {
    Sym aref = {0};
    CType t1;
    int i;
    if (p->sec || (type->t & VT_ARRAY))
    {
      // Make Init_Putv/Vstore Believe It Were A Struct
      aref.c = elem_size;
      t1.t = VT_STRUCT, t1.ref = &aref;
      type = &t1;
    }
    if (p->sec)
      vpush_ref(type, p->sec, c, elem_size);
    else
      vset(type, VT_LOCAL | VT_LVAL, c);
    for (i = 1; i < nb_elems; i++)
    {
      vdup();
      init_putv(p, type, c + elem_size *i);
    }
    vpop();
  }

  c += nb_elems * elem_size;
  if (c - corig > al)
    al = c - corig;
  return al;
}

/* Initializer offsets can refer to a stack allocation or to an existing
   object reached through a placement address. */
static void init_push_lvalue(init_params *p, CType *type, unsigned long offset)
{
  if (p->indirect_base)
  {
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, p->indirect_base);
    if (offset) { vpushi(offset); gen_op('+'); }
    vtop->type = *type;
    mk_pointer(&vtop->type);
    indir();
  }
  else
    vset(type, VT_LOCAL | VT_LVAL, offset);
}

// Store A Value Or An Expression Directly In Global Data Or In Local Array
static void init_putv(init_params *p, CType *type, unsigned long c)
{
  int bt;
  void *ptr;
  CType dtype;
  int size, align;
  Section *sec = p->sec;
  uint64_t val;

  dtype = *type;
  dtype.t &= ~VT_CONSTANT; // need to do that to avoid false warning
  if (is_reference_type(&dtype))
  {
    cpp_temp_extend_lifetime(vtop);
    decay_reference_type(&dtype);
    if (is_reference_type(&vtop->type))
      decay_reference_type(&vtop->type);
    else
    {
      test_lvalue();
      mk_pointer(&vtop->type);
      gaddrof();
    }
  }

  size = type_size(type, &align);
  if (type->t & VT_BITFIELD)
    size = (BIT_POS(type->t) + BIT_SIZE(type->t) + 7) / 8;
  init_assert(p, c + size);

  if (sec)
  {
    // XXX: not portable
    // XXX: generate error if incorrect relocation
    gen_assign_cast(&dtype);
    bt = dtype.t &VT_BTYPE;

    if ((vtop->r & VT_SYM)
        && bt != VT_PTR
        && (bt != (PTR_SIZE == 8 ? VT_LLONG : VT_INT)
            || (type->t & VT_BITFIELD))
        && !((vtop->r & VT_CONST) && vtop->sym->v >= SYM_FIRST_ANOM)
       )
      cprime_error("initializer element is not computable at load time");

    if (NODATA_WANTED)
    {
      vtop--;
      return;
    }

    ptr = sec->data + c;
    val = vtop->c.i;

    // XXX: make code faster ?
    if ((vtop->r & (VT_SYM | VT_CONST)) == (VT_SYM | VT_CONST) &&
        vtop->sym->v >= SYM_FIRST_ANOM &&
        /* XXX This rejects compound literals like
           '(void *){ptr}'.  The problem is that '&sym' is
           represented the same way, which would be ruled out
           by the SYM_FIRST_ANOM check above, but also '"string"'
           in 'char *p = "string"' is represented the same
           with the type being VT_PTR and the symbol being an
           anonymous one.  That is, there's no difference in vtop
           between '(void *){x}' and '&(void *){x}'.  Ignore
           pointer typed entities here.  Hopefully no real code
           will ever use compound literals with scalar type.  */
        (vtop->type.t & VT_BTYPE) != VT_PTR)
    {
      // These come from compound literals, memcpy stuff over.
      Section *ssec;
      ObjSym *esym;
      ObjW_Rel *rel;
      esym = elfsym(vtop->sym);
      ssec = cprime_state->sections[esym->st_shndx];
      memmove (ptr, ssec->data + esym->st_value + (int)vtop->c.i, size);
      if (ssec->reloc)
      {
        /* We need to copy over all memory contents, and that
           includes relocations.  Use the fact that relocs are
           created it order, so look from the end of relocs
           until we hit one before the copied region.  */
        unsigned long relofs = ssec->reloc->data_offset;
        while (relofs >= sizeof(*rel))
        {
          relofs -= sizeof(*rel);
          rel = (ObjW_Rel *)(ssec->reloc->data + relofs);
          if (rel->r_offset >= esym->st_value + size)
            continue;
          if (rel->r_offset < esym->st_value)
            break;
          put_elf_reloca(symtab_section, sec,
                         c + rel->r_offset - esym->st_value,
                         Obj64_R_TYPE(rel->r_info),
                         Obj64_R_SYM(rel->r_info),
#if PTR_SIZE == 8
                         rel->r_addend
#else
                         0
#endif
                        );
        }
      }
    }
    else
    {
      if (type->t & VT_BITFIELD)
      {
        int bit_pos, bit_size, bits, n;
        unsigned char *p, v, m;
        bit_pos = BIT_POS(vtop->type.t);
        bit_size = BIT_SIZE(vtop->type.t);
        p = (unsigned char *)ptr + (bit_pos >> 3);
        bit_pos &= 7, bits = 0;
        while (bit_size)
        {
          n = 8 - bit_pos;
          if (n > bit_size)
            n = bit_size;
          v = val >> bits << bit_pos;
          m = ((1 << n) - 1) << bit_pos;
          *p = (*p & ~m) | (v &m);
          bits += n, bit_size -= n, bit_pos = 0, ++p;
        }
      }
      else
        switch (bt)
        {
        case VT_BOOL:
          *(char *)ptr = val != 0;
          break;
        case VT_BYTE:
          *(char *)ptr = val;
          break;
        case VT_SHORT:
          write16le(ptr, val);
          break;
        case VT_FLOAT:
          write32le(ptr, val);
          break;
        case VT_DOUBLE:
          write64le(ptr, val);
          break;
        case VT_LDOUBLE:
#if defined CPRIME_IS_NATIVE_387
          /* Host and target platform may be different but both have x87.
             On windows, cpc does not use VT_LDOUBLE, except when it is a
             cross compiler.  In this case a mingw gcc as host compiler
             comes here with 10-byte long doubles, while msvc or cpc won't.
             cpc itself can still translate by asm.
             In any case we avoid possibly random bytes 11 and 12.
          */
          if (sizeof (long double) >= 10)
            memcpy(ptr, &vtop->c.ld, 10);
#ifdef __TINYC__
          else if (sizeof (long double) == sizeof (double))
            __asm__("fldl %1\nfstpt %0\n" : "=m" (*ptr) : "m" (vtop->c.ld));
#endif
          else
#endif
            /* For other platforms it should work natively, but may not work
               for cross compilers */
            if (sizeof(long double) == LDOUBLE_SIZE)
              memcpy(ptr, &vtop->c.ld, LDOUBLE_SIZE);
            else if (sizeof(double) == LDOUBLE_SIZE)
              *(double * )ptr = (double)vtop->c.ld;
            else if (0 == memcmp(ptr, &vtop->c.ld, LDOUBLE_SIZE))
              ; // nothing to do for 0.0
#ifndef CPRIME_CROSS_TEST
            else
              cprime_error("can't cross compile long double constants");
#endif
          break;

#if PTR_SIZE == 8
        // Intptr_T May Need A Reloc Too, See Tcctest.C:Relocation_Test()
        case VT_LLONG:
        case VT_PTR:
          if (vtop->r & VT_SYM)
            greloca(sec, vtop->sym, c, R_DATA_PTR, val);
          else
            write64le(ptr, val);
          break;
        case VT_INT:
          write32le(ptr, val);
          break;
#else
        case VT_LLONG:
          write64le(ptr, val);
          break;
        case VT_PTR:
        case VT_INT:
          if (vtop->r & VT_SYM)
            greloc(sec, vtop->sym, c, R_DATA_PTR);
          write32le(ptr, val);
          break;
#endif
        default:
          //cprime_internal_error("unexpected type");
          break;
        }
    }
    vtop--;
  }
  else
  {
    if (p->elided_value)
    {
      p->elided_value = 0;
      vpop();
      return;
    }
    if ((dtype.t & VT_BTYPE) == VT_STRUCT
        && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == (VT_LOCAL | VT_LVAL)
        && (int)vtop->c.i == (int)c
        && is_compatible_unqualified_types(&dtype, &vtop->type))
    {
      /* A same-type class prvalue can already occupy the final object. */
      vpop();
      return;
    }
    if ((dtype.t & VT_BTYPE) == VT_STRUCT
        && struct_needs_memberwise_copy(&dtype))
    {
      SValue dst_ptr, src_ptr, source;
      source = *vtop;
      vtop--;
      init_push_lvalue(p, &dtype, c);
      mk_pointer(&vtop->type);
      gaddrof();
      dst_ptr = *vtop;
      vtop--;
      vpushv(&source);
      mk_pointer(&vtop->type);
      gaddrof();
      src_ptr = *vtop;
      vtop--;
      if (resolve_copy_constructor_func(&dtype, &source.type))
        copy_construct_one_lvalue_pair(&dtype, &dst_ptr, &src_ptr, 0,
          (source.type.t & VT_RVALUE_REFERENCE) != 0);
      else
        copy_construct_struct_memberwise_from_base_ptr(&dtype, &dst_ptr,
                                                       &src_ptr, 0,
          (source.type.t & VT_RVALUE_REFERENCE) != 0);
    }
    else
    {
      init_push_lvalue(p, &dtype, c);
      vswap();
      vstore();
      vpop();
    }
  }
}

static int saved_initializer_close(TokenString *expression, int start,
                                    int opening, int closing)
{
  int i, depth = 0;
  for (i = start; i < expression->len; ++i)
  {
    int token = expression->str[i];
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(expression->str, expression->len, i);
      continue;
    }
    if (token == opening) ++depth;
    else if (token == closing && !--depth) return i;
  }
  return -1;
}

static int saved_initializer_is_direct_constructor(TokenString *expression,
                                                    CType *type)
{
  TokenString *yes = NULL, *no = NULL;
  if (cpp_saved_conditional_at(expression, &yes, &no)) {
    int result = (cpp_saved_arm_is_throw(yes)
                   || saved_initializer_is_direct_constructor(yes, type))
              && (cpp_saved_arm_is_throw(no)
                   || saved_initializer_is_direct_constructor(no, type));
    tok_str_free(yes); tok_str_free(no);
    return result;
  }
  int first = tok_str_next_non_linenum_index(expression->str, expression->len, 0);
  int end = tok_str_prev_token_index(expression->str, expression->len,
                                     expression->len);
  int close, next_token;
  if (end >= 0 && expression->str[end] == TOK_EOF)
    end = tok_str_prev_token_index(expression->str, expression->len, end);
  while (first < end && expression->str[first] == '(')
  {
    close = saved_initializer_close(expression, first, '(', ')');
    if (close != end) break;
    first = tok_str_next_non_linenum_index(expression->str, expression->len,
                                           first + 1);
    end = tok_str_prev_token_index(expression->str, expression->len, end);
  }
  if (first >= end) return 0;
  if ((expression->str[first] < TOK_UIDENT && expression->str[first] != ':')
      || TOK_HAS_VALUE(expression->str[first]))
    return 0;
  if (is_cpp_lambda_marker(expression->str[first]))
  {
    int i;
    for (i = first + 1; i <= end; ++i)
    {
      int token = expression->str[i];
      if (TOK_HAS_VALUE(token))
      {
        i += tok_str_value_extra_words(expression->str, expression->len, i);
        continue;
      }
      if (token == '{')
        return saved_initializer_close(expression, i, '{', '}') == end;
    }
    return 0;
  }
  next_token = tok_str_next_non_linenum_index(expression->str, expression->len,
                                              first + 1);
  if (next_token <= end && expression->str[next_token] == TOK_LT
      && find_class_template_def(find_current_namespace_tok(expression->str[first])))
  {
    int i, depth = 0, parentheses = 0, type_end = -1;
    TokenString *type_tokens, *parent;
    const int *parent_ptr;
    CType constructed_type;
    AttributeDef ad;
    int saved_tok, unused_name = 0, parsed;
    CValue saved_tokc;
    for (i = next_token; i <= end; ++i)
    {
      int token = expression->str[i];
      if (TOK_HAS_VALUE(token))
      {
        i += tok_str_value_extra_words(expression->str, expression->len, i);
        continue;
      }
      if (token == '(' || token == '[') ++parentheses;
      else if (token == ')' || token == ']') --parentheses;
      else if (!parentheses)
      {
        if (token == TOK_LT) ++depth;
        else if (token == TOK_GT) --depth;
        else if (token == TOK_SAR) depth -= 2;
        if (!depth) { type_end = i; break; }
      }
    }
    if (type_end < 0) return 0;
    next_token = tok_str_next_non_linenum_index(expression->str, expression->len,
                                                type_end + 1);
    if (next_token > end || expression->str[next_token] != '('
        || saved_initializer_close(expression, next_token, '(', ')') != end)
      return 0;
    type_tokens = tok_str_alloc();
    for (i = first; i <= type_end; ++i)
      if (TOK_HAS_VALUE(expression->str[i]))
        tok_str_add_record(type_tokens, expression->str, expression->len, &i);
      else tok_str_add(type_tokens, expression->str[i]);
    tok_str_add(type_tokens, TOK_EOF);
    saved_tok = tok;
    saved_tokc = tokc;
    parent = macro_stack;
    parent_ptr = macro_ptr;
    memset(&ad, 0, sizeof(ad));
    begin_macro(type_tokens, 1);
    next();
    parsed = parse_btype(&constructed_type, &ad, 0);
    if (parsed) type_decl(&constructed_type, &ad, &unused_name, TYPE_ABSTRACT);
    while (macro_stack && macro_stack != parent) end_macro();
    macro_ptr = parent_ptr;
    tok = saved_tok;
    tokc = saved_tokc;
    return parsed && is_compatible_unqualified_types(type, &constructed_type);
  }
  if (class_tok_matches_unqualified_name(get_struct_type_name_tok(type),
                                          expression->str[first])
      && next_token <= end && expression->str[next_token] == '('
      && saved_initializer_close(expression, next_token, '(', ')') == end)
    return 1;
  {
    int index, angles = 0, call = -1, is_class_prvalue;
    CType result;
    /* A single call can have a qualified/member name and explicit template
       arguments. Reject operators and earlier calls before claiming storage;
       make().convert() must not give make() the final result object. */
    for (index = first; index <= end; ++index) {
      int token = expression->str[index];
      if (TOK_HAS_VALUE(token)) {
        if (!angles && token != TOK_LINENUM) return 0;
        index += tok_str_value_extra_words(expression->str, expression->len, index);
        continue;
      }
      if (!angles && token == '(') { call = index; break; }
      if (token == '<' || token == TOK_LT) ++angles;
      else if (token == TOK_GT && angles) --angles;
      else if ((token == TOK_SAR || token == TOK_SHR) && angles)
        angles = angles > 1 ? angles - 2 : 0;
      else if (!angles && token < TOK_UIDENT && token != ':'
               && token != '.' && token != TOK_ARROW) return 0;
    }
    if (call < 0 || saved_initializer_close(expression, call, '(', ')') != end)
      return 0;
    infer_saved_arg_type_and_category(expression, &result, &is_class_prvalue);
    if (!is_class_prvalue || is_reference_type(&result)) return 0;
    result.t &= ~VT_RVALUE_REFERENCE;
    return is_compatible_unqualified_types(type, &result);
  }
}

static int emit_class_initializer_expression_at(TokenString *expression,
                                                 CType *type,
                                                 unsigned long address,
                                                 int storage)
{
  TokenString macro, *parent;
  const int *parent_ptr;
  CppConstructionDestination destination, *saved_destination;
  int saved_tok, forwarding;
  CValue saved_tokc;
  prepare_saved_cpp_lambdas(expression);
  saved_tok = tok;
  saved_tokc = tokc;
  parent = macro_stack;
  parent_ptr = macro_ptr;
  saved_destination = cpp_construction_destination;
  destination.type = *type;
  destination.address = address;
  destination.storage = storage;
  destination.available = saved_initializer_is_direct_constructor(expression, type);
  forwarding = destination.available;
  destination.conditional_marker = cpp_prepare_conditional_destination(expression);
  cpp_construction_destination = &destination;
  macro = *expression;
  begin_macro(&macro, 0);
  next();
  parse_init_elem(EXPR_ANY);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  cpp_construction_destination = saved_destination;
  tok = saved_tok;
  tokc = saved_tokc;
  return forwarding && !destination.available;
}

static void emit_class_initializer_expression(TokenString *expression,
                                               CType *type,
                                               unsigned long address)
{
  emit_class_initializer_expression_at(expression, type, address,
                                        VT_LOCAL | VT_LVAL);
}

static int parse_direct_class_initializer(init_params *p, CType *type,
                                           unsigned long address)
{
  TokenString *expression;
  if (tok != '(' && tok != '[' && tok < TOK_UIDENT)
    return 0;
  /* Inspect the complete expression before forwarding storage. For example,
     in T value = T(1) + T(2), neither operand is value's result object. */
  skip_or_save_block_ex(&expression, 1);
  if (p->indirect_base)
  {
    int slot;
    init_push_lvalue(p, type, address);
    mk_pointer(&vtop->type); gaddrof();
    vtop->type = char_pointer_type;
    loc = (loc - PTR_SIZE) & -PTR_SIZE;
    slot = loc;
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, slot);
    vswap(); vstore(); vpop();
    p->elided_value = emit_class_initializer_expression_at(expression, type,
                         slot, VT_LLOCAL | VT_LVAL);
  }
  else
    emit_class_initializer_expression(expression, type, address);
  tok_str_free(expression);
  return 1;
}

static void emit_placement_aggregate_initializer(CType *type, TokenString *placement,
                                                 TokenString *initializer)
{
  init_params p = {0};
  TokenString macro = *initializer, *parent;
  const int *parent_ptr;
  int saved_tok = tok, alignment;
  CValue saved_tokc = tokc;
  Sym *old_cleanup = cur_scope->cl.s;
  int old_cleanup_depth = cur_scope->cl.n;
  CType pointer = *type;
  mk_pointer(&pointer);
  replay_saved_placement_expr(placement);
  gen_cast(&pointer);
  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  p.indirect_base = loc;
  p.local_offset = type_size(type, &alignment);
  p.track_partial_cleanups = 1;
  vdup();
  vset(&pointer, VT_LOCAL | VT_LVAL, p.indirect_base);
  vswap(); vstore(); vpop();
  parent = macro_stack;
  parent_ptr = macro_ptr;
  begin_macro(&macro, 0);
  next();
  decl_initializer(&p, type, 0, DIF_FIRST);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  cur_scope->cl.s = old_cleanup;
  cur_scope->cl.n = old_cleanup_depth;
  tok = saved_tok; tokc = saved_tokc;
}

/* 't' contains the type and storage info. 'c' is the offset of the
   object in section 'sec'. If 'sec' is NULL, it means stack based
   allocation. 'flags & DIF_FIRST' is true if array '{' must be read (multi
   dimension implicit array init handling). 'flags & DIF_SIZE_ONLY' is true if
   size only evaluation is wanted (only for arrays). */
static void decl_initializer(init_params *p, CType *type, unsigned long c, int flags)
{
  int len, n, no_oblock, i;
  int size1, align1;
  Sym *s, *f;
  Sym indexsym;
  CType *t1;
  Sym *old_cleanup = cur_scope ? cur_scope->cl.s : NULL;
  int old_cleanup_depth = cur_scope ? cur_scope->cl.n : 0;

  // Generate Line Number Info
  if (debug_modes && !(flags & DIF_SIZE_ONLY) && !p->sec)
    cprime_debug_line(cprime_state), cprime_tcov_check_line (cprime_state, 1);

  if (!(flags & DIF_HAVE_ELEM) && tok != '{' &&
      /* In case of strings we have special handling for arrays, so
         don't consume them as initializer value (which would commit them
         to some anonymous symbol).  */
      tok != TOK_LSTR && tok != TOK_STR &&
      (!(flags & DIF_SIZE_ONLY)
       /* a struct may be initialized from a struct of same type, as in
               struct {int x,y;} a = {1,2}, b = {3,4}, c[] = {a,b};
          In that case we need to parse the element in order to check
          it for compatibility below */
       || (type->t & VT_BTYPE) == VT_STRUCT)
     )
  {
    int ncw_prev = nocode_wanted;
    if ((flags & DIF_SIZE_ONLY) && !p->sec)
      ++nocode_wanted;
    if (!(!p->sec && !(flags & DIF_SIZE_ONLY)
          && is_cpp_translation_unit() && (type->t & VT_BTYPE) == VT_STRUCT
          && parse_direct_class_initializer(p, type, c)))
      parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
    nocode_wanted = ncw_prev;
    flags |= DIF_HAVE_ELEM;
  }

  if (find_cpp_member_pointer_type(type))
  {
    if (!(flags & DIF_HAVE_ELEM))
    {
      skip('{');
      if (tok == '}')
        vpushi(0);
      else
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      if (tok == ',') next();
      skip('}');
    }
    if (flags & DIF_SIZE_ONLY)
      vpop();
    else
    {
      convert_cpp_member_pointer(type, 0);
      init_putv(p, type, c);
    }
    return;
  }

  if (type->t & VT_ARRAY)
  {
    no_oblock = 1;
    if (((flags & DIF_FIRST) && tok != TOK_LSTR && tok != TOK_STR) ||
        tok == '{')
    {
      skip('{');
      no_oblock = 0;
    }

    s = type->ref;
    n = s->c;
    t1 = pointed_type(type);
    size1 = type_size(t1, &align1);

    /* only parse strings here if correct type (otherwise: handle
       them as ((w)char *) expressions */
    if ((tok == TOK_LSTR &&
#ifdef CPRIME_TARGET_PE
         (t1->t & VT_BTYPE) == VT_SHORT && (t1->t & VT_UNSIGNED)
#else
         (t1->t & VT_BTYPE) == VT_INT
#endif
        ) || (tok == TOK_STR && (t1->t & VT_BTYPE) == VT_BYTE))
    {
      len = 0;
      cstr_reset(&initstr);
      if (size1 != (tok == TOK_STR ? 1 : sizeof(nwchar_t)))
        cprime_error("unhandled string literal merging");
      while (tok == TOK_STR || tok == TOK_LSTR)
      {
        if (initstr.size)
          initstr.size -= size1;
        if (tok == TOK_STR)
          len += tokc.str.size;
        else
          len += tokc.str.size / sizeof(nwchar_t);
        len--;
        cstr_cat(&initstr, tokc.str.data, tokc.str.size);
        next();
      }
      if (tok != ')' && tok != '}' && tok != ',' && tok != ';'
          && tok != TOK_EOF)
      {
        // Not a lone literal but part of a bigger expression.
        unget_tok(size1 == 1 ? TOK_STR : TOK_LSTR);
        tokc.str.size = initstr.size;
        tokc.str.data = initstr.data;
        goto do_init_array;
      }

      decl_design_flex(p, s, len);
      if (!(flags & DIF_SIZE_ONLY))
      {
        int nb = n, ch;
        if (len < nb)
          nb = len;
        if (len > nb)
          cprime_warning("initializer-string for array is too long");
        /* in order to go faster for common case (char
           string in global variable, we handle it
           specifically */
        if (p->sec && size1 == 1)
        {
          init_assert(p, c + nb);
          if (!NODATA_WANTED)
            memcpy(p->sec->data + c, initstr.data, nb);
        }
        else
        {
          for (i = 0; i < n; i++)
          {
            if (i >= nb)
            {
              /* only add trailing zero if enough storage (no
                 warning in this case since it is standard) */
              if (flags & DIF_CLEAR)
                break;
              if (n - i >= 4)
              {
                init_putz(p, c + i *size1, (n - i) * size1);
                break;
              }
              ch = 0;
            }
            else if (size1 == 1)
              ch = ((unsigned char *)initstr.data)[i];
            else
              ch = ((nwchar_t *)initstr.data)[i];
            vpushi(ch);
            init_putv(p, t1, c + i *size1);
          }
        }
      }
    }
    else
    {

do_init_array:
      indexsym.c = 0;
      f = &indexsym;

do_init_list:
      // Zero Memory Once In Advance
      if (!(flags & (DIF_CLEAR | DIF_SIZE_ONLY)))
      {
        init_putz(p, c, n *size1);
        flags |= DIF_CLEAR;
      }

      len = 0;
      /* language extension: if the initializer is empty for a flex array,
         it's size is zero.  We won't enter the loop, so set the size
         now.  */
      decl_design_flex(p, s, len - 1);
      while (tok != '}' || (flags & DIF_HAVE_ELEM))
      {
        len = decl_designator(p, type, c, &f, flags, len);
        flags &= ~DIF_HAVE_ELEM;
        if (type->t & VT_ARRAY)
        {
          ++indexsym.c;
          /* special test for multi dimensional arrays (may not
             be strictly correct if designators are used at the
             same time) */
          if (no_oblock && len >= n * size1)
            break;
        }
        else
        {
          if (s->type.t == VT_UNION)
            f = NULL;
          else
            f = f->next;
          if (no_oblock && f == NULL)
            break;
        }

        if (tok == '}')
          break;
        skip(',');
      }
    }
    if (!no_oblock)
      skip('}');

  }
  else if ((flags & DIF_HAVE_ELEM)
           /* Use i_c_parameter_t, to strip toplevel qualifiers.
              The source type might have VT_CONSTANT set, which is
              of course assignable to non-const elements.  */
           && is_compatible_unqualified_types(type, &vtop->type))
  {
    /* Fall through to the scalar initializer path. */
    if ((flags & DIF_SIZE_ONLY))
    {
      if (flags & DIF_HAVE_ELEM)
        vpop();
      else
        skip_or_save_block(NULL);
    }
    else
    {
      if (!(flags & DIF_HAVE_ELEM))
      {
        /* This should happen only when we haven't parsed
           the init element above for fear of committing a
           string constant to memory too early.  */
        if (tok != TOK_STR && tok != TOK_LSTR)
          expect("string constant");
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      }
      if (p->capture_integral_constexpr
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
          && is_integer_btype(vtop->type.t & VT_BTYPE))
      {
        p->integral_constexpr_value = vtop->c.i;
        p->integral_constexpr_valid = 1;
      }
      if (!p->sec && (flags & DIF_CLEAR) // Container Was Already Zero'D
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
          && vtop->c.i == 0
          && btype_size(type->t & VT_BTYPE) // Not For Fp Constants
         )
        vpop();
      else
        init_putv(p, type, c);
    }
  }

  else if ((type->t & VT_BTYPE) == VT_STRUCT
           && type_is_std_initializer_list(type)
           && tok == '{')
  {
    Sym *field;
    TokenString *elements[32];
    CType *element_type;
    int element_size, element_align, element_addr = 0, element_r2 = 0;
    int i;

    len = count_saved_braced_ctor_args(elements, 32);
    if (!(flags & DIF_SIZE_ONLY))
    {
      field = type->ref->next;
      element_type = field ? pointed_type(&field->type) : NULL;
      if (!field || !element_type)
        cprime_error("initializer_list layout missing array field");
      element_size = type_size(element_type, &element_align);
      if (element_size < 0)
        cprime_error("initializer_list element has incomplete type");
      if (len)
      {
        init_params element_init = {0};
        SValue *element_storage;
        element_addr = get_temp_local_var(element_size * len, element_align,
                                          &element_r2);
        element_init.local_offset = element_addr + element_size * len;
        /* Keep the complete backing allocation live while each element is
           materialized.  Struct element expressions can allocate their own
           temporaries; without this marker the temp allocator reuses and
           overwrites the initializer_list's first slot. */
        vset(element_type, VT_LOCAL | VT_LVAL, element_addr);
        vtop->r2 = element_r2;
        element_storage = vtop;
        for (i = 0; i < len; ++i)
        {
          TokenString macro = *elements[i];
          int saved_tok = tok;
          CValue saved_tokc = tokc;
          TokenString *saved_macro_stack = macro_stack;
          const int *saved_macro_ptr = macro_ptr;
          begin_macro(&macro, 0);
          next();
          decl_initializer(&element_init, element_type,
                           element_addr + i * element_size, DIF_FIRST);
          while (macro_stack && macro_stack != saved_macro_stack)
            end_macro();
          macro_ptr = saved_macro_ptr;
          tok = saved_tok;
          tokc = saved_tokc;
          if (vtop != element_storage)
            cprime_internal_error(
              "unbalanced initializer_list element initializer");
        }
        mk_pointer(&vtop->type);
        gaddrof();
      }
      else
        vpushi(0);
      init_putv(p, &field->type, c + field->c);
      field = field->next;
      if (!field)
        cprime_error("initializer_list layout missing length field");
      vpushi(len);
      init_putv(p, &field->type, c + field->c);
    }
    for (i = 0; i < len; ++i)
      tok_str_free(elements[i]);
  }
  else if ((type->t & VT_BTYPE) == VT_STRUCT)
  {
    no_oblock = 1;
    if ((flags & DIF_FIRST) || tok == '{')
    {
      skip('{');
      no_oblock = 0;
    }
    s = type->ref;
    f = s->next;
    n = s->c;
    size1 = 1;
    goto do_init_list;

  }
  else if (tok == '{')
  {
    if (flags & DIF_HAVE_ELEM)
      skip(';');
    next();
    decl_initializer(p, type, c, flags & ~DIF_HAVE_ELEM);
    skip('}');

  }
  else
  {
    /* If we supported only ISO C we wouldn't have to accept calling
       this on anything than an array if DIF_SIZE_ONLY (and even then
       only on the outermost level, so no recursion would be needed),
       because initializing a flex array member isn't supported.
       But the language supports it, so we need to recurse even into
       subfields of structs and arrays when DIF_SIZE_ONLY is set.  */
    if ((flags & DIF_SIZE_ONLY))
    {
      // Just Skip Expression
      if (flags & DIF_HAVE_ELEM)
        vpop();
      else
        skip_or_save_block(NULL);
    }
    else
    {
      if (!(flags & DIF_HAVE_ELEM))
      {
        /* This should happen only when we haven't parsed
           the init element above for fear of committing a
           string constant to memory too early.  */
        if (tok != TOK_STR && tok != TOK_LSTR)
          expect("string constant");
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      }
      if (!p->sec && (flags & DIF_CLEAR) // Container Was Already Zero'D
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
          && vtop->c.i == 0
          && btype_size(type->t & VT_BTYPE) // Not For Fp Constants
         )
        vpop();
      else
        init_putv(p, type, c);
    }
  }
  if (p->track_partial_cleanups && !(flags & DIF_SIZE_ONLY)
      && (type->t & VT_BTYPE) == VT_STRUCT && !nocode_wanted)
  {
    Sym *destructor = resolve_autodtor_func(type);
    if (destructor)
    {
      int pointer_slot;
      /* Replace completed subobject cleanups with the complete object's
         destructor. While later fields initialize, an exception unwinds only
         those subobjects whose initialization has already succeeded. */
      cur_scope->cl.s = old_cleanup;
      cur_scope->cl.n = old_cleanup_depth;
      loc = (loc - PTR_SIZE) & -PTR_SIZE;
      pointer_slot = loc;
      init_push_lvalue(p, type, c);
      mk_pointer(&vtop->type); gaddrof();
      vtop->type = char_pointer_type;
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
      vswap(); vstore(); vpop();
      cpp_eh_register_parameter_copy(type, VT_LLOCAL | VT_LVAL, pointer_slot);
      if (cur_scope->cl.s != old_cleanup)
        cur_scope->cl.s->a.cleanup_exception_only = 1;
    }
  }
}

static void emit_automatic_class_array_initialization(Sym *symbol,
                                                      TokenString *initializer)
{
  CType element = symbol->type;
  TokenString *body = tok_str_alloc(), *path = tok_str_alloc(), *parent;
  const int *parent_ptr;
  Sym *destructor;
  int counter_tok = 0, saved_tok = tok;
  CValue saved_tokc = tokc;
  while ((element.t & VT_ARRAY) && element.ref)
    element = *pointed_type(&element);
  destructor = resolve_autodtor_func(&element);
  if (destructor && !nocode_wanted)
  {
    char name[64];
    CType count_type = {VT_LLONG | VT_UNSIGNED, NULL};
    int pointer_slot, context, alignment;
    loc = (loc - PTR_SIZE) & -PTR_SIZE;
    pointer_slot = loc;
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
    vset(&symbol->type, symbol->r, symbol->c);
    mk_pointer(&vtop->type); gaddrof();
    vtop->type = char_pointer_type;
    vstore(); vpop();
    cpp_eh_register_array_cleanup(pointer_slot, 0,
        type_size(&element, &alignment), destructor, &context);
    snprintf(name, sizeof(name), "__cpc_array_constructed_%u", anon_sym++);
    counter_tok = tok_alloc_const(name);
    sym_push(counter_tok, &count_type, VT_LOCAL | VT_LVAL,
               context + offsetof(CpcEhArray, count));
  }
  tok_str_add(body, '{');
  tok_str_add(path, symbol->v);
  append_counted_class_initialization(body, &symbol->type, path, initializer,
                                       counter_tok);
  tok_str_add(body, '}');
  tok_str_add(body, TOK_EOF);
  tok_str_free(path);
  parent = macro_stack; parent_ptr = macro_ptr;
  begin_macro(body, 1);
  next(); block(STMT_COMPOUND);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  tok = saved_tok; tokc = saved_tokc;
}

/* parse an initializer for type 't' if 'has_init' is non zero, and
   allocate space in local or global data space ('r' is either
   VT_LOCAL or VT_CONST). If 'v' is non zero, then an associated
   variable 'v' of scope 'scope' is declared before initializers
   are parsed. If 'v' is zero, then a reference to the new object
   is put in the value stack. If 'has_init' is 2, a special parsing
   is done to handle string constants. */
static void decl_initializer_alloc(CType *type, AttributeDef *ad, int r,
                                   int has_init, int has_ctor_init,
                                   TokenString *copy_ctor_init, int v, int decl_scope)
{
  int size, align, addr;
  TokenString *init_str = NULL;
  TokenString *class_array_initializer = NULL;
  Sym *ctor_func = NULL;
  int has_expr_struct_init, initialize_class_array;
  int initialize_nonlocal_vtables = 0;

  Section *sec;
  Sym *flexible_array;
  Sym *sym = NULL;
  int saved_nocode_wanted = nocode_wanted;
  int temporary_mark = cpp_temp_mark();
#ifdef CONFIG_CPRIME_BCHECK
  int bcheck = cprime_state->do_bounds_check && !NODATA_WANTED;
#endif
  init_params p = {0};
  initialize_class_array = is_cpp_translation_unit() && v
      && (r & VT_VALMASK) == VT_LOCAL && (type->t & VT_ARRAY)
      && !(type->t & VT_VLA)
      && (type_requires_class_construction(type)
          || type_requires_class_destruction(type));
  if (initialize_class_array && has_init)
  {
    skip_or_save_block(&class_array_initializer);
    complete_global_class_array_bound(type, class_array_initializer);
    has_init = 0;
  }
  has_expr_struct_init = has_init
                         && (type->t & VT_BTYPE) == VT_STRUCT
                         && (tok != '{' || type_is_std_initializer_list(type));

  if (decl_scope == VT_CONST)
  {
    // see if a global symbol was already defined
    sym = sym_find(v);
    if (sym)
    {
      patch_storage(sym, ad, type);
      // We Accept Several Definitions Of The Same Global Variable.
      if (!has_init && sym->c && elfsym(sym)->st_shndx != SHN_UNDEF)
        return;
      type = &sym->type;
    }
  }

  // Always allocate static or global variables
  if (v && (r & VT_VALMASK) == VT_CONST)
    nocode_wanted |= DATA_ONLY_WANTED;

  flexible_array = NULL;
  size = type_size(type, &align);
  if (size < 0 && (type->t & VT_BTYPE) == VT_STRUCT
      && materialize_incomplete_template_type(type))
    size = type_size(type, &align);

  /* exactly one flexible array may be initialized, either the
     toplevel array or the last member of the toplevel struct */

  if (size < 0)
  {
    // Error Out Except For Top-Level Incomplete Arrays
    // (Arrays Of Incomplete Types Are Handled In Array Parsing)
    if (!(type->t & VT_ARRAY))
      cprime_error("initialization of incomplete type");
    /* If the base type itself was an array type of unspecified
       size (like in 'typedef int arr[]; arr x = {1};') then
       we will overwrite the unknown size by the real one for
       this decl.  We need to unshare the ref symbol holding
       that size.  */
    if (IS_BT_ARRAY(type->t))
      type->ref = sym_push(SYM_FIELD, &type->ref->type, 0, type->ref->c);
    p.flex_array_ref = type->ref;

  }
  else if (has_init && (type->t & VT_BTYPE) == VT_STRUCT)
  {
    Sym *field = type->ref->next;
    if (field)
    {
      while (field->next)
        field = field->next;
      if (field->type.t & VT_ARRAY && field->type.ref->c < 0)
      {
        flexible_array = field;
        p.flex_array_ref = field->type.ref;
        size = -1;
      }
    }
  }

  if (size < 0)
  {
    // If unknown size, do a dry-run 1st pass
    if (!has_init)
      goto err_size;
    if (has_init == 2)
    {
      // Only Get Strings
      init_str = tok_str_alloc();
      while (tok == TOK_STR || tok == TOK_LSTR)
      {
        tok_str_add_tok(init_str);
        next();
      }
      tok_str_add(init_str, TOK_EOF);
    }
    else
      skip_or_save_block(&init_str);
    unget_tok(0);

    // Compute Size
    begin_macro(init_str, 1);
    next();
    decl_initializer(&p, type, 0, DIF_FIRST | DIF_SIZE_ONLY);
    // Prepare Second Initializer Parsing
    macro_ptr = init_str->str;
    next();

    // if still unknown size, error
    size = type_size(type, &align);
    if (size < 0)
err_size:
      cprime_error("unknown type size");

    /* If there's a flex member and it was used in the initializer
       adjust size.  */
    if (flexible_array && flexible_array->type.ref->c > 0)
      size += flexible_array->type.ref->c
              * pointed_size(&flexible_array->type);
  }

  // take into account specified alignment if bigger
  if (ad->a.aligned)
  {
    int speca = 1 << (ad->a.aligned - 1);
    if (speca > align)
      align = speca;
  }
  else if (ad->a.packed)
    align = 1;

  if (!v && NODATA_WANTED)
    size = 0, align = 1;

  if ((r & VT_VALMASK) == VT_LOCAL)
  {
    sec = NULL;
#ifdef CONFIG_CPRIME_BCHECK
    if (bcheck && v)
    {
      // Add Padding Between Stack Variables For Bound Checking
      loc -= align;
    }
#endif
    loc = (loc - size) & -align;
    addr = loc;
    p.local_offset = addr + size;
#ifdef CONFIG_CPRIME_BCHECK
    if (bcheck && v)
    {
      // Add Padding Between Stack Variables For Bound Checking
      loc -= align;
    }
#endif
    if (v)
    {
      // Local Variable
#ifdef CONFIG_CPRIME_ASM
      if (ad->asm_label)
      {
        int reg = asm_parse_regvar(ad->asm_label);
        if (reg >= 0)
          r = (r & ~VT_VALMASK) | reg;
      }
#endif
      sym = sym_push(v, type, r, addr);
      if (!NODATA_WANTED)
        cpp_eh_mark_local_initializing(sym);
      if (!ad->cleanup_func)
        ad->cleanup_func = resolve_autodtor_func(type);
      if (ad->cleanup_func)
      {
        Sym *cls = sym_push2(&all_cleanups,
                             SYM_FIELD | ++cur_scope->cl.n, 0, 0);
        cls->cleanup_sym = sym;
        cls->cleanup_func = ad->cleanup_func;
        cls->next = cur_scope->cl.s;
        cur_scope->cl.s = cls;
      }
      else if ((type->t & VT_BTYPE) == VT_STRUCT)
      {
        register_struct_cleanups(type, sym);
      }

      sym->a = ad->a;
    }
    else
    {
      // Push Local Reference
      vset(type, r, addr);
    }
  }
  else
  {
    // Allocate Symbol In Corresponding Section
    sec = ad->section;
    if (!sec)
    {
      CType *tp = type;
      while ((tp->t & (VT_BTYPE | VT_ARRAY)) == (VT_PTR | VT_ARRAY))
        tp = &tp->ref->type;
      if (!has_init && type_requires_class_construction(type))
        sec = bss_section;
      else if (tp->t & VT_CONSTANT)
        sec = rodata_section;
      else if (has_init)
      {
        sec = data_section;
        /*if (g_debug & 4)
            cprime_warning("rw data: %s", get_tok_str(v, 0));*/
      }
      else if (cprime_state->nocommon)
        sec = bss_section;
    }

    if (sec)
    {
      addr = section_add(sec, size, align);
#ifdef CONFIG_CPRIME_BCHECK
      // add padding if bound check
      if (bcheck)
        section_add(sec, 1, 1);
#endif
    }
    else
    {
      addr = align; // SHN_COMMON is special, symbol value is align
      sec = common_section;
    }

    if (v)
    {
      if (!sym)
      {
        sym = sym_push(v, type, r | VT_SYM, 0);
        patch_storage(sym, ad, NULL);
      }
      // Update Symbol Definition
      put_extern_sym(sym, sec, addr, size);
    }
    else
    {
      // Push Global Reference
      vpush_ref(type, sec, addr, size);
      sym = vtop->sym;
      vtop->r |= r;
    }

#ifdef CONFIG_CPRIME_BCHECK
    /* handles bounds now because the symbol must be defined
       before for the relocation */
    if (bcheck)
    {
      addr_t *bounds_ptr;

      greloca(bounds_section, sym, bounds_section->data_offset, R_DATA_PTR, 0);
      // Then Add Global Bound Info
      bounds_ptr = section_ptr_add(bounds_section, 2 * sizeof(addr_t));
      bounds_ptr[0] = 0; // Relocated
      bounds_ptr[1] = size;
    }
#endif
  }

  if (type->t & VT_VLA)
  {
    int a;

    if (has_init)
      cprime_error("variable length array cannot be initialized");

    if (NODATA_WANTED)
      goto no_alloc;

    // save before-VLA stack pointer if needed
    if (cur_scope->vla.num == 0)
    {
      if (cur_scope->prev && cur_scope->prev->vla.num)
        cur_scope->vla.locorig = cur_scope->prev->vla.loc;
      else
      {
        gen_vla_sp_save(loc -= PTR_SIZE);
        cur_scope->vla.locorig = loc;
      }
    }

    vpush_type_size(type, &a);
    gen_vla_alloc(type, a);
#if defined CPRIME_TARGET_PE && defined CPRIME_TARGET_X86_64
    /* on _WIN64, because of the function args scratch area, the
       result of alloca differs from RSP and is returned in RAX.  */
    gen_vla_result(addr), addr = (loc -= PTR_SIZE);
#endif
    gen_vla_sp_save(addr);
    cur_scope->vla.loc = addr;
    cur_scope->vla.num++;
  }
  else if (has_init)
  {
    p.sec = sec;
    p.capture_integral_constexpr = is_cpp_translation_unit() && sym
                                   && (type->t & VT_CONSTANT)
                                   && is_integer_btype(type->t & VT_BTYPE)
                                   && !(type->t & (VT_ARRAY | VT_VLA));
    decl_initializer(&p, type, addr, DIF_FIRST);
    if (p.capture_integral_constexpr && p.integral_constexpr_valid)
    {
      uint64_t raw = (uint64_t)p.integral_constexpr_value;
      if (type->t & VT_UNSIGNED)
        sym->const_value = (long long)raw;
      else if (size == 1)
        sym->const_value = (int8_t)raw;
      else if (size == 2)
        sym->const_value = (int16_t)raw;
      else if (size == 4)
        sym->const_value = (int32_t)raw;
      else
        sym->const_value = (int64_t)raw;
      sym->a.integral_constexpr = 1;
    }
    else if (is_cpp_translation_unit() && sym && decl_scope == VT_CONST
             && (type->t & VT_CONSTANT)
             && is_integer_btype(type->t & VT_BTYPE)
             && !(type->t & (VT_ARRAY | VT_VLA))
             && sec && sec->data && size > 0 && size <= 8)
    {
      uint64_t raw = 0;
      memcpy(&raw, sec->data + addr, size);
      if (type->t & VT_UNSIGNED)
        sym->const_value = (long long)raw;
      else if (size == 1)
        sym->const_value = (int8_t)raw;
      else if (size == 2)
        sym->const_value = (int16_t)raw;
      else if (size == 4)
        sym->const_value = (int32_t)raw;
      else
        sym->const_value = (int64_t)raw;
      sym->a.integral_constexpr = 1;
    }
    // Patch Flexible Array Member Size Back To -1,
    // For Possible Subsequent Similar Declarations
    if (flexible_array)
      flexible_array->type.ref->c = -1;
  }

  if ((r & VT_VALMASK) == VT_LOCAL && sym && !NODATA_WANTED && !(type->t & VT_VLA))
  {
    if (has_ctor_init)
      ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
    else
      ctor_func = resolve_autoctor_func(type);
    if (initialize_class_array)
      emit_automatic_class_array_initialization(sym, class_array_initializer);
    else if (copy_ctor_init)
    {
      TokenString *call_args[1];
      prepare_saved_cpp_lambdas(copy_ctor_init);
      if (saved_initializer_is_direct_constructor(copy_ctor_init, type))
      {
        emit_class_initializer_expression(copy_ctor_init, type, sym->c);
        init_putv(&p, type, sym->c);
      }
      else
      {
        call_args[0] = copy_ctor_init;
        if (!ctor_func)
          ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
        call_lifecycle_constructor_saved_args(&sym->type, sym->r, sym->c,
                                              VT_CONST, sym, ctor_func,
                                              call_args, 1);
      }
    }
    else if (has_ctor_init)
      call_lifecycle_constructor(&sym->type, sym->r, sym->c, sym, ctor_func);
    else if (!copy_ctor_init)
    {
      if (!has_expr_struct_init)
        call_lifecycle_constructor_members(type, sym->r, sym->c);
      if (ctor_func && !has_expr_struct_init)
        call_lifecycle_constructor_noargs(&sym->type, sym->r, sym->c,
                                          sym, ctor_func);
    }
    /* Constructors run base initialization first, so publish the most-derived
       dispatch tables after construction.  This also covers implicit trivial
       constructors, which otherwise have no generated body in which to set a
       vptr. */
    initialize_virtual_tables_for_object(&sym->type, sym->r, sym->c, sym);
  }
  else if (sym && !NODATA_WANTED && !(type->t & VT_VLA)
           && cur_text_section)
  {
    /* Function-local static and thread-local objects live in a data section,
       but their declaration still executes in function code.  Refreshing
       their most-derived vptrs here is idempotent and covers objects whose
       otherwise-trivial construction needs no emitted constructor call. */
    initialize_nonlocal_vtables = 1;
  }

no_alloc:
  // restore parse state if needed
  if (init_str)
  {
    end_macro();
    next();
  }

  nocode_wanted = saved_nocode_wanted;
  if (initialize_nonlocal_vtables)
    initialize_virtual_tables_for_object(&sym->type, sym->r, sym->c, sym);
  if (sym && (r & VT_VALMASK) == VT_LOCAL && !NODATA_WANTED)
    cpp_eh_finish_local_initialization(sym);
  cpp_temp_end(temporary_mark);
  if (class_array_initializer) tok_str_free(class_array_initializer);
}

// Generate Vla Code Saved In Post_Type()
static void func_vla_arg_code(Sym *arg)
{
  int align;
  TokenString *vla_array_tok = NULL;

  if (arg->type.ref)
    func_vla_arg_code(arg->type.ref);

  if ((arg->type.t & VT_VLA) && arg->type.ref->vla_array_str)
  {
    loc -= type_size(&int_type, &align);
    loc &= -align;
    arg->type.ref->c = loc;

    unget_tok(0);
    vla_array_tok = tok_str_alloc();
    vla_array_tok->str = arg->type.ref->vla_array_str;
    begin_macro(vla_array_tok, 1);
    next();
    gexpr();
    end_macro();
    next();
    vpush_type_size(&arg->type.ref->type, &align);
    gen_op('*');
    vset(&int_type, VT_LOCAL | VT_LVAL, arg->type.ref->c);
    vswap();
    vstore();
    vpop();
  }
}

static void func_vla_arg(Sym *sym)
{
  Sym *arg;

  for (arg = sym->type.ref->next; arg; arg = arg->next)
    if ((arg->type.t & VT_BTYPE) == VT_PTR && (arg->type.ref->type.t & VT_VLA))
      func_vla_arg_code(arg->type.ref);
}

// Set The Local Stack Address For Function Parameter From Gfunc_Prolog()
ST_FUNC Sym *gfunc_set_param(Sym *s, int c, int byref)
{
  s = sym_find(s->v);
  if (!s) // Unnamed Parameters, Not Enabled
    return NULL;
  s->c = c;
  if (byref)
    s->r = VT_LLOCAL | VT_LVAL; // otherwise VT_LOCAL
  return s;
}

/* Publish parameter-scope types, then the actual formal parameter list.
   Deferred template work can allocate unrelated symbols between the function
   prototype and its formals. Allocation order is not parameter ownership. */
static void sym_push_params(Sym *ref)
{
  Sym *s = ref;
  while (s->next)
    s = s->next;
  while (s && s != ref)
  {
    if ((s->sym_scope && sym_is_local_type_binding(s))
        || IS_ENUM_VAL(s->type.t))
      sym_copy(s, &local_stack);
    s = s->prev;
  }
  for (s = ref->next; s; s = s->next)
    if (s->v & ~SYM_FIELD)
      sym_copy(s, &local_stack);
}

/* parse a function defined by symbol 'sym' and generate its code in
   'cur_text_section' */
