/*
 * GEL++ ELF definitions
 * Copyright (c) 2016, IRIT- université de Toulouse
 *
 * GEL++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GEL++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GEL++; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef GEL_ELF_DEFS_H
#define GEL_ELF_DEFS_H

#include <elm/types.h>
#include "common.h"
#include "../config.h"

namespace gel { namespace elf {

using namespace elm;

typedef t::uint32 Elf32_Addr;
typedef t::uint16 Elf32_Half;
typedef t::uint32 Elf32_Off;
typedef t::int32  Elf32_Sword;
typedef t::uint32 Elf32_Word;

 /* auxiliairy vector types */
 #define AT_NULL			0
 #define AT_IGNORE		1
 #define AT_EXECFD		2
 #define AT_PHDR		3
 #define AT_PHENT		4
 #define AT_PHNUM		5
 #define AT_PAGESZ		6
 #define AT_BASE		7
 #define AT_FLAGS		8
 #define AT_ENTRY		9
 #define AT_DCACHEBSIZE	10
 #define AT_ICACHEBSIZE	11
 #define AT_UCACHEBSIZE	12


typedef struct Elf32_Ehdr {
        unsigned char e_ident[EI_NIDENT];
        Elf32_Half    e_type;
        Elf32_Half    e_machine;
        Elf32_Word    e_version;
        Elf32_Addr    e_entry;
        Elf32_Off     e_phoff;
        Elf32_Off     e_shoff;
        Elf32_Word    e_flags;
        Elf32_Half    e_ehsize;
        Elf32_Half    e_phentsize;
        Elf32_Half    e_phnum;
        Elf32_Half    e_shentsize;
        Elf32_Half    e_shnum;
        Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

/* Section Header */
typedef struct Elf32_Shdr {
        Elf32_Word sh_name;
        Elf32_Word sh_type;
        Elf32_Word sh_flags;
        Elf32_Addr sh_addr;
        Elf32_Off  sh_offset;
        Elf32_Word sh_size;
        Elf32_Word sh_link;
        Elf32_Word sh_info;
        Elf32_Word sh_addralign;
        Elf32_Word sh_entsize;
} Elf32_Shdr;

/**
 * Elf file structure of a program header.
 * @ingroup		phdr
 */
typedef struct Elf32_Phdr {
        Elf32_Word p_type;		/**< Type of the program header. */
        Elf32_Off  p_offset;	/**< Offset in the file. */
        Elf32_Addr p_vaddr;		/**< Virtual address in host memory. */
        Elf32_Addr p_paddr;		/**< Physical address in host memory. */
        Elf32_Word p_filesz;	/**< Size in the file. */
        Elf32_Word p_memsz;		/**< Size in host memory */
        Elf32_Word p_flags;		/**< Flags: PF_X, PF_W and and PF_R. */
        Elf32_Word p_align;		/**< Alignment in power of two (2**n). */
} Elf32_Phdr;

#define DT_NULL		 		0
#define DT_NEEDED 	 		1	/* d_val */
#define DT_PLTRELSZ	 		2	/* d_val */
#define DT_PLTGOT	 		3	/* d_ptr */
#define DT_HASH		 		4	/* d_ptr */
#define DT_STRTAB	 		5	/* d_ptr */
#define DT_SYMTAB	 		6	/* d_ptr */
#define DT_RELA		 		7	/* d_ptr */
#define DT_RELASZ	 		8	/* d_val */
#define DT_RELAENT	 		9	/* d_val */
#define DT_STRSZ			10	/* d_val */
#define DT_SYMENT			11	/* d_val */
#define DT_INIT				12	/* d_ptr */
#define DT_FINI				13	/* d_ptr */
#define DT_SONAME			14	/* d_val */
#define DT_RPATH			15	/* d_val */
#define DT_SYMBOLIC			16
#define DT_REL				17	/* d_ptr */
#define DT_RELSZ			18	/* d_val */
#define DT_RELENT			19	/* d_val */
#define DT_PLTREL			20	/* d_val */
#define DT_DEBUG			21	/* d_ptr */
#define DT_TEXTREL			22
#define DT_JMPREL			23	/* d_ptr */
#define DT_BIND_NOW			24
#define DT_INIT_ARRAY		25	/* d_ptr */
#define DT_FINI_ARRAY		26	/* d_ptr */
#define DT_INIT_ARRAYSZ		27	/* d_val */
#define DT_FINI_ARRAYSZ		28	/* d_val */
#define DT_RUNPATH			29	/* d_val */
#define DT_FLAGS			30	/* d_val */

#define DT_ENCODING			32

#define DT_PREINIT_ARRAY	32	/* d_ptr */
#define DT_PREINIT_ARRAYSZ	33	/* d_val */
#define DT_SYMTAB_SHNDX		34	/* d_ptr */
#define DT_COUNT			35

#define DT_LOOS		0x6000000d
#define DT_HIOS		0x6ffff000
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff

#define DF_ORIGIN		0x00000001
#define DF_SYMBOLIC		0x00000002
#define DF_TEXTREL		0x00000004
#define DF_BIND_NOW		0x00000008
#define DF_STATIC_TLS	0x00000010


/* Legal values for note segment descriptor types for core files. */
typedef struct Elf32_Dyn {
  Elf32_Sword d_tag;
  union {
    Elf32_Word d_val;
    Elf32_Addr d_ptr;
  } d_un;
} Elf32_Dyn;


/* ELF32_Sym */
typedef struct Elf32_Sym {
  Elf32_Word	st_name;
  Elf32_Addr	st_value;
  Elf32_Word	st_size;
  unsigned char	st_info;
  unsigned char st_other;
  Elf32_Half	st_shndx;
} Elf32_Sym;

#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

typedef struct Elf32_Rel {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
} Elf32_Rel;

typedef struct Elf32_Rela {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
	Elf32_Sword r_addend;
} Elf32_Rela;

#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

} }		// gel::elf

#endif	/* GEL_ELF_DEFS_H */
