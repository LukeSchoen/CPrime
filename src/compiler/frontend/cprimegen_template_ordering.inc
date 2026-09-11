/* Compare function-template parameter patterns after conversion ranking.
   Template parameters on the deducing side are variables; parameters on the
   transformed side are distinct, fixed types.  Shared bindings across the
   parameter list retain constraints such as f(T, T). */
typedef struct MemberOrderPattern {
  int tokens[512], count;
  int starts[16], ends[16], parameters;
  int variables[32], variable_count;
} MemberOrderPattern;

typedef struct MemberOrderBinding {
  int start, length;
} MemberOrderBinding;

static int member_order_variable(MemberOrderPattern *pattern, int token)
{
  int i;
  for (i = 0; i < pattern->variable_count; ++i)
    if (pattern->variables[i] == token) return i + 1;
  return 0;
}

static int member_order_cv(int token)
{
  return token == TOK_CONST1 || token == TOK_CONST2 || token == TOK_CONST3
      || token == TOK_VOLATILE1 || token == TOK_VOLATILE2
      || token == TOK_VOLATILE3;
}

static int member_order_pattern(TemplateMemberDef *definition,
                                 MemberOrderPattern *pattern)
{
  TokenString *source = definition->def_str;
  int method = template_member_def_method_tok(definition);
  int i, start = -1, angle = 0, paren = 0, bracket = 0, skip_default = 0;
  int expect_variable = 0, template_depth = 0;
  for (i = 0; i < definition->nb_stripped_member_type_params; ++i)
    pattern->variables[pattern->variable_count++] =
      definition->stripped_member_type_param_toks[i];
  for (i = 0; i < source->len; ++i)
  {
    int token = source->str[i], after;
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    if (is_template_keyword_tok(token) && !template_depth)
    {
      template_depth = 1;
      continue;
    }
    if (template_depth)
    {
      if (token == TOK_LT || token == '<') ++angle;
      else if (token == TOK_GT || token == '>')
      {
        if (--angle == 0) template_depth = 0;
      }
      else if (token == TOK_CLASS || (token >= TOK_UIDENT
               && !strcmp(get_tok_str(token, NULL), "typename")))
        expect_variable = 1;
      else if (expect_variable && token >= TOK_UIDENT)
      {
        if (!member_order_variable(pattern, token))
        {
          if (pattern->variable_count == 32) return 0;
          pattern->variables[pattern->variable_count++] = token;
        }
        expect_variable = 0;
      }
      continue;
    }
    after = template_member_method_end_index(definition, i, method);
    if (!after) continue;
    after = tok_str_next_non_linenum_index(source->str, source->len, after);
    if (after < source->len && source->str[after] == '(')
    {
      start = after + 1;
      break;
    }
  }
  if (start < 0) return 0;
  angle = paren = bracket = 0;
  pattern->starts[0] = 0;
  for (i = start; i < source->len; ++i)
  {
    int token = source->str[i], variable;
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(source->str, source->len, i);
      if (token != TOK_LINENUM && !skip_default) return 0;
      continue;
    }
    if (!angle && !paren && !bracket && (token == ',' || token == ')'))
    {
      int begin = pattern->starts[pattern->parameters];
      int end = pattern->count;
      int reference_pattern;
      /* An optional declarator identifier does not participate in deduction. */
      if (end > begin + 1 && pattern->tokens[end - 1] >= TOK_UIDENT
          && !member_order_cv(pattern->tokens[end - 1])
          && pattern->tokens[end - 2] != ':')
        --end;
      /* Partial ordering compares referred-to types. Conversion ranking has
         already accounted for reference binding and top-level qualifiers. */
      reference_pattern = end > begin && (pattern->tokens[end - 1] == '&'
                                         || pattern->tokens[end - 1] == TOK_LAND);
      if (reference_pattern) --end;
      if (!reference_pattern) {
        int at, nesting = 0, outer_pointer = 0;
        for (at = begin; at < end; ++at)
        {
          int part = pattern->tokens[at];
          if (part == TOK_LT || part == '<') ++nesting;
          else if (part == TOK_GT || part == '>') --nesting;
          else if (!nesting && part == '*') outer_pointer = 1;
        }
        if (outer_pointer)
          while (begin < end && member_order_cv(pattern->tokens[end - 1])) --end;
        else
          while (begin < end && member_order_cv(pattern->tokens[begin])) ++begin;
      }
      if (begin == end) return token == ')' && !pattern->parameters;
      pattern->starts[pattern->parameters] = begin;
      pattern->ends[pattern->parameters++] = end;
      if (token == ')') return 1;
      if (pattern->parameters == 16) return 0;
      pattern->starts[pattern->parameters] = pattern->count;
      skip_default = 0;
      continue;
    }
    if (!angle && !paren && !bracket && token == '=') skip_default = 1;
    if (token == TOK_LT || token == '<') ++angle;
    else if (token == TOK_GT || token == '>') --angle;
    else if (token == TOK_SAR) angle -= 2;
    else if (token == '(') ++paren;
    else if (token == ')') --paren;
    else if (token == '[') ++bracket;
    else if (token == ']') --bracket;
    if (skip_default) continue;
    if (token == TOK_DOTS || token == TOK_EOF || paren || bracket) return 0;
    if (pattern->count >= 510) return 0;
    variable = member_order_variable(pattern, token);
    if (token == TOK_SAR)
    {
      pattern->tokens[pattern->count++] = TOK_GT;
      token = TOK_GT;
    }
    pattern->tokens[pattern->count++] = variable ? -variable : token;
  }
  return 0;
}

static int member_order_match(MemberOrderPattern *pattern, int position, int end,
                               MemberOrderPattern *actual, int first, int last,
                               MemberOrderBinding *bindings)
{
  int token, i, depth = 0;
  if (position == end) return first == last;
  if (first == last) return 0;
  token = pattern->tokens[position];
  if (token >= 0)
    return token == actual->tokens[first]
      && member_order_match(pattern, position + 1, end,
                            actual, first + 1, last, bindings);
  if (bindings[-token - 1].length)
  {
    MemberOrderBinding *binding = &bindings[-token - 1];
    if (first + binding->length > last) return 0;
    for (i = 0; i < binding->length; ++i)
      if (actual->tokens[first + i] != actual->tokens[binding->start + i]) return 0;
    return member_order_match(pattern, position + 1, end, actual,
                                first + binding->length, last, bindings);
  }
  for (i = first; i < last; ++i)
  {
    int current = actual->tokens[i];
    MemberOrderBinding trial[32];
    if (current == TOK_LT || current == '<') ++depth;
    if (current == TOK_GT || current == '>') --depth;
    if (depth < 0 || (!depth && current == ',')) break;
    if (depth) continue;
    memcpy(trial, bindings, sizeof(trial));
    trial[-token - 1].start = first;
    trial[-token - 1].length = i - first + 1;
    if (member_order_match(pattern, position + 1, end,
                            actual, i + 1, last, trial))
    {
      memcpy(bindings, trial, sizeof(trial));
      return 1;
    }
  }
  return 0;
}

static int member_order_deduces(MemberOrderPattern *pattern,
                                 MemberOrderPattern *actual)
{
  MemberOrderBinding bindings[32] = {{0, 0}};
  int i;
  if (pattern->parameters != actual->parameters) return 0;
  for (i = 0; i < pattern->parameters; ++i)
    if (!member_order_match(pattern, pattern->starts[i], pattern->ends[i],
          actual, actual->starts[i], actual->ends[i], bindings)) return 0;
  return 1;
}

static TemplateMemberDef *member_order_definition(int function_tok)
{
  MemberFunctionScope *scope;
  for (scope = member_function_scopes[(unsigned)function_tok & (MEMBER_SCOPE_BUCKETS - 1)];
       scope; scope = scope->next)
    if (scope->function_tok == function_tok) return scope->template_definition;
  return NULL;
}

static int compare_member_function_template_order_for_call(int first_tok, int second_tok, int arguments)
{
  TemplateMemberDef *first = member_order_definition(first_tok);
  TemplateMemberDef *second = member_order_definition(second_tok);
  MemberOrderPattern *a, *b;
  int result = 0;
  if (!first || !second || first == second) return 0;
  a = cprime_mallocz(sizeof(*a)); b = cprime_mallocz(sizeof(*b));
  if (member_order_pattern(first, a) && member_order_pattern(second, b)) {
    if (arguments >= 0) {
      if (a->parameters > arguments) a->parameters = arguments;
      if (b->parameters > arguments) b->parameters = arguments;
    }
    result = member_order_deduces(b, a) - member_order_deduces(a, b);
  }
  cprime_free(a); cprime_free(b);
  return result;
}

static int compare_member_function_template_order(int first_tok, int second_tok)
{
  return compare_member_function_template_order_for_call(first_tok, second_tok, -1);
}

static int compare_free_function_template_order(TemplateDef *first, TemplateDef *second)
{
  TemplateMemberDef av = {0}, bv = {0};
  MemberOrderPattern *a = cprime_mallocz(sizeof(*a));
  MemberOrderPattern *b = cprime_mallocz(sizeof(*b));
  int result = 0;
  av.def_str = first->def_str; av.method_tok = first->name_tok; av.method_tok_known = 1;
  bv.def_str = second->def_str; bv.method_tok = second->name_tok; bv.method_tok_known = 1;
  av.nb_stripped_member_type_params = first->nb_type_params;
  bv.nb_stripped_member_type_params = second->nb_type_params;
  memcpy(av.stripped_member_type_param_toks, first->type_param_toks, first->nb_type_params * sizeof(int));
  memcpy(bv.stripped_member_type_param_toks, second->type_param_toks, second->nb_type_params * sizeof(int));
  if (member_order_pattern(&av, a) && member_order_pattern(&bv, b))
    result = member_order_deduces(b, a) - member_order_deduces(a, b);
  cprime_free(a); cprime_free(b);
  return result;
}

/* Class partial specializations are ordered as the parameter types of
   fictitious functions taking Primary<pattern...>. Unlike ordinary function
   parameter ordering, their argument references and cv qualifiers remain. */
static int class_partial_order_pattern(TemplateDef *definition,
                                       MemberOrderPattern *pattern)
{
  TokenString *source = definition->def_str;
  int i, start = -1, angle = 1, paren = 0, bracket = 0;
  pattern->variable_count = definition->nb_type_params;
  if (pattern->variable_count > 32) return 0;
  for (i = 0; i < pattern->variable_count; ++i)
    pattern->variables[i] = definition->type_param_toks[i];
  for (i = 0; i + 1 < source->len; ++i) {
    if (TOK_HAS_VALUE(source->str[i])) {
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    if (source->str[i] == definition->name_tok
        && source->str[i + 1] == TOK_LT) {
      start = i + 2;
      break;
    }
  }
  if (start < 0) return 0;
  pattern->starts[0] = 0;
  for (i = start; i < source->len; ++i) {
    int token = source->str[i], copies = 1;
    if (TOK_HAS_VALUE(token)) {
      int extra = tok_str_value_extra_words(source->str, source->len, i);
      if (token != TOK_LINENUM) {
        CString literal;
        char word[40];
        int index;
        cstr_new(&literal);
        snprintf(word, sizeof(word), "__cpc_order_constant_%x", token);
        cstr_cat(&literal, word, -1);
        for (index = 1; index <= extra; ++index) {
          snprintf(word, sizeof(word), "_%x", source->str[i + index]);
          cstr_cat(&literal, word, -1);
        }
        cstr_ccat(&literal, 0);
        token = tok_alloc_const(literal.data);
        cstr_free(&literal);
      }
      i += extra;
      if (source->str[i - extra] == TOK_LINENUM) continue;
    }
    if (token == TOK_SAR) token = TOK_GT, copies = 2;
    while (copies--) {
      int boundary, variable;
      if (token == TOK_LT || token == '<') ++angle;
      else if (token == TOK_GT || token == '>') --angle;
      else if (token == '(') ++paren;
      else if (token == ')') --paren;
      else if (token == '[') ++bracket;
      else if (token == ']') --bracket;
      boundary = !angle || (angle == 1 && !paren && !bracket && token == ',');
      if (boundary) {
        int begin = pattern->starts[pattern->parameters], at;
        if (begin == pattern->count) return 0;
        /* Canonicalize the two spellings of cv-qualified deduction variables. */
        for (at = begin + 1; at < pattern->count; ++at)
          if (member_order_cv(pattern->tokens[at])
              && pattern->tokens[at - 1] < 0) {
            int qualifier = pattern->tokens[at];
            pattern->tokens[at] = pattern->tokens[at - 1];
            pattern->tokens[at - 1] = qualifier;
          }
        for (at = begin + 1; at < pattern->count; ++at)
          if (pattern->tokens[at] == TOK_CONST1
              && pattern->tokens[at - 1] == TOK_VOLATILE1) {
            pattern->tokens[at - 1] = TOK_CONST1;
            pattern->tokens[at] = TOK_VOLATILE1;
          }
        pattern->ends[pattern->parameters++] = pattern->count;
        if (!angle) return 1;
        if (pattern->parameters == 16) return 0;
        pattern->starts[pattern->parameters] = pattern->count;
        continue;
      }
      if (pattern->count >= 510 || token == TOK_EOF || token == TOK_DOTS)
        return 0;
      if (token == TOK_CONST2 || token == TOK_CONST3) token = TOK_CONST1;
      if (token == TOK_VOLATILE2 || token == TOK_VOLATILE3) token = TOK_VOLATILE1;
      variable = member_order_variable(pattern, token);
      pattern->tokens[pattern->count++] = variable ? -variable : token;
    }
  }
  return 0;
}

/* Type qualification is a set, not a token prefix. Deduction of volatile T
   from const volatile U binds T to const U, including when those types occur
   inside another template or behind a pointer. */
typedef struct ClassOrderType {
  int variable, wrapper, qualifiers;
  int nondeduced;
  int name_start, name_length, child, next;
} ClassOrderType;

typedef struct ClassOrderTypes {
  MemberOrderPattern *pattern;
  ClassOrderType nodes[512];
  int count, roots[16];
} ClassOrderTypes;

typedef struct ClassOrderTypeBinding {
  int node, removed_qualifiers;
} ClassOrderTypeBinding;

static int class_order_qualifiers(MemberOrderPattern *pattern, int *at, int end)
{
  int result = 0;
  while (*at < end && member_order_cv(pattern->tokens[*at])) {
    int token = pattern->tokens[(*at)++];
    result |= token == TOK_CONST1 || token == TOK_CONST2 || token == TOK_CONST3
               ? VT_CONSTANT : VT_VOLATILE;
  }
  return result;
}

static int class_order_type(ClassOrderTypes *types, int *at, int end)
{
  MemberOrderPattern *pattern = types->pattern;
  ClassOrderType *node;
  int root, qualifiers = class_order_qualifiers(pattern, at, end);
  int token, previous = 0;
  if (*at == end || types->count == 511) return 0;
  root = ++types->count;
  node = &types->nodes[root];
  token = pattern->tokens[*at];
  if (token >= TOK_UIDENT && !strcmp(get_tok_str(token, NULL), "typename")) {
    int scan;
    for (scan = *at + 1; scan + 1 < end; ++scan)
      if (pattern->tokens[scan] == ':' && pattern->tokens[scan + 1] == ':') {
        node->nondeduced = 1;
        *at = end;
        return root;
      }
  }
  if (token < 0) {
    node->variable = -token;
    ++*at;
  } else {
    node->name_start = *at;
    while (*at < end) {
      token = pattern->tokens[*at];
      if (member_order_cv(token) || token == '*' || token == '&'
          || token == TOK_LAND || token == TOK_LT || token == TOK_GT
          || token == ',' || token == '[' || token == '('
          || token == ')' || token == ']') break;
      if (token < 0) return 0;
      ++*at;
    }
    node->name_length = *at - node->name_start;
    if (!node->name_length) return 0;
    if (*at < end && pattern->tokens[*at] == TOK_LT) {
      ++*at;
      while (*at < end && pattern->tokens[*at] != TOK_GT) {
        int child = class_order_type(types, at, end);
        if (!child) return 0;
        if (previous) types->nodes[previous].next = child;
        else node->child = child;
        previous = child;
        if (*at == end || pattern->tokens[*at] != ',') break;
        ++*at;
      }
      if (*at == end || pattern->tokens[(*at)++] != TOK_GT) return 0;
    }
  }
  node->qualifiers = qualifiers | class_order_qualifiers(pattern, at, end);
  while (*at < end) {
    token = pattern->tokens[*at];
    if (token != '*' && token != '&' && token != TOK_LAND) break;
    ++*at;
    if (types->count == 511) return 0;
    node = &types->nodes[++types->count];
    node->wrapper = token;
    node->child = root;
    node->qualifiers = class_order_qualifiers(pattern, at, end);
    root = types->count;
  }
  return root;
}

static int class_order_types(MemberOrderPattern *pattern, ClassOrderTypes *types)
{
  int i;
  types->pattern = pattern;
  for (i = 0; i < pattern->parameters; ++i) {
    int at = pattern->starts[i];
    types->roots[i] = class_order_type(types, &at, pattern->ends[i]);
    if (!types->roots[i] || at != pattern->ends[i]) return 0;
  }
  return 1;
}

static int class_order_same_type(ClassOrderTypes *types, int first, int first_cv,
                                   int second, int second_cv)
{
  ClassOrderType *a = &types->nodes[first], *b = &types->nodes[second];
  int i, left, right;
  if (a->variable != b->variable || a->wrapper != b->wrapper
      || (a->qualifiers & ~first_cv) != (b->qualifiers & ~second_cv)
      || a->name_length != b->name_length) return 0;
  for (i = 0; i < a->name_length; ++i)
    if (types->pattern->tokens[a->name_start + i]
        != types->pattern->tokens[b->name_start + i]) return 0;
  for (left = a->child, right = b->child; left && right;
       left = types->nodes[left].next, right = types->nodes[right].next)
    if (!class_order_same_type(types, left, 0, right, 0)) return 0;
  return left == right;
}

static int class_order_deduce_type(ClassOrderTypes *pattern, int pattern_node,
                                    ClassOrderTypes *actual, int actual_node,
                                    ClassOrderTypeBinding *bindings)
{
  ClassOrderType *p = &pattern->nodes[pattern_node];
  ClassOrderType *a = &actual->nodes[actual_node];
  int i, left, right;
  if (p->nondeduced) return 1;
  if (a->nondeduced) return 0;
  if (p->variable) {
    ClassOrderTypeBinding *binding = &bindings[p->variable - 1];
    if ((a->qualifiers & p->qualifiers) != p->qualifiers) return 0;
    if (binding->node)
      return class_order_same_type(actual, binding->node,
                binding->removed_qualifiers, actual_node, p->qualifiers);
    binding->node = actual_node;
    binding->removed_qualifiers = p->qualifiers;
    return 1;
  }
  if (a->variable || p->wrapper != a->wrapper
      || p->qualifiers != a->qualifiers || p->name_length != a->name_length)
    return 0;
  for (i = 0; i < p->name_length; ++i)
    if (pattern->pattern->tokens[p->name_start + i]
        != actual->pattern->tokens[a->name_start + i]) return 0;
  for (left = p->child, right = a->child; left && right;
       left = pattern->nodes[left].next, right = actual->nodes[right].next)
    if (!class_order_deduce_type(pattern, left, actual, right, bindings)) return 0;
  return left == right;
}

static int class_order_deduces(ClassOrderTypes *pattern, ClassOrderTypes *actual)
{
  ClassOrderTypeBinding bindings[32] = {{0, 0}};
  int i;
  if (pattern->pattern->parameters != actual->pattern->parameters) return 0;
  for (i = 0; i < pattern->pattern->parameters; ++i)
    if (!class_order_deduce_type(pattern, pattern->roots[i], actual,
                                 actual->roots[i], bindings)) return 0;
  return 1;
}

/* Member-pointer patterns are not modelled by the structural type walk above.
   A member function pointer pattern is more specialized than the plain
   `T K::*` object form for the same class, because an arbitrary member type
   cannot be deduced from a synthesized function member type. Returns 0 when
   the parameter is not a member pointer, 1 for `T K::*` and 2 for
   `R (K::*)(T1...) [cv]`. */
static int class_order_member_pointer_form(MemberOrderPattern *pattern)
{
  int at, end, i, name, function_form = 0;
  if (pattern->parameters != 1) return 0;
  at = pattern->starts[0];
  end = pattern->ends[0];
  if (at >= end || pattern->tokens[at] >= 0) return 0;
  ++at;
  if (at < end && pattern->tokens[at] == '(') { function_form = 1; ++at; }
  for (i = at, name = 0; i < end && pattern->tokens[i] != ':'
       && pattern->tokens[i] != '*' && pattern->tokens[i] != '('
       && pattern->tokens[i] != ')' && pattern->tokens[i] != ','
       && pattern->tokens[i] != '&' && pattern->tokens[i] != '['
       && pattern->tokens[i] != ']'; ++i)
    ++name;
  if (!name || i + 2 >= end) return 0;
  if (pattern->tokens[i] != ':' || pattern->tokens[i + 1] != ':'
      || pattern->tokens[i + 2] != '*') return 0;
  if (!function_form)
    return i + 3 == end ? 1 : 0;
  if (i + 3 < end && pattern->tokens[i + 3] == ')')
    return 2;
  return 0;
}

static int compare_class_partial_template_order(TemplateDef *first,
                                                TemplateDef *second)
{
  MemberOrderPattern *a = cprime_mallocz(sizeof(*a));
  MemberOrderPattern *b = cprime_mallocz(sizeof(*b));
  int result = 0;
  if (class_partial_order_pattern(first, a)
      && class_partial_order_pattern(second, b)) {
    ClassOrderTypes *ta = cprime_mallocz(sizeof(*ta));
    ClassOrderTypes *tb = cprime_mallocz(sizeof(*tb));
    if (class_order_types(a, ta) && class_order_types(b, tb))
      result = class_order_deduces(tb, ta) - class_order_deduces(ta, tb);
    else
      result = member_order_deduces(b, a) - member_order_deduces(a, b);
    if (result == 0)
    {
      int first_form = class_order_member_pointer_form(a);
      int second_form = class_order_member_pointer_form(b);
      if (first_form && second_form && first_form != second_form)
        result = first_form > second_form ? 1 : -1;
    }
    cprime_free(ta);
    cprime_free(tb);
  }
  cprime_free(a);
  cprime_free(b);
  return result;
}

/* Two parameterless function templates have no parameter list to order by, so
   an explicit instantiation naming them through its result type (`template
   int* f();`) is resolved by the structure of their return-type patterns,
   exactly as class partial specializations order their argument patterns. */
static int free_function_return_order_pattern(TemplateDef *definition,
                                              MemberOrderPattern *pattern)
{
  TokenString *source = definition->def_str;
  int i, start, end, angle = 0;

  if (!template_declared_return_pattern(definition, &start, &end)) return 0;
  pattern->variable_count = definition->nb_type_params;
  for (i = 0; i < pattern->variable_count; ++i)
    pattern->variables[i] = definition->type_param_toks[i];
  for (i = start; i < end; ++i) {
    int token = source->str[i], copies = 1;
    if (TOK_HAS_VALUE(token)) {
      if (token != TOK_LINENUM) return 0;
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    if (token == TOK_SAR) token = TOK_GT, copies = 2;
    while (copies--) {
      int variable;
      if (token == TOK_LT || token == '<') ++angle;
      else if (token == TOK_GT || token == '>') --angle;
      if (token == TOK_CONST2 || token == TOK_CONST3) token = TOK_CONST1;
      else if (token == TOK_VOLATILE2 || token == TOK_VOLATILE3)
        token = TOK_VOLATILE1;
      variable = member_order_variable(pattern, token);
      if (pattern->count >= 510) return 0;
      pattern->tokens[pattern->count++] = variable ? -variable : token;
    }
  }
  if (!pattern->count) return 0;
  pattern->starts[0] = 0;
  pattern->ends[0] = pattern->count;
  pattern->parameters = 1;
  return 1;
}

static int compare_free_function_return_template_order(TemplateDef *first,
                                                       TemplateDef *second)
{
  MemberOrderPattern *a = cprime_mallocz(sizeof(*a));
  MemberOrderPattern *b = cprime_mallocz(sizeof(*b));
  int result = 0;
  if (free_function_return_order_pattern(first, a)
      && free_function_return_order_pattern(second, b)) {
    ClassOrderTypes *ta = cprime_mallocz(sizeof(*ta));
    ClassOrderTypes *tb = cprime_mallocz(sizeof(*tb));
    if (class_order_types(a, ta) && class_order_types(b, tb))
      result = class_order_deduces(tb, ta) - class_order_deduces(ta, tb);
    else
      result = member_order_deduces(b, a) - member_order_deduces(a, b);
    cprime_free(ta);
    cprime_free(tb);
  }
  cprime_free(a);
  cprime_free(b);
  return result;
}
