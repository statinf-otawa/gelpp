/*
 * gel::elf::File64 class
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
#include <gel++/elf/File64.h>
#include <gel++/elf/UnixBuilder.h>
#include <gel++/Image.h>

namespace gel { namespace elf {

/**
 * @class File64
 * Class handling executable file in ELF format (32-bits).
 * @ingroup elf
 */


/**
 * Constructor.
 * @param manager	Parent manager.
 * @param path		File path.
 * @param stream	Stream to read from.
 */
File64::File64(Manager& manager, sys::Path path, io::RandomAccessStream *stream)
:	elf::File(manager, path, stream),
	h(new Elf64_Ehdr),
	sec_buf(nullptr),
	ph_buf(nullptr)
{
	setIdent(h->e_ident);
	readAt(0, h, sizeof(Elf64_Ehdr));
	if(h->e_ident[0] != ELFMAG0
	|| h->e_ident[1] != ELFMAG1
	|| h->e_ident[2] != ELFMAG2
	|| h->e_ident[3] != ELFMAG3)
		throw Exception("not an ELF file");
	ASSERT(h->e_ident[EI_CLASS] == ELFCLASS64);
	fix(h->e_type);
	fix(h->e_machine);
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
File64::~File64(void) {
	delete h;
	if(sec_buf)
		delete [] sec_buf;
	if(ph_buf)
		delete [] ph_buf;
}


///
void File64::loadProgramHeaders(Vector<ProgramHeader *>& headers) {
	if(ph_buf == nullptr) {

		// load it
		ph_buf = new t::uint8[h->e_phentsize * h->e_phnum];
		readAt(h->e_phoff, ph_buf, h->e_phentsize * h->e_phnum);

		// build them
		headers.setLength(h->e_phnum);
		for(int i = 0; i < h->e_phnum; i++) {
			Elf64_Phdr *ph = (Elf64_Phdr *)(ph_buf + i * h->e_phentsize);
			fix(ph->p_align);
			fix(ph->p_filesz);
			fix(ph->p_flags);
			fix(ph->p_memsz);
			fix(ph->p_offset);
			fix(ph->p_paddr);
			fix(ph->p_type);
			fix(ph->p_vaddr);
			headers[i] = new ProgramHeader64(this, ph);
		}
	}
}


///
int File64::elfType() {
	return h->e_type;
}


///
int File64::elfMachine() const {
	return h->e_machine;
}


///
int File64::elfOS() const {
	return h->e_ident[EI_OSABI];
}


///
t::uint16 File64::version() {
	return h->e_version;
}


///
const t::uint8 *File64::ident() {
	return h->e_ident;
}


/**
 */
File::type_t File64::type(void) {
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
address_type_t File64::addressType(void) {
	return address_64;
}


/**
 */
address_t File64::entry(void) {
	return h->e_entry;
}


///
void File64::loadSections(Vector<Section *>& sections) {

	// allocate memory
	size_t size = h->e_shentsize * h->e_shnum;
	sec_buf = new t::uint8[size];
	array::set<uint8_t>(sec_buf, size, 0);

	// load sections
	readAt(h->e_shoff, sec_buf, size);

	// initialize sections
	sections.setLength(h->e_shnum);
	for(int i = 0; i < h->e_shnum; i++) {
		Elf64_Shdr *s = (Elf64_Shdr *)(sec_buf + i * h->e_shentsize);
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
		sections[i] = new Section64(this, s);
	}
}


///
int File64::getStrTab() {
	return h->e_shstrndx;
}


///
void File64::fetchDyn(const t::uint8 *entry, dyn_t& dyn) {
	auto e = *reinterpret_cast<const Elf64_Dyn *>(entry);
	fix(e.d_tag);
	fix(e.d_un.d_val);
	dyn.tag = e.d_tag;
	dyn.un.val = e.d_un.d_val;
}


///
class Symbol64: public Symbol {
public:
	inline Symbol64(cstring name, Elf64_Sym *info): Symbol(name), _info(info) { }

	t::uint8 elfBind()	override { return ELF64_ST_BIND(_info->st_info); }
	t::uint8 elfType()	override { return ELF64_ST_TYPE(_info->st_info); }
	int shndx()			override { return _info->st_shndx; }

	t::uint64 value()	override { return _info->st_value; }
	t::uint64 size()	override { return _info->st_size; }

	type_t type() override {
		switch(ELF32_ST_TYPE(_info->st_info)) {
		case STT_OBJECT:	return DATA;
		case STT_FUNC:		return FUNC;
		default:			return OTHER_TYPE;
		}
	}

	bind_t bind() override {
		switch(ELF32_ST_BIND(_info->st_info)) {
		case STB_LOCAL:		return LOCAL;
		case STB_GLOBAL:	return GLOBAL;
		case STB_WEAK:		return WEAK;
		default:			return OTHER_BIND;
		}
	}

private:
	Elf64_Sym *_info;
};


///
void File64::fillSymbolTable(SymbolTable& symtab, Section *sect) {

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
		Elf64_Sym *s = (Elf64_Sym *)c.here();
		c.decoder()->fix(s->st_name);
		c.decoder()->fix(s->st_value);
		c.decoder()->fix(s->st_size);
		c.decoder()->fix(s->st_shndx);
		c.skip(entsize);
		auto name = stringAt(s->st_name, str);
		symtab.put(name, new Symbol64(name, s));
	}
}


/**
 * @class Section64
 * A section in an ELF file (32-bit).
 * @ingroup elf
 */

/**
 * Builder.
 * @param file	Parent file.
 * @param entry	Section entry.
 */
Section64::Section64(elf::File64 *file, Elf64_Shdr *entry): elf::Section(file), _info(entry) {
}


///
t::uint8 *Section64::readBuf() {

	// read the data
	t::uint8 *buf = new t::uint8[_info->sh_size];
	readAt(_info->sh_offset, buf, _info->sh_size);

	// fix endianness according to the section type
	if(_info->sh_type == SHT_SYMTAB || _info->sh_type == SHT_DYNSYM) {
		if((_info->sh_size / _info->sh_entsize) * _info->sh_entsize != _info->sh_size)
			throw Exception(_ << "garbage found at end of symbol table " << name());
		Cursor c(Buffer(file(), buf, _info->sh_size));
		while(c.avail(_info->sh_entsize)) {
			Elf64_Sym *s = (Elf64_Sym *)c.here();
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
void Section64::read(t::uint8 *buf) {
	readAt(_info->sh_offset, buf, _info->sh_size);
}

///
address_t Section64::baseAddress() {
	return _info->sh_addr;
}

///
address_t Section64::loadAddress() {
	return 0;
}

///
size_t Section64::alignment() {
	return _info->sh_addralign;
}

///
bool Section64::isExecutable() {
	return (_info->sh_flags & SHF_EXECINSTR) != 0;
}

///
bool Section64::isWritable() {
	return (_info->sh_flags & SHF_WRITE) != 0;
}

///
bool Section64::hasContent() {
	return _info->sh_size != 0;
}

///
size_t Section64::fileSize() {
	return _info->sh_size;
}


/**
 * @fn const Elf32_Shdr& Section::info(void);
 * Get section information.
 */

///
t::uint32 Section64::flags() {
	return _info->sh_flags;
}

///
int Section64::type() const {
	return _info->sh_type;
}

///
t::uint32 Section64::link() const {
	return _info->sh_link;
}

///
t::uint64 Section64::offset() {
	return _info->sh_offset;
}

///
address_t Section64::addr() const {
	return _info->sh_addr;
}

///
size_t Section64::size() {
	return _info->sh_size;
}

///
size_t Section64::entsize() const {
	return _info->sh_entsize;
}

///
cstring Section64::name() {
	return file()->stringAt(_info->sh_name);
}


/**
 * @class ProgramHeader64;
 * Represents an ELF program header, area of memory ready to be loaded
 * into the program image.
 * @ingroup elf
 */


/**
 */
ProgramHeader64::ProgramHeader64(File *file, Elf64_Phdr *info): ProgramHeader(file), _info(info) {
}


/**
 * @fn const Elf64_Phdr& ProgramHeader64::info(void) const;
 * Get information on the program header.
 * @return	Program header information.
 */

t::uint32	ProgramHeader64::flags()  const	{ return _info->p_flags; }
address_t	ProgramHeader64::vaddr()  const	{ return _info->p_vaddr; }
address_t	ProgramHeader64::paddr()  const	{ return _info->p_paddr; }
size_t		ProgramHeader64::memsz()  const	{ return _info->p_memsz; }
size_t		ProgramHeader64::align()  const	{ return _info->p_align; }
int			ProgramHeader64::type()	  const	{ return _info->p_type; }
size_t 		ProgramHeader64::filesz() const { return _info->p_filesz; }
offset_t 	ProgramHeader64::offset() const { return _info->p_offset; }


///
t::uint8 *ProgramHeader64::readBuf() {
	t::uint8 *_buf = new t::uint8[_info->p_memsz];
	if(_info->p_filesz)
		readAt(_info->p_offset, _buf, _info->p_filesz);
	if(_info->p_filesz < _info->p_memsz)
		array::set(_buf, _info->p_memsz - _info->p_filesz, t::uint8(0));
	return _buf;
}

} }	// gel::file
