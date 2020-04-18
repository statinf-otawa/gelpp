/*
 * gel::elf::File32 class
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

#include <elm/array.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File32.h>
#include <gel++/elf/UnixBuilder.h>
#include <gel++/Image.h>

namespace gel { namespace elf {

/**
 * @class File32
 * Class handling executable file in ELF format (32-bits).
 * @ingroup elf
 */


/**
 * Constructor.
 * @param manager	Parent manager.
 * @param path		File path.
 * @param stream	Stream to read from.
 */
File32::File32(Manager& manager, sys::Path path, io::RandomAccessStream *stream)
:	elf::File(manager, path, stream),
	h(new Elf32_Ehdr),
	sec_buf(nullptr),
	ph_buf(nullptr)
{
	setIdent(h->e_ident);
	readAt(0, h, sizeof(Elf32_Ehdr));
	if(h->e_ident[0] != ELFMAG0
	|| h->e_ident[1] != ELFMAG1
	|| h->e_ident[2] != ELFMAG2
	|| h->e_ident[3] != ELFMAG3)
		throw Exception("not an ELF file");
	ASSERT(h->e_ident[EI_CLASS] == ELFCLASS32);
	fix(h->e_type);
	fix(h->e_version);
	fix(h->e_entry);
	fix(h->e_shnum);
	fix(h->e_phnum);
	fix(h->e_shentsize);
	fix(h->e_phentsize);
	fix(h->e_shstrndx);
	if(h->e_shstrndx >= h->e_shnum)
		throw Exception("malformed ELF");
	fix(h->e_shoff);
	fix(h->e_phoff);
}

/**
 */
File32::~File32(void) {
	delete h;
	if(sec_buf)
		delete [] sec_buf;
	if(ph_buf)
		delete [] ph_buf;
}


///
void File32::loadProgramHeaders(Vector<ProgramHeader *>& headers) {
	if(ph_buf == nullptr) {

		// load it
		ph_buf = new t::uint8[h->e_phentsize * h->e_phnum];
		readAt(h->e_phoff, ph_buf, h->e_phentsize * h->e_phnum);

		// build them
		headers.setLength(h->e_phnum);
		for(int i = 0; i < h->e_phnum; i++) {
			Elf32_Phdr *ph = (Elf32_Phdr *)(ph_buf + i * h->e_phentsize);
			fix(ph->p_align);
			fix(ph->p_filesz);
			fix(ph->p_flags);
			fix(ph->p_memsz);
			fix(ph->p_offset);
			fix(ph->p_paddr);
			fix(ph->p_type);
			fix(ph->p_vaddr);
			headers[i] = new ProgramHeader32(this, ph);
		}
	}
}


///
int File32::elfType() {
	return h->e_type;
}


///
t::uint16 File32::machine() {
	return h->e_machine;
}


///
t::uint16 File32::version() {
	return h->e_version;
}


///
const t::uint8 *File32::ident() {
	return h->e_ident;
}


/**
 */
File::type_t File32::type(void) {
	switch(h->e_type) {
	case ET_NONE:
	case ET_REL:	return no_type;
	case ET_EXEC:	return program;
	case ET_DYN:	return library;
	case ET_CORE:	return program;
	default:		return no_type;
	}
}


/**
 */
address_type_t File32::addressType(void) {
	return address_32;
}


/**
 */
address_t File32::entry(void) {
	return h->e_entry;
}


///
void File32::loadSections(Vector<Section *>& sections) {

	// allocate memory
	t::uint32 size = h->e_shentsize * h->e_shnum;
	sec_buf = new t::uint8[size];
	array::set<uint8_t>(sec_buf, size, 0);

	// load sections
	readAt(h->e_shoff, sec_buf, size);

	// initialize sections
	sections.setLength(h->e_shnum);
	for(int i = 0; i < h->e_shnum; i++) {
		Elf32_Shdr *s = (Elf32_Shdr *)(sec_buf + i * h->e_shentsize);
		fix(s->sh_addr);
		fix(s->sh_addralign);
		fix(s->sh_entsize);
		fix(s->sh_flags);
		fix(s->sh_info);
		fix(s->sh_link);
		fix(s->sh_name);
		fix(s->sh_offset);
		fix(s->sh_size);
		fix(s->sh_type);
		sections[i] = new Section32(this, s);
	}
}


///
int File32::getStrTab() {
	return h->e_shstrndx;
}


///
class Symbol32: public Symbol {
public:
	inline Symbol32(cstring name, Elf32_Sym *info): Symbol(name), _info(info) { }
	t::uint64 value()	override { return _info->st_value; }
	t::uint64 size()	override { return _info->st_size; }
	t::uint8 bind()		override { return ELF32_ST_BIND(_info->st_info); }
	t::uint8 type()		override { return ELF32_ST_TYPE(_info->st_info); }
	int shndx()			override { return _info->st_shndx; }
private:
	Elf32_Sym *_info;
};


///
void File32::fillSymbolTable(SymbolTable& symtab, Section *sect) {

	// get the data
	auto size = sect->size();
	t::uint8 *buf = new t::uint8[size];
	sect->read(buf);
	symtab.record(buf);
	int str = sect->link();

	// read the symbols
	auto entsize = sect->entsize();
	if((size / entsize) * entsize != size)
		throw Exception(_ << "garbage found at end of symbol table " << sect->name());
	Cursor c(Buffer(this, buf, size));
	while(c.avail(entsize)) {
		Elf32_Sym *s = (Elf32_Sym *)c.here();
		c.decoder()->fix(s->st_name);
		c.decoder()->fix(s->st_value);
		c.decoder()->fix(s->st_size);
		c.decoder()->fix(s->st_shndx);
		c.skip(entsize);
		auto name = stringAt(s->st_name, str);
		symtab.put(name, new Symbol32(name, s));
	}
}


/**
 * @class Section32
 * A section in an ELF file (32-bit).
 * @ingroup elf
 */

/**
 * Builder.
 * @param file	Parent file.
 * @param entry	Section entry.
 */
Section32::Section32(elf::File32 *file, Elf32_Shdr *entry): elf::Section(file), _info(entry) {
}


///
t::uint8 *Section32::readBuf() {

	// read the data
	t::uint8 *buf = new t::uint8[_info->sh_size];
	readAt(_info->sh_offset, buf, _info->sh_size);

	// fix endianness according to the section type
	if(_info->sh_type == SHT_SYMTAB || _info->sh_type == SHT_DYNSYM) {
		if((_info->sh_size / _info->sh_entsize) * _info->sh_entsize != _info->sh_size)
			throw Exception(_ << "garbage found at end of symbol table " << name());
		Cursor c(Buffer(file(), buf, _info->sh_size));
		while(c.avail(_info->sh_entsize)) {
			Elf32_Sym *s = (Elf32_Sym *)c.here();
			c.decoder()->fix(s->st_name);
			c.decoder()->fix(s->st_value);
			c.decoder()->fix(s->st_size);
			c.decoder()->fix(s->st_shndx);
			c.skip(_info->sh_entsize);
		}
	}

	return buf;
}


///
void Section32::read(t::uint8 *buf) {
	readAt(_info->sh_offset, buf, _info->sh_size);
}


/**
 * @fn const Elf32_Shdr& Section::info(void);
 * Get section information.
 */

///
t::uint32 Section32::flags() const {
	return _info->sh_flags;
}

///
int Section32::type() const {
	return _info->sh_type;
}

///
t::uint32 Section32::link() const {
	return _info->sh_link;
}

///
t::uint64 Section32::offset() const {
	return _info->sh_offset;
}

///
address_t Section32::addr() const {
	return _info->sh_addr;
}

///
size_t Section32::size() const {
	return _info->sh_size;
}

///
size_t Section32::entsize() const {
	return _info->sh_entsize;
}

///
cstring Section32::name() {
	return file()->stringAt(_info->sh_name);
}


/**
 * @class ProgramHeader32;
 * Represents an ELF program header, area of memory ready to be loaded
 * into the program image.
 * @ingroup elf
 */


/**
 */
ProgramHeader32::ProgramHeader32(File *file, Elf32_Phdr *info): ProgramHeader(file), _info(info) {
}


/**
 * @fn const Elf32_Phdr& ProgramHeader32::info(void) const;
 * Get information on the program header.
 * @return	Program header information.
 */

t::uint32	ProgramHeader32::flags()  const	{ return _info->p_flags; }
address_t	ProgramHeader32::vaddr()  const	{ return _info->p_vaddr; }
address_t	ProgramHeader32::paddr()  const	{ return _info->p_paddr; }
size_t		ProgramHeader32::memsz()  const	{ return _info->p_memsz; }
size_t		ProgramHeader32::align()  const	{ return _info->p_align; }
int			ProgramHeader32::type()	  const	{ return _info->p_type; }
size_t 		ProgramHeader32::filesz() const { return _info->p_filesz; }
offset_t 	ProgramHeader32::offset() const { return _info->p_offset; }


///
t::uint8 *ProgramHeader32::readBuf() {
	t::uint8 *_buf = new t::uint8[_info->p_memsz];
	if(_info->p_filesz)
		readAt(_info->p_offset, _buf, _info->p_filesz);
	if(_info->p_filesz < _info->p_memsz)
		array::set(_buf, _info->p_memsz - _info->p_filesz, t::uint8(0));
	return _buf;
}

} }	// gel::file
