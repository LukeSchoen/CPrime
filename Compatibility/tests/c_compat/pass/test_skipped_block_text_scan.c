// EXPECT_EXIT: 0
/* The preprocessor skip scan advances over runs of ordinary text, so the
   bytes below pin how it must classify what it walks past: strings and
   character constants that spell directives, comments holding '#' and
   quotes, a backslash-newline continuation, nested conditionals, and a
   warning line whose unbalanced quote and slash are ordinary text (the
   skip scan must resume at the newline, not inside a string). */
#define LIVE 7
#if 0
char *skipped_directive = "#endif \" not a directive";
char *skipped_escapes = "\\ \" \\\\ still text";
char skipped_char = '\'';
/* #if 0 #else #endif "quoted" 'ch' / slash */
// trailing comment with "#endif"
#warning unbalanced quote follows " and a slash /
char *after_warning = "text";
int skipped_continued = 1 + \
                        2;
#if 1
int nested = skipped_continued;
#endif
#endif
#if 1
int kept = LIVE;
#else
not compiled " text ' #endif / quote
#endif
int main(void) { return kept == 7 ? 0 : 1; }
