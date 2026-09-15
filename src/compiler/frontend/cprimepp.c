
#define USING_GLOBALS
#include "cprime.h"

// #Define To 1 To Enable (See Parse_Pp_String())
#define ACCEPT_LF_IN_STRINGS 0

//******************************************************
// Global Variables

ST_DATA int tok_flags;
ST_DATA int parse_flags;

ST_DATA struct BufferedFile *file;
ST_DATA int tok;
ST_DATA CValue tokc;
ST_DATA const int *macro_ptr;
ST_DATA CString tokcstr; // current parsed string, if any

// Display Benchmark Infos
ST_DATA int tok_ident;
ST_DATA TokenSym **table_ident;
static unsigned table_ident_capacity;
ST_DATA int pp_expr;

// -------------------------------------------------------------------------

static TokenSym **hash_ident;
static unsigned hash_ident_size;
static char token_buf[STRING_MAX_SIZE + 1];
static CString cstr_buf;
static TokenString tokstr_buf;
static TokenString unget_buf;
static unsigned char isidnum_table[256 - CH_EOF];
/* Derived from isidnum_table so the identifier scan in next_nomacro() can
   read one classification byte per character and use it directly as the
   spelling byte: entry c holds c while c can continue an identifier and 0
   otherwise, which is exactly the old IS_ID|IS_NUM test (no identifier
   character is 0).  set_idnum() is the only writer of isidnum_table, so the
   two stay in step. */
static unsigned char ident_cont[256];
/* Derived from isidnum_table in the same way: entry c is 1 while c is
   horizontal whitespace, so the tokenizer's run loop needs one table load
   and one test per byte instead of a load, mask and compare. */
static unsigned char ident_space[256];

/* Characters preprocess_skip()'s switch handles specially.  A skipped block
   is mostly ordinary text, so the scan advances over runs of other bytes
   instead of dispatching through the switch once per byte.  After #warning
   or #error the quotes and '/' are ordinary text as well. */
static unsigned char pp_skip_stop[256];
static unsigned char pp_skip_stop_msg[256];
static int pp_debug_tok, pp_debug_symv;
static int pp_counter;
/* Set while preprocessing a translation unit with a C++ source suffix; the
   language-dependent builtin spellings below follow the same rule as the
   __cplusplus predefine. */
static int cprime_cpp_mode;
static void tok_print(const int *str, const char *msg, ...);

static void next_nomacro(void);
static void parse_number(const char *p);
static void parse_string(const char *p, int len);
static int tok_str_value_extra_words(const int *str, int len, int index);

static struct TinyAlloc *toksym_alloc;
static struct TinyAlloc *tokstr_alloc;

static TokenString *macro_stack;

/* Set once any identifier with the synthetic member-receiver prefix has been
   interned.  See find_cpp_this_symbol(). */
static int cpp_this_prefixed_identifier_seen;

/* Identifier ids of the contextual spellings the declaration and expression
   paths probe by name: `using`, `typename`, `static_assert`, `template`,
   `friend`, `wchar_t`, `operator[]`, the four named casts and `delete`.  A
   spelling test against one of them otherwise costs a get_tok_str() call, a
   table_ident load and a strcmp() on every name token, and interning the
   literal on first use to compare ids instead would intern it at the first
   probe rather than at its first appearance in the source and renumber the
   identifiers in between.  Recording the id as the lexer interns the spelling
   keeps the numbering the source produces, and probing is then one integer
   compare.  A spelling that has not been interned cannot appear as a token, so
   CPC_SPELLING_UNSEEN marks "not seen yet". */
#define CPC_SPELLING_UNSEEN 0x7fffffff
ST_DATA int cpp_spelling_using_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_typename_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_static_assert_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_template_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_friend_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_wchar_t_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_index_op_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_static_cast_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_reinterpret_cast_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_const_cast_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_dynamic_cast_tok = CPC_SPELLING_UNSEEN;
ST_DATA int cpp_spelling_delete_tok = CPC_SPELLING_UNSEEN;

/* Called for each identifier the lexer interns, so the table above is filled
   at the same point the spelling first appears in the source. */
static void note_cpp_probed_spelling(int v, const char *str, int len)
{
  switch (len)
  {
  case 5:
    if (!memcmp(str, "using", 5)) cpp_spelling_using_tok = v;
    break;
  case 6:
    if (!memcmp(str, "friend", 6)) cpp_spelling_friend_tok = v;
    else if (!memcmp(str, "delete", 6)) cpp_spelling_delete_tok = v;
    break;
  case 7:
    if (!memcmp(str, "wchar_t", 7)) cpp_spelling_wchar_t_tok = v;
    break;
  case 8:
    if (!memcmp(str, "typename", 8)) cpp_spelling_typename_tok = v;
    else if (!memcmp(str, "template", 8)) cpp_spelling_template_tok = v;
    break;
  case 10:
    if (!memcmp(str, "operator[]", 10)) cpp_spelling_index_op_tok = v;
    else if (!memcmp(str, "const_cast", 10)) cpp_spelling_const_cast_tok = v;
    break;
  case 11:
    if (!memcmp(str, "static_cast", 11)) cpp_spelling_static_cast_tok = v;
    break;
  case 12:
    if (!memcmp(str, "dynamic_cast", 12)) cpp_spelling_dynamic_cast_tok = v;
    break;
  case 16:
    if (!memcmp(str, "reinterpret_cast", 16))
      cpp_spelling_reinterpret_cast_tok = v;
    break;
  case 13:
    if (!memcmp(str, "static_assert", 13)) cpp_spelling_static_assert_tok = v;
    break;
  }
}

/* The TokenSym the last next_nomacro() call interned for its identifier
   token.  next() reads the macro binding of that very token, so it can use
   the entry the tokenizer already touched instead of probing table_ident
   again; the token id is re-checked to keep the shortcut exact. */
static TokenSym *last_ident_sym;

static int cprimepp_has_suffix(const char *s, const char *suffix)
{
  size_t n, m;

  if (!s || !suffix)
    return 0;
  n = strlen(s);
  m = strlen(suffix);
  return n >= m && !strcmp(s + n - m, suffix);
}

static int cprimepp_is_cpp_filename(const char *filename)
{
  return cprimepp_has_suffix(filename, ".cpp")
         || cprimepp_has_suffix(filename, ".cxx")
         || cprimepp_has_suffix(filename, ".cc")
         || cprimepp_has_suffix(filename, ".C");
}

static const char cprime_keywords[] =
#define DEF(id, str) str "\0"
#include "cprimetok.h"
#undef DEF
  ;

// WARNING: the content of this string encodes token numbers
static const unsigned char tok_two_chars[] =
  /* outdated -- gr
      "<=\236>=\235!=\225&&\240||\241++\244--\242==\224<<\1>>\2+=\253"
      "-=\255*=\252/=\257%=\245&=\246^=\336|=\374->\313..\250##\266";
  */
{
  '<', '=', TOK_LE,
  '>', '=', TOK_GE,
  '!', '=', TOK_NE,
  '&', '&', TOK_LAND,
  '|', '|', TOK_LOR,
  '+', '+', TOK_INC,
  '-', '-', TOK_DEC,
  '=', '=', TOK_EQ,
  '<', '<', TOK_SHL,
  '>', '>', TOK_SAR,
  '+', '=', TOK_A_ADD,
  '-', '=', TOK_A_SUB,
  '*', '=', TOK_A_MUL,
  '/', '=', TOK_A_DIV,
  '%', '=', TOK_A_MOD,
  '&', '=', TOK_A_AND,
  '^', '=', TOK_A_XOR,
  '|', '=', TOK_A_OR,
  '-', '>', TOK_ARROW,
  '.', '.', TOK_TWODOTS,
  '#', '#', TOK_TWOSHARPS,
  0
};

ST_FUNC void skip(int c)
{
  if (tok != c)
  {
    char tmp[40];
    pstrcpy(tmp, sizeof tmp, get_tok_str(c, &tokc));
    cprime_error("'%s' expected (got '%s')", tmp, get_tok_str(tok, &tokc));
  }
  next();
}

ST_FUNC void expect(const char *msg)
{
  cprime_error("%s expected", msg);
}

// -------------------------------------------------------------------------
// Custom allocator for tiny objects

#define USE_TAL

#ifndef USE_TAL // May Cause Memory Leaks After Errors 
#define tal_free(al, p) cprime_free(p)
#define tal_realloc(al, p, size) cprime_realloc(p, size)
#define tal_new(a,b)
#define tal_delete(a)
#else
#if !defined(MEM_DEBUG)
#define tal_free(al, p) tal_free_impl(al, p)
#define tal_realloc(al, p, size) tal_realloc_impl(al, p, size)
#define TAL_DEBUG_PARAMS
#else
#define TAL_DEBUG MEM_DEBUG
// #define TAL_INFO 1 // collect and dump allocators stats
#define tal_free(al, p) tal_free_impl(al, p, __FILE__, __LINE__)
#define tal_realloc(al, p, size) tal_realloc_impl(al, p, size, __FILE__, __LINE__)
#define TAL_DEBUG_PARAMS , const char *sfile, int sline
#endif

#define TOKSYM_TAL_SIZE (256 * 1024) // allocator for TokenSym in table_ident
#define TOKSTR_TAL_SIZE (256 * 1024) // allocator for TokenString instances

typedef struct TinyAlloc
{
  uint8_t *p;
  uint8_t *bufend;
  struct TinyAlloc *next;
  unsigned nb_allocs;
  unsigned size;
#if TAL_INFO
  unsigned nb_peak;
  unsigned nb_total;
  uint8_t *peak_p;
#endif
  union
  {
    uint8_t buffer[1];
    size_t _aligner_;
  };
} TinyAlloc;

typedef struct tal_header_t
{
  size_t  size; // Word Align
  TinyAlloc *owner;
#if TAL_DEBUG
  int     line_num; // negative line_num used for double free check
  char    file_name[40];
#endif
} tal_header_t;

#define TAL_ALIGN(size) \
    (((size) + (sizeof (size_t) - 1)) & ~(sizeof (size_t) - 1))

// -------------------------------------------------------------------------

static TinyAlloc *tal_new(TinyAlloc **pal, unsigned size)
{
  TinyAlloc *al = cprime_malloc(sizeof(TinyAlloc) - sizeof (size_t) + size);
  al->p = al->buffer;
  al->bufend = al->buffer + size;
  al->nb_allocs = 0;
  al->next = *pal, *pal = al;
  al->size = al->next ? al->next->size : size;
#if TAL_INFO
  al->nb_peak = 0;
  al->nb_total = 0;
  al->peak_p = al->p;
#endif
  return al;
}

static void tal_delete(TinyAlloc **pal)
{
  TinyAlloc *al = *pal, *next;

#if TAL_INFO
  fprintf(stderr, "tal_delete (&tok%s_alloc):\n", pal == &toksym_alloc ? "sym" : "str");
#endif
tail_call:
#if TAL_DEBUG && TAL_DEBUG != 3 // do not check TAL leaks with -DMEM_DEBUG=3 
#if TAL_INFO
  fprintf(stderr, "  size %7d  nb_peak %5d  nb_total %6d  usage %5.1f%%\n",
          al->bufend - al->buffer, al->nb_peak, al->nb_total,
          (al->peak_p - al->buffer) * 100.0 / (al->bufend - al->buffer));
#endif
  if (al->nb_allocs > 0)
  {
    uint8_t *p;
    fprintf(stderr, "TAL_DEBUG: memory leak %d chunk(s)\n", al->nb_allocs);
    p = al->buffer;
    while (p < al->p)
    {
      tal_header_t *header = (tal_header_t *)p;
      if (header->line_num > 0)
      {
        fprintf(stderr, "%s:%d: chunk of %d bytes leaked\n",
                header->file_name, header->line_num, (int)header->size);
      }
      p += header->size + sizeof(tal_header_t);
    }
#if TAL_DEBUG == 2
    exit(2);
#endif
  }
#endif
  next = al->next;
  cprime_free(al);
  al = next;
  if (al)
    goto tail_call;
  *pal = al;
}

static void tal_free_impl(TinyAlloc **pal, void *p TAL_DEBUG_PARAMS)
{
  TinyAlloc *al, **top = pal;
  tal_header_t *header;

  if (!p)
    return;
  header = (tal_header_t *)p - 1;
#if TAL_DEBUG
  if (header->line_num < 0)
  {
    fprintf(stderr, "%s:%d: TAL_DEBUG: double frees chunk from\n",
            sfile, sline);
    fprintf(stderr, "%s:%d: %d bytes\n",
            header->file_name, (int) - header->line_num, (int)header->size);
  }
  else
    header->line_num = -header->line_num;
#endif
  al = header->owner;
  if (0 == --al->nb_allocs)
  {
    while (*pal != al) pal = &(*pal)->next;
    *pal = al->next;
    if ((al->bufend - al->buffer) > al->size)
    {
      //fprintf(stderr, "free big tal: %u\n", header->size);
      cprime_free(al);
    }
    else
    {
      // Reset And Move To Front
      al->p = al->buffer;
      al->next = *top, *top = al;
    }
  }
  else if ((uint8_t * )p + header->size == al->p)
    al->p = (uint8_t * )header;
}

static void *tal_realloc_impl(TinyAlloc **pal, void *p, unsigned size TAL_DEBUG_PARAMS)
{
  tal_header_t *header;
  void *ret;
  unsigned adj_size = TAL_ALIGN(size) + sizeof(tal_header_t);
  TinyAlloc *al = *pal;

  if (p)
  {
    // Reallpc Case
    header = (tal_header_t *)p - 1;
    al = header->owner;
    if ((uint8_t * )p + header->size == al->p)
      al->p = (uint8_t * )header; // Maybe Reuse
    if (al->p + adj_size > al->bufend)
    {
      ret = tal_realloc(pal, 0, size);
      memcpy(ret, p, header->size);
      tal_free(pal, p);
      return ret;
    }
    else if (al->p != (uint8_t * )header)
    {
      memcpy((tal_header_t *)al->p + 1, p, header->size);
#if TAL_DEBUG
      header->line_num = -header->line_num;
#endif
    }
  }
  else
  {
    // New Alloc Case
    while (al->p + adj_size > al->bufend)
    {
      al = al->next;
      if (!al)
      {
        unsigned new_size = (*pal)->size;
        if (adj_size > new_size)
        {
          new_size = adj_size;
          //fprintf(stderr, "%s:%d: alloc big tal: %u\n", file->filename, file->line_num, adj_size - sizeof(tal_header_t));
        }
        al = tal_new(pal, new_size);
        break;
      }
    }
    al->nb_allocs++;
  }
  header = (tal_header_t *)al->p;
  header->size = adj_size - sizeof(tal_header_t);
  header->owner = al;
  al->p += adj_size;
  ret = header + 1;
#if  TAL_DEBUG
  {
    int ofs = strlen(sfile) + 1 - sizeof header->file_name;
    strcpy(header->file_name, sfile + (ofs > 0 ? ofs : 0));
    header->line_num = sline;
#if TAL_INFO
    if (al->nb_peak < al->nb_allocs)
      al->nb_peak = al->nb_allocs;
    if (al->peak_p < al->p)
      al->peak_p = al->p;
    al->nb_total++;
#endif
  }
#endif
  return ret;
}

#endif // USE_TAL 

// -------------------------------------------------------------------------
// CString handling
static void cstr_realloc(CString *cstr, int new_size)
{
  int size;

  size = cstr->size_allocated;
  if (size < 8)
    size = 8; // No Need To Allocate A Too Small First String
  while (size < new_size)
    size = size * 2;
  cstr->data = cprime_realloc(cstr->data, size);
  cstr->size_allocated = size;
}

// Add A Byte
ST_INLN void cstr_ccat(CString *cstr, int ch)
{
  int size;
  size = cstr->size + 1;
  if (size > cstr->size_allocated)
    cstr_realloc(cstr, size);
  cstr->data[size - 1] = ch;
  cstr->size = size;
}

ST_INLN char *unicode_to_utf8 (char *b, uint32_t Uc)
{
  if (Uc < 0x80) *b++ = Uc;
  else if (Uc < 0x800) *b++ = 192 + Uc / 64, *b++ = 128 + Uc % 64;
  else if (Uc - 0xd800u < 0x800) goto error;
  else if (Uc < 0x10000) *b++ = 224 + Uc / 4096, *b++ = 128 + Uc / 64 % 64, *b++ = 128 + Uc % 64;
  else if (Uc < 0x110000) *b++ = 240 + Uc / 262144, *b++ = 128 + Uc / 4096 % 64, *b++ = 128 + Uc / 64 % 64, *b++ = 128 + Uc % 64;
else error: cprime_error("0x%x is not a valid universal character", Uc);
  return b;
}

// Add A Unicode Character Expanded Into Utf8
ST_INLN void cstr_u8cat(CString *cstr, int ch)
{
  char buf[4], *e;
  e = unicode_to_utf8(buf, (uint32_t)ch);
  cstr_cat(cstr, buf, e - buf);
}

// Add String Of 'Len', Or Of Its Len/Len+1 When 'Len' == -1/0
ST_FUNC void cstr_cat(CString *cstr, const char *str, int len)
{
  int size;
  if (len <= 0)
    len = strlen(str) + 1 + len;
  size = cstr->size + len;
  if (size > cstr->size_allocated)
    cstr_realloc(cstr, size);
  memmove(cstr->data + cstr->size, str, len);
  cstr->size = size;
}

// Add A Wide Char
ST_FUNC void cstr_wccat(CString *cstr, int ch)
{
  int size;
  size = cstr->size + sizeof(nwchar_t);
  if (size > cstr->size_allocated)
    cstr_realloc(cstr, size);
  *(nwchar_t *)(cstr->data + size - sizeof(nwchar_t)) = ch;
  cstr->size = size;
}

ST_FUNC void cstr_new(CString *cstr)
{
  memset(cstr, 0, sizeof(CString));
}

// free string and reset it to NULL
ST_FUNC void cstr_free(CString *cstr)
{
  cprime_free(cstr->data);
}

// Reset String To Empty
ST_FUNC void cstr_reset(CString *cstr)
{
  cstr->size = 0;
}

ST_FUNC int cstr_vprintf(CString *cstr, const char *fmt, va_list ap)
{
  va_list v;
  int len, size = 80;
  for (;;)
  {
    size += cstr->size;
    if (size > cstr->size_allocated)
      cstr_realloc(cstr, size);
    size = cstr->size_allocated - cstr->size;
    va_copy(v, ap);
    len = vsnprintf(cstr->data + cstr->size, size, fmt, v);
    va_end(v);
    if (len >= 0 && len < size)
      break;
    size *= 2;
  }
  cstr->size += len;
  return len;
}

ST_FUNC int cstr_printf(CString *cstr, const char *fmt, ...)
{
  va_list ap; int len;
  va_start(ap, fmt);
  len = cstr_vprintf(cstr, fmt, ap);
  va_end(ap);
  return len;
}

// XXX: unicode ?
static void add_char(CString *cstr, int c)
{
  if ((unsigned)c > 255) {
    char escaped[16];
    snprintf(escaped, sizeof(escaped), "\\x%X", (unsigned)c);
    cstr_cat(cstr, escaped, strlen(escaped));
    return;
  }
  if (c == '\'' || c == '\"' || c == '\\')
  {
    // XXX: could be more precise if char or string
    cstr_ccat(cstr, '\\');
  }
  if (c >= 32 && c <= 126)
    cstr_ccat(cstr, c);
  else
  {
    cstr_ccat(cstr, '\\');
    if (c == '\n')
      cstr_ccat(cstr, 'n');
    else
    {
      cstr_ccat(cstr, '0' + ((c >> 6) & 7));
      cstr_ccat(cstr, '0' + ((c >> 3) & 7));
      cstr_ccat(cstr, '0' + (c & 7));
    }
  }
}

// -------------------------------------------------------------------------
// Allocate A New Token
static TokenSym *tok_alloc_new(TokenSym **pts, const char *str, int len, unsigned hash)
{
  TokenSym *ts, **ptable;
  int i;

  if (tok_ident >= SYM_FIRST_ANOM)
    cprime_error("memory full (symbols)");

  // expand token table if needed
  i = tok_ident - TOK_IDENT;
  if ((unsigned)i == table_ident_capacity)
  {
    table_ident_capacity = table_ident_capacity ? table_ident_capacity * 2 : TOK_ALLOC_INCR;
    ptable = cprime_realloc(table_ident, table_ident_capacity * sizeof(TokenSym *));
    table_ident = ptable;
  }

  ts = tal_realloc(&toksym_alloc, 0, sizeof(TokenSym) + len);
  table_ident[i] = ts;
  ts->tok = tok_ident++;
  ts->sym_define = NULL;
  ts->sym_label = NULL;
  ts->sym_struct = NULL;
  ts->sym_identifier = NULL;
  ts->len = len;
  ts->hash = hash;
  ts->hash_next = NULL;
  memcpy(ts->str, str, len);
  ts->str[len] = '\0';
  /* The C++ member-call lowering derives synthetic receiver names from this
     prefix.  Recording that one exists keeps the receiver lookup in
     find_cpp_this_symbol() from walking the local stack for every ordinary
     call expression. */
  if (len >= 14 && !memcmp(ts->str, "__cprime_this_", 14))
    cpp_this_prefixed_identifier_seen = 1;
  note_cpp_probed_spelling(ts->tok, ts->str, len);
  *pts = ts;
  /* Keep identifier lookup proportional to the bucket load even in a
     template-heavy unity translation unit. Token addresses stay stable. */
  if ((unsigned)(tok_ident - TOK_IDENT) > hash_ident_size) {
    unsigned bucket;
    TokenSym **grown;
    hash_ident_size *= 2;
    grown = cprime_mallocz(hash_ident_size * sizeof(*grown));
    for (i = 0; i < tok_ident - TOK_IDENT; ++i) {
      TokenSym *entry = table_ident[i];
      bucket = entry->hash & (hash_ident_size - 1);
      entry->hash_next = grown[bucket];
      grown[bucket] = entry;
    }
    cprime_free(hash_ident);
    hash_ident = grown;
  }
  return ts;
}

#define TOK_HASH_INIT 1
#define TOK_HASH_FUNC(h, c) ((h) * 31 + (c))

// find a token and add it if not found
ST_FUNC TokenSym *tok_alloc(const char *str, int len)
{
  TokenSym *ts, **pts;
  int i;
  unsigned int h;

  h = TOK_HASH_INIT;
  for (i = 0; i < len; i++)
    h = TOK_HASH_FUNC(h, ((unsigned char *)str)[i]);
  pts = &hash_ident[h & (hash_ident_size - 1)];
  for (;;)
  {
    ts = *pts;
    if (!ts)
      break;
    if (ts->hash == h && ts->len == len && !memcmp(ts->str, str, len))
      return ts;
    pts = &(ts->hash_next);
  }
  return tok_alloc_new(pts, str, len, h);
}

ST_FUNC int tok_alloc_const(const char *str)
{
  return tok_alloc(str, strlen(str))->tok;
}


// XXX: buffer overflow
// XXX: float tokens
ST_FUNC const char *get_tok_str(int v, CValue *cv)
{
  char *p;
  int i, len;

  if ((unsigned)(v - TOK_IDENT) < (unsigned)(tok_ident - TOK_IDENT))
    return table_ident[v - TOK_IDENT]->str;
  cstr_reset(&cstr_buf);
  p = cstr_buf.data;

  switch (v)
  {
  case TOK_CINT:
  case TOK_CUINT:
  case TOK_CLONG:
  case TOK_CULONG:
  case TOK_CLLONG:
  case TOK_CULLONG:
    // XXX: not quite exact, but only useful for testing
    sprintf(p, "%llu", (unsigned long long)cv->i);
    break;
  case TOK_U16CHAR:
  case TOK_U32CHAR:
  case TOK_LCHAR:
    cstr_ccat(&cstr_buf, (v == TOK_U16STR || v == TOK_U16CHAR) ? 'u' : (v == TOK_U32STR || v == TOK_U32CHAR) ? 'U' : 'L');
  case TOK_U8CHAR:
  case TOK_CCHAR:
    if (v == TOK_U8CHAR) cstr_cat(&cstr_buf, "u8", 2);
    cstr_ccat(&cstr_buf, '\'');
    add_char(&cstr_buf, cv->i);
    cstr_ccat(&cstr_buf, '\'');
    cstr_ccat(&cstr_buf, '\0');
    break;
  case TOK_PPNUM:
  case TOK_PPSTR:
    return (char *)cv->str.data;
  case TOK_U16STR:
  case TOK_U32STR:
  case TOK_LSTR:
    cstr_ccat(&cstr_buf, (v == TOK_U16STR || v == TOK_U16CHAR) ? 'u' : (v == TOK_U32STR || v == TOK_U32CHAR) ? 'U' : 'L');
  case TOK_U8STR:
  case TOK_STR:
    if (v == TOK_U8STR) cstr_cat(&cstr_buf, "u8", 2);
    cstr_ccat(&cstr_buf, '\"');
    if (v == TOK_STR || v == TOK_U8STR)
    {
      len = cv->str.size - 1;
      for (i = 0; i < len; i++)
        add_char(&cstr_buf, ((unsigned char *)cv->str.data)[i]);
    }
    else
    {
      len = (cv->str.size / TOK_STRING_UNIT_SIZE(v)) - 1;
      for (i = 0; i < len; i++)
        add_char(&cstr_buf, TOK_STRING_UNIT_SIZE(v) == 2 ? ((uint16_t *)cv->str.data)[i] : ((uint32_t *)cv->str.data)[i]);
    }
    cstr_ccat(&cstr_buf, '\"');
    cstr_ccat(&cstr_buf, '\0');
    break;

  case TOK_CFLOAT:
    return strcpy(p, "<float>");
  case TOK_CDOUBLE:
    return strcpy(p, "<double>");
  case TOK_CLDOUBLE:
    return strcpy(p, "<long double>");
  case TOK_CIMAGI:
    return strcpy(p, "<imaginary int>");
  case TOK_CIMAGLL:
    return strcpy(p, "<imaginary long long>");
  case TOK_CIMAGF:
    return strcpy(p, "<imaginary float>");
  case TOK_CIMAGD:
    return strcpy(p, "<imaginary double>");
  case TOK_CIMAGL:
    return strcpy(p, "<imaginary long double>");
  case TOK_LINENUM:
    return strcpy(p, "<linenumber>");

  // above tokens have value, the ones below don't
  case TOK_LT:
    v = '<';
    goto addv;
  case TOK_GT:
    v = '>';
    goto addv;
  case TOK_DOTS:
    return strcpy(p, "...");
  case TOK_A_SHL:
    return strcpy(p, "<<=");
  case TOK_A_SAR:
    return strcpy(p, ">>=");
  case TOK_EOF:
    return strcpy(p, "<eof>");
  case 0: // Anonymous Nameless Symbols
    return strcpy(p, "<no name>");
  default:
    v &= ~(SYM_FIELD | SYM_STRUCT);
    if (v < TOK_IDENT)
    {
      // Search In Two Bytes Table
      const unsigned char *q = tok_two_chars;
      while (*q)
      {
        if (q[2] == v)
        {
          *p++ = q[0];
          *p++ = q[1];
          *p = '\0';
          return cstr_buf.data;
        }
        q += 3;
      }
      if (v >= 127 || (v < 32 && !is_space(v) && v != '\n'))
      {
        sprintf(p, "<\\x%02x>", v);
        break;
      }
addv:
      *p++ = v;
      *p = '\0';
    }
    else if (v < tok_ident)
      return table_ident[v - TOK_IDENT]->str;
    else if (v >= SYM_FIRST_ANOM)
    {
      // Special Name For Anonymous Symbol
      sprintf(p, "L.%u", v - SYM_FIRST_ANOM);
    }
    else
    {
      // Should Never Happen
      return NULL;
    }
    break;
  }
  return cstr_buf.data;
}

/* return the current character, handling end of block if necessary
   (but not stray) */
static int handle_eob(void)
{
  BufferedFile *bf = file;
  int len;

  // only tries to read if really end of buffer
  if (bf->buf_ptr >= bf->buf_end)
  {
    if (bf->fd >= 0)
    {
#if defined(PARSE_DEBUG)
      len = 1;
#else
      len = IO_BUF_SIZE;
#endif
      len = read(bf->fd, bf->buffer, len);
      if (len < 0)
        len = 0;
    }
    else
      len = 0;
    total_bytes += len;
    bf->buf_ptr = bf->buffer;
    bf->buf_end = bf->buffer + len;
    *bf->buf_end = CH_EOB;
  }
  if (bf->buf_ptr < bf->buf_end)
    return bf->buf_ptr[0];
  else
  {
    bf->buf_ptr = bf->buf_end;
    return CH_EOF;
  }
}

// Read Next Char From Current Input File And Handle End Of Input Buffer
static int next_c(void)
{
  int ch = *++file->buf_ptr;
  // End Of Buffer/File Handling
  if (ch == CH_EOB && file->buf_ptr >= file->buf_end)
    ch = handle_eob();
  return ch;
}

// Input With '\[\R]\N' Handling.
static int handle_stray_noerror(int err)
{
  int ch;
  while ((ch = next_c()) == '\\')
  {
    ch = next_c();
    if (ch == '\n')
    {
newl:
      file->line_num++;
    }
    else
    {
      if (ch == '\r')
      {
        ch = next_c();
        if (ch == '\n')
          goto newl;
        *--file->buf_ptr = '\r';
      }
      if (err)
        cprime_error("stray '\\' in program");
      // may take advantage of 'BufferedFile.unget[4}'
      return *--file->buf_ptr = '\\';
    }
  }
  return ch;
}

#define ninp() handle_stray_noerror(0)

// Handle '\\' In Strings, Comments And Skipped Regions
static int handle_bs(uint8_t **p)
{
  int c;
  file->buf_ptr = *p - 1;
  c = ninp();
  *p = file->buf_ptr;
  return c;
}

/* skip the stray and handle the \\n case. Output an error if
   incorrect char after the stray */
static int ucn_identifier_prefix(uint8_t **pointer);
static int handle_stray(uint8_t **p)
{
  int c;
  file->buf_ptr = *p - 1;
  c = handle_stray_noerror(0);
  *p = file->buf_ptr;
  if (c == '\\' && !(parse_flags & PARSE_FLAG_ACCEPT_STRAYS)
      && !ucn_identifier_prefix(p))
    cprime_error("stray '\\' in program");
  return c;
}

// Handle The Complicated Stray Case
#define PEEKC(c, p)\
{\
    c = *++p;\
    if (c == '\\')\
        c = handle_stray(&p); \
}

/* Look past a literal backslash in the logical character stream. A splice
   can separate it from the UCN prefix; put the slash back immediately before
   that prefix, using the input buffer's existing pushback space if necessary. */
static int ucn_identifier_prefix(uint8_t **pointer)
{
  uint8_t *p = *pointer;
  int c;
  PEEKC(c, p);
  *--p = '\\';
  *pointer = p;
  return c == 'u' || c == 'U';
}

static int skip_spaces(void)
{
  int ch;
  --file->buf_ptr;
  do
  {
    ch = ninp();
  }
  while (isidnum_table[ch - CH_EOF] & IS_SPC);
  return ch;
}

// single line C++ comments
static uint8_t *parse_line_comment(uint8_t *p)
{
  int c;
  for (;;)
  {
    for (;;)
    {
      c = *++p;
redo:
      if (c == '\n' || c == '\\')
        break;
      c = *++p;
      if (c == '\n' || c == '\\')
        break;
    }
    if (c == '\n')
      break;
    c = handle_bs(&p);
    if (c == CH_EOF)
      break;
    if (c != '\\')
      goto redo;
  }
  return p;
}

// C comments
static uint8_t *parse_comment(uint8_t *p)
{
  int c;
  for (;;)
  {
    // Fast Skip Loop
    for (;;)
    {
      c = *++p;
redo:
      if (c == '\n' || c == '*' || c == '\\')
        break;
      c = *++p;
      if (c == '\n' || c == '*' || c == '\\')
        break;
    }
    // Now We Can Handle All The Cases
    if (c == '\n')
      file->line_num++;
    else if (c == '*')
    {
      do
      {
        c = *++p;
      }
      while (c == '*');
      if (c == '\\')
        c = handle_bs(&p);
      if (c == '/')
        break;
      goto check_eof;
    }
    else
    {
      c = handle_bs(&p);
check_eof:
      if (c == CH_EOF)
        cprime_error("unexpected end of file in comment");
      if (c != '\\')
        goto redo;
    }
  }
  return p + 1;
}

static uint8_t *parse_pp_raw_string(uint8_t *p, CString *str)
{
  char delimiter[17];
  int length = 0, c, matched = 0;
  file->buf_ptr = p;
  while ((c = next_c()) != '(') {
    if (c == CH_EOF) cprime_error("unterminated raw string delimiter");
    if (length == 16 || c <= 32 || c == ')' || c == '\\' || c > 126)
      cprime_error("invalid raw string delimiter");
    delimiter[length++] = c;
    if (str) cstr_ccat(str, c);
  }
  if (str) cstr_ccat(str, '(');
  for (;;) {
    c = next_c();
    if (c == CH_EOF) cprime_error("unterminated raw string literal");
    if (c == '\r') {
      int following = next_c();
      if (following != '\n') *--file->buf_ptr = following;
      c = '\n';
    }
    if (c == '\n') ++file->line_num;
    if (str) cstr_ccat(str, c);
    if (matched && c == (matched <= length ? delimiter[matched - 1] : '"'))
      ++matched;
    else
      matched = c == ')';
    if (matched == length + 2) return file->buf_ptr + 1;
  }
}

// Parse A String Without Interpreting Escapes
static uint8_t *parse_pp_string(uint8_t *p, int sep, CString *str)
{
  int c;
  for (;;)
  {
    c = *++p;
redo:
    if (c == sep)
      break;
    else if (c == '\\')
    {
      c = handle_bs(&p);
      if (c == CH_EOF)
      {
unterminated_string:
        // XXX: indicate line number of start of string
        tok_flags &= ~TOK_FLAG_BOL;
        cprime_error("missing terminating %c character", sep);
      }
      else if (c == '\\')
      {
        if (str)
          cstr_ccat(str, c);
        c = *++p;
        // Add Char After '\\' Unconditionally
        if (c == '\\')
        {
          c = handle_bs(&p);
          if (c == CH_EOF)
            goto unterminated_string;
        }
        goto add_char;
      }
      else
        goto redo;
    }
    else if (c == '\n')
    {
add_lf:
      if (ACCEPT_LF_IN_STRINGS)
      {
        file->line_num++;
        goto add_char;
      }
      else if (str)     // Not Skipping
        goto unterminated_string;
      else
      {
        //cprime_warning("missing terminating %c character", sep);
        return p;
      }
    }
    else if (c == '\r')
    {
      c = *++p;
      if (c == '\\')
        c = handle_bs(&p);
      if (c == '\n')
        goto add_lf;
      if (c == CH_EOF)
        goto unterminated_string;
      if (str)
        cstr_ccat(str, '\r');
      goto redo;
    }
    else
    {
add_char:
      if (str)
        cstr_ccat(str, c);
    }
  }
  p++;
  return p;
}

/* Consume a preprocessing number without interpreting its value. In inactive
   source its identifier-like suffix must not become a literal prefix. */
static uint8_t *skip_pp_number(uint8_t *p)
{
  int previous = *p, c;
  for (;;) {
    PEEKC(c, p);
    if (c == '\'') {
      PEEKC(c, p);
      if (!(isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
          && !(c == '\\' && ucn_identifier_prefix(&p))) {
        /* PEEKC may have refilled the buffer; use its reserved unget space. */
        *--p = '\'';
        return p;
      }
    } else if (!(isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
               && !(c == '\\' && ucn_identifier_prefix(&p))
               && c != '.'
               && !((c == '+' || c == '-')
                    && (previous == 'e' || previous == 'E'
                        || previous == 'p' || previous == 'P'))) {
      return p;
    }
    if (c == '\\') {
      int digits;
      PEEKC(c, p);
      digits = c == 'u' ? 4 : 8;
      while (digits--) {
        PEEKC(c, p);
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
              || (c >= 'A' && c <= 'F')))
          expect("more hex digits in universal-character-name");
      }
      /* The last spelling digit is not an exponent marker. */
      c = 0;
    }
    previous = c;
  }
}
/* skip block of text until #else, #elif or #endif. skip also pairs of
   #if/#endif */
static void preprocess_skip(void)
{
  int a, start_of_line, c, in_warn_or_error;
  uint8_t *p;
  char prefix[3];
  int prefix_len = 0;

  p = file->buf_ptr;
  a = 0;
redo_start:
  prefix_len = 0;
  start_of_line = 1;
  in_warn_or_error = 0;
  for (;;)
  {
    c = *p;
    switch (c)
    {
    case ' ':
    case '\t':
    case '\f':
    case '\v':
    case '\r':
      prefix_len = 0;
      p++;
      continue;
    case '\n':
      file->line_num++;
      p++;
      goto redo_start;
    case '\\':
      c = handle_bs(&p);
      if (c == CH_EOF)
        expect("#endif");
      if (c == '\\') {
        prefix_len = 0;
        ++p;
      }
      continue;
    // Skip Strings
    case '\"':
    case '\'':
      if (in_warn_or_error)
        goto _default;
      tok_flags &= ~TOK_FLAG_BOL;
      if (c == '"' &&
          ((prefix_len == 1 && prefix[0] == 'R') ||
           (prefix_len == 2 && prefix[1] == 'R' &&
            (prefix[0] == 'u' || prefix[0] == 'U' || prefix[0] == 'L')) ||
           (prefix_len == 3 && prefix[0] == 'u' && prefix[1] == '8' && prefix[2] == 'R')))
        p = parse_pp_raw_string(p, NULL);
      else
        p = parse_pp_string(p, c, NULL);
      prefix_len = 0;
      break;
    // Skip Comments
    case '/':
      prefix_len = 0;
      if (in_warn_or_error)
        goto _default;
      ++p;
      c = handle_bs(&p);
      if (c == '*')
        p = parse_comment(p);
      else if (c == '/')
        p = parse_line_comment(p);
      continue;
    case '#':
      prefix_len = 0;
      p++;
      if (start_of_line)
      {
        file->buf_ptr = p;
        next_nomacro();
        p = file->buf_ptr;
        if (a == 0 &&
            (tok == TOK_ELSE || tok == TOK_ELIF || tok == TOK_ELIFDEF
             || tok == TOK_ELIFNDEF || tok == TOK_ENDIF))
          goto the_end;
        if (tok == TOK_IF || tok == TOK_IFDEF || tok == TOK_IFNDEF)
          a++;
        else if (tok == TOK_ENDIF)
          a--;
        else if ( tok == TOK_ERROR || tok == TOK_WARNING)
          in_warn_or_error = 1;
        else if (tok == TOK_LINEFEED)
          goto redo_start;
        else if (parse_flags & PARSE_FLAG_ASM_FILE)
          p = parse_line_comment(p - 1);
      }
#if !defined(CPRIME_TARGET_ARM)
      else if (parse_flags & PARSE_FLAG_ASM_FILE)
        p = parse_line_comment(p - 1);
#else
      // ARM assembly uses '#' for constants
#endif
      break;
_default:
    default:
      /* Ordinary text: advance over the whole run up to the next byte the
         switch would handle specially. */
      if (in_warn_or_error)
        while (!pp_skip_stop_msg[*p])
          ++p;
      else
        while (!pp_skip_stop[*p]) {
          if (!prefix_len && isnum(*p)) {
            p = skip_pp_number(p);
            continue;
          }
          /* Keep the candidate prefix across refills and line splices.
             Saturation excludes longer identifiers without allocating. */
          if (isidnum_table[*p - CH_EOF] & (IS_ID | IS_NUM)) {
            if (prefix_len < 3) prefix[prefix_len] = *p;
            if (prefix_len < 4) ++prefix_len;
          } else {
            prefix_len = 0;
          }
          ++p;
        }
      break;
    }
    start_of_line = 0;
  }
the_end: ;
  file->buf_ptr = p;
}

#if 0
/* return the number of additional 'ints' necessary to store the
   token */
static inline int tok_size(const int *p)
{
  switch (*p)
  {
  // 4 Bytes
  case TOK_CINT:
  case TOK_CUINT:
  case TOK_U8CHAR:
  case TOK_CCHAR:
  case TOK_U16CHAR:
  case TOK_U32CHAR:
  case TOK_LCHAR:
  case TOK_CFLOAT:
  case TOK_LINENUM:
    return 1 + 1;
  case TOK_U8STR:
  case TOK_STR:
  case TOK_U16STR:
  case TOK_U32STR:
  case TOK_LSTR:
  case TOK_PPNUM:
  case TOK_PPSTR:
    return 1 + 1 + (p[1] + 3) / 4;
  case TOK_CLONG:
  case TOK_CULONG:
    return 1 + LONG_SIZE / 4;
  case TOK_CDOUBLE:
  case TOK_CLLONG:
  case TOK_CULLONG:
    return 1 + 2;
  case TOK_CLDOUBLE:
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
    return 1 + 8 / 4;
#else
    return 1 + LDOUBLE_SIZE / 4;
#endif
  default:
    return 1 + 0;
  }
}
#endif

// Token String Handling
ST_INLN void tok_str_new(TokenString *s)
{
  s->str = NULL;
  s->len = s->need_spc = 0;
  s->allocated_len = 0;
  s->last_line_num = -1;
}

ST_FUNC TokenString *tok_str_alloc(void)
{
  TokenString *str = tal_realloc(&tokstr_alloc, 0, sizeof *str);
  tok_str_new(str);
  return str;
}

ST_FUNC void tok_str_free_str(int *str)
{
  tal_free(&tokstr_alloc, str);
}

ST_FUNC void tok_str_free(TokenString *str)
{
  tok_str_free_str(str->str);
  tal_free(&tokstr_alloc, str);
}

ST_FUNC int *tok_str_realloc(TokenString *s, int new_size)
{
  int *str, size;

  size = s->allocated_len;
  if (size < 16)
    size = 16;
  while (size < new_size)
    size = size * 2;
  if (size > s->allocated_len)
  {
    str = tal_realloc(&tokstr_alloc, s->str, size *sizeof(int));
    s->allocated_len = size;
    s->str = str;
  }
  return s->str;
}

ST_FUNC void tok_str_add(TokenString *s, int t)
{
  int len, *str;

  len = s->len;
  str = s->str;
  if (len >= s->allocated_len)
    str = tok_str_realloc(s, len + 1);
  str[len++] = t;
  s->len = len;
}

ST_FUNC void begin_macro(TokenString *str, int alloc)
{
  str->alloc = alloc;
  str->prev = macro_stack;
  str->prev_ptr = macro_ptr;
  str->save_line_num = file->line_num;
  macro_ptr = str->str;
  macro_stack = str;
}

ST_FUNC void end_macro(void)
{
  TokenString *str = macro_stack;
  macro_stack = str->prev;
  macro_ptr = str->prev_ptr;
  file->line_num = str->save_line_num;
  if (str->alloc == 3)
  {
    return;
  }
  if (str->alloc == 0)
  {
    // matters if str not alloced, may be tokstr_buf
    str->len = str->need_spc = 0;
  }
  else
  {
    if (str->alloc == 2)
      str->str = NULL; // don't free
    tok_str_free(str);
  }
}

static void tok_str_add2(TokenString *s, int t, CValue *cv)
{
  int len, *str;

  len = s->len;
  str = s->str;

  // Allocate Space For Worst Case
  if (len + TOK_MAX_SIZE >= s->allocated_len)
    str = tok_str_realloc(s, len + TOK_MAX_SIZE + 1);
  str[len++] = t;
  switch (t)
  {
  case TOK_CINT:
  case TOK_CUINT:
  case TOK_U8CHAR:
  case TOK_CCHAR:
  case TOK_U16CHAR:
  case TOK_U32CHAR:
  case TOK_LCHAR:
  case TOK_CFLOAT:
  case TOK_LINENUM:
#if LONG_SIZE == 4
  case TOK_CLONG:
  case TOK_CULONG:
#endif
  case TOK_CIMAGI:
    str[len++] = cv->tab[0];
    break;
  case TOK_PPNUM:
  case TOK_PPSTR:
  case TOK_U8STR:
  case TOK_STR:
  case TOK_U16STR:
  case TOK_U32STR:
  case TOK_LSTR:
  {
    // Insert the string into the int array.
    size_t nb_words =
      1 + (cv->str.size + sizeof(int) - 1) / sizeof(int);
    if (len + nb_words >= s->allocated_len)
      str = tok_str_realloc(s, len + nb_words + 1);
    str[len] = cv->str.size;
    /* Saved-token signature hashes include payload words. Keep the unused
       tail bytes independent of previous allocations and translation units. */
    str[len + nb_words - 1] = 0;
    memcpy(&str[len + 1], cv->str.data, cv->str.size);
    len += nb_words;
  }
  break;
  case TOK_CDOUBLE:
  case TOK_CLLONG:
  case TOK_CULLONG:
  case TOK_CIMAGLL:
  case TOK_CIMAGD:
#if LONG_SIZE == 8
  case TOK_CLONG:
  case TOK_CULONG:
#endif
    str[len++] = cv->tab[0];
    str[len++] = cv->tab[1];
    break;
  case TOK_CLDOUBLE:
  case TOK_CIMAGL:
#if LDOUBLE_SIZE == 8 || defined CPRIME_USING_DOUBLE_FOR_LDOUBLE
    str[len++] = cv->tab[0];
    str[len++] = cv->tab[1];
#elif LDOUBLE_SIZE == 12
    str[len++] = cv->tab[0];
    str[len++] = cv->tab[1];
    str[len++] = cv->tab[2];
#elif LDOUBLE_SIZE == 16
    str[len++] = cv->tab[0];
    str[len++] = cv->tab[1];
    str[len++] = cv->tab[2];
    str[len++] = cv->tab[3];
#else
#error add long double size support
#endif
    break;
  default:
    break;
  }
  s->len = len;
}

// Add The Current Parse Token In Token String 'S'
ST_FUNC void tok_str_add_tok(TokenString *s)
{
  CValue cval;

  // Save Line Number Info
  if (file->line_num != s->last_line_num)
  {
    s->last_line_num = file->line_num;
    cval.i = s->last_line_num;
    tok_str_add2(s, TOK_LINENUM, &cval);
  }
  tok_str_add2(s, tok, &tokc);
}

// like tok_str_add2(), add a space if needed
static void tok_str_add2_spc(TokenString *s, int t, CValue *cv)
{
  if (s->need_spc == 3)
    tok_str_add(s, ' ');
  s->need_spc = 2;
  tok_str_add2(s, t, cv);
}

// Get A Token From An Integer Array And Increment Pointer.
static inline void tok_get(int *t, const int **pp, CValue *cv)
{
  const int *p = *pp;
  int n, *tab;

  tab = cv->tab;
  switch (*t = *p++)
  {
#if LONG_SIZE == 4
  case TOK_CLONG:
#endif
  case TOK_CINT:
  case TOK_U8CHAR:
  case TOK_CCHAR:
  case TOK_U16CHAR:
  case TOK_U32CHAR:
  case TOK_LCHAR:
  case TOK_LINENUM:
  case TOK_CIMAGI:
    cv->i = *p++;
    break;
#if LONG_SIZE == 4
  case TOK_CULONG:
#endif
  case TOK_CUINT:
    cv->i = (unsigned) * p++;
    break;
  case TOK_CFLOAT:
  case TOK_CIMAGF:
    tab[0] = *p++;
    break;
  case TOK_U8STR:
  case TOK_STR:
  case TOK_U16STR:
  case TOK_U32STR:
  case TOK_LSTR:
  case TOK_PPNUM:
  case TOK_PPSTR:
    cv->str.size = *p++;
    cv->str.data = (char *)p;
    p += (cv->str.size + sizeof(int) - 1) / sizeof(int);
    break;
  case TOK_CDOUBLE:
  case TOK_CLLONG:
  case TOK_CULLONG:
  case TOK_CIMAGLL:
  case TOK_CIMAGD:
#if LONG_SIZE == 8
  case TOK_CLONG:
  case TOK_CULONG:
#endif
    n = 2;
    goto copy;
  case TOK_CLDOUBLE:
  case TOK_CIMAGL:
#if LDOUBLE_SIZE == 8 || defined CPRIME_USING_DOUBLE_FOR_LDOUBLE
    n = 2;
#elif LDOUBLE_SIZE == 12
    n = 3;
#elif LDOUBLE_SIZE == 16
    n = 4;
#else
# error add long double size support
#endif
copy:
    do
      *tab++ = *p++;
    while (--n);
    break;
  default:
    break;
  }
  *pp = p;
}

#if 0
# define TOK_GET(t,p,c) tok_get(t,p,c)
#else
# define TOK_GET(t,p,c) do { \
    int _t = **(p); \
    if (TOK_HAS_VALUE(_t)) \
        tok_get(t, p, c); \
    else \
        *(t) = _t, ++*(p); \
    } while (0)
#endif

static int macro_is_equal(const int *a, const int *b)
{
  CValue cv;
  int t;

  if (!a || !b)
    return 1;

  while (*a && *b)
  {
    cstr_reset(&tokcstr);
    TOK_GET(&t, &a, &cv);
    cstr_cat(&tokcstr, get_tok_str(t, &cv), 0);
    TOK_GET(&t, &b, &cv);
    if (strcmp(tokcstr.data, get_tok_str(t, &cv)))
      return 0;
  }
  return !(*a || *b);
}

// Defines Handling
ST_INLN void define_push(int v, int macro_type, int *str, Sym *first_arg)
{
  Sym *s, *o;

  o = define_find(v);
  s = sym_push2(&define_stack, v, macro_type, 0);
  s->d = str;
  s->next = first_arg;
  table_ident[v - TOK_IDENT]->sym_define = s;

  if (o && !macro_is_equal(o->d, s->d))
    cprime_warning("%s redefined", get_tok_str(v, NULL));
}

// undefined a define symbol. Its name is just set to zero
ST_FUNC void define_undef(Sym *s)
{
  int v = s->v;
  if (v >= TOK_IDENT && v < tok_ident)
    table_ident[v - TOK_IDENT]->sym_define = NULL;
}

ST_INLN Sym *define_find(int v)
{
  v -= TOK_IDENT;
  if ((unsigned)v >= (unsigned)(tok_ident - TOK_IDENT))
    return NULL;
  return table_ident[v]->sym_define;
}

// Free Define Stack Until Top Reaches 'B'
ST_FUNC void free_defines(Sym *b)
{
  while (define_stack != b)
  {
    Sym *top = define_stack;
    define_stack = top->prev;
    tok_str_free_str(top->d);
    define_undef(top);
    sym_free(top);
  }
}

// Fake The Nth "#If Defined Test_..." For Cpc -Dt -Run
static void maybe_run_test(CPRIMEState *s)
{
  const char *p;
  if (s->include_stack_ptr != s->include_stack)
    return;
  p = get_tok_str(tok, NULL);
  if (0 != memcmp(p, "test_", 5))
    return;
  if (0 != --s->run_test)
    return;
  fprintf(s->ppfp, &"\n[%s]\n"[!(s->dflag & 32)], p), fflush(s->ppfp);
  define_push(tok, MACRO_OBJ, NULL, NULL);
}

ST_FUNC void skip_to_eol(int warn)
{
  if (tok == TOK_LINEFEED)
    return;
  if (warn)
    cprime_warning("extra tokens after directive");
  while (macro_stack)
    end_macro();
  file->buf_ptr = parse_line_comment(file->buf_ptr - 1);
  next_nomacro();
}

/* Return the value of the current TOK_PPNUM when it consists only of decimal
   digits.  Linemarker flags are written as one or more such numbers. */
static int parse_linemarker_flag(int *value)
{
  const char *q = tokc.str.data;
  int n = 0;

  if (!q || !*q)
    return 0;
  while (*q)
  {
    if (!isnum(*q))
      return 0;
    n = n * 10 + *q - '0';
    ++q;
  }
  *value = n;
  return 1;
}

static CachedInclude *
search_cached_include(CPRIMEState *s1, const char *filename, int add);

/* A compilation observes a fixed include search tree. Enumerate a directory
   once instead of asking the OS about each absent header in every -I path.
   The cache is discarded with preprocessor state, including between inputs. */
#ifdef _WIN32
typedef struct IncludeDirectoryFile {
  struct IncludeDirectoryFile *next;
  char name[1];
} IncludeDirectoryFile;
typedef struct IncludeDirectory {
  struct IncludeDirectory *next;
  IncludeDirectoryFile *files[64];
  int complete, probes;
  char path[1];
} IncludeDirectory;
static IncludeDirectory *include_directories[256];
typedef struct IncludeSearch {
  struct IncludeSearch *next;
  char *name, *origin, *filename;
  int start, index;
} IncludeSearch;
static IncludeSearch *include_searches[256];

static unsigned include_path_hash(const char *name)
{
  unsigned h = TOK_HASH_INIT;
  while (*name) h = TOK_HASH_FUNC(h, toup((unsigned char)*name++));
  return h;
}

static void include_directory_add(IncludeDirectory *dir, const char *name)
{
  unsigned h = include_path_hash(name) & 63;
  IncludeDirectoryFile *entry = cprime_malloc(sizeof(*entry) + strlen(name));
  strcpy(entry->name, name);
  entry->next = dir->files[h];
  dir->files[h] = entry;
}

static IncludeSearch *include_search(const char *name, const char *origin, int start)
{
  unsigned h = (include_path_hash(name) + include_path_hash(origin) + start) & 255;
  IncludeSearch *entry;
  for (entry = include_searches[h]; entry; entry = entry->next)
    if (entry->start == start && !PATHCMP(entry->name, name)
        && !PATHCMP(entry->origin, origin)) return entry;
  entry = cprime_mallocz(sizeof(*entry));
  entry->name = cprime_strdup(name);
  entry->origin = cprime_strdup(origin);
  entry->start = start;
  entry->next = include_searches[h];
  include_searches[h] = entry;
  return entry;
}

static void include_search_resolved(IncludeSearch *entry, const char *path, int index)
{
  if (!entry->filename) entry->filename = cprime_strdup(path);
  entry->index = index;
}

static int include_candidate_may_exist(const char *filename)
{
  char path[1024], pattern[1024];
  const char *name = cprime_basename(filename);
  IncludeDirectory *dir;
  IncludeDirectoryFile *entry;
  unsigned h;
  size_t len = name - filename;
  size_t stem = strcspn(name, ".");
  char device[9];
  unsigned k;
  if (stem < sizeof(device)) {
    for (k = 0; k < stem; ++k) device[k] = toup((unsigned char)name[k]);
    device[stem] = 0;
    if (!strcmp(device, "NUL") || !strcmp(device, "CON")
        || !strcmp(device, "PRN") || !strcmp(device, "AUX")
        || !strcmp(device, "CONIN$") || !strcmp(device, "CONOUT$")
        || (stem == 4 && device[3] >= '1' && device[3] <= '9'
            && (!memcmp(device, "COM", 3) || !memcmp(device, "LPT", 3))))
      return 1;
  }
  /* Preserve unusual Windows path spellings/device names through open(). */
  if (!*name || !strcmp(name, "-") || strchr(name, ':') || strchr(name, '*') || strchr(name, '?')
      || name[strlen(name)-1] == '.' || name[strlen(name)-1] == ' '
      || len + 2 >= sizeof(path)) return 1;
  memcpy(path, filename, len);
  path[len] = 0;
  h = include_path_hash(path) & 255;
  for (dir = include_directories[h]; dir; dir = dir->next)
    if (!PATHCMP(dir->path, path)) break;
  if (!dir) {
    dir = cprime_mallocz(sizeof(*dir) + len);
    strcpy(dir->path, path);
    dir->next = include_directories[h];
    include_directories[h] = dir;
  }
  /* A directory scan pays off only after repeated include searches. Small
     translation units should use the ordinary exact-file open instead. */
  if (dir->probes < 4) {
    WIN32_FIND_DATAA found;
    HANDLE handle;
    if (++dir->probes < 4) return 1;
    strcpy(pattern, path);
    strcat(pattern, "*");
    handle = FindFirstFileA(pattern, &found);
    if (handle == INVALID_HANDLE_VALUE) {
      DWORD error = GetLastError();
      dir->complete = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    } else {
      do {
        include_directory_add(dir, found.cFileName);
        if (found.cAlternateFileName[0])
          include_directory_add(dir, found.cAlternateFileName);
      } while (FindNextFileA(handle, &found));
      dir->complete = GetLastError() == ERROR_NO_MORE_FILES;
      FindClose(handle);
    }
  }
  if (!dir->complete) return 1;
  for (entry = dir->files[include_path_hash(name) & 63]; entry; entry = entry->next)
    if (!PATHCMP(entry->name, name)) return 1;
  return 0;
}

static void free_include_directories(void)
{
  int i, j;
  for (i = 0; i < 256; ++i) {
    IncludeDirectory *dir;
    IncludeSearch *search;
    while ((search = include_searches[i]) != NULL) {
      include_searches[i] = search->next;
      cprime_free(search->name);
      cprime_free(search->origin);
      cprime_free(search->filename);
      cprime_free(search);
    }
    while ((dir = include_directories[i]) != NULL) {
      include_directories[i] = dir->next;
      for (j = 0; j < 64; ++j) {
        IncludeDirectoryFile *entry;
        while ((entry = dir->files[j]) != NULL) {
          dir->files[j] = entry->next;
          cprime_free(entry);
        }
      }
      cprime_free(dir);
    }
  }
}
#endif

static int parse_include(CPRIMEState *s1, int do_next, int test)
{
  int c, i;
  char name[1024], buf[1024], *p;
  CachedInclude *e;
#ifdef _WIN32
  IncludeSearch *resolved;
  char origin[1024];
#endif

  c = skip_spaces();
  if (c == '<' || c == '\"')
  {
    cstr_reset(&tokcstr);
    file->buf_ptr = parse_pp_string(file->buf_ptr, c == '<' ? '>' : c, &tokcstr);
    i = tokcstr.size;
    pstrncpy(name, sizeof name, tokcstr.data, i);
    next_nomacro();
  }
  else
  {
    /* computed #include : concatenate tokens until result is one of
       the two accepted forms.  Don't convert pp-tokens to tokens here. */
    parse_flags = PARSE_FLAG_PREPROCESS
                  | PARSE_FLAG_LINEFEED
                  | (parse_flags &PARSE_FLAG_ASM_FILE);
    name[0] = 0;
    for (;;)
    {
      next();
      p = name, i = strlen(p) - 1;
      if (i > 0
          && ((p[0] == '"' && p[i] == '"')
              || (p[0] == '<' && p[i] == '>')))
        break;
      if (tok == TOK_LINEFEED)
        cprime_error("'#include' expects \"FILENAME\" or <FILENAME>");
      pstrcat(name, sizeof name, get_tok_str(tok, &tokc));
    }
    c = p[0];
    // Remove '<>|""'
    memmove(p, p + 1, i - 1), p[i - 1] = 0;
  }

  if (!test)
    skip_to_eol(1);

  i = do_next ? file->include_next_index : -1;
#ifdef _WIN32
  origin[0] = 0;
  if (c == '"')
    pstrncpy(origin, sizeof(origin), file->true_filename,
             cprime_basename(file->true_filename) - file->true_filename);
  resolved = include_search(name, origin, i);
  if (resolved->filename) {
    pstrcpy(buf, sizeof(buf), resolved->filename);
    i = resolved->index;
    goto resolved_include_candidate;
  }
#endif
  for (;;)
  {
    ++i;
    if (i == 0)
    {
      // Check Absolute Include Path
      if (!IS_ABSPATH(name))
        continue;
      buf[0] = '\0';
    }
    else if (i == 1)
    {
      // search in file's dir if "header.h"
      if (c != '\"')
        continue;
      p = file->true_filename;
      pstrncpy(buf, sizeof buf, p, cprime_basename(p) - p);
    }
    else
    {
      int j = i - 2, k = j - s1->nb_include_paths;
      if (k < 0)
        p = s1->include_paths[j];
      else if (k < s1->nb_sysinclude_paths)
        p = s1->sysinclude_paths[k];
      else if (test)
        return 0;
      else
        cprime_error("include file '%s' not found", name);
      pstrcpy(buf, sizeof buf, p);
      pstrcat(buf, sizeof buf, "/");
    }
    pstrcat(buf, sizeof buf, name);
#ifdef _WIN32
resolved_include_candidate:
#endif
    e = search_cached_include(s1, buf, 0);
    if (e && (define_find(e->ifndef_macro) || e->once))
    {
#ifdef _WIN32
      include_search_resolved(resolved, buf, i);
#endif
      /* no need to parse the include because the 'ifndef macro'
         is defined (or had #pragma once) */
#ifdef INC_DEBUG
      printf("%s: skipping cached %s\n", file->filename, buf);
#endif
      if ((s1->verbose | 1) == 3) // -Vv[V]
        printf("=> %*s%s\n",
               (int)(s1->include_stack_ptr - s1->include_stack), "", buf);
      return 1;
    }
#ifdef _WIN32
    if (!include_candidate_may_exist(buf)) continue;
#endif
    if (cprime_open(s1, buf) >= 0) {
#ifdef _WIN32
      include_search_resolved(resolved, buf, i);
#endif
      break;
    }
  }

  if (test)
    cprime_close();
  else
  {
    if (s1->include_stack_ptr >= s1->include_stack + INCLUDE_STACK_SIZE)
      cprime_error("#include recursion too deep");
    // Push Previous File On Stack
    *s1->include_stack_ptr++ = file->prev;
    file->include_next_index = i;
#ifdef INC_DEBUG
    printf("%s: including %s\n", file->prev->filename, file->filename);
#endif
    // Update Target Deps
    if (s1->gen_deps)
    {
      BufferedFile *bf = file;
      while (i == 1 && (bf = bf->prev))
        i = bf->include_next_index;
      // Skip System Include Files
      if (s1->include_sys_deps || i - 2 < s1->nb_include_paths)
        dynarray_add(&s1->target_deps, &s1->nb_target_deps,
                     cprime_strdup(buf));
    }
    // Add Include File Debug Info
    cprime_debug_bincl(s1);
  }
  return 1;
}

static int cprime_name_in_list(const char *name, const char *const *names,
                               unsigned count);

/* Attribute spellings reported through __has_attribute(). The queried name
   is macro substituted before the query is answered, so the answer has to
   come from the spelling: names without a keyword token (alloc_size,
   deprecated, musttail) arrive as ordinary identifiers. The list mirrors the
   attributes the frontend parses or harmlessly ignores. */
static const char *const cprime_attribute_names[] =
{
  "__always_inline__", "__const__", "__constructor__", "__destructor__",
  "__malloc__", "__mode__", "__noinline__", "__noreturn__", "__packed__",
  "__pure__", "__unused__", "__used__", "__visibility__",
  "__warn_unused_result__",
  "aligned", "alias", "alloc_align", "alloc_size", "always_inline",
  "cleanup", "cold", "const", "constructor", "deprecated", "destructor",
  "error", "flatten", "format", "format_arg", "gnu_inline", "hot", "leaf",
  "malloc", "mode", "musttail", "noinline", "nonnull", "noreturn", "nothrow",
  "packed", "pure", "returns_nonnull", "section", "sentinel", "unused",
  "used", "visibility", "warning", "warn_unused_result", "weak",
};

static int cprime_has_attribute(int attribute)
{
  return cprime_name_in_list(get_tok_str(attribute, NULL),
                             cprime_attribute_names,
                             sizeof(cprime_attribute_names)
                               / sizeof(cprime_attribute_names[0]));
}

/* Names GCC answers __has_builtin() for in both languages. Some are declared
   through cprimedefs.h rather than through a compiler token, and the library
   aliases (abs, isalpha) have no __builtin_ spelling. */
static const char *const cprime_common_builtin_names[] =
{
  "abs",
  "isalpha",
  "__builtin_abs",
  "__builtin_isalpha",
  "__builtin__Exit",
  "__builtin_alloca",
  "__builtin_apply",
  "__builtin_bswap16",
  "__builtin_bswap32",
  "__builtin_frob_return_addr",
  "__builtin_has_attribute",
  "__builtin_ia32_pause",
  "__builtin_inf",
  "__builtin_longjmp",
  "__builtin_nan",
  "__builtin_object_size",
  "__builtin_return",
  "__builtin_setjmp",
  "__builtin_trap",
  "__builtin_add_overflow",
  "__builtin_add_overflow_p",
  "__builtin_sadd_overflow",
  "__sync_add_and_fetch",
  "__sync_and_and_fetch",
  "__sync_bool_compare_and_swap",
  "__sync_fetch_and_add",
  "__sync_fetch_and_and",
  "__sync_fetch_and_nand",
  "__sync_fetch_and_or",
  "__sync_fetch_and_sub",
  "__sync_fetch_and_xor",
  "__sync_lock_release",
  "__sync_lock_test_and_set",
  "__sync_nand_and_fetch",
  "__sync_or_and_fetch",
  "__sync_sub_and_fetch",
  "__sync_synchronize",
  "__sync_val_compare_and_swap",
  "__sync_xor_and_fetch",
};

/* C++-only spellings. GCC answers __has_builtin() for the standard trait
   builtins and for the cast helpers only while compiling C++. */
static const char *const cprime_cpp_builtin_names[] =
{
  "__add_lvalue_reference",
  "__add_pointer",
  "__add_rvalue_reference",
  "__array_rank",
  "__decay",
  "__remove_all_extents",
  "__remove_cv",
  "__remove_cvref",
  "__remove_extent",
  "__remove_pointer",
  "__remove_reference",
  "__underlying_type",
  "__reference_constructs_from_temporary",
  "__reference_converts_from_temporary",
  "__has_nothrow_assign",
  "__has_nothrow_constructor",
  "__has_nothrow_copy",
  "__has_trivial_assign",
  "__has_trivial_constructor",
  "__has_trivial_copy",
  "__has_trivial_destructor",
  "__has_unique_object_representations",
  "__has_virtual_destructor",
  "__is_abstract",
  "__is_aggregate",
  "__is_array",
  "__is_assignable",
  "__is_base_of",
  "__is_bounded_array",
  "__is_class",
  "__is_const",
  "__is_constructible",
  "__is_convertible",
  "__is_empty",
  "__is_enum",
  "__is_final",
  "__is_function",
  "__is_invocable",
  "__is_layout_compatible",
  "__is_literal_type",
  "__is_member_function_pointer",
  "__is_member_object_pointer",
  "__is_member_pointer",
  "__is_nothrow_assignable",
  "__is_nothrow_constructible",
  "__is_nothrow_convertible",
  "__is_nothrow_invocable",
  "__is_object",
  "__is_pod",
  "__is_pointer",
  "__is_pointer_interconvertible_base_of",
  "__is_polymorphic",
  "__is_reference",
  "__is_same",
  "__is_same_as",
  "__is_scoped_enum",
  "__is_standard_layout",
  "__is_trivial",
  "__is_trivially_assignable",
  "__is_trivially_constructible",
  "__is_trivially_copyable",
  "__is_unbounded_array",
  "__is_union",
  "__is_volatile",
  "__builtin_bit_cast",
  "__builtin_is_constant_evaluated",
  "__builtin_is_corresponding_member",
  "__builtin_is_pointer_interconvertible_with_class",
  "__builtin_source_location",
};

static int cprime_name_in_list(const char *name, const char *const *names,
                               unsigned count)
{
  unsigned i;
  for (i = 0; i < count; ++i)
    if (!strcmp(name, names[i]))
      return 1;
  return 0;
}

static int cprime_has_builtin(int builtin)
{
  const char *name = get_tok_str(builtin, NULL);
  /* These tokens have compiler expression handlers. Macro-backed varargs
     operations are also available during preprocessing-only queries. */
  if (builtin == TOK_builtin_types_compatible_p)
    return !cprime_cpp_mode;
  if (builtin == TOK_builtin_addressof || builtin == TOK_builtin_launder)
    return cprime_cpp_mode;
  if ((builtin >= TOK_builtin_types_compatible_p && builtin <= TOK_builtin_unreachable)
      || (builtin >= TOK___atomic_store && builtin <= TOK___atomic_compare_exchange_n))
    return 1;
  if (cprime_name_in_list(name, cprime_common_builtin_names,
                          sizeof(cprime_common_builtin_names) / sizeof(cprime_common_builtin_names[0]))
      || (cprime_cpp_mode
          && cprime_name_in_list(name, cprime_cpp_builtin_names,
                                 sizeof(cprime_cpp_builtin_names) / sizeof(cprime_cpp_builtin_names[0]))))
    return 1;
  if (!strcmp(name, "__builtin_va_start") || !strcmp(name, "__builtin_va_arg")
      || !strcmp(name, "__builtin_va_end") || !strcmp(name, "__builtin_va_copy"))
    return 1;
  return !strncmp(name, "__builtin_", 10) && define_find(builtin) != NULL;
}

/* Names GCC answers __has_feature()/__has_extension() for in every language. */
static const char *const cprime_common_feature_names[] =
{
  "__enumerator_attributes__",
  "__attribute_deprecated_with_message__",
  "__attribute_unavailable_with_message__",
  "__tls__",
  "enumerator_attributes",
  "attribute_deprecated_with_message",
  "attribute_unavailable_with_message",
  "tls",
};

/* Spellings GCC reports as an extension only. */
static const char *const cprime_extension_feature_names[] =
{
  "__gnu_asm_goto_with_outputs__",
  "__gnu_asm_goto_with_outputs_full__",
  "gnu_asm_goto_with_outputs",
  "gnu_asm_goto_with_outputs_full",
};

/* C++ language features. cxx_exceptions and cxx_rtti stay unknown because the
   __cpp_exceptions/__cpp_rtti predefines are not emitted. */
static const char *const cprime_cpp_feature_names[] =
{
  "cxx_access_control_sfinae",
  "cxx_aggregate_nsdmi",
  "cxx_alias_templates",
  "cxx_alignas",
  "cxx_alignof",
  "cxx_attributes",
  "cxx_auto_type",
  "cxx_binary_literals",
  "cxx_constexpr",
  "cxx_decltype",
  "cxx_decltype_auto",
  "cxx_decltype_incomplete_return_types",
  "cxx_default_function_template_args",
  "cxx_defaulted_functions",
  "cxx_delegating_constructors",
  "cxx_deleted_functions",
  "cxx_explicit_conversions",
  "cxx_generalized_initializers",
  "cxx_generic_lambdas",
  "cxx_implicit_moves",
  "cxx_inheriting_constructors",
  "cxx_init_captures",
  "cxx_inline_namespaces",
  "cxx_lambdas",
  "cxx_local_type_template_args",
  "cxx_noexcept",
  "cxx_nonstatic_member_init",
  "cxx_nullptr",
  "cxx_override_control",
  "cxx_range_for",
  "cxx_raw_string_literals",
  "cxx_reference_qualified_functions",
  "cxx_relaxed_constexpr",
  "cxx_return_type_deduction",
  "cxx_rvalue_references",
  "cxx_static_assert",
  "cxx_strong_enums",
  "cxx_thread_local",
  "cxx_trailing_return",
  "cxx_unicode_literals",
  "cxx_unrestricted_unions",
  "cxx_user_literals",
  "cxx_variable_templates",
  "cxx_variadic_templates",
};

static int cprime_has_feature(const char *name, int extension)
{
  if (cprime_name_in_list(name, cprime_common_feature_names,
                          sizeof(cprime_common_feature_names) / sizeof(cprime_common_feature_names[0])))
    return 1;
  if (cprime_name_in_list(name, cprime_extension_feature_names,
                          sizeof(cprime_extension_feature_names) / sizeof(cprime_extension_feature_names[0])))
    return extension;
  if (cprime_cpp_mode
      && cprime_name_in_list(name, cprime_cpp_feature_names,
                             sizeof(cprime_cpp_feature_names) / sizeof(cprime_cpp_feature_names[0])))
    return 1;
  return 0;
}

/* 0 = attribute, 1 = builtin, 2 = feature, 3 = extension. */
static int cprime_capability_kind(int t)
{
  switch (t)
  {
  case TOK___HAS_ATTRIBUTE: return 0;
  case TOK___HAS_BUILTIN: return 1;
  case TOK___HAS_FEATURE: return 2;
  default: return 3;
  }
}

/* Leave the closing parenthesis for the caller's normal token advance. */
static int cprime_capability_query(int kind)
{
  int name, supported_scope = 1;
  next();
  if (tok != '(') expect("'('");
  next();
  if (tok < TOK_IDENT) expect("capability name");
  name = tok;
  next();
  if (kind < 2 && tok == ':') {
    const char *scope = get_tok_str(name, NULL);
    supported_scope = !strcmp(scope, "gnu") || !strcmp(scope, "__gnu__")
      || !strcmp(scope, "clang") || !strcmp(scope, "__clang__");
    next();
    if (tok != ':') expect("'::'");
    next();
    if (tok < TOK_IDENT) expect("attribute name");
    name = tok;
    next();
  }
  if (tok != ')') expect("')'");
  if (kind == 0)
    return supported_scope && cprime_has_attribute(name);
  if (kind == 1)
    return supported_scope && cprime_has_builtin(name);
  return cprime_has_feature(get_tok_str(name, NULL), kind == 3);
}

// Eval An Expression For #If/#Elif
static int expr_preprocess(CPRIMEState *s1)
{
  int c, t;
  int t0 = tok;
  TokenString *str;

  str = tok_str_alloc();
  pp_expr = 1;
  while (1)
  {
    next(); // do macro subst
    t = tok;
    if (tok < TOK_IDENT)
    {
      if (tok == TOK_LINEFEED || tok == TOK_EOF)
        break;
      if ((tok >= TOK_STR && tok <= TOK_CLDOUBLE)
          || (tok >= TOK_CIMAGI && tok <= TOK_CIMAGL))
        cprime_error("invalid constant in preprocessor expression");

    }
    else if (tok == TOK_DEFINED)
    {
      parse_flags &= ~PARSE_FLAG_PREPROCESS; // No Macro Subst
      next();
      t = tok;
      if (t == '(')
        next();
      parse_flags |= PARSE_FLAG_PREPROCESS;
      if (tok < TOK_IDENT)
        expect("identifier after 'defined'");
      if (s1->run_test)
        maybe_run_test(s1);
      c = 0;
     if (define_find(tok)
          || tok == TOK___HAS_INCLUDE
          || tok == TOK___HAS_INCLUDE_NEXT
          || tok == TOK___HAS_ATTRIBUTE || tok == TOK___HAS_BUILTIN
          || tok == TOK___HAS_FEATURE || tok == TOK___HAS_EXTENSION)
        c = 1;
      if (t == '(')
      {
        next();
        if (tok != ')')
          expect("')'");
      }
      goto c_number;
    }
    else if (tok == TOK___HAS_ATTRIBUTE || tok == TOK___HAS_BUILTIN
             || tok == TOK___HAS_FEATURE || tok == TOK___HAS_EXTENSION)
    {
      c = cprime_capability_query(cprime_capability_kind(tok));
      goto c_number;
    }
    else if (tok == TOK___HAS_INCLUDE ||
             tok == TOK___HAS_INCLUDE_NEXT)
    {
      t = tok;
      next();
      if (tok != '(')
        expect("'('");
      c = parse_include(s1, t - TOK___HAS_INCLUDE, 1);
      if (tok != ')')
        expect("')'");
      goto c_number;
    }
    else
    {
      // if undefined macro, replace with zero
      c = 0;
c_number:
      tok = TOK_CLLONG; // Type Intmax_T
      tokc.i = c;
    }
    tok_str_add_tok(str);
  }
  if (0 == str->len)
    cprime_error("#%s with no expression", get_tok_str(t0, 0));
  tok_str_add(str, TOK_EOF); // Simulate End Of File
  pp_expr = t0; // Redirect Pre-Processor Expression Error Messages
  t = tok;
  // now evaluate C constant expression
  begin_macro(str, 1);
  next();
  c = expr_const();
  if (tok != TOK_EOF)
    cprime_error("...");
  pp_expr = 0;
  end_macro();
  tok = t; // restore LF or EOF
  return c != 0;
}

ST_FUNC void pp_error(CString *cs)
{
  cstr_printf(cs, "bad preprocessor expression: #%s", get_tok_str(pp_expr, 0));
  macro_ptr = macro_stack->str;
  while (next(), tok != TOK_EOF)
    cstr_printf(cs, " %s", get_tok_str(tok, &tokc));
}

// Parse After #Define
ST_FUNC void parse_define(void)
{
  Sym *s, *first, **ps;
  int v, t, varg, is_vaargs, t0;
  int saved_parse_flags = parse_flags;
  TokenString str;

  v = tok;
  if (v < TOK_IDENT || v == TOK_DEFINED)
    cprime_error("invalid macro name '%s'", get_tok_str(tok, &tokc));
  first = NULL;
  t = MACRO_OBJ;
  /* We have to parse the whole define as if not in asm mode, in particular
     no line comment with '#' must be ignored.  Also for function
     macros the argument list must be parsed without '.' being an ID
     character.  */
  parse_flags = ((parse_flags & ~PARSE_FLAG_ASM_FILE) | PARSE_FLAG_SPACES);
  // '(' must be just after macro definition for MACRO_FUNC
  next_nomacro();
  parse_flags &= ~PARSE_FLAG_SPACES;
  is_vaargs = 0;
  if (tok == '(')
  {
    int dotid = set_idnum('.', 0);
    next_nomacro();
    ps = &first;
    if (tok != ')') for (;;)
      {
        varg = tok;
        next_nomacro();
        is_vaargs = 0;
        if (varg == TOK_DOTS)
        {
          varg = TOK___VA_ARGS__;
          is_vaargs = 1;
        }
        else if (tok == TOK_DOTS && non_iso)
        {
          is_vaargs = 1;
          next_nomacro();
        }
        if (varg < TOK_IDENT)
bad_list:
          cprime_error("bad macro parameter list");
        s = sym_push2(&define_stack, varg | SYM_FIELD, is_vaargs, 0);
        *ps = s;
        ps = &s->next;
        if (tok == ')')
          break;
        if (tok != ',' || is_vaargs)
          goto bad_list;
        next_nomacro();
      }
    parse_flags |= PARSE_FLAG_SPACES;
    next_nomacro();
    t = MACRO_FUNC;
    set_idnum('.', dotid);
  }

  /* The body of a macro definition should be parsed such that identifiers
     are parsed like the file mode determines (i.e. with '.' being an
     ID character in asm mode).  But '#' should be retained instead of
     regarded as line comment leader, so still don't set ASM_FILE
     in parse_flags. */
  parse_flags |= PARSE_FLAG_ACCEPT_STRAYS | PARSE_FLAG_SPACES | PARSE_FLAG_LINEFEED;
  tok_str_new(&str);
  t0 = 0;
  while (tok != TOK_LINEFEED && tok != TOK_EOF)
  {
    if (is_space(tok))
      str.need_spc |= 1;
    else
    {
      if (TOK_TWOSHARPS == tok)
      {
        if (0 == t0)
          goto bad_twosharp;
        tok = TOK_PPJOIN;
        t |= MACRO_JOIN;
      }
      tok_str_add2_spc(&str, tok, &tokc);
      t0 = tok;
    }
    next_nomacro();
  }
  parse_flags = saved_parse_flags;
  tok_str_add(&str, 0);
  if (t0 == TOK_PPJOIN)
bad_twosharp:
    cprime_error("'##' cannot appear at either end of macro");
  define_push(v, t, str.str, first);
  //tok_print(str.str, "#define (%d) %s %d:", t | is_vaargs * 4, get_tok_str(v, 0));
}

static CachedInclude *search_cached_include(CPRIMEState *s1, const char *filename, int add)
{
  const char *s, *basename;
  unsigned int h;
  CachedInclude *e;
  int c, i, len;

  s = basename = cprime_basename(filename);
  h = TOK_HASH_INIT;
  while ((c = (unsigned char) * s) != 0)
  {
#ifdef _WIN32
    h = TOK_HASH_FUNC(h, toup(c));
#else
    h = TOK_HASH_FUNC(h, c);
#endif
    s++;
  }
  h &= (CACHED_INCLUDES_HASH_SIZE - 1);

  i = s1->cached_includes_hash[h];
  for (;;)
  {
    if (i == 0)
      break;
    e = s1->cached_includes[i - 1];
    if (0 == PATHCMP(filename, e->filename))
      return e;
    if (e->once
        && 0 == PATHCMP(basename, cprime_basename(e->filename))
        && 0 == normalized_PATHCMP(filename, e->filename)
       )
      return e;
    i = e->hash_next;
  }
  if (!add)
    return NULL;

  e = cprime_malloc(sizeof(CachedInclude) + (len = strlen(filename)));
  memcpy(e->filename, filename, len + 1);
  e->ifndef_macro = e->once = 0;
  dynarray_add(&s1->cached_includes, &s1->nb_cached_includes, e);
  // Add In Hash Table
  e->hash_next = s1->cached_includes_hash[h];
  s1->cached_includes_hash[h] = s1->nb_cached_includes;
#ifdef INC_DEBUG
  printf("adding cached '%s'\n", filename);
#endif
  return e;
}

static int pragma_parse(CPRIMEState *s1)
{
  next_nomacro();
  if (tok == TOK_push_macro || tok == TOK_pop_macro)
  {
    int t = tok, v;
    Sym *s;

    if (next(), tok != '(')
      goto pragma_err;
    if (next(), tok != TOK_STR)
      goto pragma_err;
    v = tok_alloc(tokc.str.data, tokc.str.size - 1)->tok;
    if (next(), tok != ')')
      goto pragma_err;
    if (t == TOK_push_macro)
    {
      while (NULL == (s = define_find(v)))
        define_push(v, 0, NULL, NULL);
      s->type.ref = s; // Set Push Boundary
    }
    else
    {
      for (s = define_stack; s; s = s->prev)
        if (s->v == v && s->type.ref == s)
        {
          s->type.ref = NULL;
          break;
        }
    }
    if (s)
      table_ident[v - TOK_IDENT]->sym_define = s->d ? s : NULL;
    else
      cprime_warning("unbalanced #pragma pop_macro");
    pp_debug_tok = t, pp_debug_symv = v;

  }
  else if (tok == TOK_once)
    search_cached_include(s1, file->true_filename, 1)->once = 1;

  else if (s1->output_type == CPRIME_OUTPUT_PREPROCESS)
  {
    // cpc -E: keep pragmas below unchanged
    unget_tok(' ');
    unget_tok(TOK_PRAGMA);
    unget_tok('#');
    unget_tok(TOK_LINEFEED);
    return 1;

  }
  else if (tok == TOK_pack)
  {
    /* This may be:
       #pragma pack(1) // Set
       #pragma pack() // Reset To Default
       #pragma pack(push) // Push Current
       #pragma pack(push,1) // Push & Set
       #pragma pack(pop) // restore previous */
    next();
    skip('(');
    if (tok == TOK_ASM_pop)
    {
      next();
      if (s1->pack_stack_ptr <= s1->pack_stack)
      {
stk_error:
        cprime_error("out of pack stack");
      }
      s1->pack_stack_ptr--;
    }
    else
    {
      int val = 0;
      if (tok != ')')
      {
        if (tok == TOK_ASM_push)
        {
          next();
          if (s1->pack_stack_ptr >= s1->pack_stack + PACK_STACK_SIZE - 1)
            goto stk_error;
          val = *s1->pack_stack_ptr++;
          if (tok != ',')
            goto pack_set;
          next();
        }
        if (tok != TOK_CINT)
          goto pragma_err;
        val = tokc.i;
        if (val < 1 || val > 16 || (val & (val - 1)) != 0)
          goto pragma_err;
        next();
      }
pack_set:
      *s1->pack_stack_ptr = val;
    }
    if (tok != ')')
      goto pragma_err;

  }
  else if (tok == TOK_comment)
  {
    char *p;
    char pragma_arg[1024];
    int t;
    next();
    skip('(');
    t = tok;
    next();
    skip(',');
    if (tok != TOK_STR)
      goto pragma_err;
    pragma_arg[0] = '\0';
    while (tok == TOK_STR)
    {
      pstrcat(pragma_arg, sizeof(pragma_arg), tokc.str.data);
      next();
    }
    p = cprime_strdup(pragma_arg);
    if (tok != ')')
      goto pragma_err;
    if (t == TOK_lib)
    {
      cprime_add_pragma_library(s1, p);
      cprime_free(p);
    }
    else
    {
      if (t == TOK_option)
        cprime_set_options(s1, p);
      cprime_free(p);
    }

  }
  else
  {
    cprime_warning_c(warn_all)("#pragma %s ignored", get_tok_str(tok, &tokc));
    return 0;
  }
  next();
  return 1;
pragma_err:
  cprime_error("malformed #pragma directive");
}

// Put Alternative Filename
ST_FUNC void cprimepp_putfile(const char *filename)
{
  char buf[1024];
  buf[0] = 0;
  if (!IS_ABSPATH(filename))
  {
    // Prepend Directory From Real File
    pstrcpy(buf, sizeof buf, file->true_filename);
    *cprime_basename(buf) = 0;
  }
  pstrcat(buf, sizeof buf, filename);
#ifdef _WIN32
  normalize_slashes(buf);
#endif
  if (0 == strcmp(file->filename, buf))
    return;
  //printf("new file '%s'\n", buf);
  if (file->true_filename == file->filename)
    file->true_filename = cprime_strdup(file->filename);
  pstrcpy(file->filename, sizeof file->filename, buf);
  cprime_debug_newfile(cprime_state);
}

// is_bof is true if first non space token at beginning of file
ST_FUNC void preprocess(int is_bof)
{
  CPRIMEState *s1 = cprime_state;
  int c, n, saved_parse_flags, elif_directive;
  char buf[1024], *q;
  Sym *s;

  saved_parse_flags = parse_flags;
  parse_flags = PARSE_FLAG_PREPROCESS
                | PARSE_FLAG_TOK_NUM
                | PARSE_FLAG_TOK_STR
                | PARSE_FLAG_LINEFEED
                | (parse_flags &PARSE_FLAG_ASM_FILE)
                ;

  next_nomacro();
redo:
  elif_directive = 0;
  switch (tok)
  {
  case TOK_DEFINE:
    pp_debug_tok = tok;
    next_nomacro();
    pp_debug_symv = tok;
    parse_define();
    break;
  case TOK_UNDEF:
    pp_debug_tok = tok;
    next_nomacro();
    pp_debug_symv = tok;
    s = define_find(tok);
    // Undefine Symbol By Putting An Invalid Name
    if (s)
      define_undef(s);
    next_nomacro();
    break;
  case TOK_INCLUDE:
  case TOK_INCLUDE_NEXT:
    parse_include(s1, tok - TOK_INCLUDE, 0);
    goto the_end;
  case TOK_IFNDEF:
    c = 1;
    goto do_ifdef;
  case TOK_IF:
    c = expr_preprocess(s1);
    goto do_if;
  case TOK_IFDEF:
    c = 0;
do_ifdef:
    next_nomacro();
    if (tok < TOK_IDENT)
      cprime_error("invalid argument for '#%s%sdef'", elif_directive ? "elif" : "if",
                   c ? "n" : "");
    if (is_bof && !elif_directive)
    {
      if (c)
      {
#ifdef INC_DEBUG
        printf("#ifndef %s\n", get_tok_str(tok, NULL));
#endif
        file->ifndef_macro = tok;
      }
    }
    if (define_find(tok)
        || tok == TOK___HAS_INCLUDE
        || tok == TOK___HAS_INCLUDE_NEXT
          || tok == TOK___HAS_ATTRIBUTE || tok == TOK___HAS_BUILTIN
          || tok == TOK___HAS_FEATURE || tok == TOK___HAS_EXTENSION)
      c ^= 1;
    next_nomacro();
    if (elif_directive) {
      s1->ifdef_stack_ptr[-1] = c;
      goto test_else;
    }
do_if:
    if (s1->ifdef_stack_ptr >= s1->ifdef_stack + IFDEF_STACK_SIZE)
      cprime_error("memory full (ifdef)");
    *s1->ifdef_stack_ptr++ = c;
    goto test_skip;
  case TOK_ELSE:
    next_nomacro();
    if (s1->ifdef_stack_ptr == s1->ifdef_stack)
      cprime_error("#else without matching #if");
    if (s1->ifdef_stack_ptr[-1] & 2)
      cprime_error("#else after #else");
    c = (s1->ifdef_stack_ptr[-1] ^= 3);
    goto test_else;
  case TOK_ELIF:
  case TOK_ELIFDEF:
  case TOK_ELIFNDEF:
    elif_directive = tok;
    if (s1->ifdef_stack_ptr == s1->ifdef_stack)
      cprime_error("#elif without matching #if");
    c = s1->ifdef_stack_ptr[-1];
    if (c > 1)
      cprime_error("#elif after #else");
    // Last #If/#Elif Expression Was True: We Skip
    if (c == 1)
    {
      skip_to_eol(0);
      c = 0;
    }
    else
    {
      if (elif_directive != TOK_ELIF) {
        c = elif_directive == TOK_ELIFNDEF;
        goto do_ifdef;
      }
      c = expr_preprocess(s1);
      s1->ifdef_stack_ptr[-1] = c;
    }
test_else:
    if (s1->ifdef_stack_ptr == file->ifdef_stack_ptr + 1)
      file->ifndef_macro = 0;
test_skip:
    if (!(c & 1))
    {
      skip_to_eol(1);
      preprocess_skip();
      is_bof = 0;
      goto redo;
    }
    break;
  case TOK_ENDIF:
    next_nomacro();
    if (s1->ifdef_stack_ptr <= file->ifdef_stack_ptr)
      cprime_error("#endif without matching #if");
    s1->ifdef_stack_ptr--;
    /* '#ifndef macro' was at the start of file. Now we check if
       an '#endif' is exactly at the end of file */
    if (file->ifndef_macro &&
        s1->ifdef_stack_ptr == file->ifdef_stack_ptr)
    {
      file->ifndef_macro_saved = file->ifndef_macro;
      /* need to set to zero to avoid false matches if another
         #ifndef at middle of file */
      file->ifndef_macro = 0;
      tok_flags |= TOK_FLAG_ENDIF;
    }
    break;

  case TOK_LINE:
    parse_flags &= ~PARSE_FLAG_TOK_NUM;
    next();
    if (tok != TOK_PPNUM)
    {
_line_err:
      cprime_error("wrong #line format");
    }
    c = 1;
    goto _line_num;
  case TOK_PPNUM:
    if (parse_flags & PARSE_FLAG_ASM_FILE)
      goto ignore;
    c = 0; // No Error With Extra Tokens
_line_num:
    for (n = 0, q = tokc.str.data; *q; ++q)
    {
      if (!isnum(*q))
        goto _line_err;
      n = n * 10 + *q - '0';
    }
    parse_flags &= ~PARSE_FLAG_TOK_STR; // don't parse escape sequences
    if (c == 0)
      parse_flags &= ~PARSE_FLAG_TOK_NUM;
    next();
    if (tok != TOK_LINEFEED)
    {
      int marker_flags = 0;

      if (tok != TOK_PPSTR || tokc.str.data[0] != '"')
        goto _line_err;
      tokc.str.data[tokc.str.size - 2] = 0;
      cprimepp_putfile(tokc.str.data + 1);
      next();
      if (c == 0)
      {
        /* Linemarkers carry flags after the file name.  Flag 3 marks the
           following text as coming from a system header. */
        while (tok == TOK_PPNUM)
        {
          int flag;
          if (!parse_linemarker_flag(&flag))
            break;
          marker_flags |= flag;
          next();
        }
        file->sys_header = (marker_flags & 3) != 0;
      }
      // Skip Optional Level Number & Advance To Next Line
      skip_to_eol(c);
    }
    if (file->fd > 0)
      total_lines += file->line_num - n;
    file->line_num = n;
    break;

  case TOK_ERROR:
  case TOK_WARNING:
  {
    q = buf;
    c = skip_spaces();
    while (c != '\n' && c != CH_EOF)
    {
      if ((q - buf) < sizeof(buf) - 1)
        *q++ = c;
      c = ninp();
    }
    *q = '\0';
    if (tok == TOK_ERROR)
      cprime_error("#error %s", buf);
    else
      cprime_warning("#warning %s", buf);
    next_nomacro();
    break;
  }
  case TOK_PRAGMA:
    if (!pragma_parse(s1))
      goto ignore;
    break;
  case TOK_LINEFEED:
    goto the_end;
  default:
    // ignore gas line comment in an 'S' file.
    if (saved_parse_flags & PARSE_FLAG_ASM_FILE)
      goto ignore;
    if (tok == '!' && is_bof)
      // '#!' is ignored at beginning to allow C scripts.
      goto ignore;
    cprime_warning("ignoring unknown preprocessing directive #%s", get_tok_str(tok, &tokc));
ignore:
    skip_to_eol(0);
    goto the_end;
  }
  skip_to_eol(1);
the_end:
  parse_flags = saved_parse_flags;
}

// Evaluate Escape Codes In A String.
static void parse_escape_string(CString *outstr, const uint8_t *buf, int is_long)
{
  int c, n, i, unicode_escape;
  const uint8_t *p;

  p = buf;
  for (;;)
  {
    c = *p;
    if (c == '\0')
      break;
    if (c == '\\')
    {
      p++;
      // Escape
      c = *p;
      unicode_escape = c == 'u' || c == 'U';
      switch (c)
      {
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        // At Most Three Octal Digits
        n = c - '0';
        p++;
        c = *p;
        if (isoct(c))
        {
          n = n * 8 + c - '0';
          p++;
          c = *p;
          if (isoct(c))
          {
            n = n * 8 + c - '0';
            p++;
          }
        }
        c = n;
        goto add_char_nonext;
      case 'x': i = 0; goto parse_hex_or_ucn;
      case 'u': i = 4; goto parse_hex_or_ucn;
      case 'U': i = 8; goto parse_hex_or_ucn;
parse_hex_or_ucn:
        p++;
        n = 0;
        do
        {
          c = *p;
          if (c >= 'a' && c <= 'f')
            c = c - 'a' + 10;
          else if (c >= 'A' && c <= 'F')
            c = c - 'A' + 10;
          else if (isnum(c))
            c = c - '0';
          else if (i >= 0)
            expect("more hex digits in universal-character-name");
          else
            goto add_hex_or_ucn;
          if (!unicode_escape && (is_long == 'u' || is_long == 'U')) {
            unsigned limit = is_long == 'u' ? 0xFFFFu : UINT32_MAX;
            if ((unsigned)n > (limit - (unsigned)c) / 16)
              cprime_error("numeric escape exceeds encoded code-unit range");
          }
          n = (unsigned) n * 16 + c;
          p++;
        }
        while (--i);
        if (unicode_escape && ((unsigned)n > 0x10FFFF
            || (n >= 0xD800 && n <= 0xDFFF)))
          cprime_error("invalid universal character name");
        if (is_long)
        {
add_hex_or_ucn:
          c = n;
          goto add_char_nonext;
        }
        cstr_u8cat(outstr, n);
        continue;
      case 'a':
        c = '\a';
        break;
      case 'b':
        c = '\b';
        break;
      case 'f':
        c = '\f';
        break;
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      case 'v':
        c = '\v';
        break;
      case 'e':
        if (!non_iso)
          goto invalid_escape;
        c = 27;
        break;
      case '\'':
      case '\"':
      case '\\':
      case '?':
        break;
      default:
invalid_escape:
        if (c >= '!' && c <= '~')
          cprime_warning("unknown escape sequence: \'\\%c\'", c);
        else
          cprime_warning("unknown escape sequence: \'\\x%x\'", c);
        break;
      }
    }
    else if (is_long && c >= 0x80)
    {
      // assume we are processing UTF-8 sequence
      // reference: The Unicode Standard, Version 10.0, ch3.9

      int cont; // Count Of Continuation Bytes
      int skip; // How Many Bytes Should Skip When Error Occurred
      int i;

      // Decode Leading Byte
      if (c < 0xC2)
      {
        skip = 1; goto invalid_utf8_sequence;
      }
      else if (c <= 0xDF)
      {
        cont = 1; n = c & 0x1f;
      }
      else if (c <= 0xEF)
      {
        cont = 2; n = c & 0xf;
      }
      else if (c <= 0xF4)
      {
        cont = 3; n = c & 0x7;
      }
      else
      {
        skip = 1; goto invalid_utf8_sequence;
      }

      // Decode Continuation Bytes
      for (i = 1; i <= cont; i++)
      {
        int l = 0x80, h = 0xBF;

        // Adjust Limit For Second Byte
        if (i == 1)
        {
          switch (c)
          {
          case 0xE0: l = 0xA0; break;
          case 0xED: h = 0x9F; break;
          case 0xF0: l = 0x90; break;
          case 0xF4: h = 0x8F; break;
          }
        }

        if (p[i] < l || p[i] > h)
        {
          skip = i; goto invalid_utf8_sequence;
        }

        n = (n << 6) | (p[i] & 0x3f);
      }

      // Advance Pointer
      p += 1 + cont;
      c = n;
      goto add_char_nonext;

      // Error Handling
invalid_utf8_sequence:
      cprime_warning("ill-formed UTF-8 subsequence starting with: \'\\x%x\'", c);
      c = 0xFFFD;
      p += skip;
      goto add_char_nonext;

    }
    p++;
add_char_nonext:
    if (!is_long)
      cstr_ccat(outstr, c);
    else
    {
      int width = is_long == 'u' ? 2 : is_long == 'U' ? 4 : sizeof(nwchar_t);
      if (width == 2) {
        uint16_t unit;
        if (c < 0x10000) {
          unit = c;
          cstr_cat(outstr, (char *)&unit, 2);
        } else {
          c -= 0x10000;
          unit = (c >> 10) + 0xD800;
          cstr_cat(outstr, (char *)&unit, 2);
          unit = (c & 0x3FF) + 0xDC00;
          cstr_cat(outstr, (char *)&unit, 2);
        }
      } else {
        uint32_t unit = c;
        cstr_cat(outstr, (char *)&unit, 4);
      }
    }
  }
  // Add A Trailing '\0'
  if (!is_long)
    cstr_ccat(outstr, '\0');
  else
  {
    uint32_t zero = 0;
    cstr_cat(outstr, (char *)&zero, is_long == 'u' ? 2 : is_long == 'U' ? 4 : sizeof(nwchar_t));
  }
}

static void parse_string(const char *s, int len)
{
  uint8_t buf[1000], *p = buf;
  int is_long, sep, raw = 0;

  is_long = (*s == 'L' || *s == 'u' || *s == 'U') ? *s : 0;
  if (is_long == 'u' && s[1] == '8') {
    is_long = '8';
    ++s; --len;
  }
  if (is_long)
    ++s, --len;
  if (*s == 'R') { raw = 1; ++s; --len; }
  sep = *s++;
  len -= 2;
  if (raw) {
    const char *opening = strchr(s, '(');
    int delimiter_length;
    if (!opening) cprime_error("invalid raw string literal");
    delimiter_length = opening - s;
    len -= 2 * delimiter_length + 2;
    if (len < 0) cprime_error("invalid raw string literal");
    s = opening + 1;
  }
  if (len >= sizeof buf)
    p = cprime_malloc(len + 1);
  memcpy(p, s, len);
  p[len] = 0;

  cstr_reset(&tokcstr);
  if (raw) {
    CString escaped;
    int i;
    cstr_new(&escaped);
    for (i = 0; i < len; ++i) {
      if (p[i] == '\\') cstr_ccat(&escaped, '\\');
      cstr_ccat(&escaped, p[i]);
    }
    cstr_ccat(&escaped, 0);
    parse_escape_string(&tokcstr, escaped.data, is_long == '8' ? 0 : is_long);
    cstr_free(&escaped);
  } else
    parse_escape_string(&tokcstr, p, is_long == '8' ? 0 : is_long);
  if (p != buf)
    cprime_free(p);

  if (sep == '\'')
  {
    int char_size, i, n, c;
    // XXX: make it portable
    if (!is_long)
      tok = TOK_CCHAR, char_size = 1;
    else if (is_long == '8')
      tok = TOK_U8CHAR, char_size = 1;
    else if (is_long == 'u')
      tok = TOK_U16CHAR, char_size = 2;
    else if (is_long == 'U')
      tok = TOK_U32CHAR, char_size = 4;
    else
      tok = TOK_LCHAR, char_size = sizeof(nwchar_t);
    n = tokcstr.size / char_size - 1;
    if (n < 1)
      cprime_error("empty character constant");
    if (n > 1)
    {
      if (is_long == 'u' || is_long == 'U' || is_long == '8')
        cprime_error("encoded character literal requires one code unit");
      cprime_warning_c(warn_all)("multi-character character constant");
      if (!is_long)
        tok = TOK_CINT;
    }
    for (c = i = 0; i < n; ++i)
    {
      if (is_long && is_long != '8')
        c = char_size == 2 ? ((uint16_t *)tokcstr.data)[i] : ((uint32_t *)tokcstr.data)[i];
      else
        c = (c << 8) | ((char *)tokcstr.data)[i];
    }
    tokc.i = c;
  }
  else
  {
    tokc.str.size = tokcstr.size;
    tokc.str.data = tokcstr.data;
    if (!is_long)
      tok = TOK_STR;
    else if (is_long == '8') tok = TOK_U8STR;
    else if (is_long == 'u') tok = TOK_U16STR;
    else if (is_long == 'U') tok = TOK_U32STR;
    else
      tok = TOK_LSTR;
  }
}

#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
// We Use 64 Bit (52 Needed) Numbers
#define BN_SIZE 2
#else
// We Use 128 Bit (64/112 Needed) Numbers
#define BN_SIZE 4
#endif

// Bn = (Bn << Shift) | Or_Val
static int bn_lshift(unsigned int *bn, int shift, int or_val)
{
  int i;
  unsigned int v;
  if (bn[BN_SIZE - 1] >> (32 - shift))
    return shift;
  for (i = 0; i < BN_SIZE; i++)
  {
    v = bn[i];
    bn[i] = (v << shift) | or_val;
    or_val = v >> (32 - shift);
  }
  return 0;
}

static void bn_zero(unsigned int *bn)
{
  int i;
  for (i = 0; i < BN_SIZE; i++)
    bn[i] = 0;
}

/* Numeric-constant suffix letters: an optional f/F or l/L conversion and
   the imaginary unit i/j, in either order ([GNU] Imaginary Constants).
   'CH' is the first candidate byte and *PP points just past it, the same
   lookahead convention parse_number() keeps elsewhere.  The suffix is
   consumed and the first byte that is not part of it is returned.  *CVT
   receives 'F', 'L' or 0 and *IMAG receives 'I', 'J' or 0. */
static int scan_number_suffix(const char **pp, int ch, int *cvt, int *imag)
{
  int i, c;
  *cvt = 0;
  *imag = 0;
  for (i = 0; i < 2; ++i)
  {
    c = toup(ch);
    if ((c == 'F' || c == 'L') && !*cvt)
      *cvt = c;
    else if ((c == 'I' || c == 'J') && !*imag)
      *imag = c;
    else
      break;
    ch = *(*pp)++;
  }
  return ch;
}

/* parse number in null terminated string 'p' and return it in the
   current token */
static void parse_number(const char *p)
{
  int b, t, shift, frac_bits, s, exp_val, ch;
  char *q;
  unsigned int bn[BN_SIZE];
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
  double d;
#else
  long double d;
#endif

  if (strchr(p, '\'')) {
    char cleaned[STRING_MAX_SIZE + 1];
    int count = 0, base = 10, previous = -1;
    const char *source = p;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) base = 16;
    else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) base = 2;
    while (*source) {
      int current = *source++;
      int digit = current >= '0' && current <= '9' ? current - '0'
                : current >= 'a' && current <= 'f' ? current - 'a' + 10
                : current >= 'A' && current <= 'F' ? current - 'A' + 10 : -1;
      if (current == '\'') {
        int following = *source;
        int next_digit = following >= '0' && following <= '9' ? following - '0'
                       : following >= 'a' && following <= 'f' ? following - 'a' + 10
                       : following >= 'A' && following <= 'F' ? following - 'A' + 10 : -1;
        if (previous < 0 || previous >= base || next_digit < 0 || next_digit >= base)
          cprime_error("digit separator must occur between digits");
        continue;
      }
      if ((base == 10 && (current == 'e' || current == 'E'))
          || (base != 10 && (current == 'p' || current == 'P'))) {
        base = 10;
        digit = -1;
      }
      previous = digit;
      if (count == STRING_MAX_SIZE) cprime_error("number too long");
      cleaned[count++] = current;
    }
    cleaned[count] = 0;
    parse_number(cleaned);
    return;
  }
  // Number
  q = token_buf;
  ch = *p++;
  t = ch;
  ch = *p++;
  *q++ = t;
  b = 10;
  if (t == '.')
    goto float_frac_parse;
  else if (t == '0')
  {
    if (ch == 'x' || ch == 'X')
    {
      q--;
      ch = *p++;
      b = 16;
    }
    else if (cprime_state->cprime_ext && (ch == 'b' || ch == 'B'))
    {
      q--;
      ch = *p++;
      b = 2;
    }
  }
  /* parse all digits. cannot check octal numbers at this stage
     because of floating point constants */
  while (1)
  {
    if (ch >= 'a' && ch <= 'f')
      t = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
      t = ch - 'A' + 10;
    else if (isnum(ch))
      t = ch - '0';
    else
      break;
    if (t >= b)
      break;
    if (q >= token_buf + STRING_MAX_SIZE)
    {
num_too_long:
      cprime_error("number too long");
    }
    *q++ = ch;
    ch = *p++;
  }
  if (ch == '.' ||
      ((ch == 'e' || ch == 'E') && b == 10) ||
      ((ch == 'p' || ch == 'P') && (b == 16 || b == 2)))
  {
    if (b != 10)
    {
      /* NOTE: strtox should support that for hexa numbers, but
         non ISOC99 libcs do not support it, so we prefer to do
         it by hand */
      // Hexadecimal Or Binary Floats
      // XXX: handle overflows
      frac_bits = 0;
      *q = '\0';
      if (b == 16)
        shift = 4;
      else
        shift = 1;
      bn_zero(bn);
      q = token_buf;
      while (1)
      {
        t = *q++;
        if (t == '\0')
          break;
        else if (t >= 'a')
          t = t - 'a' + 10;
        else if (t >= 'A')
          t = t - 'A' + 10;
        else
          t = t - '0';
        frac_bits -= bn_lshift(bn, shift, t);
      }
      if (ch == '.')
      {
        ch = *p++;
        while (1)
        {
          t = ch;
          if (t >= 'a' && t <= 'f')
            t = t - 'a' + 10;
          else if (t >= 'A' && t <= 'F')
            t = t - 'A' + 10;
          else if (t >= '0' && t <= '9')
            t = t - '0';
          else
            break;
          if (t >= b)
            cprime_error("invalid digit");
          frac_bits -= bn_lshift(bn, shift, t);
          frac_bits += shift;
          ch = *p++;
        }
      }
      if (ch != 'p' && ch != 'P')
        expect("exponent");
      ch = *p++;
      s = 1;
      exp_val = 0;
      if (ch == '+')
        ch = *p++;
      else if (ch == '-')
      {
        s = -1;
        ch = *p++;
      }
      if (ch < '0' || ch > '9')
        expect("exponent digits");
      while (ch >= '0' && ch <= '9')
      {
        // If exp_val is this large ldexp will return HUGE_VAL
        if (exp_val < 100000000)
          exp_val = exp_val * 10 + ch - '0';
        ch = *p++;
      }
      exp_val = exp_val * s;

      // Now We Can Generate The Number
      // XXX: should patch directly float number
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
      d = (double)bn[1] * 4294967296.0 + (double)bn[0];
      d = ldexp(d, exp_val - frac_bits);
#else
      d = (long double)bn[3] * 79228162514264337593543950336.0L +
          (long double)bn[2] * 18446744073709551616.0L +
          (long double)bn[1] * 4294967296.0L +
          (long double)bn[0];
      d = ldexpl(d, exp_val - frac_bits);
#endif
      {
        int cvt, imag;
        ch = scan_number_suffix((const char **)&p, ch, &cvt, &imag);
        if (cvt == 'F')
        {
          tok = imag ? TOK_CIMAGF : TOK_CFLOAT;
          // Float : Should Handle Overflow
          tokc.f = (float)d;
        }
        else if (cvt == 'L')
        {
          tok = imag ? TOK_CIMAGL : TOK_CLDOUBLE;
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
          tokc.d = d;
#else
          tokc.ld = d;
#endif
        }
        else
        {
          tok = imag ? TOK_CIMAGD : TOK_CDOUBLE;
          tokc.d = (double)d;
        }
      }
    }
    else
    {
      // Decimal Floats
      if (ch == '.')
      {
        if (q >= token_buf + STRING_MAX_SIZE)
          goto num_too_long;
        *q++ = ch;
        ch = *p++;
float_frac_parse:
        while (ch >= '0' && ch <= '9')
        {
          if (q >= token_buf + STRING_MAX_SIZE)
            goto num_too_long;
          *q++ = ch;
          ch = *p++;
        }
      }
      if (ch == 'e' || ch == 'E')
      {
        if (q >= token_buf + STRING_MAX_SIZE)
          goto num_too_long;
        *q++ = ch;
        ch = *p++;
        if (ch == '-' || ch == '+')
        {
          if (q >= token_buf + STRING_MAX_SIZE)
            goto num_too_long;
          *q++ = ch;
          ch = *p++;
        }
        if (ch < '0' || ch > '9')
          expect("exponent digits");
        while (ch >= '0' && ch <= '9')
        {
          if (q >= token_buf + STRING_MAX_SIZE)
            goto num_too_long;
          *q++ = ch;
          ch = *p++;
        }
      }
      *q = '\0';
      {
        int cvt, imag;
        errno = 0;
        ch = scan_number_suffix((const char **)&p, ch, &cvt, &imag);
        if (cvt == 'F')
        {
          tok = imag ? TOK_CIMAGF : TOK_CFLOAT;
          tokc.f = strtof(token_buf, NULL);
        }
        else if (cvt == 'L')
        {
          tok = imag ? TOK_CIMAGL : TOK_CLDOUBLE;
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
          tokc.d = strtod(token_buf, NULL);
#else
          tokc.ld = strtold(token_buf, NULL);
#endif
        }
        else
        {
          tok = imag ? TOK_CIMAGD : TOK_CDOUBLE;
          tokc.d = strtod(token_buf, NULL);
        }
      }
    }
  }
  else
  {
    unsigned long long n = 0, n1 = 0;
    int lcount, ucount, ov = 0;
    const char *p1;

    // Integer Number
    *q = '\0';
    q = token_buf;
    if (b == 10 && *q == '0')
    {
      b = 8;
      q++;
    }
    while (1)
    {
      t = *q++;
      // No Need For Checks Except For Base 10 / 8 Errors
      if (t == '\0')
        break;
      else if (t >= 'a')
        t = t - 'a' + 10;
      else if (t >= 'A')
        t = t - 'A' + 10;
      else
        t = t - '0';
      if (t >= b)
        cprime_error("invalid digit");
      n = n * b + t;
      if (!ov)
      {
        // Detect Overflow
        if (n1 >= 0x1000000000000000ULL && n / b != n1)
          ov = 1;
        else
          n1 = n;
      }
    }

    /* Determine the characteristics (unsigned and/or 64bit) the type of
       the constant must have according to the constant suffix(es) */
    lcount = ucount = 0;
    p1 = p;
    for (;;)
    {
      t = toup(ch);
      if (t == 'L')
      {
        if (lcount >= 2)
          cprime_error("three 'l's in integer constant");
        if (lcount && *(p - 1) != ch)
          cprime_error("incorrect integer suffix: %s", p1);
        lcount++;
        ch = *p++;
      }
      else if (t == 'U')
      {
        if (ucount >= 1)
          cprime_error("two 'u's in integer constant");
        ucount++;
        ch = *p++;
      }
      else
        break;
    }

    /* An integer constant may end in the imaginary unit ([GNU] Imaginary
       Constants).  The constant then has a complex type whose element type
       follows the integer suffix; the unsigned suffix is not allowed there. */
    if (ch == 'i' || ch == 'j')
    {
      if (ucount)
        cprime_error("invalid suffix on imaginary constant");
      ch = *p++;
      tok = lcount ? TOK_CIMAGLL : TOK_CIMAGI;
      tokc.i = n;
      if (ch)
        cprime_error("invalid number");
      return;
    }

    // In #If/#Elif Expressions, All Numbers Have Type (U)Intmax_T Anyway
    if (pp_expr)
      lcount = 2;

    // Determine if it needs 64 bits and/or unsigned in order to fit
    if (ucount == 0 && b == 10)
    {
      if (lcount <= (LONG_SIZE == 4))
      {
        if (n >= 0x80000000U)
          lcount = (LONG_SIZE == 4) + 1;
      }
      if (n >= 0x8000000000000000ULL)
        ov = 1, ucount = 1;
    }
    else
    {
      if (lcount <= (LONG_SIZE == 4))
      {
        if (n >= 0x100000000ULL)
          lcount = (LONG_SIZE == 4) + 1;
        else if (n >= 0x80000000U)
          ucount = 1;
      }
      if (n >= 0x8000000000000000ULL)
        ucount = 1;
    }

    if (ov)
      /* Give a warning with values in case of an overflow. This helps to
         spot the 0 too much in 0x8000'0000'0000'0000'0. It may even be
         better to use a 0x8000'0000'0000'0000 from n1 instead of a 0 from
         n after the overflow. This is at least undefined behavior.
         The C99 to C23 standards state:
         "Each constant shall have a type and the value of a constant
         shall be in the range of representable values for its type."
         "If an integer constant cannot be represented by any type ...,
         then the integer constant has no type."
         The C++ standards state:
         "A program is ill-formed if one of its translation units contains
         an integer-literal that cannot be represented by any of the
         allowed types." */
      cprime_warning(
        b == 8
        ? "integer constant overflow, using %#llo; did you mean %#llo?"
        : b == 10
        ? "integer constant overflow, using %llu, did you mean %llu?"
        : "integer constant overflow, using %#llx; did you mean %#llx?",
        n, n1);

    tok = TOK_CINT;
    if (lcount)
    {
      tok = TOK_CLONG;
      if (lcount == 2)
        tok = TOK_CLLONG;
    }
    if (ucount)
      ++tok; // TOK_CU...
    tokc.i = n;
  }
  if (ch)
    cprime_error("invalid number");
}


#define PARSE2(c1, tok1, c2, tok2)              \
    case c1:                                    \
        PEEKC(c, p);                            \
        if (c == c2) {                          \
            p++;                                \
            tok = tok2;                         \
        } else {                                \
            tok = tok1;                         \
        }                                       \
        break;

// Return Next Token Without Macro Substitution
static void next_nomacro(void)
{
  int t, c, is_long, len;
  TokenSym *ts;
  uint8_t *p, *p1;
  unsigned int h;

  p = file->buf_ptr;
redo_no_start:
  c = *p;
  switch (c)
  {
  case ' ':
  case '\t':
    tok = c;
    p++;
maybe_space:
    if (parse_flags & PARSE_FLAG_SPACES)
      goto keep_tok_flags;
    while (ident_space[*p])
      ++p;
    goto redo_no_start;
  case '\f':
  case '\v':
  case '\r':
    p++;
    goto redo_no_start;
  case '\\':
    // first look if it is in fact an end of buffer
    c = handle_stray(&p);
    if (c == '\\') {
      if (ucn_identifier_prefix(&p)) {
        p1 = p;
        len = 0;
        goto parse_ident_slow;
      }
      goto parse_simple;
    }
    if (c == CH_EOF)
    {
      CPRIMEState *s1 = cprime_state;
      if (!(tok_flags & TOK_FLAG_BOL))
      {
        // Add Implicit Newline
        goto maybe_newline;
      }
      else if (!(parse_flags & PARSE_FLAG_PREPROCESS))
        tok = TOK_EOF;
      else if (s1->ifdef_stack_ptr != file->ifdef_stack_ptr)
        cprime_error("missing #endif");
      else if (s1->include_stack_ptr == s1->include_stack)
      {
        // No Include Left : End Of File.
        tok = TOK_EOF;
      }
      else
      {
        // Pop Include File

        /* test if previous '#endif' was after a #ifdef at
           start of file */
        if (tok_flags & TOK_FLAG_ENDIF)
        {
#ifdef INC_DEBUG
          printf("#endif %s\n", get_tok_str(file->ifndef_macro_saved, NULL));
#endif
          search_cached_include(s1, file->true_filename, 1)
          ->ifndef_macro = file->ifndef_macro_saved;
          tok_flags &= ~TOK_FLAG_ENDIF;
        }

        // Add End Of Include File Debug Info
        cprime_debug_eincl(cprime_state);
        // Pop Include Stack
        cprime_close();
        s1->include_stack_ptr--;
        p = file->buf_ptr;
        goto maybe_newline;
      }
    }
    else
      goto redo_no_start;
    break;

  case '\n':
    file->line_num++;
    p++;
maybe_newline:
    tok_flags |= TOK_FLAG_BOL;
    if (0 == (parse_flags & PARSE_FLAG_LINEFEED))
      goto redo_no_start;
    tok = TOK_LINEFEED;
    goto keep_tok_flags;

  case '#':
    // XXX: simplify
    PEEKC(c, p);
    if ((tok_flags & TOK_FLAG_BOL) &&
        (parse_flags & PARSE_FLAG_PREPROCESS))
    {
      tok_flags &= ~TOK_FLAG_BOL;
      file->buf_ptr = p;
      preprocess(tok_flags &TOK_FLAG_BOF);
      p = file->buf_ptr;
      goto maybe_newline;
    }
    else
    {
      if (c == '#')
      {
        p++;
        tok = TOK_TWOSHARPS;
      }
      else
      {
#if !defined(CPRIME_TARGET_ARM)
        if (parse_flags & PARSE_FLAG_ASM_FILE)
        {
          p = parse_line_comment(p - 1);
          goto redo_no_start;
        }
        else
#endif
        {
          tok = '#';
        }
      }
    }
    break;

  // dollar is allowed to start identifiers when not parsing asm
  case '$':
    if (!(isidnum_table['$' - CH_EOF] & IS_ID)
        || (parse_flags & PARSE_FLAG_ASM_FILE))
      goto parse_simple;

  case 'a': case 'b': case 'c': case 'd':
  case 'e': case 'f': case 'g': case 'h':
  case 'i': case 'j': case 'k': case 'l':
  case 'm': case 'n': case 'o': case 'p':
  case 'q': case 'r': case 's': case 't':
  case 'v': case 'w': case 'x':
  case 'y': case 'z':
  case 'A': case 'B': case 'C': case 'D':
  case 'E': case 'F': case 'G': case 'H':
  case 'I': case 'J': case 'K':
  case 'M': case 'N': case 'O': case 'P':
  case 'Q': case 'S': case 'T':
  case 'V': case 'W': case 'X':
  case 'Y': case 'Z':
  case '_':
parse_ident_fast:
    p1 = p;
    h = TOK_HASH_INIT;
    h = TOK_HASH_FUNC(h, c);
    while (t = ident_cont[c = *++p], t)
      h = TOK_HASH_FUNC(h, t);
    len = p - p1;
    if (c != '\\')
    {
      TokenSym **pts;

      /* fast case : no stray found, so we have the full token
         and we have already hashed it */
      pts = &hash_ident[h & (hash_ident_size - 1)];
      for (;;)
      {
        ts = *pts;
        if (!ts)
          break;
        if (ts->hash == h && ts->len == len && !memcmp(ts->str, p1, len))
          goto token_found;
        pts = &(ts->hash_next);
      }
      ts = tok_alloc_new(pts, (char *) p1, len, h);
token_found: ;
    }
    else
    {
      // Slower Case
parse_ident_slow:
      cstr_reset(&tokcstr);
      if (len) cstr_cat(&tokcstr, (char *) p1, len);
      p--;
      PEEKC(c, p);
      while ((isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
             || (c == '\\' && ucn_identifier_prefix(&p)))
      {
        if (c == '\\') {
          int digits;
          /* Keep UCN spelling in preprocessing identifiers. Stringification
             must distinguish the original hex case and escape width. */
          cstr_ccat(&tokcstr, c);
          PEEKC(c, p);
          digits = c == 'u' ? 4 : 8;
          cstr_ccat(&tokcstr, c);
          while (digits--) {
            PEEKC(c, p);
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
                  || (c >= 'A' && c <= 'F')))
              expect("more hex digits in universal-character-name");
            cstr_ccat(&tokcstr, c);
          }
          PEEKC(c, p);
          continue;
        }
        cstr_ccat(&tokcstr, c);
        PEEKC(c, p);
      }
      ts = tok_alloc(tokcstr.data, tokcstr.size);
    }
    /* Recognize prefixes after scanning the complete preprocessing identifier.
       The scanner handles refills and line splices, including inside a prefix. */
    if (c == '"' || c == '\'') {
      const char *prefix = ts->str;
      int prefix_len = ts->len;
      int raw = prefix_len && prefix[prefix_len - 1] == 'R';
      if (raw) --prefix_len;
      is_long = -1;
      if (!prefix_len && raw)
        is_long = 0;
      else if (prefix_len == 1
               && (prefix[0] == 'u' || prefix[0] == 'U' || prefix[0] == 'L'))
        is_long = prefix[0];
      else if (prefix_len == 2 && prefix[0] == 'u' && prefix[1] == '8')
        is_long = '8';
      if (is_long >= 0) {
        if (raw && c == '"') goto raw_str_const;
        if (!raw) goto str_const;
      }
    }
    tok = ts->tok;
    last_ident_sym = ts;
    break;
  case 'R':
  case 'u':
  case 'U':
  case 'L':
    goto parse_ident_fast;
  case '0': case '1': case '2': case '3':
  case '4': case '5': case '6': case '7':
  case '8': case '9':
    t = c;
    PEEKC(c, p);
    /* after the first digit, accept digits, alpha, '.' or sign if
       prefixed by 'eEpP' */
parse_num:
    cstr_reset(&tokcstr);
    for (;;)
    {
      cstr_ccat(&tokcstr, t);
pp_number_continuation:
      if (c == '\'') {
        PEEKC(c, p);
        if (!(isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
            && !(c == '\\' && ucn_identifier_prefix(&p))) {
          *--p = '\'';
          break;
        }
        cstr_ccat(&tokcstr, '\'');
      }
      if (c == '\\' && ucn_identifier_prefix(&p)) {
        int digits;
        cstr_ccat(&tokcstr, c);
        PEEKC(c, p);
        digits = c == 'u' ? 4 : 8;
        cstr_ccat(&tokcstr, c);
        while (digits--) {
          PEEKC(c, p);
          if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
                || (c >= 'A' && c <= 'F')))
            expect("more hex digits in universal-character-name");
          cstr_ccat(&tokcstr, c);
        }
        PEEKC(c, p);
        /* A UCN's final spelling digit is not an exponent marker. */
        t = 0;
        goto pp_number_continuation;
      }
      if (!((isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
            || c == '.'
            || ((c == '+' || c == '-')
                && (((t == 'e' || t == 'E')
                     && !(parse_flags & PARSE_FLAG_ASM_FILE
                          // 0Xe+1 Is 3 Tokens In Asm
                          && ((char * )tokcstr.data)[0] == '0'
                          && toup(((char * )tokcstr.data)[1]) == 'X'))
                    || t == 'p' || t == 'P'))))
        break;
      t = c;
      PEEKC(c, p);
    }
    // We add a trailing '\0' to ease parsing
    cstr_ccat(&tokcstr, '\0');
    tokc.str.size = tokcstr.size;
    tokc.str.data = tokcstr.data;
    tok = TOK_PPNUM;
    break;

  case '.':
    // special dot handling because it can also start a number
    PEEKC(c, p);
    if (isnum(c))
    {
      t = '.';
      goto parse_num;
    }
    else if ((isidnum_table['.' - CH_EOF] & IS_ID)
             && (isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM)))
    {
      *--p = c = '.';
      goto parse_ident_fast;
    }
    else if (c == '.')
    {
      PEEKC(c, p);
      if (c == '.')
      {
        p++;
        tok = TOK_DOTS;
      }
      else
      {
        *--p = '.'; // May Underflow Into File->Unget[]
        tok = '.';
      }
    }
    else
      tok = '.';
    break;
  case '\'':
  case '\"':
    is_long = 0;
    goto str_const;
raw_str_const:
    cstr_reset(&tokcstr);
    if (is_long == '8') cstr_cat(&tokcstr, "u8", 2);
    else if (is_long) cstr_ccat(&tokcstr, is_long);
    cstr_cat(&tokcstr, "R\"", 2);
    p = parse_pp_raw_string(p, &tokcstr);
    goto string_token_ready;
str_const:
    cstr_reset(&tokcstr);
    if (is_long == '8') cstr_cat(&tokcstr, "u8", 2);
    else if (is_long)
      cstr_ccat(&tokcstr, is_long);
    cstr_ccat(&tokcstr, c);
    p = parse_pp_string(p, c, &tokcstr);
    cstr_ccat(&tokcstr, c);
string_token_ready:
    cstr_ccat(&tokcstr, '\0');
    tokc.str.size = tokcstr.size;
    tokc.str.data = tokcstr.data;
    tok = TOK_PPSTR;
    break;

  case '<':
    PEEKC(c, p);
    if (c == '=')
    {
      p++;
      tok = TOK_LE;
    }
    else if (c == '<')
    {
      PEEKC(c, p);
      if (c == '=')
      {
        p++;
        tok = TOK_A_SHL;
      }
      else
        tok = TOK_SHL;
    }
    else
      tok = TOK_LT;
    break;
  case '>':
    PEEKC(c, p);
    if (c == '=')
    {
      p++;
      tok = TOK_GE;
    }
    else if (c == '>')
    {
      PEEKC(c, p);
      if (c == '=')
      {
        p++;
        tok = TOK_A_SAR;
      }
      else
        tok = TOK_SAR;
    }
    else
      tok = TOK_GT;
    break;

  case '&':
    PEEKC(c, p);
    if (c == '&')
    {
      p++;
      tok = TOK_LAND;
    }
    else if (c == '=')
    {
      p++;
      tok = TOK_A_AND;
    }
    else
      tok = '&';
    break;

  case '|':
    PEEKC(c, p);
    if (c == '|')
    {
      p++;
      tok = TOK_LOR;
    }
    else if (c == '=')
    {
      p++;
      tok = TOK_A_OR;
    }
    else
      tok = '|';
    break;

  case '+':
    PEEKC(c, p);
    if (c == '+')
    {
      p++;
      tok = TOK_INC;
    }
    else if (c == '=')
    {
      p++;
      tok = TOK_A_ADD;
    }
    else
      tok = '+';
    break;

  case '-':
    PEEKC(c, p);
    if (c == '-')
    {
      p++;
      tok = TOK_DEC;
    }
    else if (c == '=')
    {
      p++;
      tok = TOK_A_SUB;
    }
    else if (c == '>')
    {
      p++;
      tok = TOK_ARROW;
    }
    else
      tok = '-';
    break;

    PARSE2('!', '!', '=', TOK_NE)
    PARSE2('=', '=', '=', TOK_EQ)
    PARSE2('*', '*', '=', TOK_A_MUL)
    PARSE2('%', '%', '=', TOK_A_MOD)
    PARSE2('^', '^', '=', TOK_A_XOR)

  // Comments Or Operator
  case '/':
    PEEKC(c, p);
    if (c == '*')
    {
      p = parse_comment(p);
      // Comments Replaced By A Blank
      tok = ' ';
      goto maybe_space;
    }
    else if (c == '/')
    {
      p = parse_line_comment(p);
      tok = ' ';
      goto maybe_space;
    }
    else if (c == '=')
    {
      p++;
      tok = TOK_A_DIV;
    }
    else
      tok = '/';
    break;

  // Simple Tokens
  case '@': // Only Used In Assembler
#ifdef CPRIME_TARGET_ARM // Comment On Arm Asm 
    if (parse_flags & PARSE_FLAG_ASM_FILE)
    {
      p = parse_line_comment(p);
      goto redo_no_start;
    }
#endif
  case '(':
  case ')':
  case '[':
  case ']':
  case '{':
  case '}':
  case ',':
  case ';':
  case ':':
  case '?':
  case '~':
parse_simple:
    tok = c;
    p++;
    break;
  case 0xEF: // UTF8 BOM ?
    if (p[1] == 0xBB && p[2] == 0xBF && p == file->buffer)
    {
      p += 3;
      goto redo_no_start;
    }
  default:
    if (c >= 0x80 && c <= 0xFF) // Utf8 Identifiers
      goto parse_ident_fast;
    if (parse_flags & PARSE_FLAG_ASM_FILE)
      goto parse_simple;
    cprime_error("unrecognized character \\x%02x", c);
    break;
  }
  tok_flags = 0;
keep_tok_flags:
  file->buf_ptr = p;
#if defined(PARSE_DEBUG)
  printf("token = %d %s\n", tok, get_tok_str(tok, &tokc));
#endif
}

#ifdef PP_DEBUG
static int indent;
static void define_print(CPRIMEState *s1, int v);
static void pp_print(const char *msg, int v, const int *str)
{
  FILE *fp = cprime_state->ppfp;

  if (msg[0] == '#' && indent == 0)
    fprintf(fp, "\n");
  else if (msg[0] == '+')
    ++indent, ++msg;
  else if (msg[0] == '-')
    --indent, ++msg;

  fprintf(fp, "%*s", indent, "");
  if (msg[0] == '#')
    define_print(cprime_state, v);
  else
    tok_print(str, v ? "%s %s" : "%s", msg, get_tok_str(v, 0));
}
#define PP_PRINT(x) pp_print x
#else
#define PP_PRINT(x)
#endif

static int macro_subst(
  TokenString *tok_str,
  Sym **nested_list,
  const int *macro_str
);

/* substitute arguments in replacement lists in macro_str by the values in
   args (field d) and return allocated string */
static int *macro_arg_subst(Sym **nested_list, const int *macro_str, Sym *args)
{
  int t, t0, t1, t2, n;
  const int *st;
  Sym *s;
  CValue cval;
  TokenString str;

#ifdef PP_DEBUG
  PP_PRINT(("asubst:", 0, macro_str));
  for (s = args, n = 0; s; s = s->prev, ++n);
  while (n--)
  {
    for (s = args, t = 0; t < n; s = s->prev, ++t);
    tok_print(s->d, "%*s - arg: %s:", indent, "", get_tok_str(s->v, 0));
  }
#endif

  tok_str_new(&str);
  t0 = t1 = 0;
  while (1)
  {
    TOK_GET(&t, &macro_str, &cval);
    if (!t)
      break;
    if (t == '#')
    {
      // Stringize
      do t = *macro_str++; while (t == ' ');
      s = sym_find2(args, t);
      if (s)
      {
        cstr_reset(&tokcstr);
        cstr_ccat(&tokcstr, '\"');
        st = s->d;
        while (*st != TOK_EOF)
        {
          const char *s;
          TOK_GET(&t, &st, &cval);
          s = get_tok_str(t, &cval);
          while (*s)
          {
            if (t == TOK_PPSTR && *s != '\'')
              add_char(&tokcstr, *s);
            else
              cstr_ccat(&tokcstr, *s);
            ++s;
          }
        }
        cstr_ccat(&tokcstr, '\"');
        cstr_ccat(&tokcstr, '\0');
        //printf("\nstringize: <%s>\n", (char *)tokcstr.data);
        // Add String
        cval.str.size = tokcstr.size;
        cval.str.data = tokcstr.data;
        tok_str_add2(&str, TOK_PPSTR, &cval);
#ifdef CPRIME_TARGET_ARM
      }
      else if ((parse_flags & PARSE_FLAG_ASM_FILE) && t == TOK_PPNUM)
      {
        // For Example: Mov R1,#0
        --macro_str, tok_str_add(&str, '#');
#endif
      }
      else
        expect("macro parameter after '#'");
    }
    else if (t >= TOK_IDENT)
    {
      s = sym_find2(args, t);
      if (s)
      {
        st = s->d;
        n = 0;
        while ((t2 = macro_str[n]) == ' ')
          ++n;
        // if '##' is present before or after, no arg substitution
        if (t2 == TOK_PPJOIN || t1 == TOK_PPJOIN)
        {
          /* special case for var arg macros : ## eats the ','
             if empty VA_ARGS variable. */
          if (t1 == TOK_PPJOIN && t0 == ',' && non_iso && s->type.t)
          {
            int c = str.str[str.len - 1];
            while (str.str[--str.len] != ',')
              ;
            if (*st == TOK_EOF)
            {
              // Suppress ',' '##'
            }
            else
            {
              // Suppress '##' And Add Variable
              str.len++;
              if (c == ' ')
                str.str[str.len++] = c;
              goto add_var;
            }
          }
          else
          {
            if (*st == TOK_EOF)
              tok_str_add(&str, TOK_PLCHLDR);
          }
        }
        else
        {
add_var:
          if (!s->e)
          {
            /* Expand arguments tokens and store them.  In most
               cases we could also re-expand each argument if
               used multiple times, but not if the argument
               contains the __COUNTER__ macro.  */
            TokenString str2;
            tok_str_new(&str2);
            macro_subst(&str2, nested_list, st);
            tok_str_add(&str2, TOK_EOF);
            s->e = str2.str;
          }
          st = s->e;
        }
        while (*st != TOK_EOF)
        {
          TOK_GET(&t2, &st, &cval);
          tok_str_add2(&str, t2, &cval);
        }
      }
      else
        tok_str_add(&str, t);
    }
    else
      tok_str_add2(&str, t, &cval);
    if (t != ' ')
      t0 = t1, t1 = t;
  }
  tok_str_add(&str, 0);
  PP_PRINT(("areslt:", 0, str.str));
  return str.str;
}

// Handle The '##' Operator. Return The Resulting String (Which Must Be Freed).
static inline int *macro_twosharps(const int *ptr0)
{
  int t1, t2, n, l;
  CValue cv1, cv2;
  TokenString macro_str1;
  const int *ptr;

  tok_str_new(&macro_str1);
  cstr_reset(&tokcstr);
  for (ptr = ptr0;;)
  {
    TOK_GET(&t1, &ptr, &cv1);
    if (t1 == 0)
      break;
    for (;;)
    {
      n = 0;
      while ((t2 = ptr[n]) == ' ')
        ++n;
      if (t2 != TOK_PPJOIN)
        break;
      ptr += n;
      while ((t2 = *++ptr) == ' ' || t2 == TOK_PPJOIN)
        ;
      TOK_GET(&t2, &ptr, &cv2);
      if (t2 == TOK_PLCHLDR)
        continue;
      if (t1 != TOK_PLCHLDR)
      {
        cstr_cat(&tokcstr, get_tok_str(t1, &cv1), -1);
        t1 = TOK_PLCHLDR;
      }
      cstr_cat(&tokcstr, get_tok_str(t2, &cv2), -1);
    }
    if (tokcstr.size)
    {
      cstr_ccat(&tokcstr, 0);
      cprime_open_bf(cprime_state, ":paste:", tokcstr.size);
      memcpy(file->buffer, tokcstr.data, tokcstr.size);
      tok_flags = 0; // don't interpret '#'
      for (n = 0;; n = l)
      {
        next_nomacro();
        tok_str_add2(&macro_str1, tok, &tokc);
        if (*file->buf_ptr == 0)
          break;
        tok_str_add(&macro_str1, ' ');
        l = file->buf_ptr - file->buffer;
        cprime_warning("pasting \"%.*s\" and \"%s\" does not give a valid"
                    " preprocessing token", l - n, file->buffer + n, file->buf_ptr);
      }
      cprime_close();
      cstr_reset(&tokcstr);
    }
    if (t1 != TOK_PLCHLDR)
      tok_str_add2(&macro_str1, t1, &cv1);
  }
  tok_str_add(&macro_str1, 0);
  PP_PRINT(("pasted:", 0, macro_str1.str));
  return macro_str1.str;
}

static int peek_file (TokenString *ws_str)
{
  uint8_t *p = file->buf_ptr - 1;
  int c;
  for (;;)
  {
    PEEKC(c, p);
    switch (c)
    {
    case '/':
      PEEKC(c, p);
      if (c == '*')
        p = parse_comment(p);
      else if (c == '/')
        p = parse_line_comment(p);
      else
      {
        c = *--p = '/';
        goto leave;
      }
      --p, c = ' ';
      break;
    case ' ': case '\t':
      break;
    case '\f': case '\v': case '\r':
      continue;
    case '\n':
      file->line_num++, tok_flags |= TOK_FLAG_BOL;
      break;
default: leave:
      file->buf_ptr = p;
      return c;
    }
    if (ws_str)
      tok_str_add(ws_str, c);
  }
}

/* peek or read [ws_str == NULL] next token from function macro call,
   walking up macro levels up to the file if necessary */
static int next_argstream(Sym **nested_list, TokenString *ws_str)
{
  int t;
  Sym *sa;

  while (macro_ptr)
  {
    const int *m = macro_ptr;
    while ((t = *m) != 0)
    {
      if (ws_str)
      {
        if (t != ' ')
          return t;
        ++m;
      }
      else
      {
        TOK_GET(&tok, &macro_ptr, &tokc);
        return tok;
      }
    }
    end_macro();
    // Also, End Of Scope For Nested Defined Symbol
    sa = *nested_list;
    if (sa)
      *nested_list = sa->prev, sym_free(sa);
  }
  if (ws_str)
    return peek_file(ws_str);
  else
  {
    next_nomacro();
    if (tok == '\t' || tok == TOK_LINEFEED)
      tok = ' ';
    return tok;
  }
}

/* do macro substitution of current token with macro 's' and add
   result to (tok_str,tok_len). 'nested_list' is the list of all
   macros we got inside to avoid recursing. Return non zero if no
   substitution needs to be done */
static int macro_subst_tok(
  TokenString *tok_str,
  Sym **nested_list,
  Sym *s)
{
  int t;
  int v = s->v;

  PP_PRINT(("#", v, s->d));
  if (s->d)
  {
    int *mstr = s->d;
    int *jstr;
    Sym *sa;
    int ret;

    if (s->type.t & MACRO_FUNC)
    {
      int saved_parse_flags = parse_flags;
      TokenString str;
      int parlevel, i;
      Sym *sa1, *args;

      parse_flags |= PARSE_FLAG_SPACES | PARSE_FLAG_LINEFEED
                     | PARSE_FLAG_ACCEPT_STRAYS;

      tok_str_new(&str);
      // Peek Next Token From Argument Stream
      t = next_argstream(nested_list, &str);
      if (t != '(')
      {
        /* not a macro substitution after all, restore the
         * macro token plus all whitespace we've read.
         * whitespace is intentionally not merged to preserve
         * newlines. */
        parse_flags = saved_parse_flags;
        tok_str_add2_spc(tok_str, v, 0);
        if (parse_flags & PARSE_FLAG_SPACES)
          for (i = 0; i < str.len; i++)
            tok_str_add(tok_str, str.str[i]);
        tok_str_free_str(str.str);
        return 0;
      }
      else
        tok_str_free_str(str.str);

      // Argument Macro
      args = NULL;
      sa = s->next;
      // NOTE: empty args are allowed, except if no args
      i = 2; // Eat '('
      for (;;)
      {
        do
        {
          t = next_argstream(nested_list, NULL);
        }
        while (t == ' ' || --i);

        if (!sa)
        {
          if (t == ')') // Handle '()' Case
            break;
          cprime_error("macro '%s' used with too many args",
                    get_tok_str(v, 0));
        }
empty_arg:
        tok_str_new(&str);
        parlevel = 0;
        // NOTE: non zero sa->type.t indicates VA_ARGS
        while (parlevel > 0
               || (t != ')' && (t != ',' || sa->type.t)))
        {
          if (t == TOK_EOF)
            cprime_error("EOF in invocation of macro '%s'",
                      get_tok_str(v, 0));
          if (t == '(')
            parlevel++;
          if (t == ')')
            parlevel--;
          if (t == ' ')
            str.need_spc |= 1;
          else
            tok_str_add2_spc(&str, t, &tokc);
          t = next_argstream(nested_list, NULL);
        }
        tok_str_add(&str, TOK_EOF);
        sa1 = sym_push2(&args, sa->v & ~SYM_FIELD, sa->type.t, 0);
        sa1->d = str.str;
        sa = sa->next;
        if (t == ')')
        {
          if (!sa)
            break;
          /* special case for gcc var args: add an empty
             var arg argument if it is omitted */
          if (sa->type.t && non_iso)
            goto empty_arg;
          cprime_error("macro '%s' used with too few args",
                    get_tok_str(v, 0));
        }
        i = 1;
      }

      // Now Subst Each Arg
      mstr = macro_arg_subst(nested_list, mstr, args);
      // Free Memory
      sa = args;
      while (sa)
      {
        sa1 = sa->prev;
        tok_str_free_str(sa->d);
        tok_str_free_str(sa->e);
        sym_free(sa);
        sa = sa1;
      }
      parse_flags = saved_parse_flags;
    }

    // Process '##'S (If Present)
    jstr = mstr;
    if (s->type.t & MACRO_JOIN)
      jstr = macro_twosharps(mstr);

    sa = sym_push2(nested_list, v, 0, 0);
    ret = macro_subst(tok_str, nested_list, jstr);
    // Pop Nested Defined Symbol
    if (sa == *nested_list)
      *nested_list = sa->prev, sym_free(sa);

    if (jstr != mstr)
      tok_str_free_str(jstr);
    if (mstr != s->d)
      tok_str_free_str(mstr);
    return ret;

  }
  else
  {
    CValue cval;
    char buf[32], *cstrval = buf;

    // Special Macros
    if (v == TOK___LINE__ || v == TOK___COUNTER__)
    {
      t = v == TOK___LINE__ ? file->line_num : pp_counter++;
      snprintf(buf, sizeof(buf), "%d", t);
      t = TOK_PPNUM;
      goto add_cstr1;

    }
    else if (v == TOK___FILE__)
    {
      cstrval = file->filename;
      goto add_cstr;

    }
    else if (v == TOK___DATE__ || v == TOK___TIME__)
    {
      time_t ti;
      struct tm *tm;
      time(&ti);
      tm = localtime(&ti);
      if (v == TOK___DATE__)
      {
        static char const ab_month_name[12][4] =
        {
          "Jan", "Feb", "Mar", "Apr", "May", "Jun",
          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        snprintf(buf, sizeof(buf), "%s %2d %d",
                 ab_month_name[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
      }
      else
      {
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
      }
add_cstr:
      t = TOK_STR;
add_cstr1:
      cval.str.size = strlen(cstrval) + 1;
      cval.str.data = cstrval;
      tok_str_add2_spc(tok_str, t, &cval);
    }
    return 0;
  }
}

/* do macro substitution of macro_str and add result to
   (tok_str,tok_len). 'nested_list' is the list of all macros we got
   inside to avoid recursing. */
static int macro_subst(
  TokenString *tok_str,
  Sym **nested_list,
  const int *macro_str
)
{
  Sym *s;
  int t, nosubst = 0;
  CValue cval;
  TokenString *str;

#ifdef PP_DEBUG
  int tlen = tok_str->len;
  PP_PRINT(("+expand:", 0, macro_str));
#endif

  while (1)
  {
    TOK_GET(&t, &macro_str, &cval);
    if (t == 0 || t == TOK_EOF)
      break;
    if (t >= TOK_IDENT)
    {
      s = define_find(t);
      if (s == NULL || nosubst)
        goto no_subst;
      // if nested substitution, do nothing
      if (sym_find2(*nested_list, t))
      {
        // and mark so it doesn't get subst'd again
        t |= SYM_FIELD;
        goto no_subst;
      }
      str = tok_str_alloc();
      str->str = (int *)macro_str; // Setup Stream For Possible Arguments
      begin_macro(str, 2);
      nosubst = macro_subst_tok(tok_str, nested_list, s);
      if (macro_stack != str)
      {
        // Already Finished By Reading Function Macro Arguments
        break;
      }
      macro_str = macro_ptr;
      end_macro ();
    }
    else if (t == ' ')
    {
      if (parse_flags & PARSE_FLAG_SPACES)
        tok_str->need_spc |= 1;
    }
    else
    {
no_subst:
      tok_str_add2_spc(tok_str, t, &cval);
      if (nosubst && t != '(')
        nosubst = 0;
      // GCC supports 'defined' as result of a macro substitution
      if (t == TOK_DEFINED && pp_expr)
        nosubst = 1;
    }
  }

#ifdef PP_DEBUG
  tok_str_add(tok_str, 0), --tok_str->len;
  PP_PRINT(("-result:", 0, tok_str->str + tlen));
#endif
  return nosubst;
}

/* Return the encoding of a preprocessing string, excluding character tokens. */
static int pp_string_encoding(const char *s)
{
  int encoding = 0;
  if (s[0] == 'u' && s[1] == '8') { encoding = '8'; s += 2; }
  else if (*s == 'u' || *s == 'U' || *s == 'L') encoding = *s++;
  if (*s == 'R') ++s;
  return *s == '"' ? encoding : -1;
}

/* Select the encoding before the parser creates an array type. Decode each
   component independently: a hex escape cannot consume the next literal. */
static void parse_string_sequence(void)
{
  TokenString *pieces = tok_str_alloc();
  CString combined, spelling;
  int encoding = 0, saved_flags = parse_flags, *cursor, piece_tok;
  CValue piece;
  cstr_new(&combined);
  cstr_new(&spelling);
  parse_flags &= ~(PARSE_FLAG_TOK_STR | PARSE_FLAG_TOK_NUM);
  do {
    int current = pp_string_encoding(tokc.str.data);
    if (current && encoding && current != encoding)
      cprime_error("incompatible string literal encodings");
    if (current) encoding = current;
    tok_str_add2(pieces, tok, &tokc);
    next();
  } while (tok == TOK_PPSTR && pp_string_encoding(tokc.str.data) >= 0);
  parse_flags = saved_flags;
  tok_str_add(pieces, 0);
  unget_tok(TOK_STR);
  cursor = pieces->str;
  while (*cursor) {
    const char *source;
    int length;
    tok_get(&piece_tok, &cursor, &piece);
    source = piece.str.data;
    length = piece.str.size - 1;
    if (!pp_string_encoding(source) && encoding) {
      cstr_reset(&spelling);
      if (encoding == '8') cstr_cat(&spelling, "u8", 2);
      else cstr_ccat(&spelling, encoding);
      cstr_cat(&spelling, source, length);
      length = spelling.size;
      cstr_ccat(&spelling, 0);
      source = spelling.data;
    }
    parse_string(source, length);
    if (combined.size) combined.size -= TOK_STRING_UNIT_SIZE(tok);
    cstr_cat(&combined, tokc.str.data, tokc.str.size);
  }
  cstr_reset(&tokcstr);
  cstr_cat(&tokcstr, combined.data, combined.size);
  tokc.str.data = tokcstr.data;
  tokc.str.size = tokcstr.size;
  cstr_free(&spelling);
  cstr_free(&combined);
  tok_str_free(pieces);
}
// Return Next Token With Macro Substitution
ST_FUNC void next(void)
{
  int t;
  while (macro_ptr)
  {
redo:
    t = *macro_ptr;
    if (TOK_HAS_VALUE(t))
    {
      tok_get(&tok, &macro_ptr, &tokc);
      if (t == TOK_LINENUM)
      {
        file->line_num = tokc.i;
        goto redo;
      }
      goto convert;
    }
    else if (t == 0)
    {
      // End Of Macro Or Unget Token String
      end_macro();
      continue;
    }
    else if (t == TOK_EOF)
    {
      // do nothing
    }
    else
    {
      ++macro_ptr;
      t &= ~SYM_FIELD; // Remove 'Nosubst' Marker
      if (t == '\\')
      {
        if (!(parse_flags & PARSE_FLAG_ACCEPT_STRAYS))
          cprime_error("stray '\\' in program");
      }
    }
    tok = t;
    return;
  }

  next_nomacro();
  t = tok;
  if (t >= TOK_IDENT && (parse_flags & PARSE_FLAG_PREPROCESS))
  {
    // if reading from file, try to substitute macros
    /* next_nomacro() has just interned this identifier, so its TokenSym is
       the one define_find() would look up, and its fields are in cache. */
    TokenSym *ts = last_ident_sym;
    Sym *s = (ts && ts->tok == t) ? ts->sym_define : define_find(t);
    if (s)
    {
      Sym *nested_list = NULL;
      macro_subst_tok(&tokstr_buf, &nested_list, s);
      tok_str_add(&tokstr_buf, 0);
      begin_macro(&tokstr_buf, 0);
      goto redo;
    }
    return;
  }

convert:
  // convert preprocessor tokens into C tokens
  if (t == TOK_PPNUM)
  {
    if  ((parse_flags & PARSE_FLAG_TOK_NUM)
         && !(cprime_cpp_mode && strchr(tokc.str.data, '_')))
      parse_number(tokc.str.data);
  }
  else if (t == TOK_PPSTR)
  {
    if (parse_flags & PARSE_FLAG_TOK_STR) {
      if (pp_string_encoding(tokc.str.data) >= 0) parse_string_sequence();
      else parse_string(tokc.str.data, tokc.str.size - 1);
    }
  }
}

/* push back current token and set current token to 'last_tok'. Only
   identifier case handled for labels. */
ST_INLN void unget_tok(int last_tok)
{
  TokenString *str = &unget_buf;
  int alloc = 0;
  if (str->len) // use static buffer except if already in use
    str = tok_str_alloc(), alloc = 1;
  if (tok != TOK_EOF)
    tok_str_add2(str, tok, &tokc);
  tok_str_add(str, 0);
  begin_macro(str, alloc);
  tok = last_tok;
}

// -------------------------------------------------------------------------
// Init Preprocessor

static const char *const target_os_defs =
#ifdef CPRIME_TARGET_PE
  "_WIN32\0"
  "__CPRIME_NATIVE_CRT__\0"
# if CONFIG_CPRIME_UCRT
  "__CPRIME_UCRT__\0"
# endif
# if PTR_SIZE == 8
  "_WIN64\0"
# endif
#else
#endif
  ;

static void putdef(CString *cs, const char *p)
{
  cstr_printf(cs, "#define %s%s\n", p, &" 1"[!!strchr(p, ' ') * 2]);
}

static void putdefs(CString *cs, const char *p)
{
  while (*p)
    putdef(cs, p), p = strchr(p, 0) + 1;
}

static void cprime_predefs(CPRIMEState *s1, CString *cs, int is_asm)
{
  cstr_printf(cs, "#define __TINYC__ 9%.2s\n", &CPRIME_VERSION[4]);
  putdefs(cs, target_machine_defs);
  putdefs(cs, target_os_defs);

#ifdef CPRIME_TARGET_ARM
  if (s1->float_abi == ARM_HARD_FLOAT)
    putdef(cs, "__ARM_PCS_VFP");
#endif
  if (is_asm)
    putdef(cs, "__ASSEMBLER__");
  if (s1->output_type == CPRIME_OUTPUT_PREPROCESS)
    putdef(cs, "__CPRIME_PP__");
  if (s1->output_type == CPRIME_OUTPUT_MEMORY)
    putdef(cs, "__CPRIME_RUN__");
#ifdef CONFIG_CPRIME_BACKTRACE
  if (s1->do_backtrace)
    putdef(cs, "__CPRIME_BACKTRACE__");
#endif
#ifdef CONFIG_CPRIME_BCHECK
  if (s1->do_bounds_check)
    putdef(cs, "__CPRIME_BCHECK__");
#endif
  if (s1->char_is_unsigned)
    putdef(cs, "__CHAR_UNSIGNED__");
  if (s1->optimize > 0)
    putdef(cs, "__OPTIMIZE__");
  if (s1->option_pthread)
    putdef(cs, "_REENTRANT");
  if (s1->leading_underscore)
    putdef(cs, "__leading_underscore");
  cstr_printf(cs, "#define __SIZEOF_POINTER__ %d\n", PTR_SIZE);
  cstr_printf(cs, "#define __SIZEOF_LONG__ %d\n", LONG_SIZE);
  cstr_printf(cs, "#define __BIGGEST_ALIGNMENT__ %d\n", MAX_ALIGN);
#ifdef CPRIME_USING_DOUBLE_FOR_LDOUBLE
  cstr_printf(cs, "#define __SIZEOF_LONG_DOUBLE__ 8\n");
#else
  cstr_printf(cs, "#define __SIZEOF_LONG_DOUBLE__ %d\n", LDOUBLE_SIZE);
#endif
  cstr_cat(cs,
    "#define __SIZEOF_SHORT__ 2\n"
    "#define __SIZEOF_FLOAT__ 4\n"
    "#define __SIZEOF_DOUBLE__ 8\n"
    "#define __CHAR_BIT__ 8\n"
    "#define __SCHAR_MAX__ 127\n"
    "#define __SHRT_MAX__ 32767\n"
    "#define __ATOMIC_RELAXED 0\n"
    "#define __ATOMIC_CONSUME 1\n"
    "#define __ATOMIC_ACQUIRE 2\n"
    "#define __ATOMIC_RELEASE 3\n"
    "#define __ATOMIC_ACQ_REL 4\n"
    "#define __ATOMIC_SEQ_CST 5\n"
    "#define __FLT_RADIX__ 2\n"
    "#define __FLT_MANT_DIG__ 24\n"
    "#define __FLT_MIN__ 1.17549435082228750797e-38F\n"
    "#define __FLT_MAX__ 3.40282346638528859812e+38F\n"
    "#define __FLT_EPSILON__ 1.1920928955078125e-7F\n"
    "#define __FLT_DENORM_MIN__ 1.40129846432481707092e-45F\n"
    "#define __DBL_MANT_DIG__ 53\n"
    "#define __DBL_MIN__ 2.22507385850720138309e-308\n"
    "#define __DBL_MAX__ 1.79769313486231570815e+308\n"
    "#define __DBL_EPSILON__ 2.22044604925031308085e-16\n"
    "#define __DBL_DENORM_MIN__ 4.94065645841246544177e-324\n", -1);
  cstr_printf(cs, "#define __PTRDIFF_MAX__ %s\n", PTR_SIZE == 8
    ? "9223372036854775807LL" : "2147483647");
  cstr_printf(cs, "#define __SIZE_MAX__ %s\n", PTR_SIZE == 8
    ? "18446744073709551615ULL" : "4294967295U");
  if (!is_asm)
  {
    putdef(cs, "__STDC__");
    cprime_cpp_mode = cprimepp_is_cpp_filename(file->filename);
    if (cprimepp_is_cpp_filename(file->filename))
    {
      cstr_cat(cs, "#define __CPRIME_CPP__ 1\n", -1);
      cstr_cat(cs, "#define __cplusplus 201703L\n", -1);
      /* The alternative operator spellings are keywords in C++.  Expanding
         them here leaves the parser's ordinary punctuator path unchanged. */
      cstr_cat(cs,
        "#define and &&\n"
        "#define and_eq &=\n"
        "#define bitand &\n"
        "#define bitor |\n"
        "#define compl ~\n"
        "#define not !\n"
        "#define not_eq !=\n"
        "#define or ||\n"
        "#define or_eq |=\n"
        "#define xor ^\n"
        "#define xor_eq ^=\n", -1);
      cstr_cat(cs, "#define __GXX_RTTI 1\n", -1);
      cstr_cat(cs, "#define __GXX_EXPERIMENTAL_CXX0X__ 1\n", -1);
      cstr_cat(cs, "#define __STDC_LIMIT_MACROS 1\n", -1);
      cstr_cat(cs, "#define __STDC_CONSTANT_MACROS 1\n", -1);
      if (s1->coroutines)
        cstr_cat(cs, "#define __cpp_impl_coroutine 201902L\n", -1);
    }
    cstr_printf(cs, "#define __STDC_HOSTED__ %d\n", s1->nostdlib ? 0 : 1);
    cstr_printf(cs, "#define __STDC_VERSION__ %dL\n", s1->cversion);
    cstr_cat(cs,
             // Load More Predefs And __Builtins
#if CONFIG_CPRIME_PREDEFS
#include "cprimedefs_.h" // Include As Strings 
#else
             "#include <cprimedefs.h>\n" // Load At Runtime
#endif
             , -1);
    if (cprimepp_is_cpp_filename(file->filename)) cstr_cat(cs, "extern \"C\" {\n", -1);
    cstr_cat(cs,
      "int __builtin_printf(const char *, ...) __asm__(\"printf\");\n"
      "void __builtin_exit(int) __asm__(\"exit\") __attribute__((noreturn));\n"
      "void __builtin_trap(void) __asm__(\"abort\") __attribute__((noreturn));\n", -1);
    if (cprimepp_is_cpp_filename(file->filename)) cstr_cat(cs, "}\n", -1);
  }
  cstr_printf(cs, "#define __BASE_FILE__ \"%s\"\n", file->filename);
}

ST_FUNC void preprocess_start(CPRIMEState *s1, int filetype)
{
  int is_asm = !!(filetype & (AFF_TYPE_ASM | AFF_TYPE_ASMPP));

  cprimepp_new(s1);

  s1->include_stack_ptr = s1->include_stack;
  s1->ifdef_stack_ptr = s1->ifdef_stack;
  file->ifdef_stack_ptr = s1->ifdef_stack_ptr;
  pp_expr = 0;
  pp_counter = 0;
  pp_debug_tok = pp_debug_symv = 0;
  s1->pack_stack[0] = 0;
  s1->pack_stack_ptr = s1->pack_stack;

  set_idnum('$', !is_asm && s1->dollars_in_identifiers ? IS_ID : 0);
  set_idnum('.', is_asm ? IS_ID : 0);

  if (!(filetype & AFF_TYPE_ASM))
  {
    CString cstr;
    cstr_new(&cstr);
    cprime_predefs(s1, &cstr, is_asm);
    if (s1->cmdline_defs.size)
      cstr_cat(&cstr, s1->cmdline_defs.data, s1->cmdline_defs.size);
    if (s1->cmdline_incl.size)
      cstr_cat(&cstr, s1->cmdline_incl.data, s1->cmdline_incl.size);
    //printf("%.*s\n", cstr.size, (char*)cstr.data);
    *s1->include_stack_ptr++ = file;
    cprime_open_bf(s1, "<command line>", cstr.size);
    memcpy(file->buffer, cstr.data, cstr.size);
    cstr_free(&cstr);
  }
  parse_flags = is_asm ? PARSE_FLAG_ASM_FILE : 0;
}

// Cleanup From Error/Setjmp
ST_FUNC void preprocess_end(CPRIMEState *s1)
{
  while (macro_stack)
    end_macro();
  macro_ptr = NULL;
  while (file)
    cprime_close();
  cprimepp_delete(s1);
}

ST_FUNC int set_idnum(int c, int val)
{
  int prev = isidnum_table[c - CH_EOF];
  isidnum_table[c - CH_EOF] = val;
  if ((unsigned)c < 256)
  {
    ident_cont[c] = (val & (IS_ID | IS_NUM)) ? c : 0;
    ident_space[c] = (val & IS_SPC) ? 1 : 0;
  }
  return prev;
}

ST_FUNC void cprimepp_new(CPRIMEState *s)
{
  int i, c;
  const char *p, *r;

  // Init Isid Table
  for (i = CH_EOF; i < 128; i++)
    set_idnum(i,
              is_space(i) ? IS_SPC
              : isid(i) ? IS_ID
              : isnum(i) ? IS_NUM
              : 0);

  for (i = 128; i < 256; i++)
    set_idnum(i, IS_ID);

  // Init Skip Stop Table
  memset(pp_skip_stop, 0, sizeof(pp_skip_stop));
  memset(pp_skip_stop_msg, 0, sizeof(pp_skip_stop_msg));
  for (i = 0; i < 256; i++)
  {
    int stop = i == '\n' || i == '\\' || i == '#';
    if (is_space(i))
      stop = 1;
    pp_skip_stop[i] = stop || i == '\"' || i == '\'' || i == '/';
    pp_skip_stop_msg[i] = stop;
  }

  // Init Allocators
  tal_new(&toksym_alloc, TOKSYM_TAL_SIZE);
  tal_new(&tokstr_alloc, TOKSTR_TAL_SIZE);

  hash_ident_size = TOK_HASH_SIZE;
  hash_ident = cprime_mallocz(hash_ident_size * sizeof(*hash_ident));
  memset(s->cached_includes_hash, 0, sizeof s->cached_includes_hash);

  cstr_new(&tokcstr);
  cstr_new(&cstr_buf);
  cstr_realloc(&cstr_buf, STRING_MAX_SIZE);
  tok_str_new(&unget_buf);
  tok_str_realloc(&unget_buf, TOKSTR_MAX_SIZE);
  tok_str_new(&tokstr_buf);
  tok_str_realloc(&tokstr_buf, TOKSTR_MAX_SIZE);

  tok_ident = TOK_IDENT;
  p = cprime_keywords;
  while (*p)
  {
    r = p;
    for (;;)
    {
      c = *r++;
      if (c == '\0')
        break;
    }
    tok_alloc(p, r - p - 1);
    p = r;
  }

  /* we add dummy defines for some special macros to speed up tests
     and to have working defined() */
  define_push(TOK___LINE__, MACRO_OBJ, NULL, NULL);
  define_push(TOK___FILE__, MACRO_OBJ, NULL, NULL);
  define_push(TOK___DATE__, MACRO_OBJ, NULL, NULL);
  define_push(TOK___TIME__, MACRO_OBJ, NULL, NULL);
  define_push(TOK___COUNTER__, MACRO_OBJ, NULL, NULL);
}

ST_FUNC void cprimepp_delete(CPRIMEState *s)
{
  int i, n;

#ifdef _WIN32
  free_include_directories();
#endif

  dynarray_reset(&s->cached_includes, &s->nb_cached_includes);

  // Free Tokens
  n = tok_ident - TOK_IDENT;
  if (n > total_idents)
    total_idents = n;
  for (i = n; --i >= 0;)
    tal_free(&toksym_alloc, table_ident[i]);
  cprime_free(table_ident);
  table_ident = NULL;
  table_ident_capacity = 0;

  // Free Static Buffers
  cstr_free(&tokcstr);
  cstr_free(&cstr_buf);
  tok_str_free_str(tokstr_buf.str);
  tok_str_free_str(unget_buf.str);

  // Free Allocators
  tal_delete(&toksym_alloc);
  tal_delete(&tokstr_alloc);

  // Reset parser globals for the next translation unit.
  cprime_free(hash_ident);
  hash_ident = NULL;
  hash_ident_size = 0;
  file = NULL;
  macro_ptr = NULL;
  macro_stack = NULL;
  tok = TOK_EOF;
  tok_flags = 0;
  parse_flags = 0;
  memset(&tokc, 0, sizeof tokc);
  memset(&tokcstr, 0, sizeof tokcstr);
  memset(&cstr_buf, 0, sizeof cstr_buf);
  memset(&tokstr_buf, 0, sizeof tokstr_buf);
  memset(&unget_buf, 0, sizeof unget_buf);
  tok_ident = TOK_IDENT;
  cpp_spelling_using_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_typename_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_static_assert_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_template_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_friend_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_wchar_t_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_index_op_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_static_cast_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_reinterpret_cast_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_const_cast_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_dynamic_cast_tok = CPC_SPELLING_UNSEEN;
  cpp_spelling_delete_tok = CPC_SPELLING_UNSEEN;
  pp_expr = 0;
  pp_debug_tok = 0;
  pp_debug_symv = 0;
  pp_counter = 0;
}

// -------------------------------------------------------------------------
// cpc -E [-P[1]] [-dD} support

static int pp_need_space(int a, int b);

static void tok_print(const int *str, const char *msg, ...)
{
  FILE *fp = cprime_state->ppfp;
  va_list ap;
  int t, t0, s;
  CValue cval;

  va_start(ap, msg);
  vfprintf(fp, msg, ap);
  va_end(ap);

  s = t0 = 0;
  while (str)
  {
    TOK_GET(&t, &str, &cval);
    if (t == 0 || t == TOK_EOF)
      break;
    if (pp_need_space(t0, t))
      s = 0;
    fprintf(fp, &" %s"[s], t == TOK_PLCHLDR ? "<>" : get_tok_str(t, &cval));
    s = 1, t0 = t;
  }
  fprintf(fp, "\n");
}

static void pp_line(CPRIMEState *s1, BufferedFile *f, int level)
{
  int d = f->line_num - f->line_ref;

  if (s1->dflag & 4)
    return;

  if (s1->Pflag == LINE_MACRO_OUTPUT_FORMAT_NONE)
    ;
  else if (level == 0 && f->line_ref && d < 8)
  {
    while (d > 0)
      fputs("\n", s1->ppfp), --d;
  }
  else if (s1->Pflag == LINE_MACRO_OUTPUT_FORMAT_STD)
    fprintf(s1->ppfp, "#line %d \"%s\"\n", f->line_num, f->filename);
  else
  {
    fprintf(s1->ppfp, "# %d \"%s\"%s\n", f->line_num, f->filename,
            level > 0 ? " 1" : level < 0 ? " 2" : "");
  }
  f->line_ref = f->line_num;
}

static void define_print(CPRIMEState *s1, int v)
{
  FILE *fp;
  Sym *s;

  s = define_find(v);
  if (NULL == s || NULL == s->d)
    return;

  fp = s1->ppfp;
  fprintf(fp, "#define %s", get_tok_str(v, NULL));
  if (s->type.t & MACRO_FUNC)
  {
    Sym *a = s->next;
    fprintf(fp, "(");
    if (a)
      for (;;)
      {
        fprintf(fp, "%s", get_tok_str(a->v, NULL));
        if (!(a = a->next))
          break;
        fprintf(fp, ",");
      }
    fprintf(fp, ")");
  }
  tok_print(s->d, "");
}

static void pp_debug_defines(CPRIMEState *s1)
{
  int v, t;
  const char *vs;
  FILE *fp;

  t = pp_debug_tok;
  if (t == 0)
    return;

  file->line_num--;
  pp_line(s1, file, 0);
  file->line_ref = ++file->line_num;

  fp = s1->ppfp;
  v = pp_debug_symv;
  vs = get_tok_str(v, NULL);
  if (t == TOK_DEFINE)
    define_print(s1, v);
  else if (t == TOK_UNDEF)
    fprintf(fp, "#undef %s\n", vs);
  else if (t == TOK_push_macro)
    fprintf(fp, "#pragma push_macro(\"%s\")\n", vs);
  else if (t == TOK_pop_macro)
    fprintf(fp, "#pragma pop_macro(\"%s\")\n", vs);
  pp_debug_tok = 0;
}

// Add a space between tokens a and b to avoid unwanted textual pasting
static int pp_need_space(int a, int b)
{
  return 'E' == a ? '+' == b || '-' == b
         : '+' == a ? TOK_INC == b || '+' == b
         : '-' == a ? TOK_DEC == b || '-' == b
         : a >= TOK_IDENT || a == TOK_PPNUM ? b >= TOK_IDENT || b == TOK_PPNUM
         : 0;
}

// Maybe Hex Like 0X1E
static int pp_check_he0xE(int t, const char *p)
{
  if (t == TOK_PPNUM && toup(strchr(p, 0)[-1]) == 'E')
    return 'E';
  return t;
}

// Preprocess the current file
ST_FUNC int cprime_preprocess(CPRIMEState *s1)
{
  BufferedFile **iptr;
  int token_seen, spcs, level;
  const char *p;
  char white[400];

  parse_flags = PARSE_FLAG_PREPROCESS
                | (parse_flags &PARSE_FLAG_ASM_FILE)
                | PARSE_FLAG_LINEFEED
                | PARSE_FLAG_SPACES
                | PARSE_FLAG_ACCEPT_STRAYS
                ;
  if (s1->Pflag == LINE_MACRO_OUTPUT_FORMAT_P10)
    parse_flags |= PARSE_FLAG_TOK_NUM, s1->Pflag = 1;

  if (s1->do_bench)
  {
    // for PP benchmarks
    do next(); while (tok != TOK_EOF);
    return 0;
  }

  token_seen = TOK_LINEFEED, spcs = 0, level = 0;
  if (file->prev)
    pp_line(s1, file->prev, level++);
  pp_line(s1, file, level);

  for (;;)
  {
    iptr = s1->include_stack_ptr;
    next();
    if (tok == TOK_EOF)
      break;

    level = s1->include_stack_ptr - iptr;
    if (level)
    {
      if (level > 0)
        pp_line(s1, *iptr, 0);
      pp_line(s1, file, level);
    }
    if (s1->dflag & 7)
    {
      pp_debug_defines(s1);
      if (s1->dflag & 4)
        continue;
    }

    if (is_space(tok))
    {
      if (spcs < sizeof white - 1)
        white[spcs++] = tok;
      continue;
    }
    else if (tok == TOK_LINEFEED)
    {
      spcs = 0;
      if (token_seen == TOK_LINEFEED)
        continue;
      ++file->line_ref;
    }
    else if (token_seen == TOK_LINEFEED)
      pp_line(s1, file, 0);
    else if (spcs == 0 && pp_need_space(token_seen, tok))
      white[spcs++] = ' ';

    white[spcs] = 0, fputs(white, s1->ppfp), spcs = 0;
    fputs(p = get_tok_str(tok, &tokc), s1->ppfp);
    token_seen = pp_check_he0xE(tok, p);
  }
  return 0;
}





