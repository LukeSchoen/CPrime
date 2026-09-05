/* Native AMD64 COFF objects, including members of Microsoft .lib archives.
   Keep the compiler's internal ELF sections/relocations; translate at input. */

struct pe_coff_section
{
  IMAGE_SECTION_HEADER header;
  Section *section;
  unsigned long offset;
  unsigned char selection, duplicate;
  unsigned short associated;
};

static int pe_coff_read(int fd, unsigned long base, unsigned long size,
                        unsigned long offset, void *data, unsigned long count)
{
  return offset <= size && count <= size - offset
      && read_mem(fd, base + offset, data, count);
}

static const char *pe_coff_name(BYTE *raw, const char *strings,
                                unsigned string_size, char short_name[9])
{
  unsigned offset;
  if (!read32le(raw))
  {
    offset = read32le(raw + 4);
    if (offset < 4 || offset >= string_size
        || !memchr(strings + offset, 0, string_size - offset))
      return NULL;
    return strings + offset;
  }
  memcpy(short_name, raw, 8);
  short_name[8] = 0;
  return short_name;
}

static int pe_load_short_import(CPRIMEState *s1, int fd, unsigned long base,
                                unsigned long size, BYTE *header)
{
  unsigned length = read32le(header + 12), bits = read16le(header + 18);
  char *names, *dll;
  int dllindex;
  if (read16le(header + 6) != IMAGE_FILE_MACHINE || length < 3
      || length > size - 20)
    return cprime_error_noabort("invalid COFF import object");
  names = cprime_malloc(length);
  if (!pe_coff_read(fd, base, size, 20, names, length)
      || !(dll = memchr(names, 0, length))
      || ++dll >= names + length
      || !memchr(dll, 0, names + length - dll))
  {
    cprime_free(names);
    return cprime_error_noabort("invalid COFF import names");
  }
  if (((bits >> 2) & 7) > 1)
  {
    cprime_free(names);
    return cprime_error_noabort("unsupported COFF import name transformation");
  }
  dllindex = cprime_add_dllref(s1, dll, 0)->index;
  pe_putimport(s1, dllindex, names, ((bits >> 2) & 7) ? 0 : read16le(header + 16));
  cprime_free(names);
  return 0;
}

ST_FUNC void pe_parse_linker_directives(CPRIMEState *s1, char *directives)
{
  char *p = directives;
  while (*p)
  {
    char *token, *out;
    int quoted = 0;
    while (*p && (unsigned char)*p <= ' ') ++p;
    token = out = p;
    while (*p && (quoted || (unsigned char)*p > ' '))
    {
      if (*p == '"') { quoted = !quoted; ++p; }
      else *out++ = *p++;
    }
    if (*p) ++p;
    *out = 0;
    if (!strnicmp(token, "/defaultlib:", 12) || !strnicmp(token, "-defaultlib:", 12))
      cprime_add_pragma_library(s1, token + 12);
    else if (!strnicmp(token, "/include:", 9) || !strnicmp(token, "-include:", 9))
      set_elf_sym(symtab_section, 0, 0, Obj64_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0, SHN_UNDEF, token + 9);
  }
}

ST_FUNC void pe_store_pragma_libraries(CPRIMEState *s1)
{
  Section *section;
  int i;
  if (!s1->nb_pragma_libs)
    return;
  /* CPC's relocatable objects use ELF internally. Keep PE linker metadata
     in the same non-allocated section used by native COFF objects, so -c,
     archive extraction and -r preserve source-level library dependencies. */
  section = find_section(s1, ".drectve");
  section->sh_flags &= ~SHF_ALLOC;
  section->sh_addralign = 1;
  for (i = 0; i < s1->nb_pragma_libs; ++i)
  {
    const char *library = s1->pragma_libs[i];
    size_t length = strlen(library);
    char *out = section_ptr_add(section, length + 16);
    memcpy(out, " /DEFAULTLIB:\"", 14);
    memcpy(out + 14, library, length);
    out[14 + length] = '"';
    out[15 + length] = ' ';
  }
}

ST_FUNC int pe_load_coff_object(CPRIMEState *s1, int fd, unsigned long base)
{
  IMAGE_FILE_HEADER header;
  struct pe_coff_section *sections = NULL;
  struct pe_rsrc_symbol *symbols = NULL;
  int *symbol_map = NULL;
  char *strings = NULL;
  unsigned long size, end = lseek(fd, 0, SEEK_END), table_end;
  unsigned string_size = 0, i, j;
  int result = -1;

  if (base > end || end - base < sizeof header)
    goto invalid;
  size = end - base;
  if (!pe_coff_read(fd, base, size, 0, &header, sizeof header))
    goto invalid;
  if (!header.Machine && header.NumberOfSections == 0xffff)
    return pe_load_short_import(s1, fd, base, size, (BYTE *)&header);
  if (header.Machine != IMAGE_FILE_MACHINE || header.SizeOfOptionalHeader
      || !header.NumberOfSections || header.PointerToSymbolTable > size
      || header.NumberOfSymbols > (size - header.PointerToSymbolTable) / sizeof *symbols)
    goto invalid;
  table_end = header.PointerToSymbolTable + header.NumberOfSymbols * sizeof *symbols;
  if (!pe_coff_read(fd, base, size, table_end, &string_size, 4)
      || string_size < 4 || string_size > size - table_end)
    goto invalid;
  strings = cprime_malloc(string_size);
  symbols = cprime_malloc(header.NumberOfSymbols * sizeof *symbols);
  symbol_map = cprime_mallocz(header.NumberOfSymbols * sizeof *symbol_map);
  sections = cprime_mallocz(header.NumberOfSections * sizeof *sections);
  if (!pe_coff_read(fd, base, size, table_end, strings, string_size)
      || !pe_coff_read(fd, base, size, header.PointerToSymbolTable,
                      symbols, header.NumberOfSymbols * sizeof *symbols))
    goto invalid;
  for (i = 0; i < header.NumberOfSections; ++i)
  {
    IMAGE_SECTION_HEADER *h = &sections[i].header;
    if (!pe_coff_read(fd, base, size, sizeof header + i * sizeof *h, h, sizeof *h)
        || h->PointerToRawData > size
        || (h->PointerToRawData && h->SizeOfRawData > size - h->PointerToRawData)
        || h->PointerToRelocations > size
        || h->NumberOfRelocations > (size - h->PointerToRelocations) / sizeof(struct pe_rsrc_reloc))
      goto invalid;
  }
  /* Section definition auxiliaries carry COMDAT selection and association. */
  for (i = 0; i < header.NumberOfSymbols; i += 1 + symbols[i].auxiliary_count)
  {
    struct pe_rsrc_symbol *symbol = &symbols[i];
    if (symbol->auxiliary_count >= header.NumberOfSymbols - i)
      goto invalid;
    if (symbol->storage_class == 3 && symbol->auxiliary_count
        && symbol->section > 0 && symbol->section <= header.NumberOfSections)
    {
      BYTE *aux = (BYTE *)(symbol + 1);
      sections[symbol->section - 1].selection = aux[14];
      sections[symbol->section - 1].associated = read16le(aux + 12);
    }
  }
  /* Bind repeated COMDAT sections to the selected definition before copying
     data or publishing local section symbols and their relocations. */
  for (i = 0; i < header.NumberOfSymbols; i += 1 + symbols[i].auxiliary_count)
  {
    struct pe_rsrc_symbol *symbol = &symbols[i];
    struct pe_coff_section *part;
    ObjW(Sym) *previous;
    const char *name;
    char short_name[9];
    int old;
    if (symbol->storage_class != 2 || symbol->section <= 0
        || symbol->section > header.NumberOfSections)
      continue;
    part = &sections[symbol->section - 1];
    if (!(part->header.Characteristics & 0x1000))
      continue;
    name = pe_coff_name(symbol->name, strings, string_size, short_name);
    if (!name) goto invalid;
    old = find_elf_sym(symtab_section, name);
    if (!old) continue;
    previous = &((ObjW(Sym) *)symtab_section->data)[old];
    if (previous->st_shndx == SHN_UNDEF) continue;
    if (part->selection != 2 && part->selection != 3 && part->selection != 4)
    {
      cprime_error_noabort("unsupported or conflicting COFF COMDAT selection %d for '%s'", part->selection, name);
      goto done;
    }
    if (previous->st_shndx >= s1->nb_sections || previous->st_value < symbol->value)
      goto invalid;
    part->section = s1->sections[previous->st_shndx];
    part->offset = previous->st_value - symbol->value;
    if ((part->selection == 3 || part->selection == 4)
        && previous->st_size != part->header.SizeOfRawData)
    {
      cprime_error_noabort("COFF COMDAT size mismatch for '%s'", name);
      goto done;
    }
    if (part->selection == 4 && part->header.SizeOfRawData)
    {
      BYTE *compare = cprime_malloc(part->header.SizeOfRawData);
      int equal = pe_coff_read(fd, base, size, part->header.PointerToRawData,
                              compare, part->header.SizeOfRawData)
          && !memcmp(compare, part->section->data + part->offset, part->header.SizeOfRawData);
      cprime_free(compare);
      if (!equal) { cprime_error_noabort("COFF COMDAT contents mismatch for '%s'", name); goto done; }
    }
    part->duplicate = 1;
  }
  for (i = 0; i < header.NumberOfSections; ++i)
  {
    struct pe_coff_section *part = &sections[i];
    IMAGE_SECTION_HEADER *h = &part->header;
    char short_name[9], *output_name, *suffix;
    const char *name;
    unsigned align_bits = (h->Characteristics >> 20) & 15;
    unsigned align = align_bits ? 1U << (align_bits - 1) : 16;
    int flags = SHF_ALLOC, type = SHT_PROGBITS;
    if (part->duplicate) continue;
    if (part->selection == 5)
    {
      if (!part->associated || part->associated > header.NumberOfSections) goto invalid;
      if (sections[part->associated - 1].duplicate) { part->duplicate = 1; continue; }
    }
    memcpy(short_name, h->Name, 8); short_name[8] = 0;
    name = short_name;
    if (*name == '/')
    {
      unsigned name_offset = strtoul(name + 1, NULL, 10);
      if (name_offset < 4 || name_offset >= string_size
          || !memchr(strings + name_offset, 0, string_size - name_offset)) goto invalid;
      name = strings + name_offset;
    }
    if (!strcmp(name, ".drectve") && h->SizeOfRawData)
    {
      char *directives = cprime_malloc(h->SizeOfRawData + 1);
      if (!pe_coff_read(fd, base, size, h->PointerToRawData, directives, h->SizeOfRawData))
      { cprime_free(directives); goto invalid; }
      directives[h->SizeOfRawData] = 0;
      pe_parse_linker_directives(s1, directives);
      cprime_free(directives);
    }
    /* Linker directives and debug information have no mapped runtime data. */
    if (!(h->Characteristics & 0xe00000e0) || (h->Characteristics & 0x800)
        || !strncmp(name, ".debug", 6))
      continue;
    if (h->Characteristics & 0x80) type = SHT_NOBITS;
    if (h->Characteristics & 0x80000000U) flags |= SHF_WRITE;
    if (h->Characteristics & 0x20000000U) flags |= SHF_EXECINSTR;
    /* COFF subsection suffixes share the corresponding output section. */
    output_name = cprime_strdup(name);
    suffix = strchr(output_name, '$');
    if (suffix) *suffix = 0;
    part->section = have_section(s1, output_name);
    if (!part->section)
    {
      part->section = new_section(s1, output_name, type, flags);
      /* Preserve input alignment through -r. The generic section default
         is pointer alignment, which would insert padding between packed
         records such as the 12-byte .pdata entries on a later link. */
      part->section->sh_addralign = align;
    }
    cprime_free(output_name);
    part->offset = section_add(part->section, h->SizeOfRawData, align);
    if (type != SHT_NOBITS && h->SizeOfRawData
        && !pe_coff_read(fd, base, size, h->PointerToRawData,
                         part->section->data + part->offset, h->SizeOfRawData))
      goto invalid;
  }
  for (i = 0; i < header.NumberOfSymbols; i += 1 + symbols[i].auxiliary_count)
  {
    struct pe_rsrc_symbol *symbol = &symbols[i];
    const char *name;
    char short_name[9];
    unsigned long value = symbol->value, symbol_size = 0;
    int section = SHN_UNDEF, bind = symbol->storage_class == 2 ? STB_GLOBAL : STB_LOCAL;
    int type = symbol->type & 0x20 ? STT_FUNC : STT_NOTYPE;
    /* Native unwind tables can relocate against local code labels, not
       just function and section symbols. IMAGE_SYM_CLASS_LABEL has ordinary
       section-relative value semantics and must remain a local symbol. */
    if (symbol->storage_class != 2 && symbol->storage_class != 3
        && symbol->storage_class != 6)
      continue;
    name = pe_coff_name(symbol->name, strings, string_size, short_name);
    if (!name) goto invalid;
    if (symbol->section > 0)
    {
      struct pe_coff_section *part;
      if (symbol->section > header.NumberOfSections) goto invalid;
      part = &sections[symbol->section - 1];
      if (!part->section) continue;
      section = part->section->sh_num;
      value += part->offset;
      if (part->header.Characteristics & 0x1000)
      {
        symbol_size = part->header.SizeOfRawData;
        if (bind == STB_GLOBAL) bind = STB_WEAK;
      }
    }
    else if (symbol->section == -1) section = SHN_ABS;
    else if (symbol->section < 0) continue;
    else if (value) { section = SHN_COMMON; symbol_size = value; value = 16; }
    symbol_map[i] = set_elf_sym(symtab_section, value, symbol_size,
                               Obj64_ST_INFO(bind, type), 0, section, name);
  }
  for (i = 0; i < header.NumberOfSections; ++i)
  {
    struct pe_coff_section *part = &sections[i];
    IMAGE_SECTION_HEADER *h = &part->header;
    if (!part->section || part->duplicate) continue;
    for (j = 0; j < h->NumberOfRelocations; ++j)
    {
      struct pe_rsrc_reloc rel;
      unsigned width = 4;
      int type;
      long long addend = 0;
      if (!pe_coff_read(fd, base, size, h->PointerToRelocations + j * sizeof rel, &rel, sizeof rel)) goto invalid;
      if (!rel.type) continue;
      if (rel.type == 1) { type = R_X86_64_64; width = 8; }
      else if (rel.type == 2) type = R_X86_64_32;
      else if (rel.type == 3) type = R_X86_64_RELATIVE;
      else if (rel.type >= 4 && rel.type <= 9) { type = R_X86_64_PC32; addend = -(long long)rel.type; }
      else { cprime_error_noabort("unsupported AMD64 COFF relocation %d", rel.type); goto done; }
      if (rel.offset > h->SizeOfRawData || width > h->SizeOfRawData - rel.offset
          || rel.symbol_index >= header.NumberOfSymbols || !symbol_map[rel.symbol_index]) goto invalid;
      put_elf_reloca(symtab_section, part->section, part->offset + rel.offset,
                     type, symbol_map[rel.symbol_index], addend);
    }
  }
  result = 0;
  goto done;
invalid:
  cprime_error_noabort("invalid AMD64 COFF object");
done:
  cprime_free(sections);
  cprime_free(symbols);
  cprime_free(symbol_map);
  cprime_free(strings);
  return result;
}
