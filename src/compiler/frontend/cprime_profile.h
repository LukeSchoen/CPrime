/* Opt-in driver instrumentation for the Cost area.

   CPC_PROFILE_PHASES already splits a run into setup, translation-unit work,
   output writing and cleanup, and CPC_PROFILE_SCANS counts template and
   overload lookup work.  Neither answers where inside the translation unit the
   time goes.  These accumulators add the two largest recorded leaves:

     - next_nomacro() calls and their total elapsed time
     - gen_function() calls and their total elapsed time, not counting the
       time spent in a nested definition (a queued member body compiled from
       inside another function counts for the outer function only once)

   The function total is then split into entry setup, prolog, body and epilog
   stages.  fastopt_ms is a subset of epilog_ms.  A finer split also separates
   function setup, pending-helper work and all other time from those stages.
   Inside fastopt_ms the counters split the optimizer into decode, stack-slot
   promotion, rewrite/simplification, relocation validation and final commit.
   Stage timing is charged only to the outermost function so nested
   definitions cannot double-count time.

   The same outer-function accounting also charges each function to the
   translation-unit region in which it runs.  Driver and frontend boundary
   switches can then split the non-function time into input setup,
   preprocessing/generator setup, top-level declarations, deferred emission,
   object finalization, cleanup and output writing.

   With CPC_PROFILE_DETAIL unset every hook is one predictable branch on an
   already-resolved environment check, and profile_detail_now_ns() is not called
   more than once every PROFILE_DETAIL_LEXER_WINDOW tokens. */

#ifndef CPRIME_PROFILE_DETAIL_H
#define CPRIME_PROFILE_DETAIL_H

static unsigned long long profile_detail_lexer_calls;
static unsigned long long profile_detail_lexer_sampled_calls;
static unsigned long long profile_detail_lexer_ns;
static unsigned long long profile_detail_lexer_started;
static unsigned long long profile_detail_function_calls;
static unsigned long long profile_detail_function_ns;
enum
{
  PROFILE_DETAIL_STAGE_ENTRY,
  PROFILE_DETAIL_STAGE_PROLOG,
  PROFILE_DETAIL_STAGE_BODY,
  PROFILE_DETAIL_STAGE_EPILOG,
  PROFILE_DETAIL_STAGE_COUNT
};
static unsigned long long profile_detail_stage_ns[PROFILE_DETAIL_STAGE_COUNT];
static unsigned long long profile_detail_fastopt_ns;
enum
{
  PROFILE_FASTOPT_DECODE,
  PROFILE_FASTOPT_PROMOTE,
  PROFILE_FASTOPT_REWRITE,
  PROFILE_FASTOPT_RELOCATE,
  PROFILE_FASTOPT_COMMIT,
  PROFILE_FASTOPT_STAGE_COUNT
};
static unsigned long long profile_detail_fastopt_stage_ns[PROFILE_FASTOPT_STAGE_COUNT];
static unsigned long long profile_detail_function_setup_ns;
static unsigned long long profile_detail_function_prolog_ns;
static unsigned long long profile_detail_function_body_ns;
static unsigned long long profile_detail_function_epilog_ns;
static unsigned long long profile_detail_function_pending_ns;
static int profile_detail_enabled;
static int profile_detail_initialized;
static int profile_detail_function_depth;

enum
{
  PROFILE_TU_INPUT,
  PROFILE_TU_PREPROCESS_START,
  PROFILE_TU_GEN_INIT,
  PROFILE_TU_ELF_BEGIN,
  PROFILE_TU_FRONTEND_SETUP,
  PROFILE_TU_TOP_DECL,
  PROFILE_TU_DEFERRED,
  PROFILE_TU_ELF_END,
  PROFILE_TU_PREPROCESS,
  PROFILE_TU_ASSEMBLE,
  PROFILE_TU_GEN_FINISH,
  PROFILE_TU_PREPROCESS_END,
  PROFILE_TU_OBJECT_WRITE,
  PROFILE_TU_DEPS_WRITE,
  PROFILE_TU_COUNT
};
static const char *const profile_tu_names[PROFILE_TU_COUNT] =
{
  "input",
  "preprocess_start",
  "gen_init",
  "elf_begin",
  "frontend_setup",
  "top_decl",
  "deferred",
  "elf_end",
  "preprocess",
  "assemble",
  "gen_finish",
  "preprocess_end",
  "object_write",
  "deps_write"
};
static unsigned long long profile_tu_ns[PROFILE_TU_COUNT];
static unsigned long long profile_tu_function_ns[PROFILE_TU_COUNT];
static unsigned long long profile_tu_started;
static int profile_tu_active = -1;

enum
{
  PROFILE_TOP_DISPATCH,
  PROFILE_TOP_PARSE_BTYPE,
  PROFILE_TOP_TYPE_DECL,
  PROFILE_TOP_DECL_TAIL,
  PROFILE_TOP_FUNCTION_PREFIX,
  PROFILE_TOP_INLINE_CAPTURE,
  PROFILE_TOP_COUNT
};
static const char *const profile_top_names[PROFILE_TOP_COUNT] =
{
  "dispatch",
  "parse_btype",
  "type_decl",
  "decl_tail",
  "function_prefix",
  "inline_capture"
};
static unsigned long long profile_top_ns[PROFILE_TOP_COUNT];
static unsigned long long profile_top_function_ns[PROFILE_TOP_COUNT];
static unsigned long long profile_top_calls[PROFILE_TOP_COUNT];
static unsigned long long profile_top_started[PROFILE_TOP_COUNT];
static int profile_top_depth[PROFILE_TOP_COUNT];

/* Statements inside block() are split into declaration, expression and
   control/nested-block dispatch.  Each block() frame subtracts the elapsed
   time of its direct nested blocks from the statement that contained them,
   so nested work is charged once, at the level where it ran. */
enum
{
  PROFILE_DETAIL_BODY_DECL,
  PROFILE_DETAIL_BODY_EXPR,
  PROFILE_DETAIL_BODY_CONTROL_IF,
  PROFILE_DETAIL_BODY_CONTROL_LOOP,
  PROFILE_DETAIL_BODY_CONTROL_SWITCH,
  PROFILE_DETAIL_BODY_CONTROL_RETURN,
  PROFILE_DETAIL_BODY_CONTROL_JUMP,
  PROFILE_DETAIL_BODY_CONTROL_OTHER,
  PROFILE_DETAIL_BODY_CATEGORY_COUNT
};
#define PROFILE_DETAIL_BODY_MAX_CATEGORY_COUNT 8
static const char *const profile_detail_body_names[PROFILE_DETAIL_BODY_MAX_CATEGORY_COUNT] =
{
  "stmt_decl",
  "stmt_expr",
  "stmt_if",
  "stmt_loop",
  "stmt_switch",
  "stmt_return",
  "stmt_jump",
  "stmt_control_other"
};
#define PROFILE_DETAIL_BODY_MAX_DEPTH 128
typedef struct ProfileDetailBodyFrame
{
  unsigned long long block_started;
  unsigned long long nested_ns;
  unsigned long long statement_started;
  unsigned long long statement_nested_start;
  int statement_category;
  int statement_active;
} ProfileDetailBodyFrame;
static ProfileDetailBodyFrame profile_detail_body_frames[PROFILE_DETAIL_BODY_MAX_DEPTH];
static unsigned long long profile_detail_body_ns[PROFILE_DETAIL_BODY_CATEGORY_COUNT];
static unsigned long long profile_detail_body_calls[PROFILE_DETAIL_BODY_CATEGORY_COUNT];
static int profile_detail_body_depth;

enum
{
  PROFILE_DETAIL_EXTRA_EXPR_PARSE,
  PROFILE_DETAIL_EXTRA_EXPR_TAIL,
  PROFILE_DETAIL_EXTRA_IF_COND,
  PROFILE_DETAIL_EXTRA_IF_NEW_SCOPE,
  PROFILE_DETAIL_EXTRA_IF_PREV_SCOPE,
  PROFILE_DETAIL_EXTRA_DECL_FASTPATH,
  PROFILE_DETAIL_EXTRA_EXPR_EQ,
  PROFILE_DETAIL_EXTRA_EXPR_COND,
  PROFILE_DETAIL_EXTRA_UNARY,
  PROFILE_DETAIL_EXTRA_UNARY_PREFIX,
  PROFILE_DETAIL_EXTRA_UNARY_POST,
  PROFILE_DETAIL_EXTRA_COUNT
};
static const char *const profile_detail_extra_names[PROFILE_DETAIL_EXTRA_COUNT] =
{
  "expr_parse",
  "expr_tail",
  "if_cond",
  "if_new_scope",
  "if_prev_scope",
  "decl_fastpath",
  "expr_eq",
  "expr_cond",
  "unary",
  "unary_prefix",
  "unary_post"
};
static unsigned long long profile_detail_extra_ns[PROFILE_DETAIL_EXTRA_COUNT];
static unsigned long long profile_detail_extra_calls[PROFILE_DETAIL_EXTRA_COUNT];
static unsigned long long profile_detail_extra_started[PROFILE_DETAIL_EXTRA_COUNT];
static int profile_detail_extra_depth[PROFILE_DETAIL_EXTRA_COUNT];
static unsigned long long profile_detail_unary_started;
static unsigned long long profile_detail_unary_phase_started;
static int profile_detail_unary_depth;
static int profile_detail_unary_post_phase;

static void profile_detail_init(void)
{
  if (!profile_detail_initialized)
  {
    profile_detail_initialized = 1;
    profile_detail_enabled = getenv("CPC_PROFILE_DETAIL") != NULL;
  }
}

static unsigned long long profile_detail_now_ns(void)
{
#ifdef _WIN32
  LARGE_INTEGER counter, frequency;
  QueryPerformanceCounter(&counter);
  QueryPerformanceFrequency(&frequency);
  return (unsigned long long)(counter.QuadPart / frequency.QuadPart) * 1000000000ULL
       + (unsigned long long)(counter.QuadPart % frequency.QuadPart) * 1000000000ULL
         / (unsigned long long)frequency.QuadPart;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long long)tv.tv_sec * 1000000000ULL
       + (unsigned long long)tv.tv_usec * 1000ULL;
#endif
}

/* Translation-unit regions are switched only at phase boundaries.  A nested
   function is charged to whichever region is active when it completes; the
   outer function timer already includes nested definitions. */
static void profile_tu_switch(int region)
{
  unsigned long long now;

  if (!profile_detail_enabled)
    return;
  now = profile_detail_now_ns();
  if (profile_tu_active >= 0 && profile_tu_active < PROFILE_TU_COUNT)
    profile_tu_ns[profile_tu_active] += now - profile_tu_started;
  profile_tu_active = region;
  profile_tu_started = now;
}

static void profile_tu_end(void)
{
  profile_tu_switch(-1);
}

static void profile_top_begin(int region)
{
  if (profile_top_depth[region]++ == 0)
  {
    profile_top_started[region] = profile_detail_now_ns();
    ++profile_top_calls[region];
  }
}

static void profile_top_end(int region)
{
  if (--profile_top_depth[region] == 0)
    profile_top_ns[region] +=
        profile_detail_now_ns() - profile_top_started[region];
}

#define PROFILE_TOP_SEG_BEGIN(region) \
  do { if (profile_detail_enabled) profile_top_begin(region); } while (0)
#define PROFILE_TOP_SEG_END(region) \
  do { if (profile_detail_enabled) profile_top_end(region); } while (0)

/* Exclusive split of the top-level declaration tail.  Prefix covers the
   shared initialization before the type-specific branches.  Param covers the
   VT_CMP parameter completion, Typedef covers the typedef save and
   compatibility path, Init covers initializer/static emission, ConstObject
   covers the pending constant/constexpr object emission, and Other covers the
   void-declaration and alias tails.  The child is open only while the
   outermost top-level decl_tail frame owns the region. */
enum
{
  PROFILE_DECL_TAIL_PREFIX,
  PROFILE_DECL_TAIL_PARAM,
  PROFILE_DECL_TAIL_TYPEDEF,
  PROFILE_DECL_TAIL_INIT_PRE,
  PROFILE_DECL_TAIL_INIT_ALLOC,
  PROFILE_DECL_TAIL_INIT_POST,
  PROFILE_DECL_TAIL_CONST_OBJECT,
  PROFILE_DECL_TAIL_OTHER,
  PROFILE_DECL_TAIL_ALLOC_PREFIX,
  PROFILE_DECL_TAIL_ALLOC_LOOKUP,
  PROFILE_DECL_TAIL_ALLOC_SIZE,
  PROFILE_DECL_TAIL_ALLOC_SIZE_KNOWN,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN,
  PROFILE_DECL_TAIL_ALLOC_STORAGE,
  PROFILE_DECL_TAIL_ALLOC_INIT,
  PROFILE_DECL_TAIL_ALLOC_INIT_SETUP,
  PROFILE_DECL_TAIL_ALLOC_INIT_EXPR,
  PROFILE_DECL_TAIL_ALLOC_INIT_POST,
  PROFILE_DECL_TAIL_ALLOC_TAIL,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_LEXER,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_MACRO,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_SUBST,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND_GROWTH,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND_COPY,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_REPLAY,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_REPLAY_SETUP,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_REPLAY_INIT,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_REPLAY_RESET,
  PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_EXTENT,
  PROFILE_DECL_TAIL_INIT_IMPL_PREFIX,
  PROFILE_DECL_TAIL_INIT_IMPL_MEMBER_POINTER,
  PROFILE_DECL_TAIL_INIT_IMPL_ARRAY,
  PROFILE_DECL_TAIL_INIT_IMPL_ARRAY_STRING,
  PROFILE_DECL_TAIL_INIT_IMPL_ARRAY_LOOP,
  PROFILE_DECL_TAIL_INIT_IMPL_ARRAY_DESIGNATOR,
  PROFILE_DECL_TAIL_INIT_IMPL_ARRAY_BODY,
  PROFILE_DECL_TAIL_INIT_IMPL_ARRAY_ELEM_DECL,
  PROFILE_DECL_TAIL_INIT_IMPL_ARRAY_ELEM_POST,
  PROFILE_DECL_TAIL_SCALAR_FAST_LITERAL,
  PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_PAREN,
  PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_STRING,
  PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_OTHER,
  PROFILE_DECL_TAIL_SCALAR_FAST_PUT,
  PROFILE_DECL_TAIL_INIT_IMPL_SCALAR_ELEM,
  PROFILE_DECL_TAIL_INIT_IMPL_LIST,
  PROFILE_DECL_TAIL_INIT_IMPL_STRUCT,
  PROFILE_DECL_TAIL_INIT_IMPL_BRACE,
  PROFILE_DECL_TAIL_INIT_IMPL_SCALAR,
  PROFILE_DECL_TAIL_COUNT
};
static const char *const profile_decl_tail_names[PROFILE_DECL_TAIL_COUNT] =
{
  "prefix",
  "param",
  "typedef",
  "init_pre",
  "init_alloc",
  "init_post",
  "const_object",
  "other",
  "alloc_prefix",
  "alloc_lookup",
  "alloc_size",
  "alloc_size_known",
  "alloc_size_unknown",
  "alloc_storage",
  "alloc_init",
  "alloc_init_setup",
  "alloc_init_expr",
  "alloc_init_post",
  "alloc_tail",
  "alloc_size_unknown_save",
  "alloc_size_unknown_save_next",
  "alloc_size_unknown_save_next_lexer",
  "alloc_size_unknown_save_next_macro",
  "alloc_size_unknown_save_next_subst",
  "alloc_size_unknown_save_append",
  "alloc_size_unknown_save_append_growth",
  "alloc_size_unknown_save_append_copy",
  "alloc_size_unknown_replay",
  "alloc_size_unknown_replay_setup",
  "alloc_size_unknown_replay_init",
  "alloc_size_unknown_replay_reset",
  "alloc_size_unknown_extent",
  "init_impl_prefix",
  "init_impl_member_pointer",
  "init_impl_array",
  "init_impl_array_string",
  "init_impl_array_loop",
  "init_impl_array_designator",
  "init_impl_array_body",
  "init_impl_array_elem_decl",
  "init_impl_array_elem_post",
  "scalar_fast_literal",
  "scalar_fast_expr_paren",
  "scalar_fast_expr_string",
  "scalar_fast_expr_other",
  "scalar_fast_put",
  "init_impl_scalar_elem",
  "init_impl_list",
  "init_impl_struct",
  "init_impl_brace",
  "init_impl_scalar"
};
static unsigned long long profile_decl_tail_ns[PROFILE_DECL_TAIL_COUNT];
static unsigned long long profile_decl_tail_calls[PROFILE_DECL_TAIL_COUNT];
static int profile_decl_tail_active = -1;
static unsigned long long profile_decl_tail_started;
static int profile_alloc_unknown_active = -1;
static unsigned long long profile_alloc_unknown_started;
static int profile_alloc_unknown_replay_active = -1;
static unsigned long long profile_alloc_unknown_replay_started;
#define PROFILE_ALLOC_UNKNOWN_SAVE_WINDOW 64
static unsigned long long profile_alloc_unknown_save_tokens;
static unsigned long long profile_alloc_unknown_save_samples;
static unsigned long long profile_alloc_unknown_save_next_ns;
static unsigned long long profile_alloc_unknown_save_append_ns;
static unsigned long long profile_alloc_unknown_save_append_growth_ns;
static unsigned long long profile_alloc_unknown_save_append_growth_calls;
static int profile_alloc_unknown_save_append_active;
enum
{
  PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_LEXER,
  PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_MACRO,
  PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_SUBST,
  PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_PART_COUNT
};
static unsigned long long profile_alloc_unknown_save_next_part_ns[
    PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_PART_COUNT];
static int profile_alloc_unknown_save_next_active;
static int profile_alloc_unknown_save_next_region = -1;
static unsigned long long profile_alloc_unknown_save_next_region_started;

/* Exclusive split of the macro-stream loop inside next().  The three rows
   cover value-token consumption through tok_get(), non-value token
   advancement, and end_macro() stack transitions.  Times are sampled by the
   same one-in-64 token window as the save-next split; the counters are
   deterministic and classify every macro-loop path. */
enum
{
  PROFILE_MACRO_LOOP_IDLE = -1,
  PROFILE_MACRO_LOOP_TOP,
  PROFILE_MACRO_LOOP_VALUE,
  PROFILE_MACRO_LOOP_END_MACRO,
  PROFILE_MACRO_LOOP_NONVALUE,
  PROFILE_MACRO_LOOP_POST,
  PROFILE_MACRO_LOOP_CONVERT,
  PROFILE_MACRO_LOOP_GAP,
  PROFILE_MACRO_LOOP_PART_COUNT
};
static const char *const profile_macro_loop_names[
    PROFILE_MACRO_LOOP_PART_COUNT] =
{
  "top",
  "value",
  "end_macro",
  "nonvalue",
  "post",
  "convert",
  "gap"
};
static unsigned long long profile_macro_loop_part_ns[
    PROFILE_MACRO_LOOP_PART_COUNT];
static unsigned long long profile_macro_loop_part_calls[
    PROFILE_MACRO_LOOP_PART_COUNT];
static int profile_macro_loop_region = PROFILE_MACRO_LOOP_IDLE;
static unsigned long long profile_macro_loop_region_started;

enum
{
  PROFILE_MACRO_NONVALUE_SPACE,
  PROFILE_MACRO_NONVALUE_LINEFEED,
  PROFILE_MACRO_NONVALUE_MARKED,
  PROFILE_MACRO_NONVALUE_PUNCT,
  PROFILE_MACRO_NONVALUE_OTHER,
  PROFILE_MACRO_NONVALUE_CLASS_COUNT
};
static const char *const profile_macro_nonvalue_names[
    PROFILE_MACRO_NONVALUE_CLASS_COUNT] =
{
  "nonvalue_space",
  "nonvalue_linefeed",
  "nonvalue_marked",
  "nonvalue_punct",
  "nonvalue_other"
};
static unsigned long long profile_macro_nonvalue_calls[
    PROFILE_MACRO_NONVALUE_CLASS_COUNT];

static void profile_macro_loop_switch(int region)
{
  unsigned long long now;

  if (!profile_alloc_unknown_save_next_active
      || profile_macro_loop_region == region)
    return;
  now = profile_detail_now_ns();
  if (profile_macro_loop_region >= 0)
    profile_macro_loop_part_ns[profile_macro_loop_region] +=
        now - profile_macro_loop_region_started;
  profile_macro_loop_region = region;
  profile_macro_loop_region_started = now;
}

static void profile_macro_loop_close(void)
{
  if (profile_macro_loop_region < 0)
    return;
  profile_macro_loop_part_ns[profile_macro_loop_region] +=
      profile_detail_now_ns() - profile_macro_loop_region_started;
  profile_macro_loop_region = PROFILE_MACRO_LOOP_IDLE;
}

#define PROFILE_MACRO_LOOP_SWITCH(region) \
  do { if (profile_detail_enabled) \
    profile_macro_loop_switch(region); } while (0)
#define PROFILE_MACRO_LOOP_CLOSE() \
  do { if (profile_detail_enabled) \
    profile_macro_loop_close(); } while (0)
#define PROFILE_MACRO_LOOP_COUNT(region) \
  do { if (profile_detail_enabled) \
    ++profile_macro_loop_part_calls[region]; } while (0)

static void profile_alloc_unknown_save_next_sample_begin(void)
{
  profile_alloc_unknown_save_next_active = 1;
  profile_alloc_unknown_save_next_region = -1;
  profile_macro_loop_region = PROFILE_MACRO_LOOP_IDLE;
}

static void profile_alloc_unknown_save_next_switch(int region)
{
  unsigned long long now;

  if (!profile_alloc_unknown_save_next_active
      || profile_alloc_unknown_save_next_region == region)
    return;
  now = profile_detail_now_ns();
  if (profile_alloc_unknown_save_next_region >= 0)
    profile_alloc_unknown_save_next_part_ns[
        profile_alloc_unknown_save_next_region] +=
        now - profile_alloc_unknown_save_next_region_started;
  profile_alloc_unknown_save_next_region = region;
  profile_alloc_unknown_save_next_region_started = now;
}

static void profile_alloc_unknown_save_next_sample_end(void)
{
  if (!profile_alloc_unknown_save_next_active)
    return;
  profile_macro_loop_close();
  if (profile_alloc_unknown_save_next_region >= 0)
    profile_alloc_unknown_save_next_part_ns[
        profile_alloc_unknown_save_next_region] +=
        profile_detail_now_ns() - profile_alloc_unknown_save_next_region_started;
  profile_alloc_unknown_save_next_active = 0;
  profile_alloc_unknown_save_next_region = -1;
}
#define PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_SWITCH(region) \
  do { if (profile_detail_enabled) \
    profile_alloc_unknown_save_next_switch(region); } while (0)
enum
{
  PROFILE_SCALAR_FAST_EXPR_IDENT,
  PROFILE_SCALAR_FAST_EXPR_UNARY,
  PROFILE_SCALAR_FAST_EXPR_LITERAL,
  PROFILE_SCALAR_FAST_EXPR_OTHER,
  PROFILE_SCALAR_FAST_EXPR_COUNT
};
static unsigned long long profile_scalar_fast_expr_calls[PROFILE_SCALAR_FAST_EXPR_COUNT];

static int profile_scalar_fast_expr_is_integer_literal(int token)
{
  return token == TOK_CINT || token == TOK_CUINT
      || token == TOK_CLLONG || token == TOK_CULLONG
      || token == TOK_CLONG || token == TOK_CULONG;
}

static void profile_scalar_fast_expr_count(int token)
{
  int category;

  if (token >= TOK_UIDENT && token < SYM_FIRST_ANOM)
    category = PROFILE_SCALAR_FAST_EXPR_IDENT;
  else if (token == '+' || token == '-')
    category = PROFILE_SCALAR_FAST_EXPR_UNARY;
  else if (profile_scalar_fast_expr_is_integer_literal(token))
    category = PROFILE_SCALAR_FAST_EXPR_LITERAL;
  else
  {
    category = PROFILE_SCALAR_FAST_EXPR_OTHER;
  }
  ++profile_scalar_fast_expr_calls[category];
}

static void profile_decl_tail_close(unsigned long long now)
{
  if (profile_decl_tail_active < 0)
    return;
  profile_decl_tail_ns[profile_decl_tail_active] +=
      now - profile_decl_tail_started;
  profile_decl_tail_active = -1;
}

static void profile_decl_tail_begin(void)
{
  if (profile_top_depth[PROFILE_TOP_DECL_TAIL] != 1)
    return;
  profile_decl_tail_active = PROFILE_DECL_TAIL_PREFIX;
  profile_decl_tail_started = profile_detail_now_ns();
  ++profile_decl_tail_calls[PROFILE_DECL_TAIL_PREFIX];
}

static void profile_decl_tail_switch(int region)
{
  unsigned long long now;

  if (profile_decl_tail_active < 0
      || profile_top_depth[PROFILE_TOP_DECL_TAIL] != 1
      || profile_decl_tail_active == region)
    return;
  now = profile_detail_now_ns();
  profile_decl_tail_close(now);
  profile_decl_tail_active = region;
  profile_decl_tail_started = now;
  ++profile_decl_tail_calls[region];
}

static int profile_decl_tail_alloc_owned(void)
{
  return profile_decl_tail_active >= PROFILE_DECL_TAIL_ALLOC_PREFIX
      && profile_decl_tail_active <= PROFILE_DECL_TAIL_ALLOC_TAIL;
}

static void profile_decl_tail_alloc_switch(int region)
{
  if (profile_decl_tail_alloc_owned())
    profile_decl_tail_switch(region);
}

static void profile_alloc_unknown_switch(int region)
{
  unsigned long long now;

  if (!profile_detail_enabled
      || profile_decl_tail_active != PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN
      || profile_alloc_unknown_active == region)
    return;
  now = profile_detail_now_ns();
  if (profile_alloc_unknown_active >= 0)
    profile_decl_tail_ns[profile_alloc_unknown_active] +=
        now - profile_alloc_unknown_started;
  profile_alloc_unknown_active = region;
  profile_alloc_unknown_started = now;
  ++profile_decl_tail_calls[region];
}

static void profile_alloc_unknown_end(void)
{
  if (profile_alloc_unknown_active < 0)
    return;
  profile_decl_tail_ns[profile_alloc_unknown_active] +=
      profile_detail_now_ns() - profile_alloc_unknown_started;
  profile_alloc_unknown_active = -1;
}

static void profile_alloc_unknown_replay_switch(int region)
{
  unsigned long long now;

  if (!profile_detail_enabled
      || profile_alloc_unknown_active
          != PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_REPLAY
      || profile_alloc_unknown_replay_active == region)
    return;
  now = profile_detail_now_ns();
  if (profile_alloc_unknown_replay_active >= 0)
    profile_decl_tail_ns[profile_alloc_unknown_replay_active] +=
        now - profile_alloc_unknown_replay_started;
  profile_alloc_unknown_replay_active = region;
  profile_alloc_unknown_replay_started = now;
  ++profile_decl_tail_calls[region];
}

static void profile_alloc_unknown_replay_end(void)
{
  if (profile_alloc_unknown_replay_active < 0)
    return;
  profile_decl_tail_ns[profile_alloc_unknown_replay_active] +=
      profile_detail_now_ns() - profile_alloc_unknown_replay_started;
  profile_alloc_unknown_replay_active = -1;
}

static int profile_decl_init_depth;
static int profile_decl_init_owned;

static void profile_decl_init_enter(void)
{
  if (!profile_detail_enabled)
    return;
  if (profile_decl_init_depth == 0
      && profile_decl_tail_active != PROFILE_DECL_TAIL_ALLOC_INIT_EXPR)
    return;
  ++profile_decl_init_depth;
  if (profile_decl_init_depth == 1)
  {
    profile_decl_init_owned = 1;
    profile_decl_tail_switch(PROFILE_DECL_TAIL_INIT_IMPL_PREFIX);
  }
}

static void profile_decl_init_leave(void)
{
  if (!profile_detail_enabled || profile_decl_init_depth == 0)
    return;
  --profile_decl_init_depth;
  if (profile_decl_init_depth == 0)
    profile_decl_init_owned = 0;
}

/* The scalar-fast fallback owns the generic constant-expression parse.  Its
   leading token decides which exclusive row charges that parse, so a
   parenthesized expression, a string literal and every other leading token
   are measured apart instead of as one aggregate. */
static int profile_decl_scalar_fast_expr_region(int token)
{
  if (token == '(')
    return PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_PAREN;
  if (TOK_IS_STRING(token))
    return PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_STRING;
  return PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_OTHER;
}

static int profile_decl_scalar_fast_expr_owned(void)
{
  return profile_decl_tail_active == PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_PAREN
      || profile_decl_tail_active == PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_STRING
      || profile_decl_tail_active == PROFILE_DECL_TAIL_SCALAR_FAST_EXPR_OTHER;
}

static void profile_decl_nested_switch(int region)
{
  if (profile_decl_tail_active == PROFILE_DECL_TAIL_INIT_IMPL_ARRAY_ELEM_DECL
      || profile_decl_tail_active == PROFILE_DECL_TAIL_SCALAR_FAST_LITERAL
      || profile_decl_scalar_fast_expr_owned()
      || profile_decl_tail_active == PROFILE_DECL_TAIL_SCALAR_FAST_PUT)
    profile_decl_tail_switch(region);
}

static void profile_decl_tail_end(void)
{
  if (profile_decl_tail_active >= 0)
    profile_decl_tail_close(profile_detail_now_ns());
}

/* The size-only replay runs outside profile_decl_tail's normal ownership.
   These exclusive rows stay on the outermost size-only array loop so the
   replay's designator, element-call and aggregate-skip costs are measurable
   separately.  Nested arrays leave the outermost row in charge. */
enum
{
  PROFILE_INIT_IMPL_ARRAY_IDLE = -1,
  PROFILE_INIT_IMPL_ARRAY_LOOP,
  PROFILE_INIT_IMPL_ARRAY_DESIGNATOR,
  PROFILE_INIT_IMPL_ARRAY_ELEM_DECL,
  PROFILE_INIT_IMPL_ARRAY_SKIP,
  PROFILE_INIT_IMPL_ARRAY_BODY,
  PROFILE_INIT_IMPL_ARRAY_COUNT
};
static const char *const profile_init_impl_array_names[
    PROFILE_INIT_IMPL_ARRAY_COUNT] =
{
  "loop",
  "designator",
  "elem_decl",
  "skip",
  "body"
};
static unsigned long long profile_init_impl_array_ns[
    PROFILE_INIT_IMPL_ARRAY_COUNT];
static unsigned long long profile_init_impl_array_calls[
    PROFILE_INIT_IMPL_ARRAY_COUNT];
static int profile_init_impl_array_depth;
static int profile_init_impl_array_region = PROFILE_INIT_IMPL_ARRAY_IDLE;
static unsigned long long profile_init_impl_array_started;

static void profile_init_impl_array_begin(void)
{
  ++profile_init_impl_array_depth;
  if (profile_init_impl_array_depth != 1)
    return;
  profile_init_impl_array_region = PROFILE_INIT_IMPL_ARRAY_LOOP;
  profile_init_impl_array_started = profile_detail_now_ns();
  ++profile_init_impl_array_calls[PROFILE_INIT_IMPL_ARRAY_LOOP];
}

static void profile_init_impl_array_switch(int region)
{
  unsigned long long now;

  if (profile_init_impl_array_depth != 1
      || profile_init_impl_array_region == region)
    return;
  now = profile_detail_now_ns();
  if (profile_init_impl_array_region >= 0)
    profile_init_impl_array_ns[profile_init_impl_array_region] +=
        now - profile_init_impl_array_started;
  profile_init_impl_array_region = region;
  profile_init_impl_array_started = now;
  ++profile_init_impl_array_calls[region];
}

static void profile_init_impl_array_end(void)
{
  if (profile_init_impl_array_depth == 0)
    return;
  if (profile_init_impl_array_depth == 1
      && profile_init_impl_array_region >= 0)
  {
    profile_init_impl_array_ns[profile_init_impl_array_region] +=
        profile_detail_now_ns() - profile_init_impl_array_started;
    profile_init_impl_array_region = PROFILE_INIT_IMPL_ARRAY_IDLE;
  }
  --profile_init_impl_array_depth;
}

#define PROFILE_TOP_DECL_TAIL_BEGIN() \
  do { if (profile_detail_enabled) profile_decl_tail_begin(); } while (0)
#define PROFILE_TOP_DECL_TAIL_SEG(region) \
  do { if (profile_detail_enabled) profile_decl_tail_switch(region); } while (0)
#define PROFILE_TOP_DECL_TAIL_ALLOC_BEGIN() \
  do { if (profile_detail_enabled \
      && profile_decl_tail_active == PROFILE_DECL_TAIL_INIT_ALLOC) \
    profile_decl_tail_switch(PROFILE_DECL_TAIL_ALLOC_PREFIX); } while (0)
#define PROFILE_TOP_DECL_TAIL_ALLOC_SEG(region) \
  do { if (profile_detail_enabled) \
    profile_decl_tail_alloc_switch(region); } while (0)
#define PROFILE_TOP_DECL_TAIL_ALLOC_UNKNOWN_SEG(region) \
  do { if (profile_detail_enabled) profile_alloc_unknown_switch(region); } while (0)
#define PROFILE_TOP_DECL_TAIL_ALLOC_UNKNOWN_END() \
  do { if (profile_detail_enabled) profile_alloc_unknown_end(); } while (0)
#define PROFILE_TOP_DECL_TAIL_ALLOC_UNKNOWN_REPLAY_SEG(region) \
  do { if (profile_detail_enabled) \
    profile_alloc_unknown_replay_switch(region); } while (0)
#define PROFILE_TOP_DECL_TAIL_ALLOC_UNKNOWN_REPLAY_END() \
  do { if (profile_detail_enabled) profile_alloc_unknown_replay_end(); } while (0)
#define PROFILE_TOP_DECL_TAIL_INIT_BEGIN() \
  do { if (profile_detail_enabled) profile_decl_init_enter(); } while (0)
#define PROFILE_TOP_DECL_TAIL_INIT_SEG(region) \
  do { if (profile_detail_enabled && profile_decl_init_owned \
      && profile_decl_init_depth == 1) \
    profile_decl_tail_switch(region); } while (0)
#define PROFILE_TOP_DECL_TAIL_INIT_END() \
  do { if (profile_detail_enabled) profile_decl_init_leave(); } while (0)
#define PROFILE_TOP_DECL_TAIL_NESTED_SEG(region) \
  do { if (profile_detail_enabled) profile_decl_nested_switch(region); } while (0)
#define PROFILE_INIT_IMPL_ARRAY_BEGIN() \
  do { if (profile_detail_enabled) profile_init_impl_array_begin(); } while (0)
#define PROFILE_INIT_IMPL_ARRAY_SEG(region) \
  do { if (profile_detail_enabled) \
    profile_init_impl_array_switch(region); } while (0)
#define PROFILE_INIT_IMPL_ARRAY_END() \
  do { if (profile_detail_enabled) profile_init_impl_array_end(); } while (0)
#define PROFILE_SCALAR_FAST_EXPR_COUNT(token) \
  do { if (profile_detail_enabled) profile_scalar_fast_expr_count(token); } while (0)
#define PROFILE_TOP_DECL_TAIL_END() \
  do { if (profile_detail_enabled) profile_decl_tail_end(); } while (0)

/* Exclusive split of the outermost parse_btype() call.  Prefix covers the
   leading storage/qualifier/attribute specifiers and the simple basic-type
   cases.  Base covers the class-name and template resolution the default
   branch reaches after those.  Template covers the C++ qualified-name and
   template-token probes.  Tag covers the identifier path's owner/class-name
   probe and the local-shadow check; Typedef covers the final
   sym_find()/global_symbol_find() fallback; Finish covers the shared the_end
   finalization that every successful call runs; Tagdecl covers the
   struct/union/class/enum keywords, whose bodies (including every nested
   member declaration) are charged there; Typeid covers the
   _Atomic/typeof/decltype specifiers.  The depth check keeps nested
   parse_btype() calls inside the enclosing phase. */
enum
{
  PROFILE_BTYPE_PREFIX,
  PROFILE_BTYPE_BASE,
  PROFILE_BTYPE_TEMPLATE,
  PROFILE_BTYPE_TAG,
  PROFILE_BTYPE_TYPEDEF,
  PROFILE_BTYPE_FINISH,
  PROFILE_BTYPE_TAGDECL,
  PROFILE_BTYPE_TYPEID,
  PROFILE_BTYPE_COUNT
};
static const char *const profile_btype_names[PROFILE_BTYPE_COUNT] =
{
  "btype_prefix",
  "btype_base",
  "btype_template",
  "btype_tag",
  "btype_typedef",
  "btype_finish",
  "btype_tagdecl",
  "btype_typeid"
};
static unsigned long long profile_btype_ns[PROFILE_BTYPE_COUNT];
static unsigned long long profile_btype_calls[PROFILE_BTYPE_COUNT];
static int profile_btype_depth;
static int profile_btype_phase;
static unsigned long long profile_btype_started;
static void profile_tagdecl_member_btype_begin(void);
static void profile_tagdecl_member_btype_switch(int phase);
static void profile_tagdecl_member_btype_end(void);

static void profile_btype_begin(void)
{
  if (profile_btype_depth++ != 0)
    return;
  profile_btype_phase = PROFILE_BTYPE_PREFIX;
  profile_btype_started = profile_detail_now_ns();
  ++profile_btype_calls[PROFILE_BTYPE_PREFIX];
}

static void profile_btype_switch(int phase)
{
  unsigned long long now;

  if (profile_btype_depth != 1 || profile_btype_phase == phase)
    return;
  now = profile_detail_now_ns();
  profile_btype_ns[profile_btype_phase] += now - profile_btype_started;
  profile_btype_phase = phase;
  profile_btype_started = now;
  ++profile_btype_calls[phase];
}

static void profile_btype_end(void)
{
  unsigned long long now;

  if (profile_btype_depth == 1)
  {
    now = profile_detail_now_ns();
    profile_btype_ns[profile_btype_phase] += now - profile_btype_started;
  }
  --profile_btype_depth;
}

#define PROFILE_BTYPE_BEGIN() \
  do { if (profile_detail_enabled) { \
    profile_btype_begin(); \
    profile_tagdecl_member_btype_begin(); \
  } } while (0)
#define PROFILE_BTYPE_PHASE(phase) \
  do { if (profile_detail_enabled) { \
    profile_btype_switch(phase); \
    profile_tagdecl_member_btype_switch(phase); \
  } } while (0)
#define PROFILE_BTYPE_END() \
  do { if (profile_detail_enabled) { \
    profile_tagdecl_member_btype_end(); \
    profile_btype_end(); \
  } } while (0)
#define PROFILE_BTYPE_RETURN(value) \
  do { PROFILE_BTYPE_END(); return (value); } while (0)

/* Exclusive split of the outermost struct_decl() call.  Header covers tag
   and owner resolution before the body, Members covers the struct/union
   member loop, Enum covers the enum enumerator loop, Tail covers layout and
   registration after the body, and Other covers queued member-function
   definition work and the remaining epilogue.  The depth check keeps nested
   struct_decl() calls inside the enclosing phase. */
enum
{
  PROFILE_TAGDECL_HEADER,
  PROFILE_TAGDECL_MEMBERS,
  PROFILE_TAGDECL_ENUM,
  PROFILE_TAGDECL_TAIL,
  PROFILE_TAGDECL_OTHER,
  PROFILE_TAGDECL_COUNT
};
static const char *const profile_tagdecl_names[PROFILE_TAGDECL_COUNT] =
{
  "header",
  "members",
  "enum",
  "tail",
  "other"
};
static unsigned long long profile_tagdecl_total_ns;
static unsigned long long profile_tagdecl_total_calls;
static unsigned long long profile_tagdecl_ns[PROFILE_TAGDECL_COUNT];
static unsigned long long profile_tagdecl_calls[PROFILE_TAGDECL_COUNT];
static int profile_tagdecl_depth;
static int profile_tagdecl_phase;
static unsigned long long profile_tagdecl_started;
static unsigned long long profile_tagdecl_phase_started;

static void profile_tagdecl_member_decl_pause(void);
static void profile_tagdecl_member_decl_resume(void);

/* Exclusive split of the layout/registration tail in struct_decl_impl(). */
enum
{
  PROFILE_TAGDECL_TAIL_PREFIX,
  PROFILE_TAGDECL_TAIL_CHECK_FIELDS,
  PROFILE_TAGDECL_TAIL_LAYOUT,
  PROFILE_TAGDECL_TAIL_DEFERRED,
  PROFILE_TAGDECL_TAIL_LOCAL_CLASS,
  PROFILE_TAGDECL_TAIL_EMIT,
  PROFILE_TAGDECL_TAIL_OTHER,
  PROFILE_TAGDECL_TAIL_COUNT
};
static const char *const profile_tagdecl_tail_names[PROFILE_TAGDECL_TAIL_COUNT] =
{
  "prefix",
  "check_fields",
  "layout",
  "deferred",
  "local_class",
  "emit",
  "other"
};
static unsigned long long profile_tagdecl_tail_ns[PROFILE_TAGDECL_TAIL_COUNT];
static unsigned long long profile_tagdecl_tail_calls[PROFILE_TAGDECL_TAIL_COUNT];
static int profile_tagdecl_tail_started;
static int profile_tagdecl_tail_phase;
static unsigned long long profile_tagdecl_tail_phase_started;

static void profile_tagdecl_begin(void)
{
  if (profile_tagdecl_depth++ != 0)
  {
    profile_tagdecl_member_decl_pause();
    return;
  }
  profile_tagdecl_started = profile_detail_now_ns();
  profile_tagdecl_phase = PROFILE_TAGDECL_HEADER;
  profile_tagdecl_phase_started = profile_tagdecl_started;
  ++profile_tagdecl_total_calls;
  ++profile_tagdecl_calls[PROFILE_TAGDECL_HEADER];
}

static void profile_tagdecl_switch(int phase)
{
  unsigned long long now;

  if (profile_tagdecl_depth != 1 || profile_tagdecl_phase == phase)
    return;
  now = profile_detail_now_ns();
  profile_tagdecl_ns[profile_tagdecl_phase] +=
      now - profile_tagdecl_phase_started;
  profile_tagdecl_phase = phase;
  profile_tagdecl_phase_started = now;
  ++profile_tagdecl_calls[phase];
}

static void profile_tagdecl_end(void)
{
  unsigned long long now;

  if (profile_tagdecl_depth == 1)
  {
    now = profile_detail_now_ns();
    profile_tagdecl_ns[profile_tagdecl_phase] +=
        now - profile_tagdecl_phase_started;
    profile_tagdecl_total_ns += now - profile_tagdecl_started;
  }
  --profile_tagdecl_depth;
  if (profile_tagdecl_depth == 1)
    profile_tagdecl_member_decl_resume();
}

static void profile_tagdecl_tail_begin(void)
{
  if (profile_tagdecl_depth != 1
      || profile_tagdecl_phase != PROFILE_TAGDECL_TAIL)
    return;
  profile_tagdecl_tail_started = 1;
  profile_tagdecl_tail_phase = PROFILE_TAGDECL_TAIL_PREFIX;
  profile_tagdecl_tail_phase_started = profile_detail_now_ns();
  ++profile_tagdecl_tail_calls[PROFILE_TAGDECL_TAIL_PREFIX];
}

static void profile_tagdecl_tail_switch(int phase)
{
  unsigned long long now;

  if (!profile_tagdecl_tail_started || profile_tagdecl_depth != 1
      || profile_tagdecl_phase != PROFILE_TAGDECL_TAIL
      || profile_tagdecl_tail_phase == phase)
    return;
  now = profile_detail_now_ns();
  profile_tagdecl_tail_ns[profile_tagdecl_tail_phase] +=
      now - profile_tagdecl_tail_phase_started;
  profile_tagdecl_tail_phase = phase;
  profile_tagdecl_tail_phase_started = now;
  ++profile_tagdecl_tail_calls[phase];
}

static void profile_tagdecl_tail_end(void)
{
  unsigned long long now;

  if (!profile_tagdecl_tail_started || profile_tagdecl_depth != 1
      || profile_tagdecl_phase != PROFILE_TAGDECL_TAIL)
    return;
  now = profile_detail_now_ns();
  profile_tagdecl_tail_ns[profile_tagdecl_tail_phase] +=
      now - profile_tagdecl_tail_phase_started;
  profile_tagdecl_tail_started = 0;
}

#define PROFILE_TAGDECL_BEGIN() \
  do { if (profile_detail_enabled) profile_tagdecl_begin(); } while (0)
#define PROFILE_TAGDECL_PHASE(phase) \
  do { if (profile_detail_enabled) profile_tagdecl_switch(phase); } while (0)
#define PROFILE_TAGDECL_END() \
  do { if (profile_detail_enabled) profile_tagdecl_end(); } while (0)
#define PROFILE_TAGDECL_TAIL_BEGIN() \
  do { if (profile_detail_enabled) profile_tagdecl_tail_begin(); } while (0)
#define PROFILE_TAGDECL_TAIL_PHASE(phase) \
  do { if (profile_detail_enabled) profile_tagdecl_tail_switch(phase); } while (0)
#define PROFILE_TAGDECL_TAIL_END() \
  do { if (profile_detail_enabled) profile_tagdecl_tail_end(); } while (0)

/* Exclusive split of the non-enum member loop in struct_decl_impl().  Prefix
   covers line markers, access labels, template-member probes and the
   constructor/lifecycle lookahead before the member declaration is parsed.
   Declarator covers the declaration type and function/lifecycle handling.
   Initializer covers saved bodies and member initializers.  Field covers
   bit-field parsing and field registration.  Next covers the loop's
   skip/next/continue paths.  Only the outermost member loop opens a frame. */
enum
{
  PROFILE_TAGDECL_MEMBER_PREFIX,
  PROFILE_TAGDECL_MEMBER_DECLARATOR,
  PROFILE_TAGDECL_MEMBER_INITIALIZER,
  PROFILE_TAGDECL_MEMBER_FIELD,
  PROFILE_TAGDECL_MEMBER_NEXT,
  PROFILE_TAGDECL_MEMBER_COUNT
};
static const char *const profile_tagdecl_member_names[PROFILE_TAGDECL_MEMBER_COUNT] =
{
  "prefix",
  "declarator",
  "initializer",
  "field",
  "next"
};
static unsigned long long profile_tagdecl_member_ns[PROFILE_TAGDECL_MEMBER_COUNT];
static unsigned long long profile_tagdecl_member_calls[PROFILE_TAGDECL_MEMBER_COUNT];
static int profile_tagdecl_member_active = -1;
static unsigned long long profile_tagdecl_member_started;

/* Exclusive split of the member declarator region.  Prefix covers the C++
   friend/using/typedef/lifecycle lookahead before the type is parsed.  The
   remaining regions cover parse_btype(), type_decl() and the ordinary
   declarator loop, the C++ function and static-member branches, and the
   incomplete-type/type_size() checks before field registration. */
enum
{
  PROFILE_TAGDECL_MEMBER_DECL_PREFIX,
  PROFILE_TAGDECL_MEMBER_DECL_PARSE_BTYPE,
  PROFILE_TAGDECL_MEMBER_DECL_TYPE_DECL,
  PROFILE_TAGDECL_MEMBER_DECL_CPP_FUNCTION,
  PROFILE_TAGDECL_MEMBER_DECL_CPP_STATIC,
  PROFILE_TAGDECL_MEMBER_DECL_SIZE,
  PROFILE_TAGDECL_MEMBER_DECL_COUNT
};
static const char *const profile_tagdecl_member_decl_names[PROFILE_TAGDECL_MEMBER_DECL_COUNT] =
{
  "prefix",
  "parse_btype",
  "type_decl",
  "cpp_function",
  "cpp_static",
  "size"
};
static unsigned long long profile_tagdecl_member_decl_ns[PROFILE_TAGDECL_MEMBER_DECL_COUNT];
static unsigned long long profile_tagdecl_member_decl_calls[PROFILE_TAGDECL_MEMBER_DECL_COUNT];
static int profile_tagdecl_member_decl_active = -1;
static unsigned long long profile_tagdecl_member_decl_started;
static int profile_tagdecl_member_decl_paused;

static void profile_tagdecl_member_decl_pause(void)
{
  unsigned long long now;

  if (profile_tagdecl_member_decl_active < 0
      || profile_tagdecl_member_decl_paused)
    return;
  now = profile_detail_now_ns();
  profile_tagdecl_member_decl_ns[profile_tagdecl_member_decl_active] +=
      now - profile_tagdecl_member_decl_started;
  profile_tagdecl_member_decl_paused = 1;
}

static void profile_tagdecl_member_decl_resume(void)
{
  if (profile_tagdecl_member_decl_active < 0
      || !profile_tagdecl_member_decl_paused)
    return;
  profile_tagdecl_member_decl_started = profile_detail_now_ns();
  profile_tagdecl_member_decl_paused = 0;
}

static void profile_tagdecl_member_decl_close(void)
{
  unsigned long long now;

  if (profile_tagdecl_depth != 1
      || profile_tagdecl_phase != PROFILE_TAGDECL_MEMBERS
      || profile_tagdecl_member_active != PROFILE_TAGDECL_MEMBER_DECLARATOR
      || profile_tagdecl_member_decl_active < 0)
    return;
  now = profile_detail_now_ns();
  profile_tagdecl_member_decl_ns[profile_tagdecl_member_decl_active] +=
      now - profile_tagdecl_member_decl_started;
  profile_tagdecl_member_decl_active = -1;
  profile_tagdecl_member_decl_paused = 0;
}

static void profile_tagdecl_member_decl_switch(int region)
{
  if (profile_tagdecl_depth != 1
      || profile_tagdecl_phase != PROFILE_TAGDECL_MEMBERS
      || profile_tagdecl_member_active != PROFILE_TAGDECL_MEMBER_DECLARATOR
      || profile_tagdecl_member_decl_active == region)
    return;
  profile_tagdecl_member_decl_close();
  profile_tagdecl_member_decl_active = region;
  profile_tagdecl_member_decl_started = profile_detail_now_ns();
  ++profile_tagdecl_member_decl_calls[region];
}

static void profile_tagdecl_member_decl_end(void)
{
  profile_tagdecl_member_decl_close();
}

static void profile_tagdecl_member_close(void)
{
  unsigned long long now;

  if (profile_tagdecl_depth != 1
      || profile_tagdecl_phase != PROFILE_TAGDECL_MEMBERS
      || profile_tagdecl_member_active < 0)
    return;
  profile_tagdecl_member_decl_close();
  now = profile_detail_now_ns();
  profile_tagdecl_member_ns[profile_tagdecl_member_active] +=
      now - profile_tagdecl_member_started;
  profile_tagdecl_member_active = -1;
}

static void profile_tagdecl_member_switch(int region)
{
  if (profile_tagdecl_depth != 1
      || profile_tagdecl_phase != PROFILE_TAGDECL_MEMBERS
      || profile_tagdecl_member_active == region)
    return;
  profile_tagdecl_member_close();
  profile_tagdecl_member_active = region;
  profile_tagdecl_member_started = profile_detail_now_ns();
  ++profile_tagdecl_member_calls[region];
}

static void profile_tagdecl_member_end(void)
{
  profile_tagdecl_member_close();
}

#define PROFILE_TAGDECL_MEMBER_SEG(region) \
  do { if (profile_detail_enabled) profile_tagdecl_member_switch(region); } while (0)
#define PROFILE_TAGDECL_MEMBER_DECL_SEG(region) \
  do { if (profile_detail_enabled) profile_tagdecl_member_decl_switch(region); } while (0)
#define PROFILE_TAGDECL_MEMBER_DECL_END() \
  do { if (profile_detail_enabled) profile_tagdecl_member_decl_end(); } while (0)
#define PROFILE_TAGDECL_MEMBER_END() \
  do { if (profile_detail_enabled) profile_tagdecl_member_end(); } while (0)

/* Exclusive split of the member declarator's parse_btype() call.  Only the
   outermost member-restricted parse_btype() frame accumulates, so a nested
   tag body's own parse_btype() calls stay inside the enclosing tagdecl
   phase.  Prefix covers the basic/prefix specifier scan and type-id
   modifiers.  Tagdecl covers a struct/union/class/enum body.  Typedef covers
   class-name lookup and the typedef fallback; Finish covers the shared
   the_end finalization. */
enum
{
  PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX,
  PROFILE_TAGDECL_MEMBER_BTYPE_TAGDECL,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF,
  PROFILE_TAGDECL_MEMBER_BTYPE_FINISH,
  PROFILE_TAGDECL_MEMBER_BTYPE_COUNT
};
static const char *const profile_tagdecl_member_btype_names[PROFILE_TAGDECL_MEMBER_BTYPE_COUNT] =
{
  "prefix",
  "tagdecl",
  "typedef",
  "finish"
};
static unsigned long long profile_tagdecl_member_btype_ns[PROFILE_TAGDECL_MEMBER_BTYPE_COUNT];
static unsigned long long profile_tagdecl_member_btype_calls[PROFILE_TAGDECL_MEMBER_BTYPE_COUNT];
static int profile_tagdecl_member_btype_depth;
static int profile_tagdecl_member_btype_active = -1;
static int profile_tagdecl_member_btype_owner_depth = -1;
static int profile_tagdecl_member_btype_request;
static unsigned long long profile_tagdecl_member_btype_started;

/* Exclusive split of the member-restricted prefix region.  Probe covers the
   per-call preamble and the per-iteration C++-only keyword/scope probes that
   precede the specifier dispatch; Dispatch covers the switch (tok) specifier
   body.  The children are open only while the member frame owns the prefix
   region; the alias-template window keeps its existing TEMPLATE attribution,
   so probe + dispatch account for the prefix total except that window. */
enum
{
  PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_PROBE,
  PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_DISPATCH,
  PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_COUNT
};
static const char *const profile_tagdecl_member_btype_prefix_names[
    PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_COUNT] =
{
  "probe",
  "dispatch"
};
static unsigned long long profile_tagdecl_member_btype_prefix_ns[
    PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_COUNT];
static unsigned long long profile_tagdecl_member_btype_prefix_calls[
    PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_COUNT];
static int profile_tagdecl_member_btype_prefix_active = -1;
static unsigned long long profile_tagdecl_member_btype_prefix_started;

static void profile_tagdecl_member_btype_prefix_close(unsigned long long now)
{
  if (profile_tagdecl_member_btype_prefix_active < 0)
    return;
  profile_tagdecl_member_btype_prefix_ns[
      profile_tagdecl_member_btype_prefix_active] +=
      now - profile_tagdecl_member_btype_prefix_started;
  profile_tagdecl_member_btype_prefix_active = -1;
}

static void profile_tagdecl_member_btype_prefix_switch(int region)
{
  unsigned long long now;

  if (profile_tagdecl_member_btype_depth != profile_tagdecl_member_btype_owner_depth
      || profile_tagdecl_member_btype_active != PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX
      || profile_tagdecl_member_btype_prefix_active == region)
    return;
  now = profile_detail_now_ns();
  profile_tagdecl_member_btype_prefix_close(now);
  profile_tagdecl_member_btype_prefix_active = region;
  profile_tagdecl_member_btype_prefix_started = now;
  ++profile_tagdecl_member_btype_prefix_calls[region];
}

/* Exclusive split of the member-restricted typedef path.  Entry covers the
   initial cached-C or ordinary lookup, Lookup covers the scoped/global
   typedef fallback pair, and Accept covers the final VT_TYPEDEF test.  The
   successful path then keeps `next` as the inclusive container for the whole
   token advance, split into the exclusive advance, label and nested-typedef
   loop children. */
enum
{
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_ENTRY,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_LOOKUP,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_ACCEPT,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_NEXT,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_CONSTRUCT,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_ATTR,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_ADVANCE,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_LABEL,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_LOOP,
  PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_COUNT
};
static const char *const profile_tagdecl_member_btype_typedef_names[
    PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_COUNT] =
{
  "entry",
  "lookup",
  "accept",
  "next",
  "construct",
  "attr",
  "advance",
  "label",
  "loop"
};
static unsigned long long profile_tagdecl_member_btype_typedef_ns[
    PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_COUNT];
static unsigned long long profile_tagdecl_member_btype_typedef_calls[
    PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_COUNT];
static int profile_tagdecl_member_btype_typedef_active = -1;
static unsigned long long profile_tagdecl_member_btype_typedef_started;

static void profile_tagdecl_member_btype_typedef_close(unsigned long long now)
{
  if (profile_tagdecl_member_btype_typedef_active < 0)
    return;
  profile_tagdecl_member_btype_typedef_ns[
      profile_tagdecl_member_btype_typedef_active] +=
      now - profile_tagdecl_member_btype_typedef_started;
  profile_tagdecl_member_btype_typedef_active = -1;
}

static void profile_tagdecl_member_btype_typedef_switch(int region)
{
  unsigned long long now;

  if (profile_tagdecl_member_btype_depth != profile_tagdecl_member_btype_owner_depth
      || profile_tagdecl_member_btype_active != PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF
      || profile_tagdecl_member_btype_typedef_active == region)
    return;
  now = profile_detail_now_ns();
  profile_tagdecl_member_btype_typedef_close(now);
  profile_tagdecl_member_btype_typedef_active = region;
  profile_tagdecl_member_btype_typedef_started = now;
  ++profile_tagdecl_member_btype_typedef_calls[region];
}

static void profile_tagdecl_member_btype_typedef_end(void)
{
  if (profile_tagdecl_member_btype_typedef_active >= 0)
    profile_tagdecl_member_btype_typedef_close(profile_detail_now_ns());
}

static int profile_tagdecl_member_btype_region(int phase)
{
  switch (phase)
  {
  case PROFILE_BTYPE_PREFIX:
  case PROFILE_BTYPE_TYPEID:
    return PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX;
  case PROFILE_BTYPE_TAGDECL:
    return PROFILE_TAGDECL_MEMBER_BTYPE_TAGDECL;
  case PROFILE_BTYPE_FINISH:
    return PROFILE_TAGDECL_MEMBER_BTYPE_FINISH;
  default:
    return PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF;
  }
}

static void profile_tagdecl_member_btype_begin(void)
{
  ++profile_tagdecl_member_btype_depth;
  if (!profile_tagdecl_member_btype_request)
    return;
  profile_tagdecl_member_btype_request = 0;
  if (profile_tagdecl_member_decl_active != PROFILE_TAGDECL_MEMBER_DECL_PARSE_BTYPE)
    return;
  profile_tagdecl_member_btype_owner_depth = profile_tagdecl_member_btype_depth;
  profile_tagdecl_member_btype_active = PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX;
  profile_tagdecl_member_btype_started = profile_detail_now_ns();
  ++profile_tagdecl_member_btype_calls[PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX];
  profile_tagdecl_member_btype_prefix_active =
      PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_PROBE;
  profile_tagdecl_member_btype_prefix_started =
      profile_tagdecl_member_btype_started;
  ++profile_tagdecl_member_btype_prefix_calls[
      PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_PROBE];
}

static void profile_tagdecl_member_btype_switch(int phase)
{
  int region;
  unsigned long long now;

  if (profile_tagdecl_member_btype_depth != profile_tagdecl_member_btype_owner_depth
      || profile_tagdecl_member_btype_active < 0)
    return;
  region = profile_tagdecl_member_btype_region(phase);
  if (profile_tagdecl_member_btype_active == region)
    return;
  now = profile_detail_now_ns();
  if (profile_tagdecl_member_btype_active == PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX)
    profile_tagdecl_member_btype_prefix_close(now);
  if (profile_tagdecl_member_btype_active == PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF)
    profile_tagdecl_member_btype_typedef_close(now);
  profile_tagdecl_member_btype_ns[profile_tagdecl_member_btype_active] +=
      now - profile_tagdecl_member_btype_started;
  profile_tagdecl_member_btype_active = region;
  profile_tagdecl_member_btype_started = now;
  ++profile_tagdecl_member_btype_calls[region];
}

static void profile_tagdecl_member_btype_end(void)
{
  unsigned long long now;

  if (profile_tagdecl_member_btype_depth == profile_tagdecl_member_btype_owner_depth
      && profile_tagdecl_member_btype_active >= 0)
  {
    now = profile_detail_now_ns();
    profile_tagdecl_member_btype_prefix_close(now);
    profile_tagdecl_member_btype_typedef_close(now);
    profile_tagdecl_member_btype_ns[profile_tagdecl_member_btype_active] +=
        now - profile_tagdecl_member_btype_started;
    profile_tagdecl_member_btype_active = -1;
    profile_tagdecl_member_btype_owner_depth = -1;
  }
  --profile_tagdecl_member_btype_depth;
}

#define PROFILE_TAGDECL_MEMBER_DECL_PARSE_BTYPE_BEGIN() \
  do { if (profile_detail_enabled) { \
    profile_tagdecl_member_decl_switch(PROFILE_TAGDECL_MEMBER_DECL_PARSE_BTYPE); \
    profile_tagdecl_member_btype_request = 1; \
  } } while (0)
#define PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_SEG(region) \
  do { if (profile_detail_enabled) \
    profile_tagdecl_member_btype_typedef_switch(region); } while (0)
#define PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_SEG(region) \
  do { if (profile_detail_enabled) \
    profile_tagdecl_member_btype_prefix_switch(region); } while (0)

/* next_nomacro() runs once per token, so timing every call would cost more
   than the tokenizer.  Time one call in every PROFILE_DETAIL_LEXER_WINDOW and
   scale the sampled time by the total call count. */
#define PROFILE_DETAIL_LEXER_WINDOW 64

static void profile_lexer_begin(void)
{
  ++profile_detail_lexer_calls;
  if (!profile_detail_enabled)
    return;
  if ((profile_detail_lexer_calls & (PROFILE_DETAIL_LEXER_WINDOW - 1)) == 0)
  {
    ++profile_detail_lexer_sampled_calls;
    profile_detail_lexer_started = profile_detail_now_ns();
  }
}

static void profile_lexer_end(void)
{
  if (!profile_detail_enabled)
    return;
  if ((profile_detail_lexer_calls & (PROFILE_DETAIL_LEXER_WINDOW - 1)) == 0)
    profile_detail_lexer_ns += profile_detail_now_ns() - profile_detail_lexer_started;
}

/* Enter a nested definition.  Only the outermost application accumulates, so
   an inner call's elapsed time lands in the outer call's total instead of
   being measured twice. */
static int profile_function_begin(unsigned long long *started)
{
  ++profile_detail_function_depth;
  if (!profile_detail_enabled)
    return 0;
  ++profile_detail_function_calls;
  if (profile_detail_function_depth != 1)
    return 0;
  *started = profile_detail_now_ns();
  return 1;
}

static void profile_function_end(int outermost, unsigned long long started)
{
  if (outermost)
  {
    unsigned long long now = profile_detail_now_ns();
    unsigned long long elapsed = now - started;
    profile_detail_function_ns += elapsed;
    if (profile_tu_active >= 0 && profile_tu_active < PROFILE_TU_COUNT)
      profile_tu_function_ns[profile_tu_active] += elapsed;
    if (profile_detail_enabled)
    {
      int region;
      for (region = 0; region < PROFILE_TOP_COUNT; ++region)
        if (profile_top_depth[region])
          profile_top_function_ns[region] += elapsed;
    }
  }
  --profile_detail_function_depth;
}

/* Stage accounting is only meaningful for the outermost function.  A nested
   function is already included in its caller's current stage, so charging it
   again would double-count the same wall time. */
static int profile_stage_begin(unsigned long long *started)
{
  if (!profile_detail_enabled || profile_detail_function_depth != 1)
    return 0;
  *started = profile_detail_now_ns();
  return 1;
}

static void profile_stage_end(int active, unsigned long long started, int stage)
{
  if (active)
    profile_detail_stage_ns[stage] += profile_detail_now_ns() - started;
}

static int profile_fastopt_begin(unsigned long long *started)
{
  if (!profile_detail_enabled)
    return 0;
  *started = profile_detail_now_ns();
  return 1;
}

static void profile_fastopt_end(int active, unsigned long long started)
{
  if (active)
    profile_detail_fastopt_ns += profile_detail_now_ns() - started;
}

typedef struct ProfileFastoptStage
{
  int stage;
  unsigned long long started;
} ProfileFastoptStage;

/* The fast optimizer has several early exits.  The caller switches the
   current stage at each phase boundary and closes it once at the shared exit,
   so an early return is charged to the phase that was active. */
static void profile_fastopt_stage_begin(ProfileFastoptStage *timer, int stage)
{
  timer->stage = -1;
  timer->started = 0;
  if (!profile_detail_enabled)
    return;
  timer->stage = stage;
  timer->started = profile_detail_now_ns();
}

static void profile_fastopt_stage_next(ProfileFastoptStage *timer, int stage)
{
  unsigned long long now;
  if (!profile_detail_enabled)
    return;
  now = profile_detail_now_ns();
  if (timer->stage >= 0)
    profile_detail_fastopt_stage_ns[timer->stage] += now - timer->started;
  timer->stage = stage;
  timer->started = now;
}

static void profile_fastopt_stage_end(ProfileFastoptStage *timer)
{
  unsigned long long now;
  if (!profile_detail_enabled || timer->stage < 0)
    return;
  now = profile_detail_now_ns();
  profile_detail_fastopt_stage_ns[timer->stage] += now - timer->started;
  timer->stage = -1;
}

/* Split the outermost function's elapsed time without charging nested
   definitions twice.  A nested gen_function() sees depth > 1 and leaves the
   finer counters untouched; its time remains part of the enclosing stage. */
static int profile_function_stage_begin(unsigned long long *started)
{
  if (!profile_detail_enabled || profile_detail_function_depth != 1)
    return 0;
  *started = profile_detail_now_ns();
  return 1;
}

static void profile_function_stage_end(int active, unsigned long long started,
                                       unsigned long long *total)
{
  if (active)
    *total += profile_detail_now_ns() - started;
}

static void profile_body_stmt_end(void);

static void profile_body_block_begin(void)
{
  if (!profile_detail_enabled)
    return;
  ++profile_detail_body_depth;
  if (profile_detail_body_depth <= PROFILE_DETAIL_BODY_MAX_DEPTH)
  {
    ProfileDetailBodyFrame *frame =
        &profile_detail_body_frames[profile_detail_body_depth - 1];
    frame->block_started = profile_detail_now_ns();
    frame->nested_ns = 0;
    frame->statement_started = 0;
    frame->statement_nested_start = 0;
    frame->statement_category = 0;
    frame->statement_active = 0;
  }
}

static void profile_body_block_end(void)
{
  int depth;
  if (!profile_detail_enabled)
    return;
  depth = profile_detail_body_depth;
  if (depth <= 0)
    return;
  if (depth <= PROFILE_DETAIL_BODY_MAX_DEPTH
      && profile_detail_body_frames[depth - 1].statement_active)
    profile_body_stmt_end();
  --profile_detail_body_depth;
  if (depth <= PROFILE_DETAIL_BODY_MAX_DEPTH)
  {
    unsigned long long elapsed =
        profile_detail_now_ns() - profile_detail_body_frames[depth - 1].block_started;
    if (depth > 1 && depth - 1 <= PROFILE_DETAIL_BODY_MAX_DEPTH)
      profile_detail_body_frames[depth - 2].nested_ns += elapsed;
  }
}

static void profile_body_stmt_begin(int category)
{
  ProfileDetailBodyFrame *frame;
  int depth = profile_detail_body_depth;
  if (!profile_detail_enabled)
    return;
  if (depth <= 0 || depth > PROFILE_DETAIL_BODY_MAX_DEPTH)
    return;
  frame = &profile_detail_body_frames[depth - 1];
  if (frame->statement_active)
    profile_body_stmt_end();
  frame->statement_active = 1;
  frame->statement_category = category;
  frame->statement_started = profile_detail_now_ns();
  frame->statement_nested_start = frame->nested_ns;
}

static void profile_body_stmt_end(void)
{
  ProfileDetailBodyFrame *frame;
  unsigned long long elapsed, nested, own;
  int depth = profile_detail_body_depth;
  if (!profile_detail_enabled)
    return;
  if (depth <= 0 || depth > PROFILE_DETAIL_BODY_MAX_DEPTH)
    return;
  frame = &profile_detail_body_frames[depth - 1];
  if (!frame->statement_active)
    return;
  elapsed = profile_detail_now_ns() - frame->statement_started;
  nested = frame->nested_ns - frame->statement_nested_start;
  own = elapsed > nested ? elapsed - nested : 0;
  profile_detail_body_ns[frame->statement_category] += own;
  ++profile_detail_body_calls[frame->statement_category];
  frame->statement_active = 0;
}

#define PROFILE_BODY_BLOCK_BEGIN() \
  do { if (profile_detail_enabled) profile_body_block_begin(); } while (0)
#define PROFILE_BODY_BLOCK_END() \
  do { if (profile_detail_enabled) profile_body_block_end(); } while (0)
#define PROFILE_BODY_STMT_BEGIN(category) \
  do { if (profile_detail_enabled) profile_body_stmt_begin(category); } while (0)
#define PROFILE_BODY_STMT_END() \
  do { if (profile_detail_enabled) profile_body_stmt_end(); } while (0)
#define PROFILE_EXTRA_TIME_BEGIN(name) \
  unsigned long long profile_extra_##name##_started = \
      profile_detail_enabled ? profile_detail_now_ns() : 0
#define PROFILE_EXTRA_TIME_END(name, slot) \
  do { if (profile_detail_enabled) \
    profile_detail_extra_ns[slot] += \
        profile_detail_now_ns() - profile_extra_##name##_started; } while (0)
#define PROFILE_EXTRA_CALL(slot) \
  do { if (profile_detail_enabled) ++profile_detail_extra_calls[slot]; } while (0)

/* Inclusive timers for mutually recursive expression layers.  Only the
   outermost active call of a layer accumulates, so a recursive call is
   counted once as part of its caller.  The depth and start state live in the
   profiling arrays rather than in a function local, so an ordinary compile
   keeps no extra live value in these hot functions. */
static void profile_detail_extra_scope_begin(int slot)
{
  if (profile_detail_extra_depth[slot]++ == 0) {
    profile_detail_extra_started[slot] = profile_detail_now_ns();
    ++profile_detail_extra_calls[slot];
  }
}

static void profile_detail_extra_scope_end(int slot)
{
  if (--profile_detail_extra_depth[slot] == 0)
    profile_detail_extra_ns[slot] +=
        profile_detail_now_ns() - profile_detail_extra_started[slot];
}

#define PROFILE_EXTRA_SCOPE_BEGIN(slot) \
  do { if (profile_detail_enabled) \
    profile_detail_extra_scope_begin(slot); } while (0)
#define PROFILE_EXTRA_SCOPE_END(slot) \
  do { if (profile_detail_enabled) \
    profile_detail_extra_scope_end(slot); } while (0)

/* Split the outermost unary() call into its prefix/switch body and its postfix
   loop.  Nested unary() calls are charged to whichever outer phase invoked
   them, so the two subphase totals partition the unary total. */
static void profile_detail_unary_begin(void)
{
  if (profile_detail_unary_depth++ != 0)
    return;
  profile_detail_unary_started = profile_detail_now_ns();
  profile_detail_unary_phase_started = profile_detail_unary_started;
  profile_detail_unary_post_phase = 0;
  ++profile_detail_extra_calls[PROFILE_DETAIL_EXTRA_UNARY];
}

static void profile_detail_unary_enter_post(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth != 1)
    return;
  now = profile_detail_now_ns();
  profile_detail_extra_ns[PROFILE_DETAIL_EXTRA_UNARY_PREFIX] +=
      now - profile_detail_unary_phase_started;
  ++profile_detail_extra_calls[PROFILE_DETAIL_EXTRA_UNARY_PREFIX];
  profile_detail_unary_post_phase = 1;
  profile_detail_unary_phase_started = now;
}

static void profile_detail_unary_end(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth == 1)
  {
    now = profile_detail_now_ns();
    if (profile_detail_unary_post_phase)
    {
      profile_detail_extra_ns[PROFILE_DETAIL_EXTRA_UNARY_POST] +=
          now - profile_detail_unary_phase_started;
      ++profile_detail_extra_calls[PROFILE_DETAIL_EXTRA_UNARY_POST];
    }
    else
    {
      profile_detail_extra_ns[PROFILE_DETAIL_EXTRA_UNARY_PREFIX] +=
          now - profile_detail_unary_phase_started;
      ++profile_detail_extra_calls[PROFILE_DETAIL_EXTRA_UNARY_PREFIX];
    }
    profile_detail_extra_ns[PROFILE_DETAIL_EXTRA_UNARY] +=
        now - profile_detail_unary_started;
  }
  --profile_detail_unary_depth;
}

#define PROFILE_UNARY_BEGIN() \
  do { if (profile_detail_enabled) profile_detail_unary_begin(); } while (0)
#define PROFILE_UNARY_ENTER_POST() \
  do { if (profile_detail_enabled) profile_detail_unary_enter_post(); } while (0)
#define PROFILE_UNARY_END() \
  do { if (profile_detail_enabled) profile_detail_unary_end(); } while (0)

/* Exclusive split of the outermost unary() post loop into the four branch
   kinds it dispatches.  One frame is active at a time.  A branch opens the
   frame when it is selected and the top of the loop closes it, which also
   covers every continue path; the close after the loop covers the last
   branch.  Only the outermost unary() opens a frame, so a nested unary()
   stays inside the enclosing branch's total and the four totals partition
   unary_post_ms. */
enum
{
  PROFILE_DETAIL_POST_INC,
  PROFILE_DETAIL_POST_MEMBER,
  PROFILE_DETAIL_POST_SUBSCRIPT,
  PROFILE_DETAIL_POST_CALL,
  PROFILE_DETAIL_POST_COUNT
};
static const char *const profile_detail_post_names[PROFILE_DETAIL_POST_COUNT] =
{
  "post_inc",
  "post_member",
  "post_subscript",
  "post_call"
};
static unsigned long long profile_detail_post_ns[PROFILE_DETAIL_POST_COUNT];
static unsigned long long profile_detail_post_calls[PROFILE_DETAIL_POST_COUNT];
static int profile_detail_post_active = -1;
static unsigned long long profile_detail_post_started;

static void profile_detail_post_begin(int branch)
{
  if (profile_detail_unary_depth != 1)
    return;
  profile_detail_post_active = branch;
  profile_detail_post_started = profile_detail_now_ns();
  ++profile_detail_post_calls[branch];
}

static void profile_detail_post_close(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth != 1 || profile_detail_post_active < 0)
    return;
  now = profile_detail_now_ns();
  profile_detail_post_ns[profile_detail_post_active] +=
      now - profile_detail_post_started;
  profile_detail_post_active = -1;
}

#define PROFILE_POST_BEGIN(branch) \
  do { if (profile_detail_enabled) profile_detail_post_begin(branch); } while (0)
#define PROFILE_POST_CLOSE() \
  do { if (profile_detail_enabled) profile_detail_post_close(); } while (0)

/* Exclusive subregions of the `.`/`->` member arm of the post loop.  The
   arm opens in dispatch, which covers the arrow chain, the receiver
   qualifiers and the member name.  It then names the region that owns the
   rest of the arm: the C++ structural probes (member templates, static
   members, destructor and conversion spellings), the C field walk with its
   offset arithmetic, the member-call resolution and emission, or the
   qualifier/address and value/materialisation half of a data member.  One
   frame is active at a time and the top of the post loop closes it, so every
   continue path is covered and the rows partition post_member_ms apart from
   the arm's own boundary overhead. */
enum
{
  PROFILE_DETAIL_MEMBER_DISPATCH,
  PROFILE_DETAIL_MEMBER_PROBE,
  PROFILE_DETAIL_MEMBER_CALL,
  PROFILE_DETAIL_MEMBER_FIELD,
  PROFILE_DETAIL_MEMBER_ADDRESS,
  PROFILE_DETAIL_MEMBER_EMIT,
  PROFILE_DETAIL_MEMBER_COUNT
};
static const char *const profile_detail_member_names[PROFILE_DETAIL_MEMBER_COUNT] =
{
  "member_dispatch",
  "member_probe",
  "member_call",
  "member_field",
  "member_address",
  "member_emit"
};
static unsigned long long profile_detail_member_ns[PROFILE_DETAIL_MEMBER_COUNT];
static unsigned long long profile_detail_member_calls[PROFILE_DETAIL_MEMBER_COUNT];
static int profile_detail_member_active = -1;
static unsigned long long profile_detail_member_started;

static void profile_detail_member_close(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth != 1 || profile_detail_member_active < 0)
    return;
  now = profile_detail_now_ns();
  profile_detail_member_ns[profile_detail_member_active] +=
      now - profile_detail_member_started;
  profile_detail_member_active = -1;
}

static void profile_detail_member_begin(int region)
{
  if (profile_detail_unary_depth != 1)
    return;
  profile_detail_member_close();
  profile_detail_member_active = region;
  profile_detail_member_started = profile_detail_now_ns();
  ++profile_detail_member_calls[region];
}

#define PROFILE_MEMBER_SEG(region) \
  do { if (profile_detail_enabled) profile_detail_member_begin(region); } while (0)
#define PROFILE_MEMBER_CLOSE() \
  do { if (profile_detail_enabled) profile_detail_member_close(); } while (0)

/* Exclusive subregions of the literal arms of the expression switch, inside
   the prefix_literal region.  One frame starts when a literal arm selects
   its bucket: character and integer constants, floating and complex
   constants, string allocation/relocation, and the remaining literal
   spellings (the keyword literals and the initializer object).  A frame
   stays active until the prefix frame closes it, so each row covers the arm
   body plus its immediate epilogue.  Arms that are not literals are charged
   to their own prefix region, so the difference between prefix_literal_ms
   and the sum of these rows is the switch dispatch, the non-literal arms
   that still share the literal region, and the profiling boundaries. */
enum
{
  PROFILE_DETAIL_LIT_INTEGER,
  PROFILE_DETAIL_LIT_FLOAT,
  PROFILE_DETAIL_LIT_STRING,
  PROFILE_DETAIL_LIT_SPECIAL,
  PROFILE_DETAIL_LIT_OTHER,
  PROFILE_DETAIL_LIT_COUNT
};
static const char *const profile_detail_lit_names[PROFILE_DETAIL_LIT_COUNT] =
{
  "lit_integer",
  "lit_float",
  "lit_string",
  "lit_special",
  "lit_other"
};
static unsigned long long profile_detail_lit_ns[PROFILE_DETAIL_LIT_COUNT];
static unsigned long long profile_detail_lit_calls[PROFILE_DETAIL_LIT_COUNT];
static int profile_detail_lit_active = -1;
static unsigned long long profile_detail_lit_started;

static void profile_detail_lit_close(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth != 1 || profile_detail_lit_active < 0)
    return;
  now = profile_detail_now_ns();
  profile_detail_lit_ns[profile_detail_lit_active] +=
      now - profile_detail_lit_started;
  profile_detail_lit_active = -1;
}

static void profile_detail_lit_begin(int region)
{
  if (profile_detail_unary_depth != 1)
    return;
  profile_detail_lit_close();
  profile_detail_lit_active = region;
  profile_detail_lit_started = profile_detail_now_ns();
  ++profile_detail_lit_calls[region];
}

#define PROFILE_LIT_SEG(region) \
  do { if (profile_detail_enabled) profile_detail_lit_begin(region); } while (0)

/* Exclusive regions of the outermost unary() prefix.  The dispatch region
   covers tok_next re-dispatch and the C++-only probes before the switch.
   The switch body starts as primary, while the type/cast/sizeof and
   identifier arms override that boundary explicitly.  The frame closes at
   unary_post or the early return, so the regions partition unary_prefix_ms. */
enum
{
  PROFILE_DETAIL_PREFIX_DISPATCH,
  PROFILE_DETAIL_PREFIX_LITERAL,
  PROFILE_DETAIL_PREFIX_CAST,
  PROFILE_DETAIL_PREFIX_PAREN,
  PROFILE_DETAIL_PREFIX_SIZEOF,
  PROFILE_DETAIL_PREFIX_TYPE_QUERY,
  PROFILE_DETAIL_PREFIX_OPERATOR,
  PROFILE_DETAIL_PREFIX_BUILTIN,
  PROFILE_DETAIL_PREFIX_IDENTIFIER,
  PROFILE_DETAIL_PREFIX_COUNT
};
static const char *const profile_detail_prefix_names[PROFILE_DETAIL_PREFIX_COUNT] =
{
  "prefix_dispatch",
  "prefix_literal",
  "prefix_cast",
  "prefix_paren",
  "prefix_sizeof",
  "prefix_type_query",
  "prefix_operator",
  "prefix_builtin",
  "prefix_identifier"
};
static unsigned long long profile_detail_prefix_ns[PROFILE_DETAIL_PREFIX_COUNT];
static unsigned long long profile_detail_prefix_calls[PROFILE_DETAIL_PREFIX_COUNT];
static int profile_detail_prefix_active = -1;
static unsigned long long profile_detail_prefix_started;

static void profile_detail_prefix_close(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth != 1)
    return;
  profile_detail_lit_close();
  if (profile_detail_prefix_active < 0)
    return;
  now = profile_detail_now_ns();
  profile_detail_prefix_ns[profile_detail_prefix_active] +=
      now - profile_detail_prefix_started;
  profile_detail_prefix_active = -1;
}

static void profile_detail_prefix_begin(int region)
{
  if (profile_detail_unary_depth != 1)
    return;
  profile_detail_prefix_close();
  profile_detail_prefix_active = region;
  profile_detail_prefix_started = profile_detail_now_ns();
  ++profile_detail_prefix_calls[region];
}

#define PROFILE_PREFIX_SEG(region) \
  do { if (profile_detail_enabled) profile_detail_prefix_begin(region); } while (0)
#define PROFILE_PREFIX_CLOSE() \
  do { if (profile_detail_enabled) profile_detail_prefix_close(); } while (0)

/* Exclusive subregions of the parenthesized prefix arm.  The frame starts
   before the speculative parse_btype() probe and moves to the region selected
   by the result.  It closes before unary_post or the early sizeof/alignof
   return, so the rows partition prefix_paren_ms. */
enum
{
  PROFILE_DETAIL_PAREN_PROBE,
  PROFILE_DETAIL_PAREN_DISAMBIG,
  PROFILE_DETAIL_PAREN_FALLBACK,
  PROFILE_DETAIL_PAREN_TYPE_COMPLETE,
  PROFILE_DETAIL_PAREN_COMPOUND,
  PROFILE_DETAIL_PAREN_STMT_EXPR,
  PROFILE_DETAIL_PAREN_COUNT
};
static const char *const profile_detail_paren_names[PROFILE_DETAIL_PAREN_COUNT] =
{
  "paren_probe",
  "paren_disambig",
  "paren_fallback",
  "paren_type_complete",
  "paren_compound",
  "paren_stmt_expr"
};
static unsigned long long profile_detail_paren_ns[PROFILE_DETAIL_PAREN_COUNT];
static unsigned long long profile_detail_paren_calls[PROFILE_DETAIL_PAREN_COUNT];
static unsigned long long profile_detail_paren_probe_skipped;
static unsigned long long profile_detail_paren_probe_ident_visible;
static unsigned long long profile_detail_paren_probe_ident_absent;
static unsigned long long profile_detail_paren_probe_keyword;
static unsigned long long profile_detail_paren_probe_other;
static int profile_detail_paren_active = -1;
static unsigned long long profile_detail_paren_started;

static void profile_detail_paren_close(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth != 1 || profile_detail_paren_active < 0)
    return;
  now = profile_detail_now_ns();
  profile_detail_paren_ns[profile_detail_paren_active] +=
      now - profile_detail_paren_started;
  profile_detail_paren_active = -1;
}

static void profile_detail_paren_begin(int region)
{
  if (profile_detail_unary_depth != 1)
    return;
  profile_detail_paren_close();
  profile_detail_paren_active = region;
  profile_detail_paren_started = profile_detail_now_ns();
  ++profile_detail_paren_calls[region];
}

#define PROFILE_PAREN_SEG(region) \
  do { if (profile_detail_enabled) profile_detail_paren_begin(region); } while (0)
#define PROFILE_PAREN_CLOSE() \
  do { if (profile_detail_enabled) profile_detail_paren_close(); } while (0)
/* Counts '(' prefixes whose tentative type probe is skipped because the
   token after it cannot begin a C type specifier. */
#define PROFILE_PAREN_PROBE_SKIPPED() \
  do { if (profile_detail_enabled) ++profile_detail_paren_probe_skipped; } while (0)

/* Exclusive segments of the call branch.  unary_post_ms alone says the call
   branch is the largest, but not whether that time is call overhead or the
   argument expressions it contains.  prelude, resolve, args and emit bracket
   the branch; arg_expr and arg_conv then split the argument loop so nested
   parsing is visible apart from the conversion work.  Each boundary closes
   the previous segment, so the rows partition the call branch minus the loop
   dispatch.  The per-argument boundaries cost one timestamp pair each, so
   treat the rows as ratios rather than absolute times. */
enum
{
  PROFILE_DETAIL_CALL_PRELUDE,
  PROFILE_DETAIL_CALL_RESOLVE,
  PROFILE_DETAIL_CALL_ARGS,
  PROFILE_DETAIL_CALL_EMIT,
  PROFILE_DETAIL_CALL_ARGEXPR,
  PROFILE_DETAIL_CALL_ARGCONV,
  PROFILE_DETAIL_CALL_SEGMENT_COUNT
};
static const char *const profile_detail_call_seg_names[PROFILE_DETAIL_CALL_SEGMENT_COUNT] =
{
  "call_prelude",
  "call_resolve",
  "call_args",
  "call_emit",
  "call_arg_expr",
  "call_arg_conv"
};
static unsigned long long profile_detail_call_seg_ns[PROFILE_DETAIL_CALL_SEGMENT_COUNT];
static unsigned long long profile_detail_call_seg_calls[PROFILE_DETAIL_CALL_SEGMENT_COUNT];
static int profile_detail_call_seg_active = -1;
static unsigned long long profile_detail_call_seg_started;

static void profile_detail_call_seg_close(void)
{
  unsigned long long now;
  if (profile_detail_unary_depth != 1 || profile_detail_call_seg_active < 0)
    return;
  now = profile_detail_now_ns();
  profile_detail_call_seg_ns[profile_detail_call_seg_active] +=
      now - profile_detail_call_seg_started;
  profile_detail_call_seg_active = -1;
}

static void profile_detail_call_seg_begin(int segment)
{
  if (profile_detail_unary_depth != 1)
    return;
  profile_detail_call_seg_close();
  profile_detail_call_seg_active = segment;
  profile_detail_call_seg_started = profile_detail_now_ns();
  ++profile_detail_call_seg_calls[segment];
}

#define PROFILE_CALL_SEG(segment) \
  do { if (profile_detail_enabled) profile_detail_call_seg_begin(segment); } while (0)

static void profile_detail_report(void)
{
  int category;
  unsigned long long accounted_ns;
  unsigned long long other_ns;
  if (!profile_detail_enabled)
    return;
  if (profile_detail_lexer_sampled_calls)
    profile_detail_lexer_ns = profile_detail_lexer_ns
                              * profile_detail_lexer_calls
                              / profile_detail_lexer_sampled_calls;
  accounted_ns = profile_detail_function_setup_ns
                 + profile_detail_function_prolog_ns
                 + profile_detail_function_body_ns
                 + profile_detail_function_epilog_ns
                 + profile_detail_function_pending_ns;
  other_ns = profile_detail_function_ns > accounted_ns
             ? profile_detail_function_ns - accounted_ns : 0;
  fprintf(stderr,
          "CPC_PROFILE_DETAIL lexer_calls=%llu lexer_ms=%.3f "
          "function_calls=%llu function_ms=%.3f "
          "entry_ms=%.3f prolog_ms=%.3f body_ms=%.3f epilog_ms=%.3f "
          "fastopt_ms=%.3f "
          "fastopt_decode_ms=%.3f fastopt_promote_ms=%.3f "
          "fastopt_rewrite_ms=%.3f fastopt_relocate_ms=%.3f "
          "fastopt_commit_ms=%.3f "
          "function_setup_ms=%.3f function_prolog_ms=%.3f "
          "function_body_ms=%.3f function_epilog_ms=%.3f "
          "function_pending_ms=%.3f function_other_ms=%.3f\n",
          profile_detail_lexer_calls, profile_detail_lexer_ns / 1000000.0,
          profile_detail_function_calls, profile_detail_function_ns / 1000000.0,
          profile_detail_stage_ns[PROFILE_DETAIL_STAGE_ENTRY] / 1000000.0,
          profile_detail_stage_ns[PROFILE_DETAIL_STAGE_PROLOG] / 1000000.0,
          profile_detail_stage_ns[PROFILE_DETAIL_STAGE_BODY] / 1000000.0,
          profile_detail_stage_ns[PROFILE_DETAIL_STAGE_EPILOG] / 1000000.0,
          profile_detail_fastopt_ns / 1000000.0,
          profile_detail_fastopt_stage_ns[PROFILE_FASTOPT_DECODE] / 1000000.0,
          profile_detail_fastopt_stage_ns[PROFILE_FASTOPT_PROMOTE] / 1000000.0,
          profile_detail_fastopt_stage_ns[PROFILE_FASTOPT_REWRITE] / 1000000.0,
          profile_detail_fastopt_stage_ns[PROFILE_FASTOPT_RELOCATE] / 1000000.0,
          profile_detail_fastopt_stage_ns[PROFILE_FASTOPT_COMMIT] / 1000000.0,
          profile_detail_function_setup_ns / 1000000.0,
          profile_detail_function_prolog_ns / 1000000.0,
          profile_detail_function_body_ns / 1000000.0,
          profile_detail_function_epilog_ns / 1000000.0,
          profile_detail_function_pending_ns / 1000000.0,
          other_ns / 1000000.0);
  for (category = 0; category < PROFILE_DETAIL_BODY_CATEGORY_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_STMT %s_ms=%.3f calls=%llu\n",
            profile_detail_body_names[category],
            profile_detail_body_ns[category] / 1000000.0,
            profile_detail_body_calls[category]);
  for (category = 0; category < PROFILE_DETAIL_EXTRA_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_EXTRA %s_ms=%.3f calls=%llu\n",
            profile_detail_extra_names[category],
            profile_detail_extra_ns[category] / 1000000.0,
            profile_detail_extra_calls[category]);
  for (category = 0; category < PROFILE_DETAIL_POST_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_POST %s_ms=%.3f calls=%llu\n",
            profile_detail_post_names[category],
            profile_detail_post_ns[category] / 1000000.0,
            profile_detail_post_calls[category]);
  for (category = 0; category < PROFILE_DETAIL_MEMBER_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_MEMBER %s_ms=%.3f calls=%llu\n",
            profile_detail_member_names[category],
            profile_detail_member_ns[category] / 1000000.0,
            profile_detail_member_calls[category]);
  for (category = 0; category < PROFILE_DETAIL_PREFIX_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_PREFIX %s_ms=%.3f calls=%llu\n",
            profile_detail_prefix_names[category],
            profile_detail_prefix_ns[category] / 1000000.0,
            profile_detail_prefix_calls[category]);
  for (category = 0; category < PROFILE_DETAIL_LIT_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_LIT %s_ms=%.3f calls=%llu\n",
            profile_detail_lit_names[category],
            profile_detail_lit_ns[category] / 1000000.0,
            profile_detail_lit_calls[category]);
  for (category = 0; category < PROFILE_DETAIL_PAREN_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_PAREN %s_ms=%.3f calls=%llu\n",
            profile_detail_paren_names[category],
            profile_detail_paren_ns[category] / 1000000.0,
            profile_detail_paren_calls[category]);
  fprintf(stderr, "CPC_PROFILE_PAREN probe_skipped=%llu\n",
          profile_detail_paren_probe_skipped);
  fprintf(stderr, "CPC_PROFILE_PAREN probe_ident_visible=%llu\n",
          profile_detail_paren_probe_ident_visible);
  fprintf(stderr, "CPC_PROFILE_PAREN probe_ident_absent=%llu\n",
          profile_detail_paren_probe_ident_absent);
  fprintf(stderr, "CPC_PROFILE_PAREN probe_keyword=%llu\n",
          profile_detail_paren_probe_keyword);
  fprintf(stderr, "CPC_PROFILE_PAREN probe_other=%llu\n",
          profile_detail_paren_probe_other);
  for (category = 0; category < PROFILE_DETAIL_CALL_SEGMENT_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_CALLSEG %s_ms=%.3f calls=%llu\n",
            profile_detail_call_seg_names[category],
            profile_detail_call_seg_ns[category] / 1000000.0,
            profile_detail_call_seg_calls[category]);
  fflush(stderr);
}

static void profile_top_report(void)
{
  int region;
  int category;
  unsigned long long pipeline_ns = 0;
  unsigned long long frontend_ns;
  unsigned long long frontend_function_ns;
  unsigned long long frontend_other_ns;

  if (!profile_detail_enabled)
    return;
  profile_tu_end();
  if (profile_alloc_unknown_save_samples)
  {
    profile_decl_tail_ns[PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT] =
        profile_alloc_unknown_save_next_ns
        * profile_alloc_unknown_save_tokens
        / profile_alloc_unknown_save_samples;
    profile_decl_tail_ns[
        PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_LEXER] =
        profile_alloc_unknown_save_next_part_ns[
            PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_LEXER]
        * profile_alloc_unknown_save_tokens
        / profile_alloc_unknown_save_samples;
    profile_decl_tail_ns[
        PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_MACRO] =
        profile_alloc_unknown_save_next_part_ns[
            PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_MACRO]
        * profile_alloc_unknown_save_tokens
        / profile_alloc_unknown_save_samples;
    profile_decl_tail_ns[
        PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_SUBST] =
    profile_alloc_unknown_save_next_part_ns[
        PROFILE_ALLOC_UNKNOWN_SAVE_NEXT_SUBST]
        * profile_alloc_unknown_save_tokens
        / profile_alloc_unknown_save_samples;
  for (category = 0; category < PROFILE_MACRO_LOOP_PART_COUNT; ++category)
  {
    fprintf(stderr,
            "CPC_PROFILE_MACRO %s_ms=%.3f calls=%llu\n",
              profile_macro_loop_names[category],
              profile_macro_loop_part_ns[category]
                  * profile_alloc_unknown_save_tokens
                  / (profile_alloc_unknown_save_samples * 1000000.0),
              profile_macro_loop_part_calls[category]);
  }
  for (category = 0; category < PROFILE_MACRO_NONVALUE_CLASS_COUNT; ++category)
    fprintf(stderr, "CPC_PROFILE_MACRO %s=%llu\n",
            profile_macro_nonvalue_names[category],
            profile_macro_nonvalue_calls[category]);
    profile_decl_tail_ns[PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND] =
        profile_alloc_unknown_save_append_ns
        * profile_alloc_unknown_save_tokens
        / profile_alloc_unknown_save_samples;
    profile_decl_tail_ns[
        PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND_GROWTH] =
        profile_alloc_unknown_save_append_growth_ns
        * profile_alloc_unknown_save_tokens
        / profile_alloc_unknown_save_samples;
    profile_decl_tail_ns[
        PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND_COPY] =
        (profile_alloc_unknown_save_append_ns
             > profile_alloc_unknown_save_append_growth_ns
         ? profile_alloc_unknown_save_append_ns
               - profile_alloc_unknown_save_append_growth_ns
         : 0)
        * profile_alloc_unknown_save_tokens
        / profile_alloc_unknown_save_samples;
    fprintf(stderr, "CPC_PROFILE_MACRO append_growth_calls=%llu\n",
            profile_alloc_unknown_save_append_growth_calls);
  }
  profile_decl_tail_calls[PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT] =
      profile_alloc_unknown_save_tokens;
  profile_decl_tail_calls[
      PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_LEXER] =
      profile_alloc_unknown_save_tokens;
  profile_decl_tail_calls[
      PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_MACRO] =
      profile_alloc_unknown_save_tokens;
  profile_decl_tail_calls[
      PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_NEXT_SUBST] =
      profile_alloc_unknown_save_tokens;
  profile_decl_tail_calls[PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND] =
      profile_alloc_unknown_save_tokens;
  profile_decl_tail_calls[
      PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND_GROWTH] =
      profile_alloc_unknown_save_tokens;
  profile_decl_tail_calls[
      PROFILE_DECL_TAIL_ALLOC_SIZE_UNKNOWN_SAVE_APPEND_COPY] =
      profile_alloc_unknown_save_tokens;
  for (region = 0; region < PROFILE_TU_COUNT; ++region)
    pipeline_ns += profile_tu_ns[region];
  frontend_ns = profile_tu_ns[PROFILE_TU_FRONTEND_SETUP]
                + profile_tu_ns[PROFILE_TU_TOP_DECL]
                + profile_tu_ns[PROFILE_TU_DEFERRED];
  frontend_function_ns = profile_tu_function_ns[PROFILE_TU_FRONTEND_SETUP]
                         + profile_tu_function_ns[PROFILE_TU_TOP_DECL]
                         + profile_tu_function_ns[PROFILE_TU_DEFERRED];
  frontend_other_ns = frontend_ns > frontend_function_ns
                      ? frontend_ns - frontend_function_ns : 0;
  fprintf(stderr,
          "CPC_PROFILE_TOP pipeline_ms=%.3f frontend_ms=%.3f "
          "frontend_function_ms=%.3f frontend_other_ms=%.3f\n",
          pipeline_ns / 1000000.0,
          frontend_ns / 1000000.0,
          frontend_function_ns / 1000000.0,
          frontend_other_ns / 1000000.0);
  for (region = 0; region < PROFILE_TU_COUNT; ++region)
  {
    unsigned long long other_ns = profile_tu_ns[region]
                                  > profile_tu_function_ns[region]
                                  ? profile_tu_ns[region]
                                    - profile_tu_function_ns[region] : 0;
    fprintf(stderr,
            "CPC_PROFILE_TOP_SEG %s_ms=%.3f function_ms=%.3f other_ms=%.3f\n",
            profile_tu_names[region],
            profile_tu_ns[region] / 1000000.0,
            profile_tu_function_ns[region] / 1000000.0,
            other_ns / 1000000.0);
  }
  for (region = 0; region < PROFILE_TOP_COUNT; ++region)
  {
    unsigned long long other_ns = profile_top_ns[region]
                                  > profile_top_function_ns[region]
                                  ? profile_top_ns[region]
                                    - profile_top_function_ns[region] : 0;
    fprintf(stderr,
            "CPC_PROFILE_DECLSEG %s_ms=%.3f function_ms=%.3f "
            "other_ms=%.3f calls=%llu\n",
            profile_top_names[region],
            profile_top_ns[region] / 1000000.0,
            profile_top_function_ns[region] / 1000000.0,
            other_ns / 1000000.0,
            profile_top_calls[region]);
  }
  for (region = 0; region < PROFILE_DECL_TAIL_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_TOP_DECL_TAIL %s_ms=%.3f calls=%llu\n",
            profile_decl_tail_names[region],
            profile_decl_tail_ns[region] / 1000000.0,
            profile_decl_tail_calls[region]);
  for (region = 0; region < PROFILE_INIT_IMPL_ARRAY_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_INIT_IMPL_ARRAY %s_ms=%.3f calls=%llu\n",
            profile_init_impl_array_names[region],
            profile_init_impl_array_ns[region] / 1000000.0,
            profile_init_impl_array_calls[region]);
  fprintf(stderr,
          "CPC_PROFILE_SCALAR_FAST_EXPR ident=%llu unary=%llu "
          "literal=%llu other=%llu\n",
          profile_scalar_fast_expr_calls[PROFILE_SCALAR_FAST_EXPR_IDENT],
          profile_scalar_fast_expr_calls[PROFILE_SCALAR_FAST_EXPR_UNARY],
          profile_scalar_fast_expr_calls[PROFILE_SCALAR_FAST_EXPR_LITERAL],
          profile_scalar_fast_expr_calls[PROFILE_SCALAR_FAST_EXPR_OTHER]);
  for (region = 0; region < PROFILE_BTYPE_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_BTYPE %s_ms=%.3f calls=%llu\n",
            profile_btype_names[region],
            profile_btype_ns[region] / 1000000.0,
            profile_btype_calls[region]);
  fprintf(stderr, "CPC_PROFILE_TAGDECL total_ms=%.3f calls=%llu\n",
          profile_tagdecl_total_ns / 1000000.0,
          profile_tagdecl_total_calls);
  for (region = 0; region < PROFILE_TAGDECL_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_TAGDECL %s_ms=%.3f calls=%llu\n",
            profile_tagdecl_names[region],
            profile_tagdecl_ns[region] / 1000000.0,
            profile_tagdecl_calls[region]);
  for (region = 0; region < PROFILE_TAGDECL_TAIL_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_TAGDECL_TAIL %s_ms=%.3f calls=%llu\n",
            profile_tagdecl_tail_names[region],
            profile_tagdecl_tail_ns[region] / 1000000.0,
            profile_tagdecl_tail_calls[region]);
  for (region = 0; region < PROFILE_TAGDECL_MEMBER_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_TAGDECL_MEMBER %s_ms=%.3f calls=%llu\n",
            profile_tagdecl_member_names[region],
            profile_tagdecl_member_ns[region] / 1000000.0,
            profile_tagdecl_member_calls[region]);
  for (region = 0; region < PROFILE_TAGDECL_MEMBER_DECL_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_TAGDECL_MEMBER_DECL %s_ms=%.3f calls=%llu\n",
            profile_tagdecl_member_decl_names[region],
            profile_tagdecl_member_decl_ns[region] / 1000000.0,
            profile_tagdecl_member_decl_calls[region]);
  for (region = 0; region < PROFILE_TAGDECL_MEMBER_BTYPE_COUNT; ++region)
    fprintf(stderr, "CPC_PROFILE_TAGDECL_MEMBER_DECL_BTYPE %s_ms=%.3f calls=%llu\n",
            profile_tagdecl_member_btype_names[region],
            profile_tagdecl_member_btype_ns[region] / 1000000.0,
            profile_tagdecl_member_btype_calls[region]);
  for (region = 0; region < PROFILE_TAGDECL_MEMBER_BTYPE_PREFIX_COUNT; ++region)
    fprintf(stderr,
            "CPC_PROFILE_TAGDECL_MEMBER_DECL_BTYPE_PREFIX %s_ms=%.3f calls=%llu\n",
            profile_tagdecl_member_btype_prefix_names[region],
            profile_tagdecl_member_btype_prefix_ns[region] / 1000000.0,
            profile_tagdecl_member_btype_prefix_calls[region]);
  for (region = 0; region < PROFILE_TAGDECL_MEMBER_BTYPE_TYPEDEF_COUNT; ++region)
    fprintf(stderr,
            "CPC_PROFILE_TAGDECL_MEMBER_DECL_BTYPE_TYPEDEF %s_ms=%.3f calls=%llu\n",
            profile_tagdecl_member_btype_typedef_names[region],
            profile_tagdecl_member_btype_typedef_ns[region] / 1000000.0,
            profile_tagdecl_member_btype_typedef_calls[region]);
  fflush(stderr);
}

#endif
