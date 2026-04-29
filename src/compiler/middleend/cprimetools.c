#include "cprime.h"

// #define ARMAG  "!<arch>\n"
#define ARFMAG "`\n"

typedef struct
{
  char ar_name[16];
  char ar_date[12];
  char ar_uid[6];
  char ar_gid[6];
  char ar_mode[8];
  char ar_size[10];
  char ar_fmag[2];
} ArHdr;

static unsigned long le2belong(unsigned long ul)
{
  return ((ul & 0xFF0000) >> 8) + ((ul & 0xFF000000) >> 24) +
         ((ul & 0xFF) << 24) + ((ul & 0xFF00) << 8);
}

static int ar_usage(int ret)
{
  fprintf(stderr, "usage: cpc -ar [crstvx] lib [files]\n");
  fprintf(stderr, "create library ([abdiopN] not supported).\n");
  return ret;
}

ST_FUNC int cprime_tool_ar(int argc, char **argv)
{
  static const ArHdr arhdr_init =
  {
    "/               ",
    "0           ",
    "0     ",
    "0     ",
    "0       ",
    "0         ",
    ARFMAG
  };

  ArHdr arhdr = arhdr_init;
  ArHdr arhdro = arhdr_init;

  FILE *fi, *fh = NULL, *fo = NULL;
  const char *created_file = NULL; // Must Delete On Error
  ObjW(Ehdr) *ehdr;
  ObjW(Shdr) *shdr;
  ObjW(Sym) *sym;
  int i, fsize, i_lib, i_obj;
  char *buf, *shstr, *symtab, *strtab;
  int symtabsize = 0;//, strtabsize = 0;
  char *anames = NULL;
  int *afpos = NULL;
  int istrlen, strpos = 0, fpos = 0, funccnt = 0, funcmax, hofs;
  char tfile[260], stmp[20];
  char *file, *name;
  int ret = 2;
  const char *ops_conflict = "habdiopN";  // unsupported but destructive if ignored.
  int extract = 0;
  int table = 0;
  int verbose = 0;

  i_lib = 0; i_obj = 0;  // Will Hold The Index Of The Lib And First Obj
  for (i = 1; i < argc; i++)
  {
    const char *a = argv[i];
    if (*a == '-' && strchr(a, '.'))
      ret = 1; // -X.Y Is Always Invalid (Same As Gnu Ar)
    if ((*a == '-') || (i == 1 && !strchr(a, '.')))    // Options Argument
    {
      if (strpbrk(a, ops_conflict))
        ret = 1;
      if (strchr(a, 'x'))
        extract = 1;
      if (strchr(a, 't'))
        table = 1;
      if (strchr(a, 'v'))
        verbose = 1;
    }
    else      // lib or obj files: don't abort - keep validating all args.
    {
      if (!i_lib)  // First File Is The Lib
        i_lib = i;
      else if (!i_obj)  // Second File Is The First Obj
        i_obj = i;
    }
  }

  if (!i_lib)  // I_Obj Implies Also I_Lib.
    ret = 1;
  i_obj = i_obj ? i_obj : argc;  // An empty archive will be generated if no input file is given

  if (ret == 1)
    return ar_usage(ret);

  if (extract || table)
  {
    if ((fh = fopen(argv[i_lib], "rb")) == NULL)
    {
      fprintf(stderr, "cpc: ar: can't open file %s\n", argv[i_lib]);
      goto finish;
    }
    fread(stmp, 1, 8, fh);
    if (memcmp(stmp, ARMAG, 8))
    {
no_ar:
      fprintf(stderr, "cpc: ar: not an ar archive %s\n", argv[i_lib]);
      goto finish;
    }
    while (fread(&arhdr, 1, sizeof(arhdr), fh) == sizeof(arhdr))
    {
      char *p, *e;

      if (memcmp(arhdr.ar_fmag, ARFMAG, 2))
        goto no_ar;
      p = arhdr.ar_name;
      for (e = p + sizeof arhdr.ar_name; e > p && e[-1] == ' ';)
        e--;
      *e = '\0';
      arhdr.ar_size[sizeof arhdr.ar_size - 1] = 0;
      fsize = atoi(arhdr.ar_size);
      buf = cprime_malloc(fsize + 1);
      fread(buf, fsize, 1, fh);
      if (strcmp(arhdr.ar_name, "/") && strcmp(arhdr.ar_name, "/SYM64/"))
      {
        if (e > p && e[-1] == '/')
          e[-1] = '\0';
        // Tv Not Implemented
        if (table || verbose)
          printf("%s%s\n", extract ? "x - " : "", arhdr.ar_name);
        if (extract)
        {
          if ((fo = fopen(arhdr.ar_name, "wb")) == NULL)
          {
            fprintf(stderr, "cpc: ar: can't create file %s\n",
                    arhdr.ar_name);
            cprime_free(buf);
            goto finish;
          }
          fwrite(buf, fsize, 1, fo);
          fclose(fo);
          // Ignore Date/Uid/Gid/Mode
        }
      }
      if (fsize & 1)
        fgetc(fh);
      cprime_free(buf);
    }
    ret = 0;
finish:
    if (fh)
      fclose(fh);
    return ret;
  }

  if ((fh = fopen(argv[i_lib], "wb")) == NULL)
  {
    fprintf(stderr, "cpc: ar: can't create file %s\n", argv[i_lib]);
    goto the_end;
  }
  created_file = argv[i_lib];

  sprintf(tfile, "%s.tmp", argv[i_lib]);
  if ((fo = fopen(tfile, "wb+")) == NULL)
  {
    fprintf(stderr, "cpc: ar: can't create temporary file %s\n", tfile);
    goto the_end;
  }

  funcmax = 250;
  afpos = cprime_realloc(NULL, funcmax *sizeof *afpos);  // 250 Func
  memcpy(&arhdro.ar_mode, "100644", 6);

  // I_Obj = First Input Object File
  while (i_obj < argc)
  {
    if (*argv[i_obj] == '-')    // By Now, All Options Start With '-'
    {
      i_obj++;
      continue;
    }
    if ((fi = fopen(argv[i_obj], "rb")) == NULL)
    {
      fprintf(stderr, "cpc: ar: can't open file %s \n", argv[i_obj]);
      goto the_end;
    }
    if (verbose)
      printf("a - %s\n", argv[i_obj]);

    fseek(fi, 0, SEEK_END);
    fsize = ftell(fi);
    fseek(fi, 0, SEEK_SET);
    buf = cprime_malloc(fsize + 1);
    fread(buf, fsize, 1, fi);
    fclose(fi);

    // Elf Header
    ehdr = (ObjW(Ehdr) *)buf;
    if (ehdr->e_ident[4] != OBJ_CLASSW)
    {
      fprintf(stderr, "cpc: ar: Unsupported Elf Class: %s\n", argv[i_obj]);
      goto the_end;
    }

    shdr = (ObjW(Shdr) *) (buf + ehdr->e_shoff + ehdr->e_shstrndx *ehdr->e_shentsize);
    shstr = (char *)(buf + shdr->sh_offset);
    symtab = strtab = NULL;
    for (i = 0; i < ehdr->e_shnum; i++)
    {
      shdr = (ObjW(Shdr) *) (buf + ehdr->e_shoff + i *ehdr->e_shentsize);
      if (!shdr->sh_offset)
        continue;
      if (shdr->sh_type == SHT_SYMTAB)
      {
        symtab = (char *)(buf + shdr->sh_offset);
        symtabsize = shdr->sh_size;
      }
      if (shdr->sh_type == SHT_STRTAB)
      {
        if (!strcmp(shstr + shdr->sh_name, ".strtab"))
        {
          strtab = (char *)(buf + shdr->sh_offset);
          //strtabsize = shdr->sh_size;
        }
      }
    }

    if (symtab && strtab)
    {
      int nsym = symtabsize / sizeof(ObjW(Sym));
      //printf("symtab: info size shndx name\n");
      for (i = 1; i < nsym; i++)
      {
        sym = (ObjW(Sym) *) (symtab + i *sizeof(ObjW(Sym)));
        if (sym->st_shndx &&
            (sym->st_info == 0x10
             || sym->st_info == 0x11
             || sym->st_info == 0x12
             || sym->st_info == 0x20
             || sym->st_info == 0x21
             || sym->st_info == 0x22
            ))
        {
          //printf("symtab: %2Xh %4Xh %2Xh %s\n", sym->st_info, sym->st_size, sym->st_shndx, strtab + sym->st_name);
          istrlen = strlen(strtab + sym->st_name) + 1;
          anames = cprime_realloc(anames, strpos + istrlen);
          strcpy(anames + strpos, strtab + sym->st_name);
          strpos += istrlen;
          if (++funccnt >= funcmax)
          {
            funcmax += 250;
            afpos = cprime_realloc(afpos, funcmax *sizeof *afpos);  // 250 Func More
          }
          afpos[funccnt] = fpos;
        }
      }
    }

    file = argv[i_obj];
    for (name = strchr(file, 0);
         name > file && name[-1] != '/' && name[-1] != '\\';
         --name);
    istrlen = strlen(name);
    if (istrlen >= sizeof(arhdro.ar_name))
      istrlen = sizeof(arhdro.ar_name) - 1;
    memset(arhdro.ar_name, ' ', sizeof(arhdro.ar_name));
    memcpy(arhdro.ar_name, name, istrlen);
    arhdro.ar_name[istrlen] = '/';
    sprintf(stmp, "%-10d", fsize);
    memcpy(&arhdro.ar_size, stmp, 10);
    fwrite(&arhdro, sizeof(arhdro), 1, fo);
    fwrite(buf, fsize, 1, fo);
    cprime_free(buf);
    i_obj++;
    fpos += (fsize + sizeof(arhdro));
    if (fpos & 1)
      fputc(0, fo), ++fpos;
  }
  hofs = 8 + sizeof(arhdr) + strpos + (funccnt + 1) * sizeof(int);
  fpos = 0;
  if ((hofs & 1)) // Align
    hofs++, fpos = 1;
  // Write Header
  fwrite(ARMAG, 8, 1, fh);
  // Create An Empty Archive
  if (!funccnt)
  {
    ret = 0;
    goto the_end;
  }
  sprintf(stmp, "%-10d", (int)(strpos + (funccnt + 1) * sizeof(int)) + fpos);
  memcpy(&arhdr.ar_size, stmp, 10);
  fwrite(&arhdr, sizeof(arhdr), 1, fh);
  afpos[0] = le2belong(funccnt);
  for (i = 1; i <= funccnt; i++)
    afpos[i] = le2belong(afpos[i] + hofs);
  fwrite(afpos, (funccnt + 1) * sizeof(int), 1, fh);
  fwrite(anames, strpos, 1, fh);
  if (fpos)
    fwrite("", 1, 1, fh);
  // Write Objects
  fseek(fo, 0, SEEK_END);
  fsize = ftell(fo);
  fseek(fo, 0, SEEK_SET);
  buf = cprime_malloc(fsize + 1);
  fread(buf, fsize, 1, fo);
  fwrite(buf, fsize, 1, fh);
  cprime_free(buf);
  ret = 0;
the_end:
  if (anames)
    cprime_free(anames);
  if (afpos)
    cprime_free(afpos);
  if (fh)
    fclose(fh);
  if (created_file && ret != 0)
    remove(created_file);
  if (fo)
    fclose(fo), remove(tfile);
  return ret;
}

#ifdef CPRIME_TARGET_PE

ST_FUNC int cprime_tool_impdef(int argc, char **argv)
{
  int ret, v, i;
  char infile[260];
  char outfile[260];

  const char *file;
  char *p, *q;
  FILE *fp, *op;

#ifdef _WIN32
  char path[260];
#endif

  infile[0] = outfile[0] = 0;
  fp = op = NULL;
  ret = 1;
  p = NULL;
  v = 0;

  for (i = 1; i < argc; ++i)
  {
    const char *a = argv[i];
    if ('-' == a[0])
    {
      if (0 == strcmp(a, "-v"))
        v = 1;
      else if (0 == strcmp(a, "-o"))
      {
        if (++i == argc)
          goto usage;
        strcpy(outfile, argv[i]);
      }
      else
        goto usage;
    }
    else if (0 == infile[0])
      strcpy(infile, a);
    else
      goto usage;
  }

  if (0 == infile[0])
  {
usage:
    fprintf(stderr,
            "usage: cpc -impdef library.dll [-v] [-o outputfile]\n"
            "create export definition file (.def) from dll\n"
           );
    goto the_end;
  }

  if (0 == outfile[0])
  {
    strcpy(outfile, cprime_basename(infile));
    q = strrchr(outfile, '.');
    if (NULL == q)
      q = strchr(outfile, 0);
    strcpy(q, ".def");
  }

  file = infile;
#ifdef _WIN32
  if (SearchPath(NULL, file, ".dll", sizeof path, path, NULL))
    file = path;
#endif
  ret = cprime_get_dllexports(file, &p);
  if (ret || !p)
  {
    fprintf(stderr, "cpc: impdef: %s '%s'\n",
            ret == -1 ? "can't find file" :
            ret ==  1 ? "can't read symbols" :
            ret ==  0 ? "no symbols found in" :
            "unknown file type", file);
    ret = 1;
    goto the_end;
  }

  if (v)
    printf("-> %s\n", file);

  op = fopen(outfile, "wb");
  if (NULL == op)
  {
    fprintf(stderr, "cpc: impdef: could not create output file: %s\n", outfile);
    goto the_end;
  }

  fprintf(op, "LIBRARY %s\n\nEXPORTS\n", cprime_basename(file));
  for (q = p, i = 0; *q; ++i)
  {
    fprintf(op, "%s\n", q);
    q += strlen(q) + 1;
  }

  if (v)
    printf("<- %s (%d symbol%s)\n", outfile, i, &"s"[i < 2]);

  ret = 0;

the_end:
  if (p)
    cprime_free(p);
  if (fp)
    fclose(fp);
  if (op)
    fclose(op);
  return ret;
}

#endif // CPRIME_TARGET_PE 

// Re-Execute The I386/X86_64 Cross-Compilers With Cpc -M32/-M64:

#if !defined CPRIME_TARGET_I386 && !defined CPRIME_TARGET_X86_64

ST_FUNC int cprime_tool_cross(char **argv, int option)
{
  fprintf(stderr, "cpc -m%d not implemented\n", option);
  return 1;
}

#else
#ifdef _WIN32
#include <process.h>

/* - Empty argument or with space/tab (not newline) requires quoting.
 * - Double-quotes at the value require '\'-escape, regardless of quoting.
 * - Consecutive (or 1) backslashes at the value all need '\'-escape only if
 *   followed by [escaped] double quote, else taken literally, e.g. <x\\y\>
 *   remains literal without quoting or esc, but <x\\"y\> becomes <x\\\\\"y\>.
 * - This "before double quote" rule applies also before delimiting quoting,
 *   e.g. <x\y \"z\> becomes <"x\y \\\"z\\"> (quoting required because space).
 *
 * https:// Learn.Microsoft.Com/En-Us/Cpp/C-Language/Parsing-C-Command-Line-Arguments
 */
static char *quote_win32(const char *s)
{
  char *o, *r = cprime_malloc(2 * strlen(s) + 3);   // Max-Esc, Quotes, \0
  int cbs = 0, quoted = !*s;  // Consecutive Backslashes Before Current

  for (o = r; *s; *o++ = *s++)
  {
    quoted |= *s == ' ' || *s == '\t';
    if (*s == '\\' || *s == '"')
      *o++ = '\\';
    else
      o -= cbs;  // undo cbs escapes, if any (not followed by DQ)
    cbs = *s == '\\' ? cbs + 1 : 0;
  }
  if (quoted)
  {
    memmove(r + 1, r, o++ - r);
    *r = *o++ = '"';
  }
  else
    o -= cbs;

  *o = 0;
  return r; // don't bother with realloc(r, o-r+1)
}

static int execvp_win32(const char *prog, char **argv)
{
  int ret; char **p;
  // Replace All " By \"
  for (p = argv; *p; ++p)
    *p = quote_win32(*p);
  ret = _spawnvp(P_NOWAIT, prog, (const char *const *)argv);
  if (-1 == ret)
    return ret;
  _cwait(&ret, ret, WAIT_CHILD);
  exit(ret);
}
#define execvp execvp_win32
#endif // _WIN32 

ST_FUNC int cprime_tool_cross(char **argv, int target)
{
  char program[4096];
  char *a0 = argv[0];
  int prefix = cprime_basename(a0) - a0;

  snprintf(program, sizeof program,
           "%.*s%s"
#ifdef CPRIME_TARGET_PE
           "-win32"
#endif
           "-cpc"
#ifdef _WIN32
           ".exe"
#endif
           , prefix, a0, target == 64 ? "x86_64" : "i386");

  if (strcmp(a0, program))
    execvp(argv[0] = program, argv);
  fprintf(stderr, "cpc: could not run '%s'\n", program);
  return 1;
}

#endif // CPRIME_TARGET_I386 && CPRIME_TARGET_X86_64 
// --------------------------------------------------------------
// Enable Commandline Wildcard Expansion (Cpc -O X.Exe *.C)

#ifdef _WIN32
const int _CRT_glob = 1;
#ifndef _CRT_glob
const int _dowildcard = 1;
#endif
#endif

static char *escape_target_dep(const char *s)
{
  char *res = cprime_malloc(strlen(s) * 2 + 1);
  int j;
  for (j = 0; *s; s++, j++)
  {
    if (is_space(*s))
      res[j++] = '\\';
    res[j] = *s;
  }
  res[j] = '\0';
  return res;
}

ST_FUNC int gen_makedeps(CPRIMEState *s1, const char *target, const char *filename)
{
  FILE *depout;
  char buf[1024];
  char **escaped_targets;
  int i, k, num_targets;

  if (!filename)
  {
    // Compute Filename Automatically: Dir/File.O -> Dir/File.D
    snprintf(buf, sizeof buf, "%.*s.d",
             (int)(cprime_fileextension(target) - target), target);
    filename = buf;
  }

  if (!strcmp(filename, "-"))
    depout = fdopen(1, "w");
  else
    // XXX return err codes instead of error() ?
    depout = fopen(filename, "w");
  if (!depout)
    return cprime_error_noabort("could not open '%s'", filename);
  if (s1->verbose)
    printf("<- %s\n", filename);

  escaped_targets = cprime_malloc(s1->nb_target_deps *sizeof(*escaped_targets));
  num_targets = 0;
  for (i = 0; i < s1->nb_target_deps; ++i)
  {
    for (k = 0; k < i; ++k)
      if (0 == strcmp(s1->target_deps[i], s1->target_deps[k]))
        goto next;
    escaped_targets[num_targets++] = escape_target_dep(s1->target_deps[i]);
next:;
  }

  fprintf(depout, "%s:", target);
  for (i = 0; i < num_targets; ++i)
    fprintf(depout, " \\\n  %s", escaped_targets[i]);
  fprintf(depout, "\n");
  if (s1->gen_phony_deps)
  {
    /* Skip first file, which is the c file.
     * Only works for single file give on command-line,
     * but other compilers have the same limitation */
    for (i = 1; i < num_targets; ++i)
      fprintf(depout, "%s:\n", escaped_targets[i]);
  }
  for (i = 0; i < num_targets; ++i)
    cprime_free(escaped_targets[i]);
  cprime_free(escaped_targets);
  fclose(depout);
  return 0;
}







