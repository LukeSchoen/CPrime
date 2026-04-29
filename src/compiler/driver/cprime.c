#ifndef ONE_SOURCE
# define ONE_SOURCE 1
#endif

#include "cprime.h"
#if ONE_SOURCE
# include "libcprime.c"
#endif
#include "cprimetools.c"

static const char help[] =
  "cprime Compiler "CPRIME_VERSION"\n"
  "Usage: cpc [options...] [-o outfile] [-c] infile(s)...\n"
  "       cpc [options...] -run infile (or --) [arguments...]\n"
  "General options:\n"
  "  -c           compile only - generate an object file\n"
  "  -o outfile   set output filename\n"
  "  -run         run compiled source [with custom stdin: -rstdin FILE]\n"
  "  -fflag       set or reset (with 'no-' prefix) 'flag' (see cpc -hh)\n"
  "  -Wwarning    set or reset (with 'no-' prefix) 'warning' (see cpc -hh)\n"
  "  -w           disable all warnings\n"
  "  -v --version show version\n"
  "  -vv          show search paths or loaded files\n"
  "  -h -hh       show this, show more help\n"
  "  -bench       show compilation statistics\n"
  "  -            use stdin pipe as infile\n"
  "  @listfile    read arguments from listfile\n"
  "Preprocessor options:\n"
  "  -Idir        add include path 'dir'\n"
  "  -Dsym[=val]  define 'sym' with value 'val'\n"
  "  -Usym        undefine 'sym'\n"
  "  -E           preprocess only\n"
  "  -nostdinc    do not use standard system include paths\n"
  "Linker options:\n"
  "  -Ldir        add library path 'dir'\n"
  "  -llib        link with dynamic or static library 'lib'\n"
  "  -nostdlib    do not link with standard crt and libraries\n"
  "  -r           generate (relocatable) object file\n"
  "  -rdynamic    export all global symbols to dynamic linker\n"
  "  -shared      generate a shared library/dll\n"
  "  -soname      set name for shared library to be used at runtime\n"
  "  -Wl,-opt[=val]  set linker option (see cpc -hh)\n"
  "Debugger options:\n"
  "  -gdwarf[-x]  generate dwarf runtime debug info\n"
#ifdef CPRIME_TARGET_PE
  "  -g.pdb       create .pdb debug database\n"
#endif
#ifdef CONFIG_CPRIME_BCHECK
  "  -b           compile with built-in memory and bounds checker (implies -g)\n"
#endif
#ifdef CONFIG_CPRIME_BACKTRACE
  "  -bt[N]       link with backtrace (stack dump) support [show max N callers]\n"
#endif
  "Misc. options:\n"
  "  -std=version define __STDC_VERSION__ according to version (c11)\n"
  "  -x[c|a|b|n]  specify type of the next infile (C,ASM,BIN,NONE)\n"
  "  -Bdir        set cpc's private include/library dir\n"
  "  -M[M]D       generate make dependency file [ignore system files]\n"
  "  -M[M]        as above but no other output\n"
  "  -MF file     specify dependency file name\n"
  "Tools:\n"
  "  create library  : cpc -ar [crstvx] lib [files]\n"
#ifdef CPRIME_TARGET_PE
  "  create def file : cpc -impdef lib.dll [-v] [-o lib.def]\n"
#endif
  "Discussion & bug reports:\n"
  "  https://lists.nongnu.org/mailman/listinfo/cprime-devel\n"
  ;

static const char help2[] =
  "cprime Compiler "CPRIME_VERSION" - More Options\n"
  "Special options:\n"
  "  -P -P1                        with -E: no/alternative #line output\n"
  "  -dD -dM                       with -E: output #define directives\n"
  "  -pthread                      same as -D_REENTRANT and -lpthread\n"
  "  -On                           same as -D__OPTIMIZE__ for n > 0\n"
  "  -Wp,-opt                      same as -opt\n"
  "  -include file                 include 'file' above each input file\n"
  "  -nostdlib                     do not link with standard crt/libs\n"
  "  -isystem dir                  add 'dir' to system include path\n"
  "  -static                       link to static libraries (not recommended)\n"
  "  -dumpversion                  print version\n"
  "  -print-search-dirs            print search paths\n"
  "  -dt                           with -run/-E: auto-define 'test_...' macros\n"
  "Ignored options:\n"
  "  -arch -C --param -pedantic -pipe -s -traditional\n"
  "-W[no-]... warnings:\n"
  "  all                           turn on some (*) warnings\n"
  "  error[=warning]               stop after warning (any or specified)\n"
  "  write-strings                 strings are const\n"
  "  unsupported                   warn about ignored options, pragmas, etc.\n"
  "  implicit-function-declaration warn for missing prototype (*)\n"
  "  discarded-qualifiers          warn when const is dropped (*)\n"
  "-f[no-]... flags:\n"
  "  unsigned-char                 default char is unsigned\n"
  "  signed-char                   default char is signed\n"
  "  common                        use common section instead of bss\n"
  "  leading-underscore            decorate extern symbols\n"
  "  ms-extensions                 allow anonymous struct in struct\n"
  "  dollars-in-identifiers        allow '$' in C symbols\n"
  "  reverse-funcargs              evaluate function arguments right to left\n"
  "  classic-inline                'extern inline' is like 'static inline'\n"
  "  asynchronous-unwind-tables    create eh_frame section [on]\n"
  "  test-coverage                 create code coverage code\n"
  "-m... target specific options:\n"
  "  ms-bitfields                  use MSVC bitfield layout\n"
  "-Wl,... linker options:\n"
  "  -nostdlib                     do not search standard library paths\n"
  "  -[no-]whole-archive           load lib(s) fully/only as needed\n"
  "  -export-all-symbols           same as -rdynamic\n"
  "  -export-dynamic               same as -rdynamic\n"
  "  -image-base= -Ttext=          set base address of executable\n"
  "  -section-alignment=           set section alignment in executable\n"
#ifdef CPRIME_TARGET_PE
  "  -file-alignment=              set PE file alignment\n"
  "  -stack=                       set PE stack reserve\n"
  "  -large-address-aware          set related PE option\n"
  "  -subsystem=[console/windows]  set PE subsystem\n"
  "  -oformat=[pe-* binary]        set executable output format\n"
  "Predefined macros:\n"
  "  cpc -E -dM - < nul\n"
#else
  "  -rpath=                       set dynamic library search path\n"
  "  -enable-new-dtags             set DT_RUNPATH instead of DT_RPATH\n"
  "  -soname=                      set DT_SONAME elf tag\n"
#if defined(CPRIME_TARGET_MACHO)
  "  -install_name=                set DT_SONAME elf tag (soname macOS alias)\n"
#else
  "  -Ipath, -dynamic-linker=path  set ELF interpreter to path\n"
#endif
  "  -Bsymbolic                    set DT_SYMBOLIC elf tag\n"
  "  -oformat=[elf32/64-* binary]  set executable output format\n"
  "  -init= -fini= -Map= -as-needed -O -z= (ignored)\n"
  "Predefined macros:\n"
  "  cpc -E -dM - < /dev/null\n"
#endif
  "See also the manual for more details.\n"
  ;

static const char version[] =
  "cpc version "CPRIME_VERSION
#ifdef CPRIME_GITHASH
  " "CPRIME_GITHASH
#endif
  " ("
#ifdef CPRIME_TARGET_X86_64
  "x86_64"
#endif
#ifdef CPRIME_TARGET_PE
  " Windows"
#elif defined(CPRIME_TARGET_MACHO)
  " Darwin"
#elif TARGETOS_FreeBSD || TARGETOS_FreeBSD_kernel
  " FreeBSD"
#elif TARGETOS_OpenBSD
  " OpenBSD"
#elif TARGETOS_NetBSD
  " NetBSD"
#else
  " Linux"
#endif
  ")\n"
  ;

static void print_dirs(const char *msg, char **paths, int nb_paths)
{
  int i;
  printf("%s:\n%s", msg, nb_paths ? "" : "  -\n");
  for (i = 0; i < nb_paths; i++)
    printf("  %s\n", paths[i]);
}

static void print_search_dirs(CPRIMEState *s)
{
  printf("install: %s\n", s->cprime_lib_path);
  // print_dirs("programs", NULL, 0);
  print_dirs("include", s->sysinclude_paths, s->nb_sysinclude_paths);
  print_dirs("libraries", s->library_paths, s->nb_library_paths);
  printf("libcprime1:\n  %s/%s\n", s->library_paths[0], CONFIG_CPRIME_CROSSPREFIX CPRIME_LIBCPRIME1);
#ifdef CPRIME_TARGET_UNIX
  print_dirs("crt", s->crt_paths, s->nb_crt_paths);
  printf("elfinterp:\n  %s\n",  DEFAULT_ELFINTERP(s));
#endif
}

static void set_environment(CPRIMEState *s)
{
  char * path;

  path = getenv("C_INCLUDE_PATH");
  if (path != NULL)
    cprime_add_sysinclude_path(s, path);
  path = getenv("CPATH");
  if (path != NULL)
    cprime_add_include_path(s, path);
  path = getenv("LIBRARY_PATH");
  if (path != NULL)
    cprime_add_library_path(s, path);
}

static char *default_outputfile(CPRIMEState *s, const char *first_file)
{
  char buf[1024];
  char *ext;
  const char *name = "a";

  if (first_file && strcmp(first_file, "-"))
    name = cprime_basename(first_file);
  if (strlen(name) + 4 >= sizeof buf)
    name = "a";
  strcpy(buf, name);
  ext = cprime_fileextension(buf);
#ifdef CPRIME_TARGET_PE
  if (s->output_type == CPRIME_OUTPUT_DLL)
    strcpy(ext, ".dll");
  else if (s->output_type == CPRIME_OUTPUT_EXE)
    strcpy(ext, ".exe");
  else
#endif
    if ((s->just_deps || s->output_type == CPRIME_OUTPUT_OBJ) && !s->option_r && *ext)
      strcpy(ext, ".o");
    else
      strcpy(buf, "a.out");
  return cprime_strdup(buf);
}

static unsigned getclock_ms(void)
{
#ifdef _WIN32
  return GetTickCount();
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + (tv.tv_usec + 500) / 1000;
#endif
}

int main(int argc, char **argv)
{
  CPRIMEState *s, *s1;
  int ret, opt, n = 0, t = 0, done;
  unsigned start_time = 0, end_time = 0;
  const char *first_file;
  int argc0 = argc;
  char **argv0 = argv;
  FILE *ppfp = stdout;

redo:
  argc = argc0, argv = argv0;
  s = s1 = cprime_new();
  opt = cprime_parse_args(s, &argc, &argv);

  if (n == 0)
  {
    ret = 0;
    if (opt == OPT_HELP)
    {
      fputs(help, stdout);
      if (s->verbose)
        goto help2;
    }
    else if (opt == OPT_HELP2)
help2: fputs(help2, stdout);
    else if (opt == OPT_M32 || opt == OPT_M64)
      ret = cprime_tool_cross(argv, opt);
    else if (s->verbose)
      printf("%s", version);

    if (opt == OPT_AR)
      ret = cprime_tool_ar(argc, argv);
#ifdef CPRIME_TARGET_PE
    if (opt == OPT_IMPDEF)
      ret = cprime_tool_impdef(argc, argv);
#endif
    if (opt == OPT_PRINT_DIRS)
    {
      // Initialize Search Dirs
      set_environment(s);
      cprime_set_output_type(s, CPRIME_OUTPUT_MEMORY);
      print_search_dirs(s);
    }
    if (opt)
    {
if (opt < 0) err:
        ret = 1;
      cprime_delete(s);
      return ret;
    }
    if (s->nb_files == 0)
      cprime_error_noabort("no input files");
    else if (s->output_type == CPRIME_OUTPUT_PREPROCESS)
    {
      if (s->outfile && 0 != strcmp("-", s->outfile))
      {
        ppfp = fopen(s->outfile, "wb");
        if (!ppfp)
          cprime_error_noabort("could not write '%s'", s->outfile);
      }
    }
    else if (s->output_type == CPRIME_OUTPUT_OBJ && !s->option_r)
    {
      if (s->nb_libraries)
        cprime_error_noabort("cannot specify libraries with -c");
      else if (s->nb_files > 1 && s->outfile)
        cprime_error_noabort("cannot specify output file with -c many files");
    }
    if (s->nb_errors)
      goto err;
    if (s->do_bench)
      start_time = getclock_ms();
  }

  set_environment(s);
  if (s->output_type == 0)
    s->output_type = CPRIME_OUTPUT_EXE;
  cprime_set_output_type(s, s->output_type);
  s->ppfp = ppfp;

  if ((s->output_type == CPRIME_OUTPUT_MEMORY
       || s->output_type == CPRIME_OUTPUT_PREPROCESS)
      && (s->dflag & 16))   // -Dt Option
  {
    if (t)
      s->dflag |= 32;
    s->run_test = ++t;
    if (n)
      --n;
  }

  // Compile Or Add Each Files Or Library
  first_file = NULL;
  do
  {
    struct filespec *f = s->files[n];
    s->filetype = f->type;
    if (f->type & AFF_TYPE_LIB)
      ret = cprime_add_library(s, f->name);
    else
    {
      if (1 == s->verbose)
        printf("-> %s\n", f->name);
      if (!first_file)
        first_file = f->name;
      ret = cprime_add_file(s, f->name);
    }
  }
  while (++n < s->nb_files
         && 0 == ret
         && (s->output_type != CPRIME_OUTPUT_OBJ || s->option_r));

  if (s->do_bench)
    end_time = getclock_ms();

  if (s->run_test)
    t = 0;
  else if (s->output_type == CPRIME_OUTPUT_PREPROCESS)
    ;
  else if (0 == ret)
  {
    if (s->output_type == CPRIME_OUTPUT_MEMORY)
    {
#ifdef CPRIME_IS_NATIVE
      ret = cprime_run(s, argc, argv);
#endif
    }
    else
    {
      if (!s->outfile)
        s->outfile = default_outputfile(s, first_file);
      if (!s->just_deps)
        ret = cprime_output_file(s, s->outfile);
      if (!ret && s->gen_deps)
        gen_makedeps(s, s->outfile, s->deps_outfile);
    }
  }

  done = 1;
  if (t)
    done = 0; // Run More Tests With -Dt -Run
  else if (ret)
  {
    if (s->nb_errors)
      ret = 1;
    // else keep the original exit code from cprime_run()
  }
  else if (n < s->nb_files)
    done = 0; // Compile More Files With -C
  else if (s->do_bench)
    cprime_print_stats(s, end_time - start_time);

  cprime_delete(s);
  if (!done)
    goto redo;
  if (ppfp && ppfp != stdout)
    fclose(ppfp);
  return ret;
}






