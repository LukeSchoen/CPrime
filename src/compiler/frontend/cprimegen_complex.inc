/* -------------------------------------------------------------------------
   _Complex / __complex__ support.

   A complex type is modelled as an aggregate whose members are the real part
   followed by the imaginary part, in the same way the GNU vector extensions
   are modelled as a struct holding one array member.  Aggregate copies,
   sizeof, parameter passing and function results then reuse the ordinary
   struct paths, and the arithmetic below lowers every operator to
   element-wise operations on the two parts.

   The element types are canonicalized per real type so that `_Complex double`
   spelled anywhere names the same type, exactly as template argument
   identity requires. */

typedef struct ComplexTypeEntry {
  int key;
  int size;
  Sym *sym;
  struct ComplexTypeEntry *next;
} ComplexTypeEntry;

static ComplexTypeEntry *complex_type_registry;

static void free_complex_type_registry(void)
{
  ComplexTypeEntry *entry;
  while ((entry = complex_type_registry) != NULL)
  {
    complex_type_registry = entry->next;
    cprime_free(entry);
  }
}

/* True for the aggregate types built by make_complex_type(). */
static int is_complex_type(const CType *type)
{
  return (type->t & VT_BTYPE) == VT_STRUCT && type->ref
         && type->ref->a.complex;
}

/* Element type of a complex type. */
static CType complex_element_type(const CType *type)
{
  CType elem = *pointed_type(&type->ref->next->type);
  elem.ref = NULL;
  return elem;
}

/* Rewrite 'type', currently the element type, into the complex type of that
   element.  A non-arithmetic element leaves the type alone. */
static void make_complex_type(CType *type)
{
  CType elem, self, array;
  ComplexTypeEntry *entry;
  Sym *fields = NULL, *member, *s;
  int esize, align, v, key, keep;

  /* Everything the element type does not own survives the rewrite: storage
     class, typedef marking and top-level qualifiers included. */
  keep = type->t & ~(VT_BTYPE | VT_LONG | VT_UNSIGNED | VT_DEFSIGN
                     | VT_ARRAY | VT_VLA | VT_BT_ARRAY);
  elem = *type;
  elem.ref = NULL;
  elem.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_ARRAY | VT_VLA | VT_BT_ARRAY);
  switch (elem.t & VT_BTYPE)
  {
  case VT_BYTE: case VT_SHORT: case VT_INT: case VT_LLONG:
  case VT_BOOL: case VT_FLOAT: case VT_DOUBLE: case VT_LDOUBLE:
    break;
  default:
    return;
  }
  esize = type_size(&elem, &align);
  if (esize <= 0)
    return;
  key = elem.t & (VT_BTYPE | VT_UNSIGNED | VT_LONG);
  for (entry = complex_type_registry; entry; entry = entry->next)
    if (entry->key == key && entry->size == esize)
    {
      type->t = entry->sym->type.t | keep;
      type->ref = entry->sym;
      return;
    }

  /* Complex types are canonical and must outlive the scope that first spelled
     them, so build every record on the global stack under a stable tag name
     the native mangler can use. */
  {
    char name[64];
    Sym *saved_local = local_stack;
    const char *elem_name;
    switch (elem.t & VT_BTYPE)
    {
    case VT_BYTE: elem_name = "char"; break;
    case VT_SHORT: elem_name = "short"; break;
    case VT_INT: elem_name = "int"; break;
    case VT_LLONG: elem_name = "llong"; break;
    case VT_BOOL: elem_name = "bool"; break;
    case VT_FLOAT: elem_name = "float"; break;
    default:
      elem_name = (elem.t & VT_LONG) ? "ldouble" : "double";
      break;
    }
    snprintf(name, sizeof name, "__cpc_complex_%s%s%d",
             (elem.t & VT_UNSIGNED) ? "u" : "", elem_name, esize);
    local_stack = NULL;
    array = elem;
    mk_pointer(&array);
    array.t |= VT_ARRAY;
    array.ref->c = 2;
    member = sym_push2(&fields, SYM_FIELD, array.t, 0);
    member->type.ref = array.ref;

    v = tok_alloc_const(name);
    self.t = VT_STRUCT;
    self.ref = NULL;
    s = sym_push(v | SYM_STRUCT, &self, 0, 2 * esize);
    local_stack = saved_local;
  }
  s->a.complex = 1;
  s->r = align;
  s->next = member;
  type->t = s->type.t | keep;
  type->ref = s;

  entry = (ComplexTypeEntry *)cprime_malloc(sizeof *entry);
  entry->key = key;
  entry->size = esize;
  entry->sym = s;
  entry->next = complex_type_registry;
  complex_type_registry = entry;
}

/* Long double as the parser represents it on this target. */
static CType complex_long_double_type(void)
{
  CType type;
  memset(&type, 0, sizeof type);
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
  type.t = VT_DOUBLE | VT_LONG;
#else
  type.t = VT_LDOUBLE;
#endif
  return type;
}

/* Integer conversion rank of a complex element type ([C11] 6.3.1.1). */
static int complex_integer_rank(CType *type)
{
  switch (type->t & VT_BTYPE)
  {
  case VT_LLONG: return (type->t & VT_LONG) ? 4 : 3;
  case VT_INT: return (type->t & VT_LONG) ? 2 : 1;
  case VT_SHORT: return 0;
  default: return -1;   /* char and bool promote to int */
  }
}

/* Element type a complex operation computes in.  Two complex operands keep
   the wider element type; a scalar operand is converted to the complex
   operand's element type ([C11] 6.3.1.8). */
static CType complex_arithmetic_element(CType *a, CType *b)
{
  CType result;
  int ba = a->t & VT_BTYPE, bb = b->t & VT_BTYPE;
  int long_a = ba == VT_LDOUBLE || (ba == VT_DOUBLE && (a->t & VT_LONG));
  int long_b = bb == VT_LDOUBLE || (bb == VT_DOUBLE && (b->t & VT_LONG));

  if (long_a || long_b)
    return complex_long_double_type();
  memset(&result, 0, sizeof result);
  if (ba == VT_DOUBLE || bb == VT_DOUBLE)
    result.t = VT_DOUBLE;
  else if (ba == VT_FLOAT || bb == VT_FLOAT)
    result.t = VT_FLOAT;
  else
  {
    int rank_a = complex_integer_rank(a), rank_b = complex_integer_rank(b);
    if (rank_a > rank_b)
      result.t = a->t & (VT_BTYPE | VT_LONG | VT_UNSIGNED);
    else if (rank_b > rank_a)
      result.t = b->t & (VT_BTYPE | VT_LONG | VT_UNSIGNED);
    else
      result.t = (a->t & (VT_BTYPE | VT_LONG))
                 | ((a->t | b->t) & VT_UNSIGNED);
    if (result.t == VT_BOOL || (result.t & VT_BTYPE) == VT_BYTE
        || (result.t & VT_BTYPE) == VT_SHORT)
      result.t = VT_INT | (result.t & VT_UNSIGNED);  /* integer promotion */
  }
  return result;
}

/* Store the value in vtop into a fresh local temporary of 'type' and return
   its stack offset.  The value is consumed. */
static int complex_store_temp(CType *type)
{
  CType tmp = *type;
  int size, align, offset;
  tmp.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_REFERENCE | VT_RVALUE_REFERENCE);
  size = type_size(&tmp, &align);
  if (size < 0)
    cprime_error("invalid complex operand");
  loc = (loc - size) & -align;
  offset = loc;
  vset(&tmp, VT_LOCAL | VT_LVAL, offset);
  vswap();
  vstore();
  vpop();
  return offset;
}

/* Push the real (IMAG == 0) or imaginary part of an operand spilled at
   'offset'. */
static void complex_push_part(int offset, int esize, int imag, CType *elem)
{
  vset(elem, VT_LOCAL | VT_LVAL, offset + (imag ? esize : 0));
}

/* Store the value in vtop into the local temporary at 'offset', consuming
   it. */
static void complex_store_part(CType *elem, int offset)
{
  vset(elem, VT_LOCAL | VT_LVAL, offset);
  vswap();
  vstore();
  vpop();
}

static long double complex_static_part(CValue *value, int type)
{
  if ((type & VT_BTYPE) == VT_FLOAT) return value->f;
  if ((type & VT_BTYPE) == VT_DOUBLE) return value->d;
  if ((type & VT_BTYPE) == VT_LDOUBLE) return value->ld;
  return value->i;
}

static void complex_static_set_part(CValue *value, int type, long double part)
{
  memset(value, 0, sizeof *value);
  if ((type & VT_BTYPE) == VT_FLOAT) value->f = (float)part;
  else if ((type & VT_BTYPE) == VT_DOUBLE) value->d = (double)part;
  else if ((type & VT_BTYPE) == VT_LDOUBLE) value->ld = part;
  else value->i = (int64_t)part;
}

/* Apply a built-in operator to complex operands.  A scalar operand is first
   converted to the complex operand's type, so only whole complex values reach
   the formulas.  On return the result is on top of the value stack: a complex
   lvalue for arithmetic, or a scalar 1/0 for the equality operators. */
static void gen_complex_binop(int op)
{
  CType left = vtop[-1].type, right = vtop->type;
  CType ct, elem;
  int lc = is_complex_type(&left), rc = is_complex_type(&right);
  int esize, ealign, csize, calign;
  int loff = 0, roff = 0, res = 0, den = 0;
  int comparison = op == TOK_EQ || op == TOK_NE;

  /* The operation's element type comes from the usual arithmetic conversions
     over the two operands' real types: a scalar operand's own type stands in
     for the corresponding real type of a complex one ([C11] 6.3.1.8). */
  {
    CType lelem = lc ? complex_element_type(&left) : left;
    CType relem = rc ? complex_element_type(&right) : right;
    elem = complex_arithmetic_element(&lelem, &relem);
  }

  make_complex_type(&elem);
  ct = elem;
  if (!is_complex_type(&ct))
    cprime_error("invalid complex element type");
  elem = complex_element_type(&ct);
  esize = type_size(&elem, &ealign);
  csize = type_size(&ct, &calign);

  if (CONST_WANTED || static_initializer_constant_fold)
  {
    CValue real, imaginary;
    long double ar, ai, br, bi;
    gen_cast(&ct); vswap(); gen_cast(&ct); vswap();
    if (vtop[-1].complex_constant && vtop->complex_constant) {
      ar = complex_static_part(&vtop[-1].c, elem.t);
      ai = complex_static_part(&vtop[-1].complex_imaginary, elem.t);
      br = complex_static_part(&vtop->c, elem.t);
      bi = complex_static_part(&vtop->complex_imaginary, elem.t);
      if (op == '+') { ar += br; ai += bi; }
      else if (op == '-') { ar -= br; ai -= bi; }
      else if (op == '*') { long double r = ar * br - ai * bi; ai = ar * bi + ai * br; ar = r; }
      else goto static_complex_runtime;
      complex_static_set_part(&real, elem.t, ar);
      complex_static_set_part(&imaginary, elem.t, ai);
      vtop--; vtop->type = ct; vtop->r = vtop->r2 = VT_CONST;
      vtop->c = real; vtop->sym = NULL; vtop->complex_constant = 1;
      vtop->complex_imaginary = imaginary;
      return;
    }
  }
static_complex_runtime:

  switch (op)
  {
  case '+': case '-': case '*': case '/':
  case TOK_EQ: case TOK_NE:
    break;
  default:
    cprime_error("invalid operand types for binary operation");
  }

  /* Bring both operands to the operation's type and spill them.  A scalar
     operand is converted to a complex value whose imaginary part is zero. */
  gen_cast(&ct);
  roff = complex_store_temp(&ct);
  gen_cast(&ct);
  loff = complex_store_temp(&ct);

  if (comparison)
  {
    complex_push_part(loff, esize, 0, &elem);
    complex_push_part(roff, esize, 0, &elem);
    gen_op(TOK_EQ);
    complex_push_part(loff, esize, 1, &elem);
    complex_push_part(roff, esize, 1, &elem);
    gen_op(TOK_EQ);
    gen_op('&');
    if (op == TOK_NE)
    {
      vpushi(0);
      gen_op(TOK_EQ);
    }
    vtop->type.t = is_cpp_translation_unit() ? VT_BOOL : VT_INT;
    return;
  }

  loc = (loc - csize) & -calign;
  res = loc;
  if (op == '/')
  {
    loc = (loc - esize) & -ealign;
    den = loc;
  }

  switch (op)
  {
  case '+':
  case '-':
    complex_push_part(loff, esize, 0, &elem);
    complex_push_part(roff, esize, 0, &elem);
    gen_op(op);
    complex_store_part(&elem, res);
    complex_push_part(loff, esize, 1, &elem);
    complex_push_part(roff, esize, 1, &elem);
    gen_op(op);
    complex_store_part(&elem, res + esize);
    break;
  case '*':
    /* (a+bi)(c+di) = (ac-bd) + (ad+bc)i */
    complex_push_part(loff, esize, 0, &elem);
    complex_push_part(roff, esize, 0, &elem);
    gen_op('*');
    complex_push_part(loff, esize, 1, &elem);
    complex_push_part(roff, esize, 1, &elem);
    gen_op('*');
    gen_op('-');
    complex_store_part(&elem, res);
    complex_push_part(loff, esize, 0, &elem);
    complex_push_part(roff, esize, 1, &elem);
    gen_op('*');
    complex_push_part(loff, esize, 1, &elem);
    complex_push_part(roff, esize, 0, &elem);
    gen_op('*');
    gen_op('+');
    complex_store_part(&elem, res + esize);
    break;
  case '/':
    /* (a+bi)/(c+di) = ((ac+bd) + (bc-ad)i) / (c*c + d*d) */
    complex_push_part(roff, esize, 0, &elem);
    complex_push_part(roff, esize, 0, &elem);
    gen_op('*');
    complex_push_part(roff, esize, 1, &elem);
    complex_push_part(roff, esize, 1, &elem);
    gen_op('*');
    gen_op('+');
    complex_store_part(&elem, den);

    complex_push_part(loff, esize, 0, &elem);
    complex_push_part(roff, esize, 0, &elem);
    gen_op('*');
    complex_push_part(loff, esize, 1, &elem);
    complex_push_part(roff, esize, 1, &elem);
    gen_op('*');
    gen_op('+');
    vset(&elem, VT_LOCAL | VT_LVAL, den);
    gen_op('/');
    complex_store_part(&elem, res);

    complex_push_part(loff, esize, 1, &elem);
    complex_push_part(roff, esize, 0, &elem);
    gen_op('*');
    complex_push_part(loff, esize, 0, &elem);
    complex_push_part(roff, esize, 1, &elem);
    gen_op('*');
    gen_op('-');
    vset(&elem, VT_LOCAL | VT_LVAL, den);
    gen_op('/');
    complex_store_part(&elem, res + esize);
    break;
  }
  vset(&ct, VT_LOCAL | VT_LVAL, res);
}

/* Negate the complex value on top of the value stack in place. */
static void gen_complex_neg(void)
{
  CType ct = vtop->type;
  CType elem = complex_element_type(&ct);
  int esize, ealign, csize, calign, off, res;
  esize = type_size(&elem, &ealign);
  csize = type_size(&ct, &calign);
  off = complex_store_temp(&ct);
  loc = (loc - csize) & -calign;
  res = loc;
  vset(&elem, VT_LOCAL | VT_LVAL, off);
  if (is_float(elem.t))
    gen_opif(TOK_NEG);
  else
  {
    vpushi(0);
    vswap();
    gen_op('-');
  }
  complex_store_part(&elem, res);
  vset(&elem, VT_LOCAL | VT_LVAL, off + esize);
  if (is_float(elem.t))
    gen_opif(TOK_NEG);
  else
  {
    vpushi(0);
    vswap();
    gen_op('-');
  }
  complex_store_part(&elem, res + esize);
  vset(&ct, VT_LOCAL | VT_LVAL, res);
}

/* Replace the complex value on top of the value stack with its conjugate. */
static void gen_complex_conj(void)
{
  CType ct = vtop->type;
  CType elem = complex_element_type(&ct);
  int esize, ealign, csize, calign, off, res;
  esize = type_size(&elem, &ealign);
  csize = type_size(&ct, &calign);
  off = complex_store_temp(&ct);
  loc = (loc - csize) & -calign;
  res = loc;
  vset(&elem, VT_LOCAL | VT_LVAL, off);
  complex_store_part(&elem, res);
  vset(&elem, VT_LOCAL | VT_LVAL, off + esize);
  if (is_float(elem.t))
    gen_opif(TOK_NEG);
  else
  {
    vpushi(0);
    vswap();
    gen_op('-');
  }
  complex_store_part(&elem, res + esize);
  vset(&ct, VT_LOCAL | VT_LVAL, res);
}

/* Push an imaginary constant: a complex value whose real part is zero and
   whose imaginary part is 'value' of element type 'elem'. */
static void push_complex_imaginary_constant(CType *elem, CValue *value)
{
  CType ct = *elem;
  CValue zero;
  int esize, ealign, csize, calign, off;
  make_complex_type(&ct);
  memset(&zero, 0, sizeof zero);
  if (CONST_WANTED || static_initializer_constant_fold)
  {
    vsetc(&ct, VT_CONST, &zero);
    vtop->complex_constant = 1;
    vtop->complex_imaginary = *value;
    return;
  }
  esize = type_size(elem, &ealign);
  csize = type_size(&ct, &calign);
  loc = (loc - csize) & -calign;
  off = loc;
  vsetc(elem, VT_CONST, &zero);
  complex_store_part(elem, off);
  vsetc(elem, VT_CONST, value);
  complex_store_part(elem, off + esize);
  vset(&ct, VT_LOCAL | VT_LVAL, off);
}

/* Convert the value on top of the value stack to TYPE when either side is a
   complex type.  Returns nonzero when the conversion was applied. */
static int gen_cast_complex(CType *type)
{
  int scomplex = is_complex_type(&vtop->type);
  int dcomplex = is_complex_type(type);
  CType src = vtop->type;

  if (!scomplex && !dcomplex)
    return 0;
  if (dcomplex && scomplex)
  {
    CType selem = complex_element_type(&src);
    CType delem = complex_element_type(type);
    int ssize, salign, dsize, dalign, csize, calign, soff, res, i;
    if (is_compatible_unqualified_types(type, &src))
    {
      vtop->type = *type;
      vtop->type.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_ARRAY);
      return 1;
    }
    ssize = type_size(&selem, &salign);
    dsize = type_size(&delem, &dalign);
    csize = type_size(type, &calign);
    soff = complex_store_temp(&src);
    loc = (loc - csize) & -calign;
    res = loc;
    for (i = 0; i < 2; ++i)
    {
      vset(&selem, VT_LOCAL | VT_LVAL, soff + i * ssize);
      gen_cast(&delem);
      complex_store_part(&delem, res + i * dsize);
    }
    vset(type, VT_LOCAL | VT_LVAL, res);
    return 1;
  }
  if (dcomplex)
  {
    CType delem = complex_element_type(type);
    int dsize, dalign, csize, calign, res;
    CValue zero;
    if (!btype_is_arithmetic_scalar(vtop->type.t & VT_BTYPE))
      cprime_error("cannot convert to complex type");
    if ((CONST_WANTED || static_initializer_constant_fold)
        && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST)
    {
      CValue real;
      memset(&zero, 0, sizeof zero);
      gen_cast(&delem);
      real = vtop->c;
      vtop->type = *type;
      vtop->r = vtop->r2 = VT_CONST;
      vtop->c = real;
      vtop->sym = NULL;
      vtop->complex_constant = 1;
      vtop->complex_imaginary = zero;
      return 1;
    }
    dsize = type_size(&delem, &dalign);
    csize = type_size(type, &calign);
    loc = (loc - csize) & -calign;
    res = loc;
    gen_cast(&delem);
    complex_store_part(&delem, res);
    memset(&zero, 0, sizeof zero);
    vsetc(&delem, VT_CONST, &zero);
    complex_store_part(&delem, res + dsize);
    vset(type, VT_LOCAL | VT_LVAL, res);
    return 1;
  }
  /* A complex value converts to bool as `z != 0`; the real and imaginary
     parts are otherwise not implicitly discarded. */
  if ((type->t & VT_BTYPE) == VT_BOOL)
  {
    vpushi(0);
    gen_op(TOK_NE);
    vtop->type.t = (type->t & (VT_CONSTANT | VT_VOLATILE)) | VT_BOOL;
    vtop->type.ref = NULL;
    return 1;
  }
  cprime_error("cannot convert a complex value to a real type");
  return 1;
}

/* __real__ / __imag__ on a complex lvalue: select the element at the part's
   offset within the aggregate. */
static void complex_part_operand(int imag)
{
  CType ct = vtop->type;
  CType elem = complex_element_type(&ct);
  int esize, ealign;
  esize = type_size(&elem, &ealign);
  test_lvalue();
  gaddrof();
  vtop->type = char_pointer_type;
  vpushi(imag ? esize : 0);
  gen_op('+');
  vtop->type = elem;
  vtop->r |= VT_LVAL;
}

/* A typedef naming a complex type is not a class, so `fcomplex(expr)` is the
   functional conversion to that type rather than a constructor call. */
static int try_parse_cpp_functional_complex_cast(int type_tok)
{
  Sym *type_sym;
  CType cast_type;

  if (tok != '(')
    return 0;
  type_sym = sym_find(type_tok);
  if (!type_sym || !(type_sym->type.t & VT_TYPEDEF))
    return 0;
  cast_type = type_sym->type;
  cast_type.t &= ~VT_TYPEDEF;
  if (!is_complex_type(&cast_type))
    return 0;

  next();
  if (tok == ')')
    vpushi(0);      /* a value-initialized complex is zero */
  else
    expr_eq();
  skip(')');
  gen_cast(&cast_type);
  return 1;
}
