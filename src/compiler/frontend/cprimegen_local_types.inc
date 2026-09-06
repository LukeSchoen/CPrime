/* Local class identity survives semantic deduction and later body emission. */
typedef struct LocalTypeReplay {
  int type_tok, end_index;
  struct LocalTypeReplay *next;
} LocalTypeReplay;
typedef struct LocalTypeFunction {
  int function_tok, owner_tok;
  struct LocalTypeFunction *next;
} LocalTypeFunction;
static LocalTypeReplay *local_type_replays[256];
static LocalTypeFunction *local_type_functions[256];
static LocalTypeReplay *local_type_replay_for(int tok)
{
  unsigned bucket = (unsigned)tok & 255;
  LocalTypeReplay *entry;
  for (entry = local_type_replays[bucket]; entry; entry = entry->next)
    if (entry->type_tok == tok) return entry;
  entry = cprime_mallocz(sizeof(*entry));
  entry->type_tok = tok;
  entry->next = local_type_replays[bucket];
  local_type_replays[bucket] = entry;
  return entry;
}
static int local_type_function_owner(int tok)
{
  LocalTypeFunction *entry;
  for (entry = local_type_functions[(unsigned)tok & 255]; entry; entry = entry->next)
    if (entry->function_tok == tok) return entry->owner_tok;
  return tok;
}
static void note_local_type_function_owner(int tok, int owner)
{
  unsigned bucket = (unsigned)tok & 255;
  LocalTypeFunction *entry;
  if (!owner) return;
  for (entry = local_type_functions[bucket]; entry; entry = entry->next)
    if (entry->function_tok == tok) {
      entry->owner_tok = owner;
      return;
    }
  entry = cprime_mallocz(sizeof(*entry));
  entry->function_tok = tok;
  entry->owner_tok = owner;
  entry->next = local_type_functions[bucket];
  local_type_functions[bucket] = entry;
}
static void free_local_type_replay(void)
{
  int i;
  for (i = 0; i < 256; ++i) {
    while (local_type_replays[i]) {
      LocalTypeReplay *entry = local_type_replays[i];
      local_type_replays[i] = entry->next;
      cprime_free(entry);
    }
    while (local_type_functions[i]) {
      LocalTypeFunction *entry = local_type_functions[i];
      local_type_functions[i] = entry->next;
      cprime_free(entry);
    }
  }
  local_type_owner_tok = local_type_declaration_index = 0;
}
