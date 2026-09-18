/* Bounded, single-function optimization for the direct x64 emitter.
   Unsupported encodings keep their original code. The pass caps function
   size, stack slots, inline expansion, and liveness iterations. Scratch
   arrays are reused by the serial emitter; every analyzed entry is rebuilt. */
#ifdef CPRIME_TARGET_PE
#define X64_FAST_BYTES 4096
#define X64_FAST_INSNS 1024
#define X64_FAST_SLOTS 64
/* Loop heads line up with the decoder's 16-byte fetch window: the measured
   phase curve for a compacted loop never enters its slow bands at a 16-byte
   boundary, while the compacted positions that the folds below produce can. */
#define X64_FAST_ALIGN 16
#define X64_FAST_CPP_PHASE 3

typedef struct X64FastInsn {
  int off, len, out, size, modrm, rex, op, op2, imm, branch;
  int base, reg, disp, width, slot;
} X64FastInsn;
typedef struct X64FastSlot { int disp, width, extent, count, reg; } X64FastSlot;
typedef struct X64FastInline {
  struct X64FastInline *next;
  int symbol, size, nrel;
  unsigned char code[192];
  ObjW_Rel rel[16];
} X64FastInline;
static X64FastInline *x64_fast_inline[1024];
static Sym *x64_fast_sym;
static int x64_fast_disabled;
static unsigned long x64_fast_reloc_start;
static int x64_fast_frame, x64_fast_saved, x64_fast_compacted;

ST_FUNC void x64_fast_disable(void) { x64_fast_disabled = 1; }

ST_FUNC void x64_fast_reset(void)
{
  int i;
  for (i = 0; i < 1024; ++i) {
    X64FastInline *p = x64_fast_inline[i];
    while (p) { X64FastInline *next = p->next; cprime_free(p); p = next; }
    x64_fast_inline[i] = NULL;
  }
  x64_fast_sym = NULL;
}

/* Only the emitter's ordinary integer instruction subset is decoded. */
static int x64_fast_decode(const unsigned char *p, int n, X64FastInsn *d,
                           unsigned *used)
{
  int i = 0, op, mod = 0, rm = 0, hasmod = 0, prefix = 0;
  memset(d, 0, sizeof(*d));
  d->base = d->slot = -1;
  if (i < n && p[i] == 0x66) { prefix = 1; ++i; }
  if (i < n && (p[i] & 0xf0) == 0x40) d->rex = p[i++];
  if (i >= n) return 0;
  op = d->op = p[i++];
  d->width = (d->rex & 8) ? 8 : prefix ? 2 : 4;
  if (op == 0x0f) {
    if (i >= n) return 0;
    d->op2 = p[i++];
    if (d->op2 >= 0x80 && d->op2 <= 0x8f) d->branch = 4;
    else if (d->op2 == 0xaf || d->op2 == 0xb6 || d->op2 == 0xb7
             || d->op2 == 0xbe || d->op2 == 0xbf
             || (d->op2 >= 0x90 && d->op2 <= 0x9f)) hasmod = 1;
    else return 0;
  } else if (op == 0xe8) d->imm = 4;
  else if (op == 0xe9) d->branch = 4;
  else if (op == 0xeb || (op >= 0x70 && op <= 0x7f)) d->branch = 1;
  else if (op >= 0xb8 && op <= 0xbf) {
    *used |= 1u << ((op & 7) + ((d->rex & 1) << 3));
    d->imm = d->width;
  } else if (op == 0x90 || op == 0x99) {
    if (op == 0x99) *used |= 5;
  } else if ((op <= 0x3b && (op & 7) <= 3)
             || op == 0x63 || op == 0x69 || op == 0x6b
             || op == 0x80 || op == 0x81 || op == 0x83
             || op == 0x84 || op == 0x85 || op == 0x88 || op == 0x89
             || op == 0x8a || op == 0x8b || op == 0x8d
             || op == 0xc0 || op == 0xc1 || op == 0xc6 || op == 0xc7
             || op == 0xd1 || op == 0xd3 || op == 0xf6 || op == 0xf7 || op == 0xff)
    hasmod = 1;
  else if (op <= 0x3d && (op & 7) == 5) {
    *used |= 1; d->imm = prefix ? 2 : 4;
  } else return 0; /* includes calls, indirect jumps, stack/asm instructions */
  if (hasmod) {
    if (i >= n) return 0;
    d->modrm = i;
    mod = p[i] >> 6; rm = p[i] & 7;
    d->reg = ((p[i++] >> 3) & 7) + ((d->rex & 4) << 1);
    if (op == 0xff && (d->reg & 7) != 2) return 0;
    *used |= 1u << d->reg; /* conservative for opcode extensions */
    if (mod == 3) *used |= 1u << (rm + ((d->rex & 1) << 3));
    else {
      d->base = rm + ((d->rex & 1) << 3);
      if (rm == 4) {
        int sib;
        if (i >= n) return 0;
        sib = p[i++];
        *used |= 1u << (((sib >> 3) & 7) + ((d->rex & 2) << 2));
        d->base = (sib & 7) + ((d->rex & 1) << 3);
        if (d->base == 5) return 0;
      }
      if (mod == 0 && rm == 5) d->base = -2;
      else *used |= 1u << d->base;
      if (mod == 1) { if (i >= n) return 0; d->disp = (signed char)p[i++]; }
      else if (mod == 2 || (mod == 0 && rm == 5)) {
        if (i + 4 > n) return 0;
        d->disp = read32le(p + i); i += 4;
      }
    }
    if (op == 0x80 || op == 0x83 || op == 0xc0 || op == 0xc1
        || op == 0xc6 || op == 0x6b) d->imm = 1;
    else if (op == 0x81 || op == 0xc7 || op == 0x69) d->imm = prefix ? 2 : 4;
    else if ((op == 0xf6 || op == 0xf7) && (d->reg & 7) == 0)
      d->imm = op == 0xf6 ? 1 : prefix ? 2 : 4;
    if (op == 0xd3) *used |= 2;
    if (op == 0xf6 || op == 0xf7) *used |= 5;
  }
  i += d->imm + d->branch;
  if (i > n) return 0;
  d->len = d->size = i;
  return 1;
}

static int x64_fast_map(X64FastInsn *ins, int count, int old, int end)
{
  int lo = 0, hi = count;
  if (old == end) return ins[count].out;
  while (lo < hi) {
    int mid = (lo + hi) / 2;
    if (ins[mid].off < old) lo = mid + 1; else hi = mid;
  }
  return lo < count && ins[lo].off == old ? ins[lo].out : -1;
}

/* Whether this instruction overwrites both halves of reg.  A 32-bit self
   move can be removed only when its zero-extension of the upper half is
   overwritten before that half can be observed. */
static int x64_fast_writes_reg_full(X64FastInsn *d, unsigned char *p, int reg)
{
  int mod, rm;
  if (!d->size || d->width < 4 || d->branch) return 0;
  if (!d->modrm) {
    return d->op >= 0xb8 && d->op <= 0xbf
           && ((d->op & 7) + ((d->rex & 1) << 3)) == reg;
  }
  mod = p[d->modrm] >> 6;
  rm = (p[d->modrm] & 7) + ((d->rex & 1) << 3);
  if (mod != 3) return 0;
  if ((d->op == 0x81 || d->op == 0x83) && (d->reg & 7) != 7)
    return rm == reg;
  if (d->op == 0x01 || d->op == 0x29 || d->op == 0x09 || d->op == 0x21
      || d->op == 0x31 || d->op == 0xc1 || d->op == 0xd1 || d->op == 0xd3
      || d->op == 0x89 || d->op == 0xc7)
    return rm == reg;
  if (d->op == 0x8b || d->op == 0x63 || d->op == 0x69 || d->op == 0x6b)
    return d->reg == reg;
  if (d->op == 0x0f && (d->op2 == 0xb6 || d->op2 == 0xb7
                        || d->op2 == 0xbe || d->op2 == 0xbf))
    return d->reg == reg;
  return 0;
}

/* Register-only copy propagation, bounded liveness, and the size-oriented
   short-encoding layout. Copy facts reset at control-flow joins. Memory
   operations and arithmetic retain their original ordering. */
static int x64_fast_simplify(unsigned char *code, int size, ObjW_Rel *rel,
                             int nrel, int align_base, int allow_cpp_size)
{
  static X64FastInsn ins[X64_FAST_INSNS + 1];
  static unsigned char rewritten[X64_FAST_INSNS + 1][16], result[X64_FAST_BYTES];
  static unsigned char short_saved[X64_FAST_INSNS + 1][2];
  static unsigned char short_used[X64_FAST_INSNS + 1];
  static unsigned char self_move_info[X64_FAST_INSNS + 1];
  static unsigned reads[X64_FAST_INSNS + 1], writes[X64_FAST_INSNS + 1], live[X64_FAST_INSNS + 1];
  static int target[X64_FAST_INSNS + 1], labels[X64_FAST_INSNS + 1];
  int alias[16], alias_width[16];
  unsigned used = 0;
  int n = 0, pos = 0, i, j, k, iteration, out = 0;
  int layout_c, layout_j, layout_b, layout_x, layout_i0, layout_h, layout_k, layout_pos;
  static int layout_out[X64_FAST_INSNS + 1];
  memset(labels, 0, sizeof(labels));
  while (pos < size) {
    if (n == X64_FAST_INSNS || !x64_fast_decode(code + pos, size - pos, &ins[n], &used)) return size;
    ins[n].off = ins[n].out = pos;
    pos += ins[n++].len;
  }
  ins[n].out = size;
  for (i = 0; i < n; ++i) {
    X64FastInsn *d = &ins[i];
    target[i] = -1;
    if (d->branch) {
      int delta = d->branch == 1 ? (signed char)code[d->off + d->len - 1]
                                : (int)read32le(code + d->off + d->len - 4);
      int at = d->off + d->len + delta;
      for (j = 0; j < n && ins[j].off != at; ++j) {}
      if (j == n && at != size) return size;
      target[i] = j; labels[j] = 1;
    }
  }
  for (j = 0; j < 16; ++j) alias[j] = -1;
  for (i = 0; i < n; ++i) {
    X64FastInsn *d = &ins[i];
    unsigned char *p = rewritten[i];
    int mod, rm, src = -1, dst = -1, source_field = 0, source_width = d->width;
    unsigned rd = 0, wr = 0;
    memcpy(p, code + d->off, d->len);
    if (labels[i]) for (j = 0; j < 16; ++j) alias[j] = -1;
    mod = d->modrm ? p[d->modrm] >> 6 : -1;
    rm = d->modrm ? (p[d->modrm] & 7) + ((d->rex & 1) << 3) : -1;
    if (d->base >= 0 && d->base != 4 && (p[d->modrm] & 7) != 4) {
      src = d->base; source_width = 8; source_field = 1;
    } else if (mod == 3 && d->op == 0x89 && d->width >= 4) {
      src = d->reg; source_field = 2;
    } else if (mod == 3 && d->op == 0x8b && d->width >= 4) {
      src = rm; source_field = 1;
    }
    if (src >= 0 && alias[src] >= 0 && alias_width[src] >= source_width) {
      int replacement = alias[src], rex = d->rex ? d->rex : 0x40;
      int rexpos = p[0] == 0x66 ? 1 : 0;
      if ((replacement & 7) != 4 && !(mod == 0 && (replacement & 7) == 5)) {
        if (source_field == 1) {
          p[d->modrm] = (p[d->modrm] & ~7) | (replacement & 7);
          rex = (rex & ~1) | (replacement >> 3);
        } else {
          p[d->modrm] = (p[d->modrm] & ~56) | ((replacement & 7) << 3);
          rex = (rex & ~4) | ((replacement >> 3) << 2);
        }
        if (d->rex) p[rexpos] = rex;
        else if (rex != 0x40) {
          memmove(p + rexpos + 1, p + rexpos, d->size - rexpos);
          p[rexpos] = rex; ++d->size;
        }
      }
    }
    {
      X64FastInsn rewritten_ins;
      self_move_info[i] = 0;
      if (!x64_fast_decode(p, d->size, &rewritten_ins, &used)) return size;
      mod = rewritten_ins.modrm ? p[rewritten_ins.modrm] >> 6 : -1;
      rm = rewritten_ins.modrm ? (p[rewritten_ins.modrm] & 7) + ((rewritten_ins.rex & 1) << 3) : -1;
      src = -1;
      if (mod == 3 && rewritten_ins.op == 0x89) { src = rewritten_ins.reg; dst = rm; }
      else if (mod == 3 && rewritten_ins.op == 0x8b) { src = rm; dst = rewritten_ins.reg; }
      /* Conservative read/write classification suffices: only register MOVs
         are deleted. Unknown effects keep every register live. */
      rd = wr = 0;
      if (src >= 0 && rewritten_ins.width >= 4) { rd = 1u << src; wr = 1u << dst; }
      else {
        unsigned address = 0, operand;
        if (rewritten_ins.base >= 0) address |= 1u << rewritten_ins.base;
        if (mod >= 0 && mod != 3 && (p[rewritten_ins.modrm] & 7) == 4) {
          int sib = p[rewritten_ins.modrm + 1];
          int index = ((sib >> 3) & 7) + ((rewritten_ins.rex & 2) << 2);
          if (index != 4) address |= 1u << index;
        }
        operand = mod == 3 ? 1u << rm : address;
        rd = 0xffff;
        if (mod == 3 && (rewritten_ins.op == 0xc7
            || (rewritten_ins.op == 0x31 && rewritten_ins.reg == rm))) { wr = 1u << rm; rd = 0; }
        else if (rewritten_ins.op >= 0xb8 && rewritten_ins.op <= 0xbf) {
          wr = 1u << ((rewritten_ins.op & 7) + ((rewritten_ins.rex & 1) << 3));
          rd = 0;
        }
        else if (rewritten_ins.op == 0x8b || rewritten_ins.op == 0x63
                 || (rewritten_ins.op == 0x0f && (rewritten_ins.op2 == 0xb6
                 || rewritten_ins.op2 == 0xb7 || rewritten_ins.op2 == 0xbe
                 || rewritten_ins.op2 == 0xbf))) { wr = 1u << rewritten_ins.reg; rd = operand; }
        else if (rewritten_ins.op == 0x89 && mod != 3) rd = address | (1u << rewritten_ins.reg);
        else if (mod == 3 && (rewritten_ins.op == 0x81 || rewritten_ins.op == 0x83)) {
          rd = operand;
          if ((rewritten_ins.reg & 7) != 7) wr = operand;
        } else if (mod == 3 && rewritten_ins.op <= 0x3b && (rewritten_ins.op & 7) <= 3) {
          rd = operand | (1u << rewritten_ins.reg);
          if (rewritten_ins.op < 0x38) wr = (rewritten_ins.op & 2) ? 1u << rewritten_ins.reg : operand;
        } else if (mod == 3 && (rewritten_ins.op == 0xc1 || rewritten_ins.op == 0xd1
                                || rewritten_ins.op == 0xd3)) {
          rd = operand | (rewritten_ins.op == 0xd3 ? 2 : 0); wr = operand;
        } else if (rewritten_ins.op == 0x85 || rewritten_ins.op == 0x84) {
          rd = operand | (1u << rewritten_ins.reg);
        } else if (rewritten_ins.op == 0x69 || rewritten_ins.op == 0x6b) {
          rd = operand; wr = 1u << rewritten_ins.reg;
        } else if (mod == 3 && rewritten_ins.op == 0x0f && rewritten_ins.op2 >= 0x90
                    && rewritten_ins.op2 <= 0x9f) { rd = wr = operand; }
        else if (rewritten_ins.op == 0xe8 || rewritten_ins.op == 0xff) {
          rd = 0x306 | (rewritten_ins.op == 0xff ? operand : 0);
          wr = 0xf07;
        } else if (rewritten_ins.op == 0x90 || d->branch) rd = 0;
        if (rewritten_ins.width < 4) rd |= wr;
        /* For copy propagation, invalidate everything for operations whose
           destination or implicit clobbers aren't modeled. */
        if (!wr && rd == 0xffff && !d->branch && rewritten_ins.op != 0x90)
          for (j = 0; j < 16; ++j) alias[j] = -1;
      }
      for (j = 0; j < 16; ++j)
        if ((wr & (1u << j)) || (alias[j] >= 0 && (wr & (1u << alias[j])))) alias[j] = -1;
      if (src >= 0 && dst != src && rewritten_ins.width >= 4) {
        alias[dst] = src; alias_width[dst] = rewritten_ins.width;
      }
      if (src >= 0 && src == dst && rewritten_ins.width >= 4)
        self_move_info[i] = (unsigned char)(1 + src
            + (rewritten_ins.width == 8 ? 32 : 0));
      if (d->branch) for (j = 0; j < 16; ++j) alias[j] = -1;
      reads[i] = rd; writes[i] = wr; live[i] = 0;
    }
  }
  live[n] = 0xf0f9; /* rax and ABI-preserved registers */
  for (iteration = 0; iteration < 32; ++iteration) {
    int changed = 0;
    for (i = n - 1; i >= 0; --i) {
      unsigned after = live[i + 1], before;
      if (target[i] >= 0) {
        if (ins[i].op == 0xe9 || ins[i].op == 0xeb) after = 0;
        after |= live[target[i]];
      }
      before = reads[i] | (after & ~writes[i]);
      if (before != live[i]) { live[i] = before; changed = 1; }
    }
    if (!changed) break;
  }
  if (iteration == 32) return size;
  /* At -O2, fold the common stack-slot promotion shape

       mov a, b; op a, c; mov b, a

     into `op b, c` when a is otherwise dead.  The value of a is exactly the
     value of b at the first instruction, so the arithmetic and its flags are
     unchanged while the two moves disappear. */
  if (cprime_state->opt_level == 2 && !is_cpp_translation_unit())
  for (i = 0; i + 2 < n; ++i) {
    X64FastInsn first, middle, last;
    unsigned local_used = 0;
    int first_mod, middle_mod, last_mod, a, b, c, k, reloc_safe = 1;
    if (labels[i + 1] || labels[i + 2]
        || !ins[i].size || !ins[i + 1].size || !ins[i + 2].size) continue;
    if (!x64_fast_decode(rewritten[i], ins[i].size, &first, &local_used)
        || !x64_fast_decode(rewritten[i + 1], ins[i + 1].size, &middle,
                            &local_used)
        || !x64_fast_decode(rewritten[i + 2], ins[i + 2].size, &last,
                            &local_used)) continue;
    first_mod = first.modrm ? rewritten[i][first.modrm] >> 6 : -1;
    if (first_mod != 3 || first.width < 4) continue;
    if (first.op == 0x89) {
      a = (rewritten[i][first.modrm] & 7) + ((first.rex & 1) << 3);
      b = first.reg;
    } else if (first.op == 0x8b) {
      a = first.reg;
      b = (rewritten[i][first.modrm] & 7) + ((first.rex & 1) << 3);
    } else continue;
    middle_mod = middle.modrm ? rewritten[i + 1][middle.modrm] >> 6 : -1;
    if (middle_mod != 3 || middle.width != first.width
        || (middle.op != 0x01 && middle.op != 0x29
            && middle.op != 0x09 && middle.op != 0x21
            && middle.op != 0x31)) continue;
    if (((rewritten[i + 1][middle.modrm] & 7) + ((middle.rex & 1) << 3)) != a)
      continue;
    c = middle.reg;
    last_mod = last.modrm ? rewritten[i + 2][last.modrm] >> 6 : -1;
    if (last_mod != 3 || last.width != first.width) continue;
    if (last.op == 0x89) {
      if (((rewritten[i + 2][last.modrm] & 7) + ((last.rex & 1) << 3)) != b
          || last.reg != a) continue;
    } else if (last.op == 0x8b) {
      if (last.reg != b
          || ((rewritten[i + 2][last.modrm] & 7) + ((last.rex & 1) << 3)) != a)
        continue;
    } else continue;
    if (a == b || c == a || a == 4 || a == 5 || b == 4 || b == 5
        || c == 4 || c == 5 || (live[i + 3] & (1u << a))) continue;
    for (k = 0; k < nrel; ++k)
      if ((rel[k].r_offset >= ins[i].out
           && rel[k].r_offset < ins[i].out + ins[i].size)
          || (rel[k].r_offset >= ins[i + 1].out
              && rel[k].r_offset < ins[i + 1].out + ins[i + 1].size)
          || (rel[k].r_offset >= ins[i + 2].out
              && rel[k].r_offset < ins[i + 2].out + ins[i + 2].size)) {
        reloc_safe = 0; break;
      }
    if (!reloc_safe) continue;
    {
      unsigned char *p = rewritten[i + 1];
      int pos = 0;
      int rex = 0x40 | (middle.width == 8 ? 8 : 0)
                | ((c >> 3) << 2) | (b >> 3);
      if (rex != 0x40) p[pos++] = rex;
      p[pos++] = middle.op;
      p[pos++] = 0xc0 | ((c & 7) << 3) | (b & 7);
      ins[i + 1].size = pos;
      ins[i].size = 0;
      ins[i + 2].size = 0;
    }
  }
  for (i = 0; i + 3 < n; ++i) {
    X64FastInsn *set = &ins[i], *extend = &ins[i + 1], *test = &ins[i + 2], *jump = &ins[i + 3];
    int reg, jump_zero;
    unsigned after;
    if (set->op != 0x0f || set->op2 < 0x90 || set->op2 > 0x9f
        || extend->op != 0x0f || extend->op2 != 0xb6 || test->op != 0x85
        || jump->op != 0x0f || (jump->op2 != 0x84 && jump->op2 != 0x85)
        || labels[i + 1] || labels[i + 2] || labels[i + 3]) continue;
    reg = (code[set->off + set->modrm] & 7) + ((set->rex & 1) << 3);
    if (reg > 2 || code[set->off + set->modrm] != 0xc0 + reg
        || code[extend->off + extend->modrm] != 0xc0 + reg * 9
        || code[test->off + test->modrm] != 0xc0 + reg * 9) continue;
    after = live[i + 4] | live[target[i + 3]];
    if (after & (1u << reg)) continue;
    jump_zero = jump->op2 == 0x84;
    rewritten[i + 3][1] = (set->op2 - 0x10) ^ jump_zero;
    set->size = extend->size = test->size = 0;
  }
  for (i = 0; i < n; ++i) {
    X64FastInsn *d = &ins[i];
    int original_mod = d->modrm ? code[d->off + d->modrm] >> 6 : -1;
    int self_move = 0;
    if (d->size && self_move_info[i]) {
      int self_reg = (self_move_info[i] - 1) & 15;
      if (self_move_info[i] & 32) self_move = 1;
      else {
        for (j = i + 1; j < n; ++j)
          if (ins[j].size) {
            self_move = x64_fast_writes_reg_full(
                &ins[j], rewritten[j], self_reg);
            break;
          }
      }
    }
    if (original_mod == 3 && (d->op == 0x89 || d->op == 0x8b)
        && d->width >= 4
        && (self_move || !(writes[i] & live[i + 1]))) d->size = 0;
    d->out = out; out += d->size;
  }
  /* At -Os, keep the compact ordered layout and make the instructions that
     remain smaller where the existing encoding set offers an equivalent
     sign-extended form:

       op r/m, imm32  -> op r/m, imm8   (0x81 -> 0x83)
       imul r/m, imm32 -> imul r/m, imm8 (0x69 -> 0x6b)
       jmp rel32      -> jmp rel8       (0xe9 -> 0xeb)
       jcc rel32      -> jcc rel8       (0x0f 8x -> 0x7x)

     The short branches are tried as one bounded batch: their final
     displacements depend on the other shrinks, so the batch is kept only
     when every retargeted branch still fits its signed byte.  The -O2
     branch exchange and loop rotation stay out; the one inner-loop head pad
     below is bounded by the bytes this shrink removed. */
  if (cprime_state->opt_level == 3
      && (!is_cpp_translation_unit() || allow_cpp_size)) {
    int sized = 0, branches = 0, ok = 1;
    memset(short_used, 0, sizeof(short_used));
    for (i = 0; i < n; ++i) {
      X64FastInsn *d = &ins[i];
      X64FastInsn t;
      unsigned local_used = 0;
      int op_pos, imm_at, value;
      if (!d->size || d->size != d->len || !d->modrm
          || (d->op != 0x81 && d->op != 0x69)) continue;
      if (!x64_fast_decode(rewritten[i], d->size, &t, &local_used)
          || t.op != d->op || !t.modrm || !t.imm) continue;
      for (k = 0; k < nrel; ++k)
        if (rel[k].r_offset >= d->off && rel[k].r_offset < d->off + d->len)
          break;
      if (k < nrel) continue;
      imm_at = t.len - t.imm;
      if (t.imm == 4) value = (int)read32le(rewritten[i] + imm_at);
      else if (t.imm == 2) value = (short)read16le(rewritten[i] + imm_at);
      else continue;
      if (value < -128 || value > 127) continue;
      op_pos = t.modrm - 1;
      if (op_pos < 0 || op_pos >= t.len) continue;
      rewritten[i][op_pos] = d->op == 0x81 ? 0x83 : 0x6b;
      d->size = t.len - (t.imm - 1);
      ++sized;
    }
    if (sized) {
      out = 0;
      for (i = 0; i < n; ++i) {
        ins[i].out = out;
        out += ins[i].size;
      }
      ins[n].out = out;
    }
    for (i = 0; i < n; ++i) {
      X64FastInsn *d = &ins[i];
      int op;
      if (d->branch != 4 || !d->size || d->size != d->len
          || d->len <= 2 || target[i] < 0) continue;
      for (k = 0; k < nrel; ++k)
        if (rel[k].r_offset >= d->off && rel[k].r_offset < d->off + d->len)
          break;
      if (k < nrel) continue;
      if (d->op == 0xe9) op = 0xeb;
      else if (d->op == 0x0f && d->op2 >= 0x80 && d->op2 <= 0x8f)
        op = 0x70 | (d->op2 & 0x0f);
      else continue;
      short_saved[i][0] = rewritten[i][0];
      short_saved[i][1] = rewritten[i][1];
      rewritten[i][0] = op;
      d->branch = 1;
      d->size = 2;
      short_used[i] = 1;
      ++branches;
    }
    if (branches) {
      out = 0;
      for (i = 0; i < n; ++i) {
        ins[i].out = out;
        out += ins[i].size;
      }
      ins[n].out = out;
      for (i = 0; i < n; ++i) {
        X64FastInsn *d = &ins[i];
        int delta;
        if (!short_used[i]) continue;
        delta = ins[target[i]].out - d->out - d->size;
        if (delta < -128 || delta > 127) {
          ok = 0;
          break;
        }
      }
      if (!ok) {
        for (i = 0; i < n; ++i) {
          X64FastInsn *d = &ins[i];
          if (!short_used[i]) continue;
          rewritten[i][0] = short_saved[i][0];
          rewritten[i][1] = short_saved[i][1];
          d->branch = 4;
          d->size = d->len;
        }
        out = 0;
        for (i = 0; i < n; ++i) {
          ins[i].out = out;
          out += ins[i].size;
        }
        ins[n].out = out;
      }
    }
  }
  /* At -O2, turn

       jcc T1; jmp T2

     into the equivalent `j!cc T2; jmp T1`.  The common fall-through path
     reaches T2 through one conditional branch instead of a conditional plus
     an unconditional jump; T1 keeps the remaining jump.  T1 and T2 are the
     same two successors as before, so liveness is unchanged. */
  if (cprime_state->opt_level == 2 && !is_cpp_translation_unit())
  for (i = 0; i + 1 < n; ++i) {
    X64FastInsn *conditional = &ins[i], *jump = &ins[i + 1];
    int old_target, k, reloc_safe = 1;
    if (conditional->op != 0x0f || conditional->op2 < 0x80
        || conditional->op2 > 0x8f || jump->op != 0xe9
        || !conditional->size || !jump->size || labels[i + 1]
        || target[i] < 0 || target[i + 1] < 0
        || target[i] == i || target[i] == i + 1
        || target[i] == i + 2
        || target[i + 1] == i || target[i + 1] == i + 1) continue;
    for (k = 0; k < nrel; ++k)
      if ((rel[k].r_offset >= ins[i].off
           && rel[k].r_offset < ins[i].off + ins[i].len)
          || (rel[k].r_offset >= ins[i + 1].off
              && rel[k].r_offset < ins[i + 1].off + ins[i + 1].len)) {
        reloc_safe = 0; break;
      }
    if (!reloc_safe) continue;
    rewritten[i][1] ^= 1;
    old_target = target[i];
    target[i] = target[i + 1];
    target[i + 1] = old_target;
  }
  /* At -O2, rotate the innermost for-loop shape after the branch exchange:
     head -> body -> increment -> head currently takes three branches.  Put
     the existing body next to the head so its trip test falls through, keep
     the increment's back edge, and keep the body's existing jump.  No code is
     copied; only the byte offsets of the existing blocks move. */
  if (cprime_state->opt_level == 2 && !is_cpp_translation_unit())
  for (layout_c = 0; layout_c + 1 < n; ++layout_c) {
    X64FastInsn *conditional = &ins[layout_c], *exit_jump = &ins[layout_c + 1];
    int accepted = 1;
    if (!conditional->size || !exit_jump->size || labels[layout_c + 1]
        || conditional->op != 0x0f || conditional->op2 < 0x80
        || conditional->op2 > 0x8f || exit_jump->op != 0xe9
        || exit_jump->branch != 4) continue;
    layout_j = layout_c + 1;
    layout_i0 = layout_j + 1;
    layout_b = target[layout_c];
    layout_x = target[layout_j];
    if (layout_b <= layout_i0 || layout_x <= layout_b || layout_x > n) continue;
    if ((ins[layout_x - 1].op != 0xe9 && ins[layout_x - 1].op != 0xeb)
        || target[layout_x - 1] != layout_i0) continue;
    layout_h = target[layout_b - 1];
    if (layout_h < 0 || layout_h > layout_c || layout_b - 1 < layout_i0
        || (ins[layout_b - 1].op != 0xe9 && ins[layout_b - 1].op != 0xeb)
        || target[layout_b - 1] != layout_h) continue;
    for (layout_k = layout_i0; layout_k < layout_b; ++layout_k)
      if (layout_k != layout_i0 && labels[layout_k]) accepted = 0;
    for (layout_k = layout_b; layout_k < layout_x; ++layout_k)
      if ((layout_k != layout_b && labels[layout_k])
          || (ins[layout_k].op == 0x0f && ins[layout_k].op2 >= 0x80
              && ins[layout_k].op2 <= 0x8f)) accepted = 0;
    for (layout_k = layout_h; layout_k <= layout_c; ++layout_k)
      if (layout_k != layout_h && layout_k != layout_c && labels[layout_k])
        accepted = 0;
    if (!accepted) continue;
    for (layout_k = 0; layout_k <= n; ++layout_k)
      layout_out[layout_k] = ins[layout_k].out;
    ins[layout_j].size = 0;
    layout_pos = ins[layout_h].out;
    for (layout_k = layout_h; layout_k <= layout_j; ++layout_k) {
      layout_out[layout_k] = layout_pos;
      layout_pos += ins[layout_k].size;
    }
    for (layout_k = layout_b; layout_k < layout_x; ++layout_k) {
      layout_out[layout_k] = layout_pos;
      layout_pos += ins[layout_k].size;
    }
    for (layout_k = layout_i0; layout_k < layout_b; ++layout_k) {
      layout_out[layout_k] = layout_pos;
      layout_pos += ins[layout_k].size;
    }
    for (layout_k = layout_x; layout_k <= n; ++layout_k) {
      layout_out[layout_k] = layout_pos;
      if (layout_k < n) layout_pos += ins[layout_k].size;
    }
    if (layout_pos > X64_FAST_BYTES) continue;
    for (layout_k = 0; layout_k <= n; ++layout_k)
      ins[layout_k].out = layout_out[layout_k];
    out = layout_pos;
    ins[n].out = out;
    rewritten[layout_c][1] ^= 1;
    target[layout_c] = layout_x;
    break;
  }
  /* The inner loop head is the branch target that matters for throughput:
     a backward branch rejoins it once per iteration, so where it sits in the
     fetch window decides the loop's speed.  At -O2 line up every loop head;
     everywhere else line up only the innermost head so the size policy pays
     for one bounded pad.  A compacted C++ function uses phase 3 instead of
     phase 0 at every level: the compacted loop bodies measured faster there
     than at phase 0, without giving back the 128-byte -Os win, and a compacted
     C++ loop that is left unpadded at -O1 can lose what the compaction saved.
     The padding only shifts code inside this function; the caller passes -1
     while caching an inline body, which is placed at an arbitrary call site. */
  if (align_base >= 0
      && (cprime_state->opt_level == 2 || cprime_state->opt_level == 3
          || is_cpp_translation_unit())) {
    int grow = 0, best = -1, span = 0;
    int phase = is_cpp_translation_unit() ? X64_FAST_CPP_PHASE : 0;
    memset(labels, 0, sizeof(labels));
    for (i = 0; i < n; ++i)
      if (target[i] >= 0 && ins[target[i]].out < ins[i].out) {
        if (cprime_state->opt_level == 2) labels[target[i]] = 1;
        else {
          int candidate = ins[i].out - ins[target[i]].out;
          if (best < 0 || candidate < span) {
            best = target[i];
            span = candidate;
          }
        }
      }
    if ((cprime_state->opt_level == 3 || is_cpp_translation_unit())
        && best >= 0) labels[best] = 1;
    for (i = 0; i < n; ++i) {
      if (labels[i])
        grow += (phase - (align_base + ins[i].out + grow))
                & (X64_FAST_ALIGN - 1);
      layout_out[i] = ins[i].out + grow;
    }
    /* The size policy may spend savings on one inner-loop pad, but it must
       not hand back all of the function's shrink or make it larger. */
    if (cprime_state->opt_level == 2 || out + grow <= size) {
      for (i = 0; i < n; ++i) ins[i].out = layout_out[i];
      out += grow;
    }
  }
  if (out > X64_FAST_BYTES) return size;
  /* The C++ size subset is only worthwhile when it actually removes bytes.
     Otherwise the changed layout can lose more on a short loop than the
     shrink saves, so keep the pre-pass code exactly as it was. */
  if (allow_cpp_size && out >= size) return size;
  ins[n].out = out;
  memset(result, 0x90, out);
  for (i = 0; i < n; ++i) {
    memcpy(result + ins[i].out, rewritten[i], ins[i].size);
  }
  for (i = 0; i < n; ++i) {
    X64FastInsn *d = &ins[i];
    if (d->branch) {
      int delta = ins[target[i]].out - d->out - d->size;
      if (d->branch == 1) {
        if (delta != (signed char)delta) return size;
        result[d->out + d->size - 1] = delta;
      } else write32le(result + d->out + d->size - 4, delta);
    }
  }
  for (j = 0; j < nrel; ++j) {
    int at = rel[j].r_offset;
    for (i = 0; i < n && !(at >= ins[i].off && at < ins[i].off + ins[i].len); ++i) {}
    if (i == n || ins[i].size != ins[i].len) return size;
  }
  for (j = 0; j < nrel; ++j) {
    int at = rel[j].r_offset;
    for (i = 0; !(at >= ins[i].off && at < ins[i].off + ins[i].len); ++i) {}
    rel[j].r_offset = ins[i].out + at - ins[i].off;
  }
  memcpy(code, result, out);
  return out;
}

static void x64_fast_optimize(int start, int end)
{
  static X64FastInsn ins[X64_FAST_INSNS + 1];
  static X64FastSlot slots[X64_FAST_SLOTS];
  static unsigned char code[X64_FAST_BYTES];
  unsigned used = 0;
  int n = 0, ns = 0, pos = 0, out = 0, i, j, available, remaining = 0;
  int bytes = end - start, nrel = 0;
  int calls = 0, escaped_negative = 0, escaped_positive = 0;
  int overflow_reloc = 0, cpp_enter = 0, cpp_compact = 0;
  ObjW_Rel rel[16];
  ProfileFastoptStage profile_fastopt;
  Section *rs = cur_text_section->reloc;
  if (!cprime_state->optimize || cprime_state->do_debug || x64_fast_disabled || !x64_fast_sym
      || bytes <= 0 || bytes > X64_FAST_BYTES || func_var
      || x86_64_asm_enabled() || cprime_state->do_bounds_check) return;
  profile_fastopt_stage_begin(&profile_fastopt, PROFILE_FASTOPT_DECODE);
  while (pos < bytes) {
    X64FastInsn *d = &ins[n];
    if (n == X64_FAST_INSNS
        || !x64_fast_decode(cur_text_section->data + start + pos,
                            bytes - pos, d, &used)) goto x64_fast_done;
    d->off = pos;
    if (d->modrm && (cur_text_section->data[start + pos + d->modrm] >> 6) == 3) {
      int rm = (cur_text_section->data[start + pos + d->modrm] & 7) + ((d->rex & 1) << 3);
      if (rm == 4 || rm == 5 || ((d->op == 0x89 || d->op == 0x8b)
                               && (d->reg == 4 || d->reg == 5))) goto x64_fast_done;
    }
    if (d->op == 0xe8 || d->op == 0xff) calls = 1;
    if (d->base == 5) {
      int eligible = (d->op == 0x8b || d->op == 0x89 || d->op == 0xc7)
                     && d->width >= 4;
      /* Escaping addresses are conservative barriers. Narrow and overlapping
         accesses keep their stack storage without blocking unrelated slots. */
      if (d->op == 0x8d) {
        if (d->disp < 0) escaped_negative = 1; else escaped_positive = 1;
        pos += d->len; ++n; continue;
      }
      if (d->op == 0x88 || d->op == 0x8a || d->op == 0xc6
          || (d->op == 0x0f && (d->op2 == 0xb6 || d->op2 == 0xbe))) d->width = 1;
      else if (d->op == 0x0f && (d->op2 == 0xb7 || d->op2 == 0xbf)) d->width = 2;
      else if (!eligible && d->op != 0x8b && d->op != 0x89 && d->op != 0xc7) goto x64_fast_done;
      for (j = 0; j < ns; ++j) if (slots[j].disp == d->disp) break;
      if (j == ns) {
        if (ns == X64_FAST_SLOTS) goto x64_fast_done;
        slots[ns].disp = d->disp; slots[ns].width = d->width;
        slots[ns].extent = d->width;
        slots[ns].count = 0; slots[ns].reg = -1; ++ns;
        if (d->disp >= 0 && (d->disp < 16 || (d->disp & 7)
            || d->op != 0x89 || d->width != 8)) slots[j].reg = -2;
      }
      /* The ABI spills incoming integer arguments as 64 bits, even when the
         source parameter is 32 bits. The low subregister retains its value. */
      if (d->width != slots[j].width) {
        if (d->disp < 16 || slots[j].count != 1
            || slots[j].width != 8 || d->width != 4) slots[j].reg = -2;
        else slots[j].width = 4;
      }
      if (!eligible) slots[j].reg = -2;
      if (d->width > slots[j].extent) slots[j].extent = d->width;
      ++slots[j].count; d->slot = j;
    }
    pos += d->len; ++n;
  }
  profile_fastopt_stage_next(&profile_fastopt, PROFILE_FASTOPT_PROMOTE);
  for (i = 0; i < ns; ++i)
    for (j = 0; j < i; ++j)
      if (slots[i].disp < slots[j].disp + slots[j].extent
          && slots[j].disp < slots[i].disp + slots[i].extent)
        slots[i].reg = slots[j].reg = -2;
  {
    int stacksum = -(loc & -16) + ((func_scratch + 15) & -16);
    int cpp_os_calls = is_cpp_translation_unit() && !x64_fast_frame
                       && cprime_state->opt_level == 3 && bytes >= 128;
    if (calls && (func_alloca || stacksum >= 4096
                  || (!x64_fast_frame && !cpp_os_calls))) goto x64_fast_done;
  }
  available = (~used) & (calls ? (x64_fast_frame ? 0xf000 : 0) : 0xf00);
  for (i = 0; i < ns; ++i)
    if ((escaped_negative && slots[i].disp < 0)
        || (escaped_positive && slots[i].disp >= 0)) slots[i].reg = -2;
  while (available) {
    int best = -1, reg;
    for (i = 0; i < ns; ++i)
      if (slots[i].reg == -1 && (best < 0 || slots[i].count > slots[best].count)) best = i;
    if (best < 0) break;
    /* Stack-passed parameters have no defining spill; do not promote them. */
    if (slots[best].disp >= 48) { slots[best].reg = -2; continue; }
    if (calls && slots[best].count < 4) { slots[best].reg = -2; continue; }
    for (reg = calls ? 12 : 8; !(available & (1 << reg)); ++reg) {}
    slots[best].reg = reg; available &= ~(1 << reg);
  }
  profile_fastopt_stage_next(&profile_fastopt, PROFILE_FASTOPT_REWRITE);
  for (i = 0; i < n; ++i) {
    X64FastInsn *d = &ins[i];
    d->out = out;
    if (d->slot >= 0 && slots[d->slot].reg >= 0)
      d->size = d->op == 0xc7 ? 7 : 3;
    else if (d->slot >= 0) ++remaining;
    if (d->size > d->len) goto x64_fast_done;
    out += d->size;
  }
  ins[n].out = out;
  for (i = 0; i < n; ++i) {
    X64FastInsn *d = &ins[i];
    unsigned char *p = code + d->out;
    const unsigned char *old = cur_text_section->data + start + d->off;
    if (d->slot >= 0 && slots[d->slot].reg >= 0) {
      int r = slots[d->slot].reg;
      p[0] = 0x41 | (d->rex & 12); p[1] = d->op;
      p[2] = 0xc0 | ((d->reg & 7) << 3) | (r & 7);
      if (d->op == 0xc7) memcpy(p + 3, old + d->len - 4, 4);
    } else memcpy(p, old, d->len);
    if (d->branch) {
      int delta = d->branch == 1 ? (signed char)old[d->len - 1]
                                : (int)read32le(old + d->len - 4);
      int target = x64_fast_map(ins, n, d->off + d->len + delta, bytes);
      if (target < 0) goto x64_fast_done;
      delta = target - d->out - d->size;
      if (d->branch == 1) {
        if (delta != (signed char)delta) goto x64_fast_done;
        p[d->size - 1] = delta;
      } else write32le(p + d->size - 4, delta);
    }
  }
  profile_fastopt_stage_next(&profile_fastopt, PROFILE_FASTOPT_RELOCATE);
  /* Validate every relocation before modifying any code or relocation. */
  if (rs) {
    ObjW_Rel *r, *rend = (ObjW_Rel *)(rs->data + rs->data_offset);
    for (r = (ObjW_Rel *)(rs->data + x64_fast_reloc_start); r < rend; ++r) {
      int at = r->r_offset - start;
      ObjSym *symbol;
      if (r->r_offset < start || r->r_offset >= end) continue;
      symbol = (ObjSym *)symtab_section->data + Obj64_R_SYM(r->r_info);
      if (symbol->st_shndx == cur_text_section->sh_num) {
        addr_t target = symbol->st_value + r->r_addend + 4;
        if (target >= start && target < end) goto x64_fast_done;
      }
      for (i = 0; i < n && !(at >= ins[i].off && at < ins[i].off + ins[i].len); ++i) {}
      if (i == n || ins[i].len != ins[i].size) goto x64_fast_done;
      if (nrel < 16) { rel[nrel] = *r; rel[nrel++].r_offset = ins[i].out + at - ins[i].off; }
      else remaining = overflow_reloc = 1;
    }
  }
  profile_fastopt_stage_next(&profile_fastopt, PROFILE_FASTOPT_REWRITE);
  /* C functions without debug locations can be compacted before the epilog
     and unwind record are emitted. C++ EH and debug instruction addresses
     retain the original layout, and the check above already rejects C++ frames
     with calls, so a C++ function that reaches this point is call-free and
     compacts like C.  The shared register pass handles every call-free frame;
     at -Os only the size-oriented compaction waits until the saved bytes
     dominate a short-loop layout change, while at -O1/-O2 compaction is worth
     keeping on its own. */
  cpp_enter = is_cpp_translation_unit() && !cpp_eh_has_code_offsets();
  cpp_compact = cpp_enter && cprime_state->opt_level == 3 && bytes >= 128;
  if (calls) for (i = 0; i < ns; ++i)
    if (slots[i].reg >= 12) x64_fast_saved |= 1 << (slots[i].reg - 12);
  if ((!is_cpp_translation_unit() || cpp_enter)
      && !cprime_state->do_debug && !overflow_reloc) {
    int ri = 0, base = start;
    /* The epilog reclaims the whole saved-register gap when no nonvolatile
       register was promoted, which moves the body up by that many bytes.  A
       loop-head pad has to be placed for the address the body finally lands
       on, not for the reserved one. */
    if (x64_fast_frame && !x64_fast_saved) base -= FUNC_FAST_GAP;
    out = x64_fast_simplify(code, out, rel, nrel, base, cpp_compact);
    /* Aligning loop heads can leave the function longer than it was, and the
       section so far only has room for the epilog that follows.  Reserve the
       bytes before copying back; the move may reallocate the section. */
    if (start + out > (int)cur_text_section->data_offset)
      section_ptr_add(cur_text_section,
                      start + out - cur_text_section->data_offset);
    if (rs) {
      ObjW_Rel *r, *rend = (ObjW_Rel *)(rs->data + rs->data_offset);
      for (r = (ObjW_Rel *)(rs->data + x64_fast_reloc_start); r < rend; ++r)
        if (r->r_offset >= start && r->r_offset < end) r->r_offset = start + rel[ri++].r_offset;
    }
    memcpy(cur_text_section->data + start, code, out);
    ind = start + out;
    x64_fast_compacted = 1;
  } else {
    for (i = 0; i < n; ++i) if (ins[i].slot >= 0 && slots[ins[i].slot].reg >= 0) {
      memcpy(cur_text_section->data + start + ins[i].off, code + ins[i].out, ins[i].size);
      memset(cur_text_section->data + start + ins[i].off + ins[i].size,
             0x90, ins[i].len - ins[i].size);
    }
  }
  profile_fastopt_stage_next(&profile_fastopt, PROFILE_FASTOPT_COMMIT);
  if (!cprime_state->no_inline && !x64_fast_sym->type.ref->f.func_noinline
      && !calls && !remaining && !escaped_negative && !escaped_positive
      && out <= 192 && x64_fast_sym->c && !x64_fast_sym->a.weak) {
    X64FastInline *fn = cprime_malloc(sizeof(*fn));
    int bucket = x64_fast_sym->c & 1023;
    out = x64_fast_simplify(code, out, rel, nrel, -1, 0);
    fn->symbol = x64_fast_sym->c; fn->size = out; fn->nrel = nrel;
    memcpy(fn->code, code, out); memcpy(fn->rel, rel, nrel * sizeof(*rel));
    fn->next = x64_fast_inline[bucket]; x64_fast_inline[bucket] = fn;
  }
x64_fast_done:
  profile_fastopt_stage_end(&profile_fastopt);
}

static int x64_fast_inline_call(void)
{
  X64FastInline *fn;
  int symbol, i, start;
  if (!cprime_state->optimize || cprime_state->no_inline || nocode_wanted || x86_64_asm_enabled()
      || (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) != (VT_CONST | VT_SYM)
      || vtop->c.i || !vtop->sym) return 0;
  if (vtop->sym->a.weak || ((vtop->sym->type.t & VT_BTYPE) == VT_FUNC
      && vtop->sym->type.ref && vtop->sym->type.ref->f.func_noinline)) return 0;
  symbol = vtop->sym->c;
  for (fn = x64_fast_inline[symbol & 1023]; fn; fn = fn->next)
    if (fn->symbol == symbol) break;
  if (!fn) return 0;
  start = ind;
  for (i = 0; i < fn->size; ++i) g(fn->code[i]);
  for (i = 0; i < fn->nrel; ++i)
    put_elf_reloca(symtab_section, cur_text_section, start + fn->rel[i].r_offset,
                   Obj64_R_TYPE(fn->rel[i].r_info), Obj64_R_SYM(fn->rel[i].r_info),
                   fn->rel[i].r_addend);
  return 1;
}
#else
ST_FUNC void x64_fast_reset(void) {}
ST_FUNC void x64_fast_disable(void) {}
static int x64_fast_inline_call(void) { return 0; }
#endif
