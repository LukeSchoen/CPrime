/* Internal regression: one process covers lazy imports, provider precedence,
   missing names and translation-unit cache invalidation. This is a test,
   never a build host. Run through Compatibility/tests/test.exe -Checks. */
#include "../compiler/middleend/libcprime.c"
#include <process.h>

/* Scratch files stay in the toolchain build tree, beside the other generated
   evidence, so a run never writes into the top level. */
#define COLD_DIR "src/build/test-cold/"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

static int run_cpc(const char *a1, const char *a2, const char *a3,
                   const char *a4, const char *a5, const char *a6) {
  char compiler[MAX_PATH];
  const char *args[8];
  int n = 0;
  if (!GetFullPathNameA("cpc.exe", sizeof compiler, compiler, NULL)) return -1;
  args[n++] = compiler;
  if (a1) args[n++] = a1; if (a2) args[n++] = a2; if (a3) args[n++] = a3;
  if (a4) args[n++] = a4; if (a5) args[n++] = a5; if (a6) args[n++] = a6;
  args[n] = NULL;
  return (int)_spawnv(_P_WAIT, compiler, args);
}

static void compare_import(CPRIMEState *state, Section *expected, const char *name)
{
  int a = find_elf_sym(state->dynsymtab_section, name);
  int b = find_elf_sym(expected, name);
  CHECK(!!a == !!b);
  if (a && b) {
    ObjW(Sym) *actual = (ObjW(Sym) *)state->dynsymtab_section->data + a;
    ObjW(Sym) *reference = (ObjW(Sym) *)expected->data + b;
    CHECK(actual->st_value == reference->st_value);
    CHECK(actual->st_size == reference->st_size);
    CHECK(actual->st_shndx == reference->st_shndx);
  }
}

static void archive_replay(void)
{
  const char *names[] = {"left", "right", "late", "main"};
  const char *sources[] = {
    "int right(void); int left(void) { return right()+1; }",
    "int late(void); int right(void) { return late()+1; }",
    "int late(void) { return 40; }",
    "int left(void); int main(void) { return left()!=42; }"
  };
  const char *products[] = {"first.a", "second.a", "linked.exe"};
  char path[256];
  int i;
  CreateDirectoryA("src/build", NULL);
  CreateDirectoryA("src/build/test-cold", NULL);
  for (i = 0; i < 3; ++i) {
    sprintf(path, COLD_DIR "cold-%s", products[i]);
    remove(path);
  }
  for (i = 0; i < 4; ++i) {
    FILE *file;
    sprintf(path, COLD_DIR "cold-%s.c", names[i]);
    file = fopen(path, "wb");
    CHECK(file != NULL);
    if (!file) return;
    fputs(sources[i], file);
    fclose(file);
    if (i < 3) {
      char object[256];
      sprintf(object, COLD_DIR "cold-%s.o", names[i]);
      CHECK(run_cpc("-c", path, "-o", object, NULL, NULL) == 0);
    }
  }
  CHECK(run_cpc("-ar", "rcs", COLD_DIR "cold-first.a", COLD_DIR "cold-left.o", COLD_DIR "cold-late.o", NULL) == 0);
  CHECK(run_cpc("-ar", "rcs", COLD_DIR "cold-second.a", COLD_DIR "cold-right.o", NULL, NULL) == 0);
  CHECK(run_cpc(COLD_DIR "cold-main.c", COLD_DIR "cold-first.a", COLD_DIR "cold-second.a", "-o", COLD_DIR "cold-linked.exe", NULL) == 0);
  {
    const char *args[] = {COLD_DIR "cold-linked.exe", NULL};
    CHECK(_spawnv(_P_WAIT, args[0], args) == 0);
  }
  for (i = 0; i < 4; ++i) {
    sprintf(path, COLD_DIR "cold-%s.c", names[i]); remove(path);
    sprintf(path, COLD_DIR "cold-%s.o", names[i]); remove(path);
  }
  for (i = 0; i < 3; ++i) {
    sprintf(path, COLD_DIR "cold-%s", products[i]); remove(path);
  }
}

int main(void)
{
  CPRIMEState *state = cprime_new();
  Section *expected;
  int provider, i, fd;
  unsigned generation;
  const char *path = COLD_DIR "cold-paths.def";
  CreateDirectoryA("src/build", NULL);
  CreateDirectoryA("src/build/test-cold", NULL);
  CHECK(cprime_set_output_type(state, CPRIME_OUTPUT_OBJ) == 0);
  expected = new_symtab(state, ".expected", SHT_SYMTAB, SHF_PRIVATE | SHF_DYNSYM,
                       ".expected_names", ".expected_hash", SHF_PRIVATE);
  for (provider = 0; provider < 3; ++provider) {
    FILE *fixture = fopen(path, "wb");
    char dll[32], name[40];
    int dllindex;
    CHECK(fixture != NULL);
    if (!fixture) return 1;
    sprintf(dll, "provider%d.dll", provider);
    dllindex = cprime_add_dllref(state, dll, 0)->index;
    fprintf(fixture, "LIBRARY %s\nEXPORTS\n", dll);
    /* Reverse order and repeated names exercise arbitrary catalogs and
       first ordinal / last unresolved precedence within and across files. */
    for (i = 95; i >= 0; --i) {
      int ordinal = (i + provider) % 5 == 0 ? i + provider + 1 : 0;
      sprintf(name, "symbol_%d", i % 48);
      fprintf(fixture, "%s", name);
      if (ordinal) fprintf(fixture, " @%d", ordinal);
      fputc('\n', fixture);
      set_elf_sym(expected, ordinal, dllindex, Obj64_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                  0, ordinal ? SHN_ABS : SHN_UNDEF, name);
    }
    fclose(fixture);
    fd = open(path, O_RDONLY | O_BINARY);
    CHECK(fd >= 0);
    if (fd < 0) return 1;
    CHECK(pe_load_def(state, fd) == 0);
    close(fd);
    if (!provider) CHECK(state->dynsymtab_section->data_offset == sizeof(ObjW(Sym)));
    for (i = 0; i < 48; i += provider == 2 ? 1 : 7) {
      sprintf(name, "symbol_%d", i);
      compare_import(state, expected, name);
    }
    compare_import(state, expected, "absent_symbol");
  }
  cprime_delete(state);
  remove(path);

  for (i = 0; i < 3; ++i) {
    state = cprime_new();
    cprime_add_include_path(state, "src/include/runtime");
    CHECK(cprime_set_output_type(state, CPRIME_OUTPUT_OBJ) == 0);
    cprime_set_options(state, i == 1 ? "-x c" : "-x c++ -std=c++17");
    generation = member_name_generation;
    CHECK(cprime_compile_string_file(state, i == 1
      ? "struct Item { int member; }; int check(void) { struct Item x = {7}; return x.member; }"
      : "template<class T> struct Item { T member; T get() const { return member; } }; int check() { Item<int> x = {9}; return x.get(); }",
      i == 1 ? "cold.c" : "cold.cpp") == 0);
    CHECK(member_name_generation != generation);
    CHECK(!pending_member_token_filter_used && !member_function_scopes_used);
    CHECK(!find_elf_sym(state->dynsymtab_section, "symbol_0"));
    cprime_delete(state);
  }
  archive_replay();
  if (!failures) puts("PASS import precedence, lazy/missing names, C++/C/C++ reset, late archive dependency");
  return failures != 0;
}
