#ifndef _CPRIME_OBJECT_H
#define _CPRIME_OBJECT_H 1

/* Internal object-format definitions used by the Windows build. */

#ifndef _WIN32
#include <inttypes.h>
#else
#ifndef __int8_t_defined
#define __int8_t_defined
typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef long long int int64_t;
typedef unsigned char           uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned int            uint32_t;
typedef unsigned long long int  uint64_t;
#endif
#endif

typedef uint16_t Obj32_Half;
typedef uint16_t Obj64_Half;
typedef Obj32_Half ObjHalf;

typedef uint32_t Obj32_Word;
typedef int32_t  Obj32_Sword;
typedef uint32_t Obj64_Word;
typedef int32_t  Obj64_Sword;
typedef Obj64_Word ObjWord;

typedef uint64_t Obj32_Xword;
typedef int64_t  Obj32_Sxword;
typedef uint64_t Obj64_Xword;
typedef int64_t  Obj64_Sxword;

typedef uint32_t Obj32_Addr;
typedef uint64_t Obj64_Addr;
typedef Obj64_Addr ObjAddr;

typedef uint32_t Obj32_Off;
typedef uint64_t Obj64_Off;
typedef Obj64_Off ObjOff;

typedef uint16_t Obj32_Section;
typedef uint16_t Obj64_Section;
typedef Obj64_Section ObjSection;

typedef Obj32_Half Obj32_Versym;
typedef Obj64_Half Obj64_Versym;
typedef Obj64_Half ObjHalfVersym;

#define EI_NIDENT (16)

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];
  Obj32_Half	e_type;
  Obj32_Half	e_machine;
  Obj32_Word	e_version;
  Obj32_Addr	e_entry;
  Obj32_Off	e_phoff;
  Obj32_Off	e_shoff;
  Obj32_Word	e_flags;
  Obj32_Half	e_ehsize;
  Obj32_Half	e_phentsize;
  Obj32_Half	e_phnum;
  Obj32_Half	e_shentsize;
  Obj32_Half	e_shnum;
  Obj32_Half	e_shstrndx;
} Obj32_Ehdr;
typedef Obj32_Ehdr Obj32_Ehdr;

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];
  Obj64_Half	e_type;
  Obj64_Half	e_machine;
  Obj64_Word	e_version;
  Obj64_Addr	e_entry;
  Obj64_Off	e_phoff;
  Obj64_Off	e_shoff;
  Obj64_Word	e_flags;
  Obj64_Half	e_ehsize;
  Obj64_Half	e_phentsize;
  Obj64_Half	e_phnum;
  Obj64_Half	e_shentsize;
  Obj64_Half	e_shnum;
  Obj64_Half	e_shstrndx;
} Obj64_Ehdr;

typedef Obj64_Ehdr ObjEhdr;
typedef Obj64_Ehdr Obj64_Ehdr;

#define EI_MAG0		0
#define ELFMAG0		0x7f

#define EI_MAG1		1
#define ELFMAG1		'E'

#define EI_MAG2		2
#define ELFMAG2		'L'

#define EI_MAG3		3
#define ELFMAG3		'F'

#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define EI_CLASS	4
#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2
#define ELFCLASSNUM	3

#define EI_DATA		5
#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2
#define ELFDATANUM	3

#define EI_VERSION	6

#define EI_OSABI	7
#define ELFOSABI_NONE		0
#define ELFOSABI_SYSV		0
#define ELFOSABI_HPUX		1
#define ELFOSABI_NETBSD		2
#define ELFOSABI_GNU		3
#define ELFOSABI_LINUX		ELFOSABI_GNU
#define ELFOSABI_SOLARIS	6
#define ELFOSABI_AIX		7
#define ELFOSABI_IRIX		8
#define ELFOSABI_FREEBSD	9
#define ELFOSABI_TRU64		10
#define ELFOSABI_MODESTO	11
#define ELFOSABI_OPENBSD	12
#define ELFOSABI_OPENVMS        13
#define ELFOSABI_NSK            14
#define ELFOSABI_AROS           15
#define ELFOSABI_FENIXOS        16
#define ELFOSABI_ARM_AEABI	64
#define ELFOSABI_C6000_LINUX    65
#define ELFOSABI_ARM		97
#define ELFOSABI_STANDALONE	255

#define EI_ABIVERSION	8

#define EI_PAD		9

#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define	ET_NUM		5
#define ET_LOOS		0xfe00
#define ET_HIOS		0xfeff
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

#define EM_NONE		 0
#define EM_M32		 1
#define EM_SPARC	 2
#define EM_386		 3
#define EM_68K		 4
#define EM_88K		 5
#define EM_860		 7
#define EM_MIPS		 8
#define EM_S370		 9
#define EM_MIPS_RS3_LE	10

#define EM_PARISC	15
#define EM_VPP500	17
#define EM_SPARC32PLUS	18
#define EM_960		19
#define EM_PPC		20
#define EM_PPC64	21
#define EM_S390		22

#define EM_V800		36
#define EM_FR20		37
#define EM_RH32		38
#define EM_RCE		39
#define EM_ARM		40
#define EM_FAKE_ALPHA	41
#define EM_SH		42
#define EM_SPARCV9	43
#define EM_TRICORE	44
#define EM_ARC		45
#define EM_H8_300	46
#define EM_H8_300H	47
#define EM_H8S		48
#define EM_H8_500	49
#define EM_IA_64	50
#define EM_MIPS_X	51
#define EM_COLDFIRE	52
#define EM_68HC12	53
#define EM_MMA		54
#define EM_PCP		55
#define EM_NCPU		56
#define EM_NDR1		57
#define EM_STARCORE	58
#define EM_ME16		59
#define EM_ST100	60
#define EM_TINYJ	61
#define EM_X86_64	62
#define EM_PDSP		63

#define EM_FX66		66
#define EM_ST9PLUS	67
#define EM_ST7		68
#define EM_68HC16	69
#define EM_68HC11	70
#define EM_68HC08	71
#define EM_68HC05	72
#define EM_SVX		73
#define EM_ST19		74
#define EM_VAX		75
#define EM_CRIS		76
#define EM_JAVELIN	77
#define EM_FIREPATH	78
#define EM_ZSP		79
#define EM_MMIX		80
#define EM_HUANY	81
#define EM_PRISM	82
#define EM_AVR		83
#define EM_FR30		84
#define EM_D10V		85
#define EM_D30V		86
#define EM_V850		87
#define EM_M32R		88
#define EM_MN10300	89
#define EM_MN10200	90
#define EM_PJ		91
#define EM_OPENRISC	92
#define EM_ARC_A5	93
#define EM_XTENSA	94
#define EM_AARCH64	183
#define EM_TILEPRO	188
#define EM_TILEGX	191
#define EM_RISCV	243
#define EM_NUM		253

#define EM_ALPHA	0x9026
#define EM_C60		0x9c60

#define EV_NONE		0
#define EV_CURRENT	1
#define EV_NUM		2

typedef struct
{
  Obj32_Word	sh_name;
  Obj32_Word	sh_type;
  Obj32_Word	sh_flags;
  Obj32_Addr	sh_addr;
  Obj32_Off	sh_offset;
  Obj32_Word	sh_size;
  Obj32_Word	sh_link;
  Obj32_Word	sh_info;
  Obj32_Word	sh_addralign;
  Obj32_Word	sh_entsize;
} Obj32_Shdr;

typedef struct
{
  Obj64_Word	sh_name;
  Obj64_Word	sh_type;
  Obj64_Xword	sh_flags;
  Obj64_Addr	sh_addr;
  Obj64_Off	sh_offset;
  Obj64_Xword	sh_size;
  Obj64_Word	sh_link;
  Obj64_Word	sh_info;
  Obj64_Xword	sh_addralign;
  Obj64_Xword	sh_entsize;
} Obj64_Shdr;

typedef Obj64_Shdr ObjShdr;
typedef Obj32_Shdr Obj32_Shdr;
typedef Obj64_Shdr Obj64_Shdr;

#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_BEFORE	0xff00
#define SHN_AFTER	0xff01
#define SHN_HIPROC	0xff1f
#define SHN_LOOS	0xff20
#define SHN_HIOS	0xff3f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_XINDEX	0xffff
#define SHN_HIRESERVE	0xffff

#define SHT_NULL	  0
#define SHT_PROGBITS	  1
#define SHT_SYMTAB	  2
#define SHT_STRTAB	  3
#define SHT_RELA	  4
#define SHT_HASH	  5
#define SHT_DYNAMIC	  6
#define SHT_NOTE	  7
#define SHT_NOBITS	  8
#define SHT_REL		  9
#define SHT_SHLIB	  10
#define SHT_DYNSYM	  11
#define SHT_INIT_ARRAY	  14
#define SHT_FINI_ARRAY	  15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP	  17
#define SHT_SYMTAB_SHNDX  18
#define	SHT_NUM		  19
#define SHT_LOOS	  0x60000000
#define SHT_GNU_ATTRIBUTES 0x6ffffff5
#define SHT_GNU_HASH	  0x6ffffff6
#define SHT_GNU_LIBLIST	  0x6ffffff7
#define SHT_CHECKSUM	  0x6ffffff8
#define SHT_LOSUNW	  0x6ffffffa
#define SHT_SUNW_move	  0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef	  0x6ffffffd
#define SHT_GNU_verneed	  0x6ffffffe
#define SHT_GNU_versym	  0x6fffffff
#define SHT_HISUNW	  0x6fffffff
#define SHT_HIOS	  0x6fffffff
#define SHT_LOPROC	  0x70000000
#define SHT_HIPROC	  0x7fffffff
#define SHT_LOUSER	  0x80000000
#define SHT_HIUSER	  0x8fffffff

#define SHF_WRITE	     (1 << 0)
#define SHF_ALLOC	     (1 << 1)
#define SHF_EXECINSTR	     (1 << 2)
#define SHF_MERGE	     (1 << 4)
#define SHF_STRINGS	     (1 << 5)
#define SHF_INFO_LINK	     (1 << 6)
#define SHF_LINK_ORDER	     (1 << 7)
#define SHF_OS_NONCONFORMING (1 << 8)
#define SHF_GROUP	     (1 << 9)
#define SHF_TLS		     (1 << 10)
#define SHF_COMPRESSED	     (1 << 11)
#define SHF_MASKOS	     0x0ff00000
#define SHF_MASKPROC	     0xf0000000
#define SHF_ORDERED	     (1 << 30)
#define SHF_EXCLUDE	     (1U << 31)

#define GRP_COMDAT	0x1

typedef struct
{
  Obj32_Word	st_name;
  Obj32_Addr	st_value;
  Obj32_Word	st_size;
  unsigned char	st_info;
  unsigned char	st_other;
  Obj32_Section	st_shndx;
} Obj32_Sym;

typedef struct
{
  Obj64_Word	st_name;
  unsigned char	st_info;
  unsigned char st_other;
  Obj64_Section	st_shndx;
  Obj64_Addr	st_value;
  Obj64_Xword	st_size;
} Obj64_Sym;

typedef Obj64_Sym ObjSym;
typedef Obj32_Sym Obj32_Sym;
typedef Obj64_Sym Obj64_Sym;

typedef struct
{
  Obj32_Half si_boundto;
  Obj32_Half si_flags;
} Obj32_Syminfo;

typedef struct
{
  Obj64_Half si_boundto;
  Obj64_Half si_flags;
} Obj64_Syminfo;

#define SYMINFO_BT_SELF		0xffff
#define SYMINFO_BT_PARENT	0xfffe
#define SYMINFO_BT_LOWRESERVE	0xff00

#define SYMINFO_FLG_DIRECT	0x0001
#define SYMINFO_FLG_PASSTHRU	0x0002
#define SYMINFO_FLG_COPY	0x0004
#define SYMINFO_FLG_LAZYLOAD	0x0008

#define SYMINFO_NONE		0
#define SYMINFO_CURRENT		1
#define SYMINFO_NUM		2

#define Obj32_ST_BIND(val)		(((unsigned char) (val)) >> 4)
#define Obj32_ST_TYPE(val)		((val) & 0xf)
#define Obj32_ST_INFO(bind, type)	(((bind) << 4) + ((type) & 0xf))

#define Obj64_ST_BIND(val)		Obj32_ST_BIND (val)
#define Obj64_ST_TYPE(val)		Obj32_ST_TYPE (val)
#define Obj64_ST_INFO(bind, type)	Obj32_ST_INFO ((bind), (type))

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define	STB_NUM		3
#define STB_LOOS	10
#define STB_GNU_UNIQUE	10
#define STB_HIOS	12
#define STB_LOPROC	13
#define STB_HIPROC	15

#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_COMMON	5
#define STT_TLS		6
#define	STT_NUM		7
#define STT_LOOS	10
#define STT_GNU_IFUNC	10
#define STT_HIOS	12
#define STT_LOPROC	13
#define STT_HIPROC	15

#define STN_UNDEF	0

#define Obj32_ST_VISIBILITY(o)	((o) & 0x03)

#define Obj64_ST_VISIBILITY(o)	Obj32_ST_VISIBILITY (o)

#define STV_DEFAULT	0
#define STV_INTERNAL	1
#define STV_HIDDEN	2
#define STV_PROTECTED	3

typedef struct
{
  Obj32_Addr	r_offset;
  Obj32_Word	r_info;
} Obj32_Rel;

typedef struct
{
  Obj64_Addr	r_offset;
  Obj64_Xword	r_info;
} Obj64_Rel;

typedef Obj64_Rel ObjRel;
typedef Obj32_Rel Obj32_Rel;
typedef Obj64_Rel Obj64_Rel;

typedef struct
{
  Obj32_Addr	r_offset;
  Obj32_Word	r_info;
  Obj32_Sword	r_addend;
} Obj32_Rela;

typedef struct
{
  Obj64_Addr	r_offset;
  Obj64_Xword	r_info;
  Obj64_Sxword	r_addend;
} Obj64_Rela;

typedef Obj64_Rela ObjRela;
typedef Obj32_Rela Obj32_Rela;
typedef Obj64_Rela Obj64_Rela;

#define Obj32_R_SYM(val)		((val) >> 8)
#define Obj32_R_TYPE(val)		((val) & 0xff)
#define Obj32_R_INFO(sym, type)		(((sym) << 8) + ((type) & 0xff))

#define Obj64_R_SYM(i)			((i) >> 32)
#define Obj64_R_TYPE(i)			((i) & 0xffffffff)
#define Obj64_R_INFO(sym,type)		((((Obj64_Xword) (sym)) << 32) + (type))

typedef struct
{
  Obj32_Word	p_type;
  Obj32_Off	p_offset;
  Obj32_Addr	p_vaddr;
  Obj32_Addr	p_paddr;
  Obj32_Word	p_filesz;
  Obj32_Word	p_memsz;
  Obj32_Word	p_flags;
  Obj32_Word	p_align;
} Obj32_Phdr;

typedef struct
{
  Obj64_Word	p_type;
  Obj64_Word	p_flags;
  Obj64_Off	p_offset;
  Obj64_Addr	p_vaddr;
  Obj64_Addr	p_paddr;
  Obj64_Xword	p_filesz;
  Obj64_Xword	p_memsz;
  Obj64_Xword	p_align;
} Obj64_Phdr;

typedef Obj64_Phdr ObjPhdr;
typedef Obj32_Phdr Obj32_Phdr;
typedef Obj64_Phdr Obj64_Phdr;

#define PN_XNUM		0xffff

#define	PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_TLS		7
#define	PT_NUM		8
#define PT_LOOS		0x60000000
#define PT_GNU_EH_FRAME	0x6474e550
#define PT_GNU_STACK	0x6474e551
#define PT_GNU_RELRO	0x6474e552
#define PT_LOSUNW	0x6ffffffa
#define PT_SUNWBSS	0x6ffffffa
#define PT_SUNWSTACK	0x6ffffffb
#define PT_HISUNW	0x6fffffff
#define PT_HIOS		0x6fffffff
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

#define PF_X		(1 << 0)
#define PF_W		(1 << 1)
#define PF_R		(1 << 2)
#define PF_MASKOS	0x0ff00000
#define PF_MASKPROC	0xf0000000

#define NT_PRSTATUS	1
#define NT_FPREGSET	2
#define NT_PRPSINFO	3
#define NT_PRXREG	4
#define NT_TASKSTRUCT	4
#define NT_PLATFORM	5
#define NT_AUXV		6
#define NT_GWINDOWS	7
#define NT_ASRS		8
#define NT_PSTATUS	10
#define NT_PSINFO	13
#define NT_PRCRED	14
#define NT_UTSNAME	15
#define NT_LWPSTATUS	16
#define NT_LWPSINFO	17
#define NT_PRFPXREG	20
#define NT_PRXFPREG	0x46e62b7f
#define NT_PPC_VMX	0x100
#define NT_PPC_SPE	0x101
#define NT_PPC_VSX	0x102
#define NT_386_TLS	0x200
#define NT_386_IOPERM	0x201
#define NT_X86_XSTATE	0x202
#define NT_S390_HIGH_GPRS	0x300
#define NT_S390_TIMER	0x301
#define NT_S390_TODCMP	0x302
#define NT_S390_TODPREG	0x303
#define NT_S390_CTRS	0x304
#define NT_S390_PREFIX	0x305
#define NT_S390_LAST_BREAK	0x306
#define NT_S390_SYSTEM_CALL	0x307
#define NT_ARM_VFP	0x400
#define NT_ARM_TLS	0x401
#define NT_ARM_HW_BREAK	0x402
#define NT_ARM_HW_WATCH	0x403

#define NT_VERSION	1

typedef struct
{
  Obj32_Sword	d_tag;
  union
    {
      Obj32_Word d_val;
      Obj32_Addr d_ptr;
    } d_un;
} Obj32_Dyn;

typedef struct
{
  Obj64_Sxword	d_tag;
  union
    {
      Obj64_Xword d_val;
      Obj64_Addr d_ptr;
    } d_un;
} Obj64_Dyn;

#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH	15
#define DT_SYMBOLIC	16
#define DT_REL		17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23
#define	DT_BIND_NOW	24
#define	DT_INIT_ARRAY	25
#define	DT_FINI_ARRAY	26
#define	DT_INIT_ARRAYSZ	27
#define	DT_FINI_ARRAYSZ	28
#define DT_RUNPATH	29
#define DT_FLAGS	30
#define DT_ENCODING	32
#define DT_PREINIT_ARRAY 32
#define DT_PREINIT_ARRAYSZ 33
#define	DT_NUM		34
#define DT_LOOS		0x6000000d
#define DT_HIOS		0x6ffff000
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff
#define	DT_PROCNUM	DT_MIPS_NUM

#define DT_VALRNGLO	0x6ffffd00
#define DT_GNU_PRELINKED 0x6ffffdf5
#define DT_GNU_CONFLICTSZ 0x6ffffdf6
#define DT_GNU_LIBLISTSZ 0x6ffffdf7
#define DT_CHECKSUM	0x6ffffdf8
#define DT_PLTPADSZ	0x6ffffdf9
#define DT_MOVEENT	0x6ffffdfa
#define DT_MOVESZ	0x6ffffdfb
#define DT_FEATURE_1	0x6ffffdfc
#define DT_POSFLAG_1	0x6ffffdfd
#define DT_SYMINSZ	0x6ffffdfe
#define DT_SYMINENT	0x6ffffdff
#define DT_VALRNGHI	0x6ffffdff
#define DT_VALTAGIDX(tag)	(DT_VALRNGHI - (tag))
#define DT_VALNUM 12

#define DT_ADDRRNGLO	0x6ffffe00
#define DT_GNU_HASH	0x6ffffef5
#define DT_TLSDESC_PLT	0x6ffffef6
#define DT_TLSDESC_GOT	0x6ffffef7
#define DT_GNU_CONFLICT	0x6ffffef8
#define DT_GNU_LIBLIST	0x6ffffef9
#define DT_CONFIG	0x6ffffefa
#define DT_DEPAUDIT	0x6ffffefb
#define DT_AUDIT	0x6ffffefc
#define	DT_PLTPAD	0x6ffffefd
#define	DT_MOVETAB	0x6ffffefe
#define DT_SYMINFO	0x6ffffeff
#define DT_ADDRRNGHI	0x6ffffeff
#define DT_ADDRTAGIDX(tag)	(DT_ADDRRNGHI - (tag))
#define DT_ADDRNUM 11

#define DT_VERSYM	0x6ffffff0

#define DT_RELACOUNT	0x6ffffff9
#define DT_RELCOUNT	0x6ffffffa

#define DT_FLAGS_1	0x6ffffffb
#define	DT_VERDEF	0x6ffffffc
#define	DT_VERDEFNUM	0x6ffffffd
#define	DT_VERNEED	0x6ffffffe
#define	DT_VERNEEDNUM	0x6fffffff
#define DT_VERSIONTAGIDX(tag)	(DT_VERNEEDNUM - (tag))
#define DT_VERSIONTAGNUM 16

#define DT_AUXILIARY    0x7ffffffd
#define DT_FILTER       0x7fffffff
#define DT_EXTRATAGIDX(tag)	((Obj32_Word)-((Obj32_Sword) (tag) <<1>>1)-1)
#define DT_EXTRANUM	3

#define DF_ORIGIN	0x00000001
#define DF_SYMBOLIC	0x00000002
#define DF_TEXTREL	0x00000004
#define DF_BIND_NOW	0x00000008
#define DF_STATIC_TLS	0x00000010

#define DF_1_NOW	0x00000001
#define DF_1_GLOBAL	0x00000002
#define DF_1_GROUP	0x00000004
#define DF_1_NODELETE	0x00000008
#define DF_1_LOADFLTR	0x00000010
#define DF_1_INITFIRST	0x00000020
#define DF_1_NOOPEN	0x00000040
#define DF_1_ORIGIN	0x00000080
#define DF_1_DIRECT	0x00000100
#define DF_1_TRANS	0x00000200
#define DF_1_INTERPOSE	0x00000400
#define DF_1_NODEFLIB	0x00000800
#define DF_1_NODUMP	0x00001000
#define DF_1_CONFALT	0x00002000
#define DF_1_ENDFILTEE	0x00004000
#define	DF_1_DISPRELDNE	0x00008000
#define	DF_1_DISPRELPND	0x00010000
#define	DF_1_NODIRECT	0x00020000
#define	DF_1_IGNMULDEF	0x00040000
#define	DF_1_NOKSYMS	0x00080000
#define	DF_1_NOHDR	0x00100000
#define	DF_1_EDITED	0x00200000
#define	DF_1_NORELOC	0x00400000
#define	DF_1_SYMINTPOSE	0x00800000
#define	DF_1_GLOBAUDIT	0x01000000
#define	DF_1_SINGLETON	0x02000000
#define	DF_1_PIE	0x08000000

#define DTF_1_PARINIT	0x00000001
#define DTF_1_CONFEXP	0x00000002

#define DF_P1_LAZYLOAD	0x00000001
#define DF_P1_GROUPPERM	0x00000002

typedef struct
{
  Obj32_Half	vd_version;
  Obj32_Half	vd_flags;
  Obj32_Half	vd_ndx;
  Obj32_Half	vd_cnt;
  Obj32_Word	vd_hash;
  Obj32_Word	vd_aux;
  Obj32_Word	vd_next;
} Obj32_Verdef;

typedef struct
{
  Obj64_Half	vd_version;
  Obj64_Half	vd_flags;
  Obj64_Half	vd_ndx;
  Obj64_Half	vd_cnt;
  Obj64_Word	vd_hash;
  Obj64_Word	vd_aux;
  Obj64_Word	vd_next;
} Obj64_Verdef;

#define VER_DEF_NONE	0
#define VER_DEF_CURRENT	1
#define VER_DEF_NUM	2

#define VER_FLG_BASE	0x1
#define VER_FLG_WEAK	0x2

#define	VER_NDX_LOCAL		0
#define	VER_NDX_GLOBAL		1
#define	VER_NDX_LORESERVE	0xff00
#define	VER_NDX_ELIMINATE	0xff01

typedef struct
{
  Obj32_Word	vda_name;
  Obj32_Word	vda_next;
} Obj32_Verdaux;

typedef struct
{
  Obj64_Word	vda_name;
  Obj64_Word	vda_next;
} Obj64_Verdaux;

typedef struct
{
  Obj32_Half	vn_version;
  Obj32_Half	vn_cnt;
  Obj32_Word	vn_file;
  Obj32_Word	vn_aux;
  Obj32_Word	vn_next;
} Obj32_Verneed;

typedef struct
{
  Obj64_Half	vn_version;
  Obj64_Half	vn_cnt;
  Obj64_Word	vn_file;
  Obj64_Word	vn_aux;
  Obj64_Word	vn_next;
} Obj64_Verneed;

#define VER_NEED_NONE	 0
#define VER_NEED_CURRENT 1
#define VER_NEED_NUM	 2

typedef struct
{
  Obj32_Word	vna_hash;
  Obj32_Half	vna_flags;
  Obj32_Half	vna_other;
  Obj32_Word	vna_name;
  Obj32_Word	vna_next;
} Obj32_Vernaux;

typedef struct
{
  Obj64_Word	vna_hash;
  Obj64_Half	vna_flags;
  Obj64_Half	vna_other;
  Obj64_Word	vna_name;
  Obj64_Word	vna_next;
} Obj64_Vernaux;

#define VER_FLG_WEAK	0x2

typedef struct
{
  uint32_t a_type;
  union
    {
      uint32_t a_val;

    } a_un;
} Obj32_auxv_t;

typedef struct
{
  uint64_t a_type;
  union
    {
      uint64_t a_val;

    } a_un;
} Obj64_auxv_t;

#define AT_NULL		0
#define AT_IGNORE	1
#define AT_EXECFD	2
#define AT_PHDR		3
#define AT_PHENT	4
#define AT_PHNUM	5
#define AT_PAGESZ	6
#define AT_BASE		7
#define AT_FLAGS	8
#define AT_ENTRY	9
#define AT_NOTELF	10
#define AT_UID		11
#define AT_EUID		12
#define AT_GID		13
#define AT_EGID		14
#define AT_CLKTCK	17

#define AT_PLATFORM	15
#define AT_HWCAP	16

#define AT_FPUCW	18

#define AT_DCACHEBSIZE	19
#define AT_ICACHEBSIZE	20
#define AT_UCACHEBSIZE	21

#define AT_IGNOREPPC	22

#define	AT_SECURE	23

#define AT_BASE_PLATFORM 24

#define AT_RANDOM	25

#define AT_EXECFN	31

#define AT_SYSINFO	32
#define AT_SYSINFO_EHDR	33

#define AT_L1I_CACHESHAPE	34
#define AT_L1D_CACHESHAPE	35
#define AT_L2_CACHESHAPE	36
#define AT_L3_CACHESHAPE	37

typedef struct
{
  Obj32_Word n_namesz;
  Obj32_Word n_descsz;
  Obj32_Word n_type;
} Obj32_Nhdr;

typedef struct
{
  Obj64_Word n_namesz;
  Obj64_Word n_descsz;
  Obj64_Word n_type;
} Obj64_Nhdr;

#define ELF_NOTE_SOLARIS	"SUNW Solaris"

#define ELF_NOTE_GNU		"GNU"

#define ELF_NOTE_PAGESIZE_HINT	1

#define NT_GNU_ABI_TAG	1
#define ELF_NOTE_ABI	NT_GNU_ABI_TAG

#define ELF_NOTE_OS_LINUX	0
#define ELF_NOTE_OS_GNU		1
#define ELF_NOTE_OS_SOLARIS2	2
#define ELF_NOTE_OS_FREEBSD	3

#define NT_GNU_HWCAP	2

#define NT_GNU_BUILD_ID	3

#define NT_GNU_GOLD_VERSION	4

typedef struct
{
  Obj32_Xword m_value;
  Obj32_Word m_info;
  Obj32_Word m_poffset;
  Obj32_Half m_repeat;
  Obj32_Half m_stride;
} Obj32_Move;

typedef struct
{
  Obj64_Xword m_value;
  Obj64_Xword m_info;
  Obj64_Xword m_poffset;
  Obj64_Half m_repeat;
  Obj64_Half m_stride;
} Obj64_Move;

#define Obj32_M_SYM(info)	((info) >> 8)
#define Obj32_M_SIZE(info)	((unsigned char) (info))
#define Obj32_M_INFO(sym, size)	(((sym) << 8) + (unsigned char) (size))

#define Obj64_M_SYM(info)	Obj32_M_SYM (info)
#define Obj64_M_SIZE(info)	Obj32_M_SIZE (info)
#define Obj64_M_INFO(sym, size)	Obj32_M_INFO (sym, size)

#define EF_CPU32	0x00810000

#define R_X86_64_NONE		0
#define R_X86_64_64		1
#define R_X86_64_PC32		2
#define R_X86_64_GOT32		3
#define R_X86_64_PLT32		4
#define R_X86_64_COPY		5
#define R_X86_64_GLOB_DAT	6
#define R_X86_64_JUMP_SLOT	7
#define R_X86_64_RELATIVE	8
#define R_X86_64_GOTPCREL	9
#define R_X86_64_32		10
#define R_X86_64_32S		11
#define R_X86_64_16		12
#define R_X86_64_PC16		13
#define R_X86_64_8		14
#define R_X86_64_PC8		15
#define R_X86_64_DTPMOD64	16
#define R_X86_64_DTPOFF64	17
#define R_X86_64_TPOFF64	18
#define R_X86_64_TLSGD		19
#define R_X86_64_TLSLD		20
#define R_X86_64_DTPOFF32	21
#define R_X86_64_GOTTPOFF	22
#define R_X86_64_TPOFF32	23
#define R_X86_64_PC64		24
#define R_X86_64_GOTOFF64	25
#define R_X86_64_GOTPC32	26
#define R_X86_64_GOT64		27
#define R_X86_64_GOTPCREL64	28
#define R_X86_64_GOTPC64	29
#define R_X86_64_GOTPLT64	30
#define R_X86_64_PLTOFF64	31
#define R_X86_64_SIZE32		32
#define R_X86_64_SIZE64		33
#define R_X86_64_GOTPC32_TLSDESC 34
#define R_X86_64_TLSDESC_CALL   35
#define R_X86_64_TLSDESC        36
#define R_X86_64_IRELATIVE	37
#define R_X86_64_RELATIVE64	38
#define R_X86_64_GOTPCRELX	41
#define R_X86_64_REX_GOTPCRELX	42

#define R_X86_64_NUM		43

#define R_AARCH64_ABS32		258
#define R_AARCH64_ABS64		257
#define R_AARCH64_PREL32		261
#define R_AARCH64_CALL26		283

#define SHT_X86_64_UNWIND       0x70000001

#define R_MN10300_NONE		0
#define R_MN10300_32		1
#define R_MN10300_16		2
#define R_MN10300_8		3
#define R_MN10300_PCREL32	4
#define R_MN10300_PCREL16	5
#define R_MN10300_PCREL8	6
#define R_MN10300_GNU_VTINHERIT	7
#define R_MN10300_GNU_VTENTRY	8
#define R_MN10300_24		9
#define R_MN10300_GOTPC32	10
#define R_MN10300_GOTPC16	11
#define R_MN10300_GOTOFF32	12
#define R_MN10300_GOTOFF24	13
#define R_MN10300_GOTOFF16	14
#define R_MN10300_PLT32		15
#define R_MN10300_PLT16		16
#define R_MN10300_GOT32		17
#define R_MN10300_GOT24		18
#define R_MN10300_GOT16		19
#define R_MN10300_COPY		20
#define R_MN10300_GLOB_DAT	21
#define R_MN10300_JMP_SLOT	22
#define R_MN10300_RELATIVE	23
#define R_MN10300_TLS_GD	24
#define R_MN10300_TLS_LD	25
#define R_MN10300_TLS_LDO	26
#define R_MN10300_TLS_GOTIE	27
#define R_MN10300_TLS_IE	28
#define R_MN10300_TLS_LE	29
#define R_MN10300_TLS_DTPMOD	30
#define R_MN10300_TLS_DTPOFF	31
#define R_MN10300_TLS_TPOFF	32
#define R_MN10300_SYM_DIFF	33
#define R_MN10300_ALIGN		34
#define R_MN10300_NUM		35

#define R_M32R_NONE		0
#define R_M32R_16		1
#define R_M32R_32		2
#define R_M32R_24		3
#define R_M32R_10_PCREL		4
#define R_M32R_18_PCREL		5
#define R_M32R_26_PCREL		6
#define R_M32R_HI16_ULO		7
#define R_M32R_HI16_SLO		8
#define R_M32R_LO16		9
#define R_M32R_SDA16		10
#define R_M32R_GNU_VTINHERIT	11
#define R_M32R_GNU_VTENTRY	12

#define R_M32R_16_RELA		33
#define R_M32R_32_RELA		34
#define R_M32R_24_RELA		35
#define R_M32R_10_PCREL_RELA	36
#define R_M32R_18_PCREL_RELA	37
#define R_M32R_26_PCREL_RELA	38
#define R_M32R_HI16_ULO_RELA	39
#define R_M32R_HI16_SLO_RELA	40
#define R_M32R_LO16_RELA	41
#define R_M32R_SDA16_RELA	42
#define R_M32R_RELA_GNU_VTINHERIT	43
#define R_M32R_RELA_GNU_VTENTRY	44
#define R_M32R_REL32		45

#define R_M32R_GOT24		48
#define R_M32R_26_PLTREL	49
#define R_M32R_COPY		50
#define R_M32R_GLOB_DAT		51
#define R_M32R_JMP_SLOT		52
#define R_M32R_RELATIVE		53
#define R_M32R_GOTOFF		54
#define R_M32R_GOTPC24		55
#define R_M32R_GOT16_HI_ULO	56
#define R_M32R_GOT16_HI_SLO	57
#define R_M32R_GOT16_LO		58
#define R_M32R_GOTPC_HI_ULO	59
#define R_M32R_GOTPC_HI_SLO	60
#define R_M32R_GOTPC_LO		61
#define R_M32R_GOTOFF_HI_ULO	62
#define R_M32R_GOTOFF_HI_SLO	63
#define R_M32R_GOTOFF_LO	64
#define R_M32R_NUM		256

#define R_TILEPRO_NONE		0
#define R_TILEPRO_32		1
#define R_TILEPRO_16		2
#define R_TILEPRO_8		3
#define R_TILEPRO_32_PCREL	4
#define R_TILEPRO_16_PCREL	5
#define R_TILEPRO_8_PCREL	6
#define R_TILEPRO_LO16		7
#define R_TILEPRO_HI16		8
#define R_TILEPRO_HA16		9
#define R_TILEPRO_COPY		10
#define R_TILEPRO_GLOB_DAT	11
#define R_TILEPRO_JMP_SLOT	12
#define R_TILEPRO_RELATIVE	13
#define R_TILEPRO_BROFF_X1	14
#define R_TILEPRO_JOFFLONG_X1	15
#define R_TILEPRO_JOFFLONG_X1_PLT 16
#define R_TILEPRO_IMM8_X0	17
#define R_TILEPRO_IMM8_Y0	18
#define R_TILEPRO_IMM8_X1	19
#define R_TILEPRO_IMM8_Y1	20
#define R_TILEPRO_MT_IMM15_X1	21
#define R_TILEPRO_MF_IMM15_X1	22
#define R_TILEPRO_IMM16_X0	23
#define R_TILEPRO_IMM16_X1	24
#define R_TILEPRO_IMM16_X0_LO	25
#define R_TILEPRO_IMM16_X1_LO	26
#define R_TILEPRO_IMM16_X0_HI	27
#define R_TILEPRO_IMM16_X1_HI	28
#define R_TILEPRO_IMM16_X0_HA	29
#define R_TILEPRO_IMM16_X1_HA	30
#define R_TILEPRO_IMM16_X0_PCREL 31
#define R_TILEPRO_IMM16_X1_PCREL 32
#define R_TILEPRO_IMM16_X0_LO_PCREL 33
#define R_TILEPRO_IMM16_X1_LO_PCREL 34
#define R_TILEPRO_IMM16_X0_HI_PCREL 35
#define R_TILEPRO_IMM16_X1_HI_PCREL 36
#define R_TILEPRO_IMM16_X0_HA_PCREL 37
#define R_TILEPRO_IMM16_X1_HA_PCREL 38
#define R_TILEPRO_IMM16_X0_GOT	39
#define R_TILEPRO_IMM16_X1_GOT	40
#define R_TILEPRO_IMM16_X0_GOT_LO 41
#define R_TILEPRO_IMM16_X1_GOT_LO 42
#define R_TILEPRO_IMM16_X0_GOT_HI 43
#define R_TILEPRO_IMM16_X1_GOT_HI 44
#define R_TILEPRO_IMM16_X0_GOT_HA 45
#define R_TILEPRO_IMM16_X1_GOT_HA 46
#define R_TILEPRO_MMSTART_X0	47
#define R_TILEPRO_MMEND_X0	48
#define R_TILEPRO_MMSTART_X1	49
#define R_TILEPRO_MMEND_X1	50
#define R_TILEPRO_SHAMT_X0	51
#define R_TILEPRO_SHAMT_X1	52
#define R_TILEPRO_SHAMT_Y0	53
#define R_TILEPRO_SHAMT_Y1	54
#define R_TILEPRO_DEST_IMM8_X1	55

#define R_TILEPRO_TLS_GD_CALL	60
#define R_TILEPRO_IMM8_X0_TLS_GD_ADD 61
#define R_TILEPRO_IMM8_X1_TLS_GD_ADD 62
#define R_TILEPRO_IMM8_Y0_TLS_GD_ADD 63
#define R_TILEPRO_IMM8_Y1_TLS_GD_ADD 64
#define R_TILEPRO_TLS_IE_LOAD	65
#define R_TILEPRO_IMM16_X0_TLS_GD 66
#define R_TILEPRO_IMM16_X1_TLS_GD 67
#define R_TILEPRO_IMM16_X0_TLS_GD_LO 68
#define R_TILEPRO_IMM16_X1_TLS_GD_LO 69
#define R_TILEPRO_IMM16_X0_TLS_GD_HI 70
#define R_TILEPRO_IMM16_X1_TLS_GD_HI 71
#define R_TILEPRO_IMM16_X0_TLS_GD_HA 72
#define R_TILEPRO_IMM16_X1_TLS_GD_HA 73
#define R_TILEPRO_IMM16_X0_TLS_IE 74
#define R_TILEPRO_IMM16_X1_TLS_IE 75
#define R_TILEPRO_IMM16_X0_TLS_IE_LO 76
#define R_TILEPRO_IMM16_X1_TLS_IE_LO 77
#define R_TILEPRO_IMM16_X0_TLS_IE_HI 78
#define R_TILEPRO_IMM16_X1_TLS_IE_HI 79
#define R_TILEPRO_IMM16_X0_TLS_IE_HA 80
#define R_TILEPRO_IMM16_X1_TLS_IE_HA 81
#define R_TILEPRO_TLS_DTPMOD32	82
#define R_TILEPRO_TLS_DTPOFF32	83
#define R_TILEPRO_TLS_TPOFF32	84
#define R_TILEPRO_IMM16_X0_TLS_LE 85
#define R_TILEPRO_IMM16_X1_TLS_LE 86
#define R_TILEPRO_IMM16_X0_TLS_LE_LO 87
#define R_TILEPRO_IMM16_X1_TLS_LE_LO 88
#define R_TILEPRO_IMM16_X0_TLS_LE_HI 89
#define R_TILEPRO_IMM16_X1_TLS_LE_HI 90
#define R_TILEPRO_IMM16_X0_TLS_LE_HA 91
#define R_TILEPRO_IMM16_X1_TLS_LE_HA 92

#define R_TILEPRO_GNU_VTINHERIT	128
#define R_TILEPRO_GNU_VTENTRY	129

#define R_TILEPRO_NUM		130

#define R_TILEGX_NONE		0
#define R_TILEGX_64		1
#define R_TILEGX_32		2
#define R_TILEGX_16		3
#define R_TILEGX_8		4
#define R_TILEGX_64_PCREL	5
#define R_TILEGX_32_PCREL	6
#define R_TILEGX_16_PCREL	7
#define R_TILEGX_8_PCREL	8
#define R_TILEGX_HW0		9
#define R_TILEGX_HW1		10
#define R_TILEGX_HW2		11
#define R_TILEGX_HW3		12
#define R_TILEGX_HW0_LAST	13
#define R_TILEGX_HW1_LAST	14
#define R_TILEGX_HW2_LAST	15
#define R_TILEGX_COPY		16
#define R_TILEGX_GLOB_DAT	17
#define R_TILEGX_JMP_SLOT	18
#define R_TILEGX_RELATIVE	19
#define R_TILEGX_BROFF_X1	20
#define R_TILEGX_JUMPOFF_X1	21
#define R_TILEGX_JUMPOFF_X1_PLT	22
#define R_TILEGX_IMM8_X0	23
#define R_TILEGX_IMM8_Y0	24
#define R_TILEGX_IMM8_X1	25
#define R_TILEGX_IMM8_Y1	26
#define R_TILEGX_DEST_IMM8_X1	27
#define R_TILEGX_MT_IMM14_X1	28
#define R_TILEGX_MF_IMM14_X1	29
#define R_TILEGX_MMSTART_X0	30
#define R_TILEGX_MMEND_X0	31
#define R_TILEGX_SHAMT_X0	32
#define R_TILEGX_SHAMT_X1	33
#define R_TILEGX_SHAMT_Y0	34
#define R_TILEGX_SHAMT_Y1	35
#define R_TILEGX_IMM16_X0_HW0	36
#define R_TILEGX_IMM16_X1_HW0	37
#define R_TILEGX_IMM16_X0_HW1	38
#define R_TILEGX_IMM16_X1_HW1	39
#define R_TILEGX_IMM16_X0_HW2	40
#define R_TILEGX_IMM16_X1_HW2	41
#define R_TILEGX_IMM16_X0_HW3	42
#define R_TILEGX_IMM16_X1_HW3	43
#define R_TILEGX_IMM16_X0_HW0_LAST 44
#define R_TILEGX_IMM16_X1_HW0_LAST 45
#define R_TILEGX_IMM16_X0_HW1_LAST 46
#define R_TILEGX_IMM16_X1_HW1_LAST 47
#define R_TILEGX_IMM16_X0_HW2_LAST 48
#define R_TILEGX_IMM16_X1_HW2_LAST 49
#define R_TILEGX_IMM16_X0_HW0_PCREL 50
#define R_TILEGX_IMM16_X1_HW0_PCREL 51
#define R_TILEGX_IMM16_X0_HW1_PCREL 52
#define R_TILEGX_IMM16_X1_HW1_PCREL 53
#define R_TILEGX_IMM16_X0_HW2_PCREL 54
#define R_TILEGX_IMM16_X1_HW2_PCREL 55
#define R_TILEGX_IMM16_X0_HW3_PCREL 56
#define R_TILEGX_IMM16_X1_HW3_PCREL 57
#define R_TILEGX_IMM16_X0_HW0_LAST_PCREL 58
#define R_TILEGX_IMM16_X1_HW0_LAST_PCREL 59
#define R_TILEGX_IMM16_X0_HW1_LAST_PCREL 60
#define R_TILEGX_IMM16_X1_HW1_LAST_PCREL 61
#define R_TILEGX_IMM16_X0_HW2_LAST_PCREL 62
#define R_TILEGX_IMM16_X1_HW2_LAST_PCREL 63
#define R_TILEGX_IMM16_X0_HW0_GOT 64
#define R_TILEGX_IMM16_X1_HW0_GOT 65
#define R_TILEGX_IMM16_X0_HW0_PLT_PCREL 66
#define R_TILEGX_IMM16_X1_HW0_PLT_PCREL 67
#define R_TILEGX_IMM16_X0_HW1_PLT_PCREL 68
#define R_TILEGX_IMM16_X1_HW1_PLT_PCREL 69
#define R_TILEGX_IMM16_X0_HW2_PLT_PCREL 70
#define R_TILEGX_IMM16_X1_HW2_PLT_PCREL 71
#define R_TILEGX_IMM16_X0_HW0_LAST_GOT 72
#define R_TILEGX_IMM16_X1_HW0_LAST_GOT 73
#define R_TILEGX_IMM16_X0_HW1_LAST_GOT 74
#define R_TILEGX_IMM16_X1_HW1_LAST_GOT 75
#define R_TILEGX_IMM16_X0_HW3_PLT_PCREL 76
#define R_TILEGX_IMM16_X1_HW3_PLT_PCREL 77
#define R_TILEGX_IMM16_X0_HW0_TLS_GD 78
#define R_TILEGX_IMM16_X1_HW0_TLS_GD 79
#define R_TILEGX_IMM16_X0_HW0_TLS_LE 80
#define R_TILEGX_IMM16_X1_HW0_TLS_LE 81
#define R_TILEGX_IMM16_X0_HW0_LAST_TLS_LE 82
#define R_TILEGX_IMM16_X1_HW0_LAST_TLS_LE 83
#define R_TILEGX_IMM16_X0_HW1_LAST_TLS_LE 84
#define R_TILEGX_IMM16_X1_HW1_LAST_TLS_LE 85
#define R_TILEGX_IMM16_X0_HW0_LAST_TLS_GD 86
#define R_TILEGX_IMM16_X1_HW0_LAST_TLS_GD 87
#define R_TILEGX_IMM16_X0_HW1_LAST_TLS_GD 88
#define R_TILEGX_IMM16_X1_HW1_LAST_TLS_GD 89

#define R_TILEGX_IMM16_X0_HW0_TLS_IE 92
#define R_TILEGX_IMM16_X1_HW0_TLS_IE 93
#define R_TILEGX_IMM16_X0_HW0_LAST_PLT_PCREL 94
#define R_TILEGX_IMM16_X1_HW0_LAST_PLT_PCREL 95
#define R_TILEGX_IMM16_X0_HW1_LAST_PLT_PCREL 96
#define R_TILEGX_IMM16_X1_HW1_LAST_PLT_PCREL 97
#define R_TILEGX_IMM16_X0_HW2_LAST_PLT_PCREL 98
#define R_TILEGX_IMM16_X1_HW2_LAST_PLT_PCREL 99
#define R_TILEGX_IMM16_X0_HW0_LAST_TLS_IE 100
#define R_TILEGX_IMM16_X1_HW0_LAST_TLS_IE 101
#define R_TILEGX_IMM16_X0_HW1_LAST_TLS_IE 102
#define R_TILEGX_IMM16_X1_HW1_LAST_TLS_IE 103

#define R_TILEGX_TLS_DTPMOD64	106
#define R_TILEGX_TLS_DTPOFF64	107
#define R_TILEGX_TLS_TPOFF64	108
#define R_TILEGX_TLS_DTPMOD32	109
#define R_TILEGX_TLS_DTPOFF32	110
#define R_TILEGX_TLS_TPOFF32	111
#define R_TILEGX_TLS_GD_CALL	112
#define R_TILEGX_IMM8_X0_TLS_GD_ADD 113
#define R_TILEGX_IMM8_X1_TLS_GD_ADD 114
#define R_TILEGX_IMM8_Y0_TLS_GD_ADD 115
#define R_TILEGX_IMM8_Y1_TLS_GD_ADD 116
#define R_TILEGX_TLS_IE_LOAD	117
#define R_TILEGX_IMM8_X0_TLS_ADD 118
#define R_TILEGX_IMM8_X1_TLS_ADD 119
#define R_TILEGX_IMM8_Y0_TLS_ADD 120
#define R_TILEGX_IMM8_Y1_TLS_ADD 121

#define R_TILEGX_GNU_VTINHERIT	128
#define R_TILEGX_GNU_VTENTRY	129

#define R_TILEGX_NUM		130

#define EF_RISCV_RVC 			0x0001
#define EF_RISCV_FLOAT_ABI 		0x0006
#define EF_RISCV_FLOAT_ABI_SOFT 	0x0000
#define EF_RISCV_FLOAT_ABI_SINGLE 	0x0002
#define EF_RISCV_FLOAT_ABI_DOUBLE 	0x0004
#define EF_RISCV_FLOAT_ABI_QUAD 	0x0006

#define R_RISCV_NONE		 0
#define R_RISCV_32		 1
#define R_RISCV_64		 2
#define R_RISCV_RELATIVE	 3
#define R_RISCV_COPY		 4
#define R_RISCV_JUMP_SLOT	 5
#define R_RISCV_TLS_DTPMOD32	 6
#define R_RISCV_TLS_DTPMOD64	 7
#define R_RISCV_TLS_DTPREL32	 8
#define R_RISCV_TLS_DTPREL64	 9
#define R_RISCV_TLS_TPREL32	10
#define R_RISCV_TLS_TPREL64	11
#define R_RISCV_BRANCH		16
#define R_RISCV_JAL		17
#define R_RISCV_CALL		18
#define R_RISCV_CALL_PLT	19
#define R_RISCV_GOT_HI20	20
#define R_RISCV_TLS_GOT_HI20	21
#define R_RISCV_TLS_GD_HI20	22
#define R_RISCV_PCREL_HI20	23
#define R_RISCV_PCREL_LO12_I	24
#define R_RISCV_PCREL_LO12_S	25
#define R_RISCV_HI20		26
#define R_RISCV_LO12_I		27
#define R_RISCV_LO12_S		28
#define R_RISCV_TPREL_HI20	29
#define R_RISCV_TPREL_LO12_I	30
#define R_RISCV_TPREL_LO12_S	31
#define R_RISCV_TPREL_ADD	32
#define R_RISCV_ADD8		33
#define R_RISCV_ADD16		34
#define R_RISCV_ADD32		35
#define R_RISCV_ADD64		36
#define R_RISCV_SUB8		37
#define R_RISCV_SUB16		38
#define R_RISCV_SUB32		39
#define R_RISCV_SUB64		40
#define R_RISCV_GNU_VTINHERIT	41
#define R_RISCV_GNU_VTENTRY	42
#define R_RISCV_ALIGN		43
#define R_RISCV_RVC_BRANCH	44
#define R_RISCV_RVC_JUMP	45
#define R_RISCV_RVC_LUI		46
#define R_RISCV_GPREL_I		47
#define R_RISCV_GPREL_S		48
#define R_RISCV_TPREL_I		49
#define R_RISCV_TPREL_S		50
#define R_RISCV_RELAX		51
#define R_RISCV_SUB6		52
#define R_RISCV_SET6		53
#define R_RISCV_SET8		54
#define R_RISCV_SET16		55
#define R_RISCV_SET32		56
#define R_RISCV_32_PCREL	57
#define R_RISCV_IRELATIVE	58
#define R_RISCV_PLT32		59
#define R_RISCV_SET_ULEB128	60
#define R_RISCV_SUB_ULEB128	61

#define R_RISCV_NUM		62

#endif







