#ifdef TARGET_DEFS_ONLY

// Number Of Available Registers
#define NB_REGS         25
#define NB_ASM_REGS     16
#define CONFIG_CPRIME_ASM

/* a register can belong to several classes. The classes must be
   sorted from more general to more precise (see gv2() code which does
   assumptions on it). */
#define RC_INT     0x0001 // Generic Integer Register 
#define RC_FLOAT   0x0002 // Generic Float Register 
#define RC_RAX     0x0004
#define RC_RDX     0x0008
#define RC_RCX     0x0010
#define RC_RSI     0x0020
#define RC_RDI     0x0040
#define RC_ST0     0x0080 // only for long double 
#define RC_R8      0x0100
#define RC_R9      0x0200
#define RC_R10     0x0400
#define RC_R11     0x0800
#define RC_XMM0    0x1000
#define RC_XMM1    0x2000
#define RC_XMM2    0x4000
#define RC_XMM3    0x8000
#define RC_XMM4    0x10000
#define RC_XMM5    0x20000
#define RC_XMM6    0x40000
#define RC_XMM7    0x80000
#define RC_IRET    RC_RAX // Function Return: Integer Register 
#define RC_IRE2    RC_RDX // Function Return: Second Integer Register 
#define RC_FRET    RC_XMM0 // Function Return: Float Register 
#define RC_FRE2    RC_XMM1 // Function Return: Second Float Register 

// Pretty Names For The Registers
enum
{
  TREG_RAX = 0,
  TREG_RCX = 1,
  TREG_RDX = 2,
  TREG_RSP = 4,
  TREG_RSI = 6,
  TREG_RDI = 7,

  TREG_R8  = 8,
  TREG_R9  = 9,
  TREG_R10 = 10,
  TREG_R11 = 11,

  TREG_XMM0 = 16,
  TREG_XMM1 = 17,
  TREG_XMM2 = 18,
  TREG_XMM3 = 19,
  TREG_XMM4 = 20,
  TREG_XMM5 = 21,
  TREG_XMM6 = 22,
  TREG_XMM7 = 23,

  TREG_ST0 = 24,

  TREG_MEM = 0x20
};

#define REX_BASE(reg) (((reg) >> 3) & 1)
#define REG_VALUE(reg) ((reg) & 7)

// Return Registers For Function
#define REG_IRET TREG_RAX // Single Word Int Return Register 
#define REG_IRE2 TREG_RDX // Second Word Return Register (For Long Long) 
#define REG_FRET TREG_XMM0 // Float Return Register 
#define REG_FRE2 TREG_XMM1 // Second Float Return Register 

// defined if function parameters must be evaluated in reverse order
#define INVERT_FUNC_PARAMS

// Pointer Size, In Bytes
#define PTR_SIZE 8

// long double size and alignment, in bytes
#define LDOUBLE_SIZE  16
#define LDOUBLE_ALIGN 16
// Maximum Alignment (For Aligned Attribute Support)
#define MAX_ALIGN     16

/* define if return values need to be extended explicitely
   at caller side (for interfacing with non-CPRIME compilers) */
#define PROMOTE_RET

#define CPRIME_TARGET_NATIVE_STRUCT_COPY
ST_FUNC void gen_struct_copy(int size);

//****************************************************
#else // ! TARGET_DEFS_ONLY 
//****************************************************
#define USING_GLOBALS
#include "cprime.h"
#include <assert.h>

ST_DATA const char *const target_machine_defs =
  "__x86_64__\0"
  "__x86_64\0"
  "__amd64__\0"
  ;

ST_DATA const int reg_classes[NB_REGS] =
{
  /* eax */ RC_INT | RC_RAX,
  /* ecx */ RC_INT | RC_RCX,
  /* edx */ RC_INT | RC_RDX,
  0,
  0,
  0,
  RC_RSI,
  RC_RDI,
  RC_R8,
  RC_R9,
  RC_R10,
  RC_R11,
  0,
  0,
  0,
  0,
  /* xmm0 */ RC_FLOAT | RC_XMM0,
  /* xmm1 */ RC_FLOAT | RC_XMM1,
  /* xmm2 */ RC_FLOAT | RC_XMM2,
  /* xmm3 */ RC_FLOAT | RC_XMM3,
  /* xmm4 */ RC_FLOAT | RC_XMM4,
  /* xmm5 */ RC_FLOAT | RC_XMM5,
  /* xmm6 an xmm7 are included so gv() can be used on them,
     but they are not tagged with RC_FLOAT because they are
     callee saved on Windows */
  RC_XMM6,
  RC_XMM7,
  /* st0 */ RC_ST0
};

static unsigned long func_sub_sp_offset;
static int func_ret_sub;

#if defined(CONFIG_CPRIME_BCHECK)
static addr_t func_bound_offset;
static unsigned long func_bound_ind;
ST_DATA int func_bound_add_epilog;
#endif

#ifdef CPRIME_TARGET_PE
static int func_scratch, func_alloca;
#endif

// XXX: make it faster ?
ST_FUNC void g(int c)
{
  int ind1;
  if (nocode_wanted)
    return;
  ind1 = ind + 1;
  if (ind1 > cur_text_section->data_allocated)
    section_realloc(cur_text_section, ind1);
  cur_text_section->data[ind] = c;
  ind = ind1;
}

ST_FUNC void o(unsigned int c)
{
  while (c)
  {
    g(c);
    c = c >> 8;
  }
}

ST_FUNC void gen_le16(int v)
{
  g(v);
  g(v >> 8);
}

ST_FUNC void gen_le32(int c)
{
  g(c);
  g(c >> 8);
  g(c >> 16);
  g(c >> 24);
}

ST_FUNC void gen_le64(int64_t c)
{
  g(c);
  g(c >> 8);
  g(c >> 16);
  g(c >> 24);
  g(c >> 32);
  g(c >> 40);
  g(c >> 48);
  g(c >> 56);
}

static void orex(int ll, int r, int r2, int b)
{
  if ((r & VT_VALMASK) >= VT_CONST)
    r = 0;
  if ((r2 & VT_VALMASK) >= VT_CONST)
    r2 = 0;
  if (ll || REX_BASE(r) || REX_BASE(r2))
    o(0x40 | REX_BASE(r) | (REX_BASE(r2) << 2) | (ll << 3));
  o(b);
}

// Output A Symbol And Patch All Calls To It
ST_FUNC void gsym_addr(int t, int a)
{
  while (t)
  {
    unsigned char *ptr = cur_text_section->data + t;
    uint32_t n = read32le(ptr); // Next Value
    write32le(ptr, a < 0 ? -a : a - t - 4);
    t = n;
  }
}

static int is64_type(int t)
{
  return ((t &VT_BTYPE) == VT_PTR ||
          (t &VT_BTYPE) == VT_FUNC ||
          (t &VT_BTYPE) == VT_LLONG);
}

// instruction + 4 bytes data. Return the address of the data
static int oad(int c, int s)
{
  int t;
  if (nocode_wanted)
    return s;
  o(c);
  t = ind;
  gen_le32(s);
  return t;
}

// Generate Jmp To A Label
#define gjmp2(instr,lbl) oad(instr,lbl)

ST_FUNC void gen_addr32(int r, Sym *sym, int c)
{
  if (r & VT_SYM)
    greloca(cur_text_section, sym, ind, R_X86_64_32S, c), c = 0;
  gen_le32(c);
}

// output constant with relocation if 'r & VT_SYM' is true
ST_FUNC void gen_addr64(int r, Sym *sym, int64_t c)
{
  if (r & VT_SYM)
    greloca(cur_text_section, sym, ind, R_X86_64_64, c), c = 0;
  gen_le64(c);
}

// output constant with relocation if 'r & VT_SYM' is true
ST_FUNC void gen_addrpc32(int r, Sym *sym, int c)
{
  if (r & VT_SYM)
    greloca(cur_text_section, sym, ind, R_X86_64_PC32, c - 4), c = 4;
  gen_le32(c - 4);
}

// Output Got Address With Relocation
static void gen_gotpcrel(int r, Sym *sym, int c)
{
#ifdef CPRIME_TARGET_PE
  cprime_error("internal error: no GOT on PE: %s %x %x | %02x %02x %02x\n",
            get_tok_str(sym->v, NULL), c, r,
            cur_text_section->data[ind - 3],
            cur_text_section->data[ind - 2],
            cur_text_section->data[ind - 1]
           );
#endif
  greloca(cur_text_section, sym, ind, R_X86_64_GOTPCREL, -4);
  gen_le32(0);
  if (c)
  {
    // We Use Add C, %Xxx For Displacement
    orex(1, r, 0, 0x81);
    o(0xc0 + REG_VALUE(r));
    gen_le32(c);
  }
}

static void gen_modrm_impl(int op_reg, int r, Sym *sym, int c, int is_got)
{
  op_reg = REG_VALUE(op_reg) << 3;
  if ((r & VT_VALMASK) == VT_CONST)
  {
    // Constant Memory Reference
    if (!(r & VT_SYM))
    {
      // Absolute memory reference
      o(0x04 | op_reg); // [Sib] | Destreg
      oad(0x25, c);     // Disp32
    }
    else
    {
      o(0x05 | op_reg); // (%Rip)+Disp32 | Destreg
      if (is_got)
        gen_gotpcrel(r, sym, c);
      else
        gen_addrpc32(r, sym, c);
    }
  }
  else if ((r & VT_VALMASK) == VT_LOCAL)
  {
    // Currently, We Use Only Ebp As Base
    if (c == (char)c)
    {
      // Short Reference
      o(0x45 | op_reg);
      g(c);
    }
    else
      oad(0x85 | op_reg, c);
  }
  else if ((r & VT_VALMASK) >= TREG_MEM)
  {
    if (c)
    {
      g(0x80 | op_reg | REG_VALUE(r));
      gen_le32(c);
    }
    else
      g(0x00 | op_reg | REG_VALUE(r));
  }
  else
    g(0x00 | op_reg | REG_VALUE(r));
}

/* generate a modrm reference. 'op_reg' contains the additional 3
   opcode bits */
static void gen_modrm(int op_reg, int r, Sym *sym, int c)
{
  gen_modrm_impl(op_reg, r, sym, c, 0);
}

/* generate a modrm reference. 'op_reg' contains the additional 3
   opcode bits */
static void gen_modrm64(int opcode, int op_reg, int r, Sym *sym, int c)
{
  int is_got;
  is_got = (op_reg &TREG_MEM) && !(sym->type.t &VT_STATIC);
  orex(1, r, op_reg, opcode);
  gen_modrm_impl(op_reg, r, sym, c, is_got);
}


// Load 'R' From Value 'Sv'
void load(int r, SValue *sv)
{
  int v, t, ft, fc, fr;
  SValue v1;

  fr = sv->r;
  ft = sv->type.t & ~VT_DEFSIGN;
  fc = sv->c.i;
  if (fc != sv->c.i && (fr & VT_SYM))
    cprime_error("64 bit addend in load");

  ft &= ~(VT_VOLATILE | VT_CONSTANT);

#ifndef CPRIME_TARGET_PE
  // We Use Indirect Access Via Got
  if ((fr & VT_VALMASK) == VT_CONST && (fr & VT_SYM) &&
      (fr & VT_LVAL) && !(sv->sym->type.t & VT_STATIC))
  {
    // Use The Result Register As A Temporal Register
    int tr = r | TREG_MEM;
    if (is_float(ft))
    {
      // We Cannot Use Float Registers As A Temporal Register
      tr = get_reg(RC_INT) | TREG_MEM;
    }
    gen_modrm64(0x8b, tr, fr, sv->sym, 0);

    // Load From The Temporal Register
    fr = tr | VT_LVAL;
  }
#endif

  v = fr &VT_VALMASK;
  if (fr & VT_LVAL)
  {
    int b, ll;
    if (v == VT_LLOCAL)
    {
      v1.type.t = VT_PTR;
      v1.r = VT_LOCAL | VT_LVAL;
      v1.c.i = fc;
      v1.sym = NULL;
      fr = r;
      if (!(reg_classes[fr] & (RC_INT | RC_R11)))
        fr = get_reg(RC_INT);
      load(fr, &v1);
    }
    if (fc != sv->c.i)
    {
      /* If the addends doesn't fit into a 32bit signed
         we must use a 64bit move.  We've checked above
         that this doesn't have a sym associated.  */
      v1.type.t = VT_LLONG;
      v1.r = VT_CONST;
      v1.c.i = sv->c.i;
      v1.sym = NULL;
      fr = r;
      if (!(reg_classes[fr] & (RC_INT | RC_R11)))
        fr = get_reg(RC_INT);
      load(fr, &v1);
      fc = 0;
    }
    ll = 0;
    /* Like GCC we can load from small enough properly sized
       structs and unions as well.
       XXX maybe move to generic operand handling, but should
       occur only with asm, so cprimeasm.c might also be a better place */
    if ((ft & VT_BTYPE) == VT_STRUCT)
    {
      int align;
      switch (type_size(&sv->type, &align))
      {
      case 1: ft = VT_BYTE; break;
      case 2: ft = VT_SHORT; break;
      case 4: ft = VT_INT; break;
      case 8: ft = VT_LLONG; break;
      default:
        cprime_error("invalid aggregate type for register load");
        break;
      }
    }
    if ((ft & VT_BTYPE) == VT_FLOAT)
    {
      b = 0x6e0f66;
      r = REG_VALUE(r); // Movd
    }
    else if ((ft & VT_BTYPE) == VT_DOUBLE)
    {
      b = 0x7e0ff3; // Movq
      r = REG_VALUE(r);
    }
    else if ((ft & VT_BTYPE) == VT_LDOUBLE)
    {
      b = 0xdb, r = 5; // Fldt
    }
    else if ((ft & VT_TYPE) == VT_BYTE || (ft & VT_TYPE) == VT_BOOL)
    {
      b = 0xbe0f;   // Movsbl
    }
    else if ((ft & VT_TYPE) == (VT_BYTE | VT_UNSIGNED) ||
             (ft & VT_TYPE) == (VT_BOOL | VT_UNSIGNED))
    {
      b = 0xb60f;   // Movzbl
    }
    else if ((ft & VT_TYPE) == VT_SHORT)
    {
      b = 0xbf0f;   // Movswl
    }
    else if ((ft & VT_TYPE) == (VT_SHORT | VT_UNSIGNED))
    {
      b = 0xb70f;   // Movzwl
    }
    else if ((ft & VT_TYPE) == (VT_VOID))
    {
      // Can happen with zero size structs
      return;
    }
    else
    {
      assert(((ft &VT_BTYPE) == VT_INT)
             || ((ft &VT_BTYPE) == VT_LLONG)
             || ((ft &VT_BTYPE) == VT_PTR)
             || ((ft &VT_BTYPE) == VT_FUNC)
            );
      ll = is64_type(ft);
      b = 0x8b;
    }
    if (ll)
      gen_modrm64(b, r, fr, sv->sym, fc);
    else
    {
      orex(ll, fr, r, b);
      gen_modrm(r, fr, sv->sym, fc);
    }
  }
  else
  {
    if (v == VT_CONST)
    {
      if (fr & VT_SYM)
      {
#ifdef CPRIME_TARGET_PE
        orex(1, 0, r, 0x8d);
        o(0x05 + REG_VALUE(r) * 8); // Lea Xx(%Rip), R
        gen_addrpc32(fr, sv->sym, fc);
#else
        if (sv->sym->type.t & VT_STATIC)
        {
          orex(1, 0, r, 0x8d);
          o(0x05 + REG_VALUE(r) * 8); // Lea Xx(%Rip), R
          gen_addrpc32(fr, sv->sym, fc);
        }
        else
        {
          orex(1, 0, r, 0x8b);
          o(0x05 + REG_VALUE(r) * 8); // Mov Xx(%Rip), R
          gen_gotpcrel(r, sv->sym, fc);
        }
#endif
      }
      else if (is64_type(ft))
      {
        if (sv->c.i >> 32)
        {
          orex(1, r, 0, 0xb8 + REG_VALUE(r)); // Movabs $Xx, R
          gen_le64(sv->c.i);
        }
        else if (sv->c.i > 0)
        {
          orex(0, r, 0, 0xb8 + REG_VALUE(r)); // Mov $Xx, R
          gen_le32(sv->c.i);
        }
        else
        {
          orex(0, r, r, 0x31); // Xor R, R
          o(0xc0 + REG_VALUE(r) * 9);
        }
      }
      else
      {
        orex(0, r, 0, 0xb8 + REG_VALUE(r)); // Mov $Xx, R
        gen_le32(fc);
      }
    }
    else if (v == VT_LOCAL)
    {
      orex(1, 0, r, 0x8d); // Lea Xxx(%Ebp), R
      gen_modrm(r, VT_LOCAL, sv->sym, fc);
    }
    else if (v == VT_CMP)
    {
      if (fc & 0x100)
      {
        v = vtop->cmp_r;
        fc &= ~0x100;
        /* This was a float compare.  If the parity bit is
        set the result was unordered, meaning false for everything
        except TOK_NE, and true for TOK_NE.  */
        orex(0, r, 0, 0xb0 + REG_VALUE(r)); // Mov $0/1,%Al
        g(v ^fc ^ (v == TOK_NE));
        o(0x037a + (REX_BASE(r) << 8));
      }
      orex(0, r, 0, 0x0f); // Setxx %Br
      o(fc);
      o(0xc0 + REG_VALUE(r));
      orex(0, r, 0, 0x0f);
      o(0xc0b6 + REG_VALUE(r) * 0x900); // Movzbl %Al, %Eax
    }
    else if (v == VT_JMP || v == VT_JMPI)
    {
      t = v & 1;
      orex(0, r, 0, 0);
      oad(0xb8 + REG_VALUE(r), t); // Mov $1, R
      o(0x05eb + (REX_BASE(r) << 8)); // Jmp After
      gsym(fc);
      orex(0, r, 0, 0);
      oad(0xb8 + REG_VALUE(r), t ^ 1); // Mov $0, R
    }
    else if (v != r)
    {
      if ((r >= TREG_XMM0) && (r <= TREG_XMM7))
      {
        if (v == TREG_ST0)
        {
          // gen_cvt_ftof(VT_DOUBLE);
          o(0xf0245cdd); // Fstpl -0X10(%Rsp)
          // movsd -0x10(%rsp),%xmmN
          o(0x100ff2);
          o(0x44 + REG_VALUE(r) * 8); // %xmmN
          o(0xf024);
        }
        else
        {
          if (!nocode_wanted)
            assert((v >= TREG_XMM0) && (v <= TREG_XMM7));
          if ((ft & VT_BTYPE) == VT_FLOAT)
            o(0x100ff3);
          else
          {
            assert((ft &VT_BTYPE) == VT_DOUBLE);
            o(0x100ff2);
          }
          o(0xc0 + REG_VALUE(v) + REG_VALUE(r) * 8);
        }
      }
      else if (r == TREG_ST0)
      {
        if (!nocode_wanted)
          assert((v >= TREG_XMM0) && (v <= TREG_XMM7));
        // gen_cvt_ftof(VT_LDOUBLE);
        // movsd %xmmN,-0x10(%rsp)
        o(0x110ff2);
        o(0x44 + REG_VALUE(r) * 8); // %xmmN
        o(0xf024);
        o(0xf02444dd); // Fldl -0X10(%Rsp)
      }
      else
      {
        orex(is64_type(ft), r, v, 0x89);
        o(0xc0 + REG_VALUE(r) + REG_VALUE(v) * 8); // Mov V, R
      }
    }
  }
}

// Store Register 'R' In Lvalue 'V'
void store(int r, SValue *v)
{
  int fr, bt, ft, fc;
  int op64 = 0;
  // store the REX prefix in this variable when PIC is enabled
  int pic = 0;

  fr = v->r &VT_VALMASK;
  ft = v->type.t;
  fc = v->c.i;
  if (fc != v->c.i && (fr & VT_SYM))
    cprime_error("64 bit addend in store");
  ft &= ~(VT_VOLATILE | VT_CONSTANT);
  bt = ft &VT_BTYPE;

#ifndef CPRIME_TARGET_PE
  // We Need To Access The Variable Via Got
  if (fr == VT_CONST
      && (v->r & VT_SYM)
      && !(v->sym->type.t & VT_STATIC))
  {
    // Mov Xx(%Rip), %R11
    o(0x1d8b4c);
    gen_gotpcrel(TREG_R11, v->sym, v->c.i);
    pic = is64_type(bt) ? 0x49 : 0x41;
  }
#endif

  // XXX: incorrect if float reg to reg
  if (bt == VT_FLOAT)
  {
    o(0x66);
    o(pic);
    o(0x7e0f); // Movd
    r = REG_VALUE(r);
  }
  else if (bt == VT_DOUBLE)
  {
    o(0x66);
    o(pic);
    o(0xd60f); // Movq
    r = REG_VALUE(r);
  }
  else if (bt == VT_LDOUBLE)
  {
    o(0xc0d9); // Fld %St(0)
    o(pic);
    o(0xdb); // Fstpt
    r = 7;
  }
  else
  {
    if (bt == VT_SHORT)
      o(0x66);
    o(pic);
    if (bt == VT_BYTE || bt == VT_BOOL)
      orex(0, 0, r, 0x88);
    else if (is64_type(bt))
      op64 = 0x89;
    else
      orex(0, 0, r, 0x89);
  }
  if (pic)
  {
    // Xxx R, (%R11) Where Xxx Is Mov, Movq, Fld, Or Etc
    if (op64)
      o(op64);
    o(3 + (r << 3));
  }
  else if (op64)
  {
    if (fr == VT_CONST || fr == VT_LOCAL || (v->r & VT_LVAL))
      gen_modrm64(op64, r, v->r, v->sym, fc);
    else if (fr != r)
    {
      orex(1, fr, r, op64);
      o(0xc0 + fr + r * 8); // Mov R, Fr
    }
  }
  else
  {
    if (fr == VT_CONST || fr == VT_LOCAL || (v->r & VT_LVAL))
      gen_modrm(r, v->r, v->sym, fc);
    else if (fr != r)
    {
      o(0xc0 + fr + r * 8); // Mov R, Fr
    }
  }
}

// 'is_jmp' is '1' if it is a jump
static void gcall_or_jmp(int is_jmp)
{
  int r;
  if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
      ((vtop->r & VT_SYM) && (vtop->c.i - 4) == (int)(vtop->c.i - 4)))
  {
    // Constant Symbolic Case -> Simple Relocation
    greloca(cur_text_section, vtop->sym, ind + 1, R_X86_64_PLT32, (int)(vtop->c.i - 4));
    oad(0xe8 + is_jmp, 0); // Call/Jmp Im
  }
  else
  {
    // Otherwise, Indirect Call
    r = TREG_R11;
    load(r, vtop);
    o(0x41); // REX
    o(0xff); // Call/Jmp *R
    o(0xd0 + REG_VALUE(r) + (is_jmp << 4));
  }
}

#if defined(CONFIG_CPRIME_BCHECK)

static void gen_bounds_call(int v)
{
  Sym *sym = external_helper_sym(v);
  oad(0xe8, 0);
  greloca(cur_text_section, sym, ind - 4, R_X86_64_PLT32, -4);
}

#ifdef CPRIME_TARGET_PE
# define TREG_FASTCALL_1 TREG_RCX
#else
# define TREG_FASTCALL_1 TREG_RDI
#endif

static void gen_bounds_prolog(void)
{
  // Leave Some Room For Bound Checking Code
  func_bound_offset = lbounds_section->data_offset;
  func_bound_ind = ind;
  func_bound_add_epilog = 0;
  o(0x0d8d48 + ((TREG_FASTCALL_1 == TREG_RDI) * 0x300000)); // Lbound Section Pointer
  gen_le32 (0);
  oad(0xb8, 0); // Call To Function
}

static void gen_bounds_epilog(void)
{
  addr_t saved_ind;
  addr_t *bounds_ptr;
  Sym *sym_data;
  int offset_modified = func_bound_offset != lbounds_section->data_offset;

  if (!offset_modified && !func_bound_add_epilog)
    return;

  // Add End Of Table Info
  bounds_ptr = section_ptr_add(lbounds_section, sizeof(addr_t));
  *bounds_ptr = 0;

  sym_data = get_sym_ref(&char_pointer_type, lbounds_section,
                         func_bound_offset, PTR_SIZE);

  // Generate Bound Local Allocation
  if (offset_modified)
  {
    saved_ind = ind;
    ind = func_bound_ind;
    greloca(cur_text_section, sym_data, ind + 3, R_X86_64_PC32, -4);
    ind = ind + 7;
    gen_bounds_call(TOK___bound_local_new);
    ind = saved_ind;
  }

  // Generate Bound Check Local Freeing
  o(0x5250); // save returned value, if any
  o(0x20ec8348); // Sub $32,%Rsp
  o(0x290f);     // Movaps %Xmm0,0X10(%Rsp)
  o(0x102444);
  o(0x240c290f); // Movaps %Xmm1,(%Rsp)
  greloca(cur_text_section, sym_data, ind + 3, R_X86_64_PC32, -4);
  o(0x0d8d48 + ((TREG_FASTCALL_1 == TREG_RDI) * 0x300000)); // Lea Xxx(%Rip), %Rcx/Rdi
  gen_le32 (0);
  gen_bounds_call(TOK___bound_local_delete);
  o(0x280f);     // Movaps 0X10(%Rsp),%Xmm0
  o(0x102444);
  o(0x240c280f); // Movaps (%Rsp),%Xmm1
  o(0x20c48348); // Add $32,%Rsp
  o(0x585a); // restore returned value, if any
}
#endif

#ifdef CPRIME_TARGET_PE

#define REGN 4
static const uint8_t arg_regs[REGN] =
{
  TREG_RCX, TREG_RDX, TREG_R8, TREG_R9
};

/* Prepare arguments in R10 and R11 rather than RCX and RDX
   because gv() will not ever use these */
static int arg_prepare_reg(int idx)
{
  if (idx == 0 || idx == 1)
    // Idx=0: R10, Idx=1: R11
    return idx + 10;
  else
    return idx >= 0 && idx < REGN ? arg_regs[idx] : 0;
}

/* Generate function call. The function address is pushed first, then
   all the parameters in call order. This functions pops all the
   parameters and the function address. */

static void gen_offs_sp(int b, int r, int d)
{
  orex(1, 0, r & 0x100 ? 0 : r, b);
  if (d == (char)d)
  {
    o(0x2444 | (REG_VALUE(r) << 3));
    g(d);
  }
  else
  {
    o(0x2484 | (REG_VALUE(r) << 3));
    gen_le32(d);
  }
}

static int using_regs(int size)
{
  return !(size > 8 || (size & (size - 1)));
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align, int *regsize)
{
  int size, align;
  *ret_align = 1; // Never have to re-align return values for x86-64
  *regsize = 8;
  size = type_size(vt, &align);
  if (!using_regs(size))
    return 0;
  if (size == 8)
    ret->t = VT_LLONG;
  else if (size == 4)
    ret->t = VT_INT;
  else if (size == 2)
    ret->t = VT_SHORT;
  else
    ret->t = VT_BYTE;
  ret->ref = NULL;
  return 1;
}

static int is_sse_float(int t)
{
  int bt;
  bt = t &VT_BTYPE;
  return bt == VT_DOUBLE || bt == VT_FLOAT;
}

static int gfunc_arg_size(CType *type)
{
  int align;
  if (type->t & (VT_ARRAY | VT_BITFIELD))
    return 8;
  return type_size(type, &align);
}

void gfunc_call(int nb_args)
{
  int size, r, args_size, i, d, bt, struct_size;
  int arg;

#ifdef CONFIG_CPRIME_BCHECK
  if (cprime_state->do_bounds_check)
    gbound_args(nb_args);
#endif

  save_regs(nb_args);

  args_size = (nb_args < REGN ? REGN : nb_args) * PTR_SIZE;
  arg = nb_args;

  /* for struct arguments, we need to call memcpy and the function
     call breaks register passing arguments we are preparing.
     So, we process arguments which will be passed by stack first. */
  struct_size = args_size;
  for (i = 0; i < nb_args; i++)
  {
    SValue *sv;

    --arg;
    sv = &vtop[-i];
    bt = (sv->type.t &VT_BTYPE);
    size = gfunc_arg_size(&sv->type);

    if (using_regs(size))
      continue; // Arguments Smaller Than 8 Bytes Passed In Registers Or On Stack

    if (bt == VT_STRUCT)
    {
      // Align To Stack Align Size
      size = (size + 15) & ~15;
      // Generate Structure Store
      r = get_reg(RC_INT);
      gen_offs_sp(0x8d, r, struct_size);
      struct_size += size;

      // Generate Memcpy Call
      vset(&sv->type, r | VT_LVAL, 0);
      vpushv(sv);
      vstore();
      --vtop;
    }
    else if (bt == VT_LDOUBLE)
    {
      gv(RC_ST0);
      gen_offs_sp(0xdb, 0x107, struct_size);
      struct_size += 16;
    }
  }

  if (func_scratch < struct_size)
    func_scratch = struct_size;

  arg = nb_args;
  struct_size = args_size;

  for (i = 0; i < nb_args; i++)
  {
    --arg;
    bt = (vtop->type.t &VT_BTYPE);

    size = gfunc_arg_size(&vtop->type);
    if (!using_regs(size))
    {
      // Align To Stack Align Size
      size = (size + 15) & ~15;
      if (arg >= REGN)
      {
        d = get_reg(RC_INT);
        gen_offs_sp(0x8d, d, struct_size);
        gen_offs_sp(0x89, d, arg * 8);
      }
      else
      {
        d = arg_prepare_reg(arg);
        gen_offs_sp(0x8d, d, struct_size);
      }
      struct_size += size;
    }
    else
    {
      if (is_sse_float(vtop->type.t))
      {
        if (cprime_state->nosse)
          cprime_error("SSE disabled");
        if (arg >= REGN)
        {
          gv(RC_XMM0);
          // Movq %Xmm0, J*8(%Rsp)
          gen_offs_sp(0xd60f66, 0x100, arg * 8);
        }
        else
        {
          // Load directly to xmmN register
          gv(RC_XMM0 << arg);
          d = arg_prepare_reg(arg);
          // mov %xmmN, %rxx
          o(0x66);
          orex(1, d, 0, 0x7e0f);
          o(0xc0 + arg * 8 + REG_VALUE(d));
        }
      }
      else
      {
        if (bt == VT_STRUCT)
        {
          vtop->type.ref = NULL;
          vtop->type.t = size > 4 ? VT_LLONG : size > 2 ? VT_INT
                         : size > 1 ? VT_SHORT : VT_BYTE;
        }

        r = gv(RC_INT);
        if (arg >= REGN)
          gen_offs_sp(0x89, r, arg * 8);
        else
        {
          d = arg_prepare_reg(arg);
          orex(1, d, r, 0x89); // Mov
          o(0xc0 + REG_VALUE(r) * 8 + REG_VALUE(d));
        }
      }
    }
    vtop--;
  }

  // Copy R10 and R11 into RCX and RDX, respectively
  if (nb_args > 0)
  {
    o(0xd1894c); // Mov %R10, %Rcx
    if (nb_args > 1)
    {
      o(0xda894c); // Mov %R11, %Rdx
    }
  }

  gcall_or_jmp(0);

  if ((vtop->r & VT_SYM) && vtop->sym->v == TOK_alloca)
  {
    // Need To Add The "Func_Scratch" Area After Alloca
    o(0x48); func_alloca = oad(0x05, func_alloca); // add $NN, %rax
#ifdef CONFIG_CPRIME_BCHECK
    if (cprime_state->do_bounds_check)
      gen_bounds_call(TOK___bound_alloca_nr); // New Region
#endif
  }
  vtop--;
}

#define FUNC_PROLOG_SIZE 11

// Generate Function Prolog Of Type 'T'
void gfunc_prolog(Sym *func_sym)
{
  CType *func_type = &func_sym->type;
  int addr, reg_param_index, bt, size;
  Sym *sym;
  CType *type;

  func_ret_sub = 0;
  func_scratch = 32;
  func_alloca = 0;
  loc = 0;

  addr = PTR_SIZE * 2;
  ind += FUNC_PROLOG_SIZE;
  func_sub_sp_offset = ind;
  reg_param_index = 0;

  sym = func_type->ref;

  /* if the function returns a structure, then add an
     implicit pointer parameter */
  size = gfunc_arg_size(&func_vt);
  if (!using_regs(size))
  {
    gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
    func_vc = addr;
    reg_param_index++;
    addr += 8;
  }

  // Define Parameters
  while ((sym = sym->next) != NULL)
  {
    type = &sym->type;
    bt = type->t &VT_BTYPE;
    size = gfunc_arg_size(type);
    if (!using_regs(size))
    {
      if (reg_param_index < REGN)
        gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
      gfunc_set_param(sym, addr, 1);
    }
    else
    {
      if (reg_param_index < REGN)
      {
        // Save Arguments Passed By Register
        if ((bt == VT_FLOAT) || (bt == VT_DOUBLE))
        {
          if (cprime_state->nosse)
            cprime_error("SSE disabled");
          o(0xd60f66); // Movq
          gen_modrm(reg_param_index, VT_LOCAL, NULL, addr);
        }
        else
          gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
      }
      gfunc_set_param(sym, addr, 0);
    }
    addr += 8;
    reg_param_index++;
  }

  while (reg_param_index < REGN)
  {
    if (func_var)
    {
      gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, addr);
      addr += 8;
    }
    reg_param_index++;
  }
#ifdef CONFIG_CPRIME_BCHECK
  if (cprime_state->do_bounds_check)
    gen_bounds_prolog();
#endif
}

// Generate Function Epilog
void gfunc_epilog(void)
{
  int v, start;

  // Align Local Size To Word & Save Local Variables
  func_scratch = (func_scratch + 15) & -16;
  loc = (loc & -16) - func_scratch;

#ifdef CONFIG_CPRIME_BCHECK
  if (cprime_state->do_bounds_check)
    gen_bounds_epilog();
#endif

  o(0xc9); // Leave
  if (func_ret_sub == 0)
  {
    o(0xc3); // Ret
  }
  else
  {
    o(0xc2); // Ret N
    g(func_ret_sub);
    g(func_ret_sub >> 8);
  }

  v = -loc;
  start = func_sub_sp_offset - FUNC_PROLOG_SIZE;
  cur_text_section->data_offset = ind;
  pe_add_unwind_data(start, ind, v);

  ind = start;
  if (v >= 4096)
  {
    Sym *sym = external_helper_sym(TOK___chkstk);
    oad(0xb8, v); // Mov Stacksize, %Eax
    oad(0xe8, 0); // Call __Chkstk, (Does The Stackframe Too)
    greloca(cur_text_section, sym, ind - 4, R_X86_64_PLT32, -4);
    o(0x90); // fill for FUNC_PROLOG_SIZE = 11 bytes
  }
  else
  {
    o(0xe5894855);  // Push %Rbp, Mov %Rsp, %Rbp
    o(0xec8148);  // Sub Rsp, Stacksize
    gen_le32(v);
  }
  ind = cur_text_section->data_offset;

  // Add The "Func_Scratch" Area After Each Alloca Seen
  gsym_addr(func_alloca, -func_scratch);
}

#else

static void gadd_sp(int val)
{
  if (val == (char)val)
  {
    o(0xc48348);
    g(val);
  }
  else
  {
    oad(0xc48148, val); // Add $Xxx, %Rsp
  }
}

typedef enum X86_64_Mode
{
  x86_64_mode_none,
  x86_64_mode_memory,
  x86_64_mode_integer,
  x86_64_mode_sse,
  x86_64_mode_x87
} X86_64_Mode;

static X86_64_Mode classify_x86_64_merge(X86_64_Mode a, X86_64_Mode b)
{
  if (a == b)
    return a;
  else if (a == x86_64_mode_none)
    return b;
  else if (b == x86_64_mode_none)
    return a;
  else if ((a == x86_64_mode_memory) || (b == x86_64_mode_memory))
    return x86_64_mode_memory;
  else if ((a == x86_64_mode_integer) || (b == x86_64_mode_integer))
    return x86_64_mode_integer;
  else if ((a == x86_64_mode_x87) || (b == x86_64_mode_x87))
    return x86_64_mode_memory;
  else
    return x86_64_mode_sse;
}

static X86_64_Mode classify_x86_64_inner(CType *ty)
{
  X86_64_Mode mode;
  Sym *f;

  switch (ty->t &VT_BTYPE)
  {
  case VT_VOID: return x86_64_mode_none;

  case VT_INT:
  case VT_BYTE:
  case VT_SHORT:
  case VT_LLONG:
  case VT_BOOL:
  case VT_PTR:
  case VT_FUNC:
    return x86_64_mode_integer;

  case VT_FLOAT:
  case VT_DOUBLE: return x86_64_mode_sse;

  case VT_LDOUBLE: return x86_64_mode_x87;

  case VT_STRUCT:
    f = ty->ref;

    mode = x86_64_mode_none;
    for (f = f->next; f; f = f->next)
      mode = classify_x86_64_merge(mode, classify_x86_64_inner(&f->type));

    return mode;
  }
  assert(0);
  return 0;
}

static X86_64_Mode classify_x86_64_arg(CType *ty, CType *ret, int *psize, int *palign, int *reg_count)
{
  X86_64_Mode mode;
  int size, align, ret_t = 0;

  if (ty->t & (VT_BITFIELD | VT_ARRAY))
  {
    *psize = 8;
    *palign = 8;
    *reg_count = 1;
    ret_t = ty->t;
    mode = x86_64_mode_integer;
  }
  else
  {
    size = type_size(ty, &align);
    *psize = (size + 7) & ~7;
    *palign = (align + 7) & ~7;
    *reg_count = 0; // Avoid Compiler Warning

    if (size > 16)
      mode = x86_64_mode_memory;
    else
    {
      mode = classify_x86_64_inner(ty);
      switch (mode)
      {
      case x86_64_mode_integer:
        if (size > 8)
        {
          *reg_count = 2;
          ret_t = VT_QLONG;
        }
        else
        {
          *reg_count = 1;
          if (size > 4)
            ret_t = VT_LLONG;
          else if (size > 2)
            ret_t = VT_INT;
          else if (size > 1)
            ret_t = VT_SHORT;
          else
            ret_t = VT_BYTE;
          if ((ty->t & VT_BTYPE) == VT_STRUCT || (ty->t & VT_UNSIGNED))
            ret_t |= VT_UNSIGNED;
        }
        break;

      case x86_64_mode_x87:
        *reg_count = 1;
        ret_t = VT_LDOUBLE;
        break;

      case x86_64_mode_sse:
        if (size > 8)
        {
          *reg_count = 2;
          ret_t = VT_QFLOAT;
        }
        else
        {
          *reg_count = 1;
          ret_t = (size > 4) ? VT_DOUBLE : VT_FLOAT;
        }
        break;
      default: break; // nothing to be done for x86_64_mode_memory and x86_64_mode_none
      }
    }
  }

  if (ret)
  {
    ret->ref = NULL;
    ret->t = ret_t;
  }

  return mode;
}

ST_FUNC int classify_x86_64_va_arg(CType *ty)
{
  // This definition must be synced with stdarg.h
  enum __va_arg_type
  {
    __va_gen_reg, __va_float_reg, __va_stack
  };
  int size, align, reg_count;
  X86_64_Mode mode = classify_x86_64_arg(ty, NULL, &size, &align, &reg_count);
  switch (mode)
  {
  default: return __va_stack;
  case x86_64_mode_integer: return __va_gen_reg;
  case x86_64_mode_sse: return __va_float_reg;
  }
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align, int *regsize)
{
  int size, align, reg_count;
  if (classify_x86_64_arg(vt, ret, &size, &align, &reg_count) == x86_64_mode_memory)
    return 0;
  *ret_align = 1; // Never have to re-align return values for x86-64
  *regsize = 8 * reg_count; // the (virtual) regsize is 16 for VT_QLONG/QFLOAT
  return 1;
}

#define REGN 6
static const uint8_t arg_regs[REGN] =
{
  TREG_RDI, TREG_RSI, TREG_RDX, TREG_RCX, TREG_R8, TREG_R9
};

static int arg_prepare_reg(int idx)
{
  if (idx == 2 || idx == 3)
    // Idx=2: R10, Idx=3: R11
    return idx + 8;
  else
    return idx >= 0 && idx < REGN ? arg_regs[idx] : 0;
}

/* Generate function call. The function address is pushed first, then
   all the parameters in call order. This functions pops all the
   parameters and the function address. */
void gfunc_call(int nb_args)
{
  X86_64_Mode mode;
  CType type;
  int size, align, r, args_size, stack_adjust, i, reg_count, k;
  int nb_reg_args = 0;
  int nb_sse_args = 0;
  int sse_reg, gen_reg;
  char *onstack = cprime_malloc((nb_args + 1) * sizeof (char));

#ifdef CONFIG_CPRIME_BCHECK
  if (cprime_state->do_bounds_check)
    gbound_args(nb_args);
#endif

  save_regs(nb_args);

  /* calculate the number of integer/float register arguments, remember
     arguments to be passed via stack (in onstack[]), and also remember
     if we have to align the stack pointer to 16 (onstack[i] == 2).  Needs
     to be done in a left-to-right pass over arguments.  */
  stack_adjust = 0;
  for (i = nb_args - 1; i >= 0; i--)
  {
    mode = classify_x86_64_arg(&vtop[-i].type, NULL, &size, &align, &reg_count);
    if (size == 0) continue;
    if (mode == x86_64_mode_sse && nb_sse_args + reg_count <= 8)
    {
      nb_sse_args += reg_count;
      onstack[i] = 0;
    }
    else if (mode == x86_64_mode_integer && nb_reg_args + reg_count <= REGN)
    {
      nb_reg_args += reg_count;
      onstack[i] = 0;
    }
    else if (mode == x86_64_mode_none)
      onstack[i] = 0;
    else
    {
      if (align == 16 && (stack_adjust &= 15))
      {
        onstack[i] = 2;
        stack_adjust = 0;
      }
      else
        onstack[i] = 1;
      stack_adjust += size;
    }
  }

  if (nb_sse_args && cprime_state->nosse)
    cprime_error("SSE disabled but floating point arguments passed");

  /* for struct arguments, we need to call memcpy and the function
     call breaks register passing arguments we are preparing.
     So, we process arguments which will be passed by stack first. */
  gen_reg = nb_reg_args;
  sse_reg = nb_sse_args;
  args_size = 0;
  stack_adjust &= 15;
  for (i = k = 0; i < nb_args;)
  {
    mode = classify_x86_64_arg(&vtop[-i].type, NULL, &size, &align, &reg_count);
    if (size)
    {
      if (!onstack[i + k])
      {
        ++i;
        continue;
      }
      /* Possibly adjust stack to align SSE boundary.  We're processing
      args from right to left while allocating happens left to right
      (stack grows down), so the adjustment needs to happen _after_
      an argument that requires it.  */
      if (stack_adjust)
      {
        o(0x50); // push %rax; aka sub $8,%rsp
        args_size += 8;
        stack_adjust = 0;
      }
      if (onstack[i + k] == 2)
        stack_adjust = 1;
    }

    vrotb(i + 1);

    switch (vtop->type.t &VT_BTYPE)
    {
    case VT_STRUCT:
      // Allocate The Necessary Size On Stack
      o(0x48);
      oad(0xec81, size); // Sub $Xxx, %Rsp
      // Generate Structure Store
      r = get_reg(RC_INT);
      orex(1, r, 0, 0x89); // Mov %Rsp, R
      o(0xe0 + REG_VALUE(r));
      vset(&vtop->type, r | VT_LVAL, 0);
      vswap();
      // Keep Stack Aligned For (__Bound_)Memmove Call
      o(0x10ec8348); // Sub $16,%Rsp
      o(0xf0e48348); // And $-16,%Rsp
      orex(0, r, 0, 0x50 + REG_VALUE(r)); // Push R (Last %Rsp)
      o(0x08ec8348); // Sub $8,%Rsp
      vstore();
      o(0x08c48348); // Add $8,%Rsp
      o(0x5c);       // Pop %Rsp
      break;

    case VT_LDOUBLE:
      gv(RC_ST0);
      oad(0xec8148, size); // Sub $Xxx, %Rsp
      o(0x7cdb); // Fstpt 0(%Rsp)
      g(0x24);
      g(0x00);
      break;

    case VT_FLOAT:
    case VT_DOUBLE:
      assert(mode == x86_64_mode_sse);
      r = gv(RC_FLOAT);
      o(0x50); // Push $Rax
      // movq %xmmN, (%rsp)
      o(0xd60f66);
      o(0x04 + REG_VALUE(r) * 8);
      o(0x24);
      break;

    default:
      assert(mode == x86_64_mode_integer);
      // Simple Type
      // XXX: implicit cast ?
      r = gv(RC_INT);
      orex(0, r, 0, 0x50 + REG_VALUE(r)); // Push R
      break;
    }
    args_size += size;

    vpop();
    --nb_args;
    k++;
  }

  cprime_free(onstack);

  /* then, we prepare register passing arguments.
     Note that we cannot set RDX and RCX in this loop because gv()
     may break these temporary registers. Let's use R10 and R11
     instead of them */
  assert(gen_reg <= REGN);
  assert(sse_reg <= 8);
  for (i = 0; i < nb_args; i++)
  {
    mode = classify_x86_64_arg(&vtop->type, &type, &size, &align, &reg_count);
    if (size == 0) continue;
    // Alter stack entry type so that gv() knows how to treat it
    vtop->type = type;
    if (mode == x86_64_mode_sse)
    {
      if (reg_count == 2)
      {
        sse_reg -= 2;
        gv(RC_FRET); // Use pair load into xmm0 & xmm1
        if (sse_reg)   // Avoid Redundant Movaps %Xmm0, %Xmm0
        {
          // movaps %xmm1, %xmmN
          o(0x280f);
          o(0xc1 + ((sse_reg + 1) << 3));
          // movaps %xmm0, %xmmN
          o(0x280f);
          o(0xc0 + (sse_reg << 3));
        }
      }
      else
      {
        assert(reg_count == 1);
        --sse_reg;
        // Load directly to register
        gv(RC_XMM0 << sse_reg);
      }
    }
    else if (mode == x86_64_mode_integer)
    {
      // Simple Type
      // XXX: implicit cast ?
      int d;
      gen_reg -= reg_count;
      r = gv(RC_INT);
      d = arg_prepare_reg(gen_reg);
      orex(1, d, r, 0x89); // Mov
      o(0xc0 + REG_VALUE(r) * 8 + REG_VALUE(d));
      if (reg_count == 2)
      {
        d = arg_prepare_reg(gen_reg + 1);
        orex(1, d, vtop->r2, 0x89); // Mov
        o(0xc0 + REG_VALUE(vtop->r2) * 8 + REG_VALUE(d));
      }
    }
    vtop--;
  }
  assert(gen_reg == 0);
  assert(sse_reg == 0);

  // Copy R10 and R11 into RDX and RCX, respectively
  if (nb_reg_args > 2)
  {
    o(0xd2894c); // Mov %R10, %Rdx
    if (nb_reg_args > 3)
    {
      o(0xd9894c); // Mov %R11, %Rcx
    }
  }

  if (vtop->type.ref->f.func_type != FUNC_NEW) // implies FUNC_OLD or FUNC_ELLIPSIS
    oad(0xb8, nb_sse_args < 8 ? nb_sse_args : 8); // Mov Nb_Sse_Args, %Eax
  gcall_or_jmp(0);
  if (args_size)
    gadd_sp(args_size);
  vtop--;
}

#define FUNC_PROLOG_SIZE 11

static void push_arg_reg(int i)
{
  loc -= 8;
  gen_modrm64(0x89, arg_regs[i], VT_LOCAL, NULL, loc);
}

// Generate Function Prolog Of Type 'T'
void gfunc_prolog(Sym *func_sym)
{
  CType *func_type = &func_sym->type;
  X86_64_Mode mode, ret_mode;
  int i, addr, align, size, reg_count;
  int param_addr = 0, reg_param_index, sse_param_index;
  Sym *sym;
  CType *type;

  sym = func_type->ref;
  addr = PTR_SIZE * 2;
  loc = 0;
  ind += FUNC_PROLOG_SIZE;
  func_sub_sp_offset = ind;
  func_ret_sub = 0;
  ret_mode = classify_x86_64_arg(&func_vt, NULL, &size, &align, &reg_count);

  if (func_var)
  {
    int seen_reg_num, seen_sse_num, seen_stack_size;
    seen_reg_num = ret_mode == x86_64_mode_memory;
    seen_sse_num = 0;
    // Frame Pointer And Return Address
    seen_stack_size = PTR_SIZE * 2;
    // Count The Number Of Seen Parameters
    sym = func_type->ref;
    while ((sym = sym->next) != NULL)
    {
      type = &sym->type;
      mode = classify_x86_64_arg(type, NULL, &size, &align, &reg_count);
      switch (mode)
      {
      default:
stack_arg:
        seen_stack_size = ((seen_stack_size + align - 1) & -align) + size;
        break;

      case x86_64_mode_integer:
        if (seen_reg_num + reg_count > REGN)
          goto stack_arg;
        seen_reg_num += reg_count;
        break;

      case x86_64_mode_sse:
        if (seen_sse_num + reg_count > 8)
          goto stack_arg;
        seen_sse_num += reg_count;
        break;
      }
    }

    loc -= 24;
    // Movl $0X????????, -0X18(%Rbp)
    o(0xe845c7);
    gen_le32(seen_reg_num * 8);
    // Movl $0X????????, -0X14(%Rbp)
    o(0xec45c7);
    gen_le32(seen_sse_num * 16 + 48);
    // Leaq $0X????????, %R11
    o(0x9d8d4c);
    gen_le32(seen_stack_size);
    // Movq %R11, -0X10(%Rbp)
    o(0xf05d894c);
    // Leaq $-200(%Rbp), %R11
    o(0x9d8d4c);
    gen_le32(-176 - 24);
    // Movq %R11, -0X8(%Rbp)
    o(0xf85d894c);

    // Save All Register Passing Arguments
    for (i = 0; i < 8; i++)
    {
      loc -= 16;
      if (!cprime_state->nosse)
      {
        o(0xd60f66); // Movq
        gen_modrm(7 - i, VT_LOCAL, NULL, loc);
      }
      // Movq $0, Loc+8(%Rbp)
      o(0x85c748);
      gen_le32(loc + 8);
      gen_le32(0);
    }
    for (i = 0; i < REGN; i++)
      push_arg_reg(REGN - 1 - i);
  }

  sym = func_type->ref;
  reg_param_index = 0;
  sse_param_index = 0;

  /* if the function returns a structure, then add an
     implicit pointer parameter */
  if (ret_mode == x86_64_mode_memory)
  {
    push_arg_reg(reg_param_index);
    func_vc = loc;
    reg_param_index++;
  }
  // Define Parameters
  while ((sym = sym->next) != NULL)
  {
    type = &sym->type;
    mode = classify_x86_64_arg(type, NULL, &size, &align, &reg_count);
    switch (mode)
    {
    case x86_64_mode_sse:
      if (cprime_state->nosse)
        cprime_error("SSE disabled but floating point arguments used");
      if (sse_param_index + reg_count <= 8)
      {
        // Save Arguments Passed By Register
        loc -= reg_count * 8;
        param_addr = loc;
        for (i = 0; i < reg_count; ++i)
        {
          o(0xd60f66); // Movq
          gen_modrm(sse_param_index, VT_LOCAL, NULL, param_addr + i * 8);
          ++sse_param_index;
        }
      }
      else
      {
        addr = (addr + align - 1) & -align;
        param_addr = addr;
        addr += size;
      }
      break;

    case x86_64_mode_memory:
    case x86_64_mode_x87:
      addr = (addr + align - 1) & -align;
      param_addr = addr;
      addr += size;
      break;

    case x86_64_mode_integer:
    {
      if (reg_param_index + reg_count <= REGN)
      {
        // Save Arguments Passed By Register
        loc -= reg_count * 8;
        param_addr = loc;
        for (i = 0; i < reg_count; ++i)
        {
          gen_modrm64(0x89, arg_regs[reg_param_index], VT_LOCAL, NULL, param_addr + i * 8);
          ++reg_param_index;
        }
      }
      else
      {
        addr = (addr + align - 1) & -align;
        param_addr = addr;
        addr += size;
      }
      break;
    }
    default: break; // nothing to be done for x86_64_mode_none
    }
    gfunc_set_param(sym, param_addr, 0);
  }

#ifdef CONFIG_CPRIME_BCHECK
  if (cprime_state->do_bounds_check)
    gen_bounds_prolog();
#endif
}

// Generate Function Epilog
void gfunc_epilog(void)
{
  int v, saved_ind;

#ifdef CONFIG_CPRIME_BCHECK
  if (cprime_state->do_bounds_check)
    gen_bounds_epilog();
#endif
  o(0xc9); // Leave
  if (func_ret_sub == 0)
  {
    o(0xc3); // Ret
  }
  else
  {
    o(0xc2); // Ret N
    g(func_ret_sub);
    g(func_ret_sub >> 8);
  }
  // Align Local Size To Word & Save Local Variables
  v = (-loc + 15) & -16;
  saved_ind = ind;
  ind = func_sub_sp_offset - FUNC_PROLOG_SIZE;
  o(0xe5894855);  // Push %Rbp, Mov %Rsp, %Rbp
  o(0xec8148);  // Sub Rsp, Stacksize
  gen_le32(v);
  ind = saved_ind;
}

#endif // not PE 

ST_FUNC void gen_fill_nops(int bytes)
{
  while (bytes--)
    g(0x90);
}

// Generate A Jump To A Label
int gjmp(int t)
{
  return gjmp2(0xe9, t);
}

// Generate A Jump To A Fixed Address
void gjmp_addr(int a)
{
  int r;
  r = a - ind - 2;
  if (r == (char)r)
  {
    g(0xeb);
    g(r);
  }
  else
    oad(0xe9, a - ind - 5);
}

ST_FUNC int gjmp_append(int n, int t)
{
  void *p;
  // Insert Vtop->C Jump List In T
  if (n)
  {
    uint32_t n1 = n, n2;
    while ((n2 = read32le(p = cur_text_section->data + n1)))
      n1 = n2;
    write32le(p, t);
    t = n;
  }
  return t;
}

ST_FUNC int gjmp_cond(int op, int t)
{
  if (op & 0x100)
  {
    /* This was a float compare.  If the parity flag is set
       the result was unordered.  For anything except != this
       means false and we don't jump (anding both conditions).
       For != this means true (oring both).
       Take care about inverting the test.  We need to jump
       to our target if the result was unordered and test wasn't NE,
       otherwise if unordered we don't want to jump.  */
    int v = vtop->cmp_r;
    op &= ~0x100;
    if (op ^v ^ (v != TOK_NE))
      o(0x067a);  // Jp +6
    else
    {
      g(0x0f);
      t = gjmp2(0x8a, t); // Jp T
    }
  }
  g(0x0f);
  t = gjmp2(op - 16, t);
  return t;
}

// Generate An Integer Binary Operation
void gen_opi(int op)
{
  int r, fr, opc, c;
  int ll, uu, cc;

  ll = is64_type(vtop[-1].type.t);
  uu = (vtop[-1].type.t &VT_UNSIGNED) != 0;
  cc = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;

  switch (op)
  {
  case '+':
  case TOK_ADDC1: // Add With Carry Generation
    opc = 0;
gen_op8:
    if (cc && (!ll || (int)vtop->c.i == vtop->c.i))
    {
      // Constant Case
      vswap();
      r = gv(RC_INT);
      vswap();
      c = vtop->c.i;
      if (c == (char)c)
      {
        // XXX: generate inc and dec for smaller code ?
        orex(ll, r, 0, 0x83);
        o(0xc0 | (opc << 3) | REG_VALUE(r));
        g(c);
      }
      else
      {
        orex(ll, r, 0, 0x81);
        oad(0xc0 | (opc << 3) | REG_VALUE(r), c);
      }
    }
    else
    {
      gv2(RC_INT, RC_INT);
      r = vtop[-1].r;
      fr = vtop[0].r;
      orex(ll, r, fr, (opc << 3) | 0x01);
      o(0xc0 + REG_VALUE(r) + REG_VALUE(fr) * 8);
    }
    vtop--;
    if (op >= TOK_ULT && op <= TOK_GT)
      vset_VT_CMP(op);
    break;
  case '-':
  case TOK_SUBC1: // Sub With Carry Generation
    opc = 5;
    goto gen_op8;
  case TOK_ADDC2: // Add With Carry Use
    opc = 2;
    goto gen_op8;
  case TOK_SUBC2: // Sub With Carry Use
    opc = 3;
    goto gen_op8;
  case '&':
    opc = 4;
    goto gen_op8;
  case '^':
    opc = 6;
    goto gen_op8;
  case '|':
    opc = 1;
    goto gen_op8;
  case '*':
    gv2(RC_INT, RC_INT);
    r = vtop[-1].r;
    fr = vtop[0].r;
    orex(ll, fr, r, 0xaf0f); // Imul Fr, R
    o(0xc0 + REG_VALUE(fr) + REG_VALUE(r) * 8);
    vtop--;
    break;
  case TOK_SHL:
    opc = 4;
    goto gen_shift;
  case TOK_SHR:
    opc = 5;
    goto gen_shift;
  case TOK_SAR:
    opc = 7;
gen_shift:
    opc = 0xc0 | (opc << 3);
    if (cc)
    {
      // Constant Case
      vswap();
      r = gv(RC_INT);
      vswap();
      orex(ll, r, 0, 0xc1); // Shl/Shr/Sar $Xxx, R
      o(opc | REG_VALUE(r));
      g(vtop->c.i & (ll ? 63 : 31));
    }
    else
    {
      // We Generate The Shift In Ecx
      gv2(RC_INT, RC_RCX);
      r = vtop[-1].r;
      orex(ll, r, 0, 0xd3); // Shl/Shr/Sar %Cl, R
      o(opc | REG_VALUE(r));
    }
    vtop--;
    break;
  case TOK_UDIV:
  case TOK_UMOD:
    uu = 1;
    goto divmod;
  case '/':
  case '%':
  case TOK_PDIV:
    uu = 0;
divmod:
    // First Operand Must Be In Eax
    // XXX: need better constraint for second operand
    gv2(RC_RAX, RC_RCX);
    r = vtop[-1].r;
    fr = vtop[0].r;
    vtop--;
    save_reg(TREG_RDX);
    orex(ll, 0, 0, uu ? 0xd231 : 0x99); // Xor %Edx,%Edx : Cqto
    orex(ll, fr, 0, 0xf7); // Div Fr, %Eax
    o((uu ? 0xf0 : 0xf8) + REG_VALUE(fr));
    if (op == '%' || op == TOK_UMOD)
      r = TREG_RDX;
    else
      r = TREG_RAX;
    vtop->r = r;
    break;
  default:
    opc = 7;
    goto gen_op8;
  }
}

void gen_opl(int op)
{
  gen_opi(op);
}

/* generate a floating point operation 'v = t1 op t2' instruction. The
   two operands are guaranteed to have the same floating point type */
// XXX: need to use ST1 too
void gen_opf(int op)
{
  int a, ft, fc, swapped, r;
  int bt = vtop->type.t &VT_BTYPE;
  int float_type = bt == VT_LDOUBLE ? RC_ST0 : RC_FLOAT;

  if (op == TOK_NEG)   // Unary Minus
  {
    gv(float_type);
    if (float_type == RC_ST0)
    {
      o(0xe0d9); // Fchs
    }
    else
    {
      save_reg(vtop->r);
      o(0x80); // Xor $0X80, $N(Rbp)
      gen_modrm(6, vtop->r, NULL, vtop->c.i + (bt == VT_DOUBLE ? 7 : 3));
      o(0x80);
    }
    return;
  }

  // Convert Constants To Memory References
  if ((vtop[-1].r & (VT_VALMASK | VT_LVAL)) == VT_CONST)
  {
    vswap();
    gv(float_type);
    vswap();
  }
  if ((vtop[0].r & (VT_VALMASK | VT_LVAL)) == VT_CONST)
    gv(float_type);

  // Must Put At Least One Value In The Floating Point Register
  if ((vtop[-1].r & VT_LVAL) &&
      (vtop[0].r & VT_LVAL))
  {
    vswap();
    gv(float_type);
    vswap();
  }
  swapped = 0;
  /* swap the stack if needed so that t1 is the register and t2 is
     the memory reference */
  if (vtop[-1].r & VT_LVAL)
  {
    vswap();
    swapped = 1;
  }
  if ((vtop->type.t & VT_BTYPE) == VT_LDOUBLE)
  {
    if (op >= TOK_ULT && op <= TOK_GT)
    {
      // Load On Stack Second Operand
      load(TREG_ST0, vtop);
      save_reg(TREG_RAX); // eax is used by FP comparison code
      if (op == TOK_GE || op == TOK_GT)
        swapped = !swapped;
      else if (op == TOK_EQ || op == TOK_NE)
        swapped = 0;
      if (swapped)
        o(0xc9d9); // Fxch %St(1)
      if (op == TOK_EQ || op == TOK_NE)
        o(0xe9da); // Fucompp
      else
        o(0xd9de); // Fcompp
      o(0xe0df); // Fnstsw %Ax
      if (op == TOK_EQ)
      {
        o(0x45e480); // And $0X45, %Ah
        o(0x40fC80); // Cmp $0X40, %Ah
      }
      else if (op == TOK_NE)
      {
        o(0x45e480); // And $0X45, %Ah
        o(0x40f480); // Xor $0X40, %Ah
        op = TOK_NE;
      }
      else if (op == TOK_GE || op == TOK_LE)
      {
        o(0x05c4f6); // Test $0X05, %Ah
        op = TOK_EQ;
      }
      else
      {
        o(0x45c4f6); // Test $0X45, %Ah
        op = TOK_EQ;
      }
      vtop--;
      vset_VT_CMP(op);
    }
    else
    {
      // no memory reference possible for long double operations
      load(TREG_ST0, vtop);
      swapped = !swapped;

      switch (op)
      {
      default:
      case '+':
        a = 0;
        break;
      case '-':
        a = 4;
        if (swapped)
          a++;
        break;
      case '*':
        a = 1;
        break;
      case '/':
        a = 6;
        if (swapped)
          a++;
        break;
      }
      ft = vtop->type.t;
      fc = vtop->c.i;
      o(0xde); // Fxxxp %St, %St(1)
      o(0xc1 + (a << 3));
      vtop--;
    }
  }
  else
  {
    if (op >= TOK_ULT && op <= TOK_GT)
    {
      // if saved lvalue, then we must reload it
      r = vtop->r;
      fc = vtop->c.i;
      if ((r & VT_VALMASK) == VT_LLOCAL)
      {
        SValue v1;
        r = get_reg(RC_INT);
        v1.type.t = VT_PTR;
        v1.r = VT_LOCAL | VT_LVAL;
        v1.c.i = fc;
        v1.sym = NULL;
        load(r, &v1);
        fc = 0;
        vtop->r = r = r | VT_LVAL;
      }

      if (op == TOK_EQ || op == TOK_NE)
        swapped = 0;
      else
      {
        if (op == TOK_LE || op == TOK_LT)
          swapped = !swapped;
        if (op == TOK_LE || op == TOK_GE)
        {
          op = 0x93; // Setae
        }
        else
        {
          op = 0x97; // Seta
        }
      }

      if (swapped)
      {
        gv(RC_FLOAT);
        vswap();
      }
      assert(!(vtop[-1].r &VT_LVAL));

      if ((vtop->type.t & VT_BTYPE) == VT_DOUBLE)
        o(0x66);
      if (op == TOK_EQ || op == TOK_NE)
        o(0x2e0f); // Ucomisd
      else
        o(0x2f0f); // Comisd

      if (vtop->r & VT_LVAL)
        gen_modrm(vtop[-1].r, r, vtop->sym, fc);
      else
        o(0xc0 + REG_VALUE(vtop[0].r) + REG_VALUE(vtop[-1].r) * 8);

      vtop--;
      vset_VT_CMP(op | 0x100);
      vtop->cmp_r = op;
    }
    else
    {
      assert((vtop->type.t &VT_BTYPE) != VT_LDOUBLE);
      switch (op)
      {
      default:
      case '+':
        a = 0;
        break;
      case '-':
        a = 4;
        break;
      case '*':
        a = 1;
        break;
      case '/':
        a = 6;
        break;
      }
      ft = vtop->type.t;
      fc = vtop->c.i;
      assert((ft &VT_BTYPE) != VT_LDOUBLE);

      r = vtop->r;
      // if saved lvalue, then we must reload it
      if ((vtop->r & VT_VALMASK) == VT_LLOCAL)
      {
        SValue v1;
        r = get_reg(RC_INT);
        v1.type.t = VT_PTR;
        v1.r = VT_LOCAL | VT_LVAL;
        v1.c.i = fc;
        v1.sym = NULL;
        load(r, &v1);
        fc = 0;
        vtop->r = r = r | VT_LVAL;
      }

      assert(!(vtop[-1].r &VT_LVAL));
      if (swapped)
      {
        assert(vtop->r &VT_LVAL);
        gv(RC_FLOAT);
        vswap();
        fc = vtop->c.i; // Bcheck May Have Saved Previous Vtop[-1]
        r = vtop->r;
      }

      if ((ft & VT_BTYPE) == VT_DOUBLE)
        o(0xf2);
      else
        o(0xf3);
      o(0x0f);
      o(0x58 + a);

      if (vtop->r & VT_LVAL)
        gen_modrm(vtop[-1].r, r, vtop->sym, fc);
      else
        o(0xc0 + REG_VALUE(vtop[0].r) + REG_VALUE(vtop[-1].r) * 8);

      vtop--;
    }
  }
}

/* convert integers to fp 't' type. Must handle 'int', 'unsigned int'
   and 'long long' cases. */
void gen_cvt_itof(int t)
{
  if ((t & VT_BTYPE) == VT_LDOUBLE)
  {
    save_reg(TREG_ST0);
    gv(RC_INT);
    if ((vtop->type.t & VT_BTYPE) == VT_LLONG)
    {
      /* signed long long to float/double/long double (unsigned case
         is handled generically) */
      o(0x50 + (vtop->r &VT_VALMASK));  // Push R
      o(0x242cdf); // Fildll (%Rsp)
      o(0x08c48348); // Add $8, %Rsp
    }
    else if ((vtop->type.t & (VT_BTYPE | VT_UNSIGNED)) ==
             (VT_INT | VT_UNSIGNED))
    {
      // unsigned int to float/double/long double
      o(0x6a); // Push $0
      g(0x00);
      o(0x50 + (vtop->r &VT_VALMASK));  // Push R
      o(0x242cdf); // Fildll (%Rsp)
      o(0x10c48348); // Add $16, %Rsp
    }
    else
    {
      // int to float/double/long double
      o(0x50 + (vtop->r &VT_VALMASK));  // Push R
      o(0x2404db); // Fildl (%Rsp)
      o(0x08c48348); // Add $8, %Rsp
    }
    vtop->r = TREG_ST0;
  }
  else
  {
    int r = get_reg(RC_FLOAT);
    gv(RC_INT);
    o(0xf2 + ((t &VT_BTYPE) == VT_FLOAT ? 1 : 0));
    if ((vtop->type.t & (VT_BTYPE | VT_UNSIGNED)) ==
        (VT_INT | VT_UNSIGNED) ||
        (vtop->type.t & VT_BTYPE) == VT_LLONG)
    {
      o(0x48); // REX
    }
    o(0x2a0f);
    o(0xc0 + (vtop->r &VT_VALMASK) + REG_VALUE(r) * 8); // Cvtsi2Sd
    vtop->r = r;
  }
}

// Convert From One Floating Point Type To Another
void gen_cvt_ftof(int t)
{
  int ft, bt, tbt;

  ft = vtop->type.t;
  bt = ft &VT_BTYPE;
  tbt = t &VT_BTYPE;

  if (bt == VT_FLOAT)
  {
    gv(RC_FLOAT);
    if (tbt == VT_DOUBLE)
    {
      o(0x140f); // Unpcklps
      o(0xc0 + REG_VALUE(vtop->r) * 9);
      o(0x5a0f); // Cvtps2Pd
      o(0xc0 + REG_VALUE(vtop->r) * 9);
    }
    else if (tbt == VT_LDOUBLE)
    {
      save_reg(RC_ST0);
      // Movss %Xmm0,-0X10(%Rsp)
      o(0x110ff3);
      o(0x44 + REG_VALUE(vtop->r) * 8);
      o(0xf024);
      o(0xf02444d9); // Flds -0X10(%Rsp)
      vtop->r = TREG_ST0;
    }
  }
  else if (bt == VT_DOUBLE)
  {
    gv(RC_FLOAT);
    if (tbt == VT_FLOAT)
    {
      o(0x140f66); // Unpcklpd
      o(0xc0 + REG_VALUE(vtop->r) * 9);
      o(0x5a0f66); // Cvtpd2Ps
      o(0xc0 + REG_VALUE(vtop->r) * 9);
    }
    else if (tbt == VT_LDOUBLE)
    {
      save_reg(RC_ST0);
      // Movsd %Xmm0,-0X10(%Rsp)
      o(0x110ff2);
      o(0x44 + REG_VALUE(vtop->r) * 8);
      o(0xf024);
      o(0xf02444dd); // Fldl -0X10(%Rsp)
      vtop->r = TREG_ST0;
    }
  }
  else
  {
    int r;
    gv(RC_ST0);
    r = get_reg(RC_FLOAT);
    if (tbt == VT_DOUBLE)
    {
      o(0xf0245cdd); // Fstpl -0X10(%Rsp)
      // Movsd -0X10(%Rsp),%Xmm0
      o(0x100ff2);
      o(0x44 + REG_VALUE(r) * 8);
      o(0xf024);
      vtop->r = r;
    }
    else if (tbt == VT_FLOAT)
    {
      o(0xf0245cd9); // Fstps -0X10(%Rsp)
      // Movss -0X10(%Rsp),%Xmm0
      o(0x100ff3);
      o(0x44 + REG_VALUE(r) * 8);
      o(0xf024);
      vtop->r = r;
    }
  }
}

// Convert Fp To Int 'T' Type
void gen_cvt_ftoi(int t)
{
  int ft, bt, size, r;
  ft = vtop->type.t;
  bt = ft &VT_BTYPE;
  if (bt == VT_LDOUBLE)
  {
    if (t != VT_INT)
    {
      vpush_helper_func(TOK___fixxfdi);
      vswap();
      gfunc_call(1);
      vpushi(0);
      vtop->r = REG_IRET;
      vtop->r2 = REG_IRE2;
      return;
    }
    gen_cvt_ftof(VT_DOUBLE);
    bt = VT_DOUBLE;
  }

  gv(RC_FLOAT);
  if (t != VT_INT)
    size = 8;
  else
    size = 4;

  r = get_reg(RC_INT);
  if (bt == VT_FLOAT)
    o(0xf3);
  else if (bt == VT_DOUBLE)
    o(0xf2);
  else
    assert(0);
  orex(size == 8, r, 0, 0x2c0f); // Cvttss2Si Or Cvttsd2Si
  o(0xc0 + REG_VALUE(vtop->r) + REG_VALUE(r) * 8);
  vtop->r = r;
}

// Generate sign extension from 32 to 64 bits:
ST_FUNC void gen_cvt_sxtw(void)
{
  int r = gv(RC_INT);
  // X86_64 Specific: Movslq
  o(0x6348);
  o(0xc0 + (REG_VALUE(r) << 3) + REG_VALUE(r));
}

// Char/Short To Int Conversion
ST_FUNC void gen_cvt_csti(int t)
{
  int r, sz, xl, ll;
  r = gv(RC_INT);
  sz = !(t &VT_UNSIGNED);
  xl = (t &VT_BTYPE) == VT_SHORT;
  ll = (vtop->type.t &VT_BTYPE) == VT_LLONG;
  orex(ll, r, 0, 0xc0b60f // Mov[Sz] %A[Xl], %Eax
       | (sz << 3 | xl) << 8
       | (REG_VALUE(r) << 3 | REG_VALUE(r)) << 16
      );
}

// Increment Tcov Counter
ST_FUNC void gen_increment_tcov (SValue *sv)
{
  o(0x058348); // Addq $1, Xxx(%Rip)
  greloca(cur_text_section, sv->sym, ind, R_X86_64_PC32, -5);
  gen_le32(0);
  o(1);
}

// Computed Goto Support
ST_FUNC void ggoto(void)
{
  gcall_or_jmp(1);
  vtop--;
}

// Save the stack pointer onto the stack and return the location of its address
ST_FUNC void gen_vla_sp_save(int addr)
{
  // Mov %Rsp,Addr(%Rbp)
  gen_modrm64(0x89, TREG_RSP, VT_LOCAL, NULL, addr);
}

// Restore the SP from a location on the stack
ST_FUNC void gen_vla_sp_restore(int addr)
{
  gen_modrm64(0x8b, TREG_RSP, VT_LOCAL, NULL, addr);
}

#ifdef CPRIME_TARGET_PE
// Save result of gen_vla_alloc onto the stack
ST_FUNC void gen_vla_result(int addr)
{
  // Mov %Rax,Addr(%Rbp)
  gen_modrm64(0x89, TREG_RAX, VT_LOCAL, NULL, addr);
}
#endif

// Subtract from the stack pointer, and push the resulting value onto the stack
ST_FUNC void gen_vla_alloc(CType *type, int align)
{
  int use_call = 0;

#if defined(CONFIG_CPRIME_BCHECK)
  use_call = cprime_state->do_bounds_check;
#endif
#ifdef CPRIME_TARGET_PE  // alloca does more than just adjust %rsp on Windows 
  use_call = 1;
#endif
  if (use_call)
  {
    vpush_helper_func(TOK_alloca);
    vswap(); // Move alloca ref past allocation size
    gfunc_call(1);
  }
  else
  {
    int r;
    r = gv(RC_INT); // Allocation Size
    // Sub R,%Rsp
    o(0x2b48);
    o(0xe0 | REG_VALUE(r));
    // We align to 16 bytes rather than align
    // And ~15, %Rsp
    o(0xf0e48348);
    vpop();
  }
}

/*
 * Assmuing the top part of the stack looks like below,
 *  src dest src
 */
ST_FUNC void gen_struct_copy(int size)
{
  int n = size / PTR_SIZE;
#ifdef CPRIME_TARGET_PE
  o(0x5756); // Push Rsi, Rdi
#endif
  gv2(RC_RDI, RC_RSI);
  if (n <= 4)
  {
    while (n)
      o(0xa548), --n;
  }
  else
  {
    vpushi(n);
    gv(RC_RCX);
    o(0xa548f3);
    vpop();
  }
  if (size & 0x04)
    o(0xa5);
  if (size & 0x02)
    o(0xa566);
  if (size & 0x01)
    o(0xa4);
#ifdef CPRIME_TARGET_PE
  o(0x5e5f); // Pop Rdi, Rsi
#endif
  vpop();
  vpop();
}

// End Of X86-64 Code Generator
//***********************************************************
#endif // ! TARGET_DEFS_ONLY 
//****************************************************






