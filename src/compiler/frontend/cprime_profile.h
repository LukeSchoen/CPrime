/* Opt-in driver instrumentation for the Cost area.

   CPC_PROFILE_PHASES already splits a run into setup, translation-unit work,
   output writing and cleanup, and CPC_PROFILE_SCANS counts template and
   overload lookup work.  Neither answers where inside the translation unit the
   time goes.  These accumulators add the two largest recorded leaves:

     - next_nomacro() calls and their total elapsed time
     - gen_function() calls and their total elapsed time, not counting the
       time spent in a nested definition (a queued member body compiled from
       inside another function counts for the outer function only once)

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
static unsigned long long profile_function_sample_started;
static int profile_detail_enabled;
static int profile_detail_initialized;
static int profile_detail_function_depth;
static int profile_detail_lexer_active;

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
    profile_detail_function_ns += profile_detail_now_ns() - started;
  --profile_detail_function_depth;
}

static void profile_detail_report(void)
{
  if (!profile_detail_enabled)
    return;
  if (profile_detail_lexer_sampled_calls)
    profile_detail_lexer_ns = profile_detail_lexer_ns
                              * profile_detail_lexer_calls
                              / profile_detail_lexer_sampled_calls;
  fprintf(stderr,
          "CPC_PROFILE_DETAIL lexer_calls=%llu lexer_ms=%.3f "
          "function_calls=%llu function_ms=%.3f\n",
          profile_detail_lexer_calls, profile_detail_lexer_ns / 1000000.0,
          profile_detail_function_calls, profile_detail_function_ns / 1000000.0);
  fflush(stderr);
}

#endif
