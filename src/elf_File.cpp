/*
 * gel::elf::File class
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
#include <gel++/elf/File.h>
#include <gel++/elf/UnixBuilder.h>
#include <gel++/Image.h>

namespace gel { namespace elf {

/**
 * @defgroup elf ELF Loader
 *
 * This group includes all classes implementing the support for the ELF file format.
 * It is based on the following documents:
 * * Tool Interface Standard (TIS), Executable and Linking Format (ELF) Specification Version 1.2. TIS Committee. May 1995.
 *
 */

class Segment: public gel::Segment {
public:
	Segment(ProgramHeader *h): _head(h) {
		if(h->info().p_flags & PF_X)
			_name = "code";
		else if(h->info().p_flags & PF_W)
			_name = "data";
		else if(h->info().p_flags & PF_R)
			_name = "rodata";
		else
			_name = "unknown";
	}

	virtual cstring name(void) throw(Exception) { return _name; }
	virtual address_t baseAddress(void) { return _head->info().p_vaddr; }
	virtual address_t loadAddress(void) { return _head->info().p_paddr; }
	virtual size_t size(void) { return _head->info().p_memsz; }
	virtual size_t alignment(void) { return _head->info().p_align; }
	virtual bool isExecutable(void) { return _head->info().p_flags & PF_X; }
	virtual bool isWritable(void)  { return _head->info().p_flags & PF_W; }
	virtual bool hasContent(void) { return true; }
	virtual Buffer buffer(void) throw(Exception) { return _head->content(); }

private:
	cstring _name;
	ProgramHeader *_head;
};


/**
 * @class File
 * Class handling executable file in ELF format (32-bits).
 * @ingroup elf
 */


void File::fix(t::uint16& i) 	{ i = ENDIAN2(h->e_ident[EI_DATA], i); }
void File::fix(t::int16& i) 	{ i = ENDIAN2(h->e_ident[EI_DATA], i); }
void File::fix(t::uint32& i)	{ i = ENDIAN4(h->e_ident[EI_DATA], i); }
void File::fix(t::int32& i)		{ i = ENDIAN4(h->e_ident[EI_DATA], i); }
void File::fix(t::uint64& i)	{ i = ENDIAN8(h->e_ident[EI_DATA], i); }
void File::fix(t::int64& i)		{ i = ENDIAN8(h->e_ident[EI_DATA], i); }

void File::unfix(t::uint16& i) 	{ i = UN_ENDIAN2(h->e_ident[EI_DATA], i); }
void File::unfix(t::int16& i) 	{ i = UN_ENDIAN2(h->e_ident[EI_DATA], i); }
void File::unfix(t::uint32& i)	{ i = UN_ENDIAN4(h->e_ident[EI_DATA], i); }
void File::unfix(t::int32& i)	{ i = UN_ENDIAN4(h->e_ident[EI_DATA], i); }
void File::unfix(t::uint64& i)	{ i = UN_ENDIAN8(h->e_ident[EI_DATA], i); }
void File::unfix(t::int64& i)	{ i = UN_ENDIAN8(h->e_ident[EI_DATA], i); }

/**
 * Constructor.
 * @param manager	Parent manager.
 * @param path		File path.
 * @param stream	Stream to read from.
 */
File::File(Manager& manager, sys::Path path, io::RandomAccessStream *stream) throw(Exception)
:	gel::File(manager, path),
	s(stream),
	h(new Elf32_Ehdr),
	sec_buf(0),
	str_tab(0),
	ph_buf(0),
	syms(0)
{
	readAt(0, h, sizeof(Elf32_Ehdr));
	if(h->e_ident[0] != ELFMAG0
	|| h->e_ident[1] != ELFMAG1
	|| h->e_ident[2] != ELFMAG2
	|| h->e_ident[3] != ELFMAG3)
		throw Exception("not an ELF file");
	if(h->e_ident[EI_CLASS] != ELFCLASS32)
		throw Exception("only 32-bits class supported");
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
File::~File(void) {
	delete s;
	delete h;
	if(syms)
		delete syms;
	if(sec_buf)
		delete [] sec_buf;
	if(ph_buf)
		delete [] ph_buf;
}

/**
 * Get the map of symbols of the file.
 * @return	Map of symbols.
 */
const File::SymbolMap& File::symbols(void) throw(Exception) {
	if(!syms) {
		syms = new SymbolMap();
		sections();
		for(int i = 0; i < sects.count(); i++) {
			Section *s = sects[i];
			if(s->info().sh_type == SHT_SYMTAB || s->info().sh_type == SHT_DYNSYM)
				for(SymbolIter sym(*this, *s); sym; sym++)
					syms->put(sym.name(), &*sym);
		}
	}
	return *syms;
}

/**
 * Get the program headers.
 * @return	Program headers.
 * @throw gel::Exception	If there is a file read error.
 */
Vector<ProgramHeader>& File::programHeaders(void) throw(gel::Exception) {
	if(!ph_buf) {

		// load it
		ph_buf = new t::uint8[h->e_phentsize * h->e_phnum];
		readAt(h->e_phoff, ph_buf, h->e_phentsize * h->e_phnum);

		// build them
		phs.setLength(h->e_phnum);
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
			phs[i] = ProgramHeader(this, ph);
		}
	}
	return phs;
}

/**
 * Get a string from the string table.
 * @param offset	Offset of the string.
 * @return			Found string.
 * @throw gel::Exception	If there is a file read error or offset is out of bound.
 */
cstring File::stringAt(t::uint32 offset) throw(gel::Exception) {
	if(!str_tab) {
		if(h->e_shstrndx >= sections().length())
			throw gel::Exception(_ << "strtab index out of bound");
		str_tab = sections()[h->e_shstrndx];
		cerr << "DEBUG: strtab = " << h->e_shstrndx << io::endl;
		cerr << "DEBUG: size = " << str_tab->info().sh_size << io::endl;
	}
	cstring r;
	str_tab->content().get(offset, r);
	return r;
}


/**
 * Read from the file and throw an exception if there is an error.
 * @param buf	Buffer to fill in.
 * @param size	Size of the buffer.
 */
void File::read(void *buf, t::uint32 size) throw(Exception) {
	if(s->read(buf, size) != size)
		throw Exception(_ << "cannot read " << size << " bytes from " << path() << ": " << s->io::InStream::lastErrorMessage());
}


/**
 * Read a lock at a particular position.
 * @param pos	Position in file.
 * @param buf	Buffer to fill in.
 * @param size	Size of the buffer.
 */
void File::readAt(t::uint32 pos, void *buf, t::uint32 size) throw(Exception) {
	if(!s->moveTo(pos))
		throw Exception(_ << "cannot move to position " << pos << " in " << path() << ": " << s->io::InStream::lastErrorMessage());
	read(buf, size);
}



/**
 */
File *File::toELF(void) {
	return this;
}


/**
 */
File::type_t File::type(void) {
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
bool File::isBigEndian(void) {
	return h->e_ident[EI_DATA] == ELFDATA2MSB;
}


/**
 */
address_type_t File::addressType(void) {
	return address_32;
}


/**
 */
address_t File::entry(void) {
	return h->e_entry;
}


/**
 */
Image *File::make(const Parameter& params) throw(Exception) {
	/*SimpleBuilder builder(this, params);
	return builder.build();*/
	UnixBuilder builder(this, params);
	return builder.build();
}


/**
 */
int File::count(void) {
	if(segs.count() == 0) {
		programHeaders();
		for(int i = 0; i < phs.count(); i++)
			if(phs[i].info().p_type == PT_LOAD)
				segs.add(new Segment(&phs[i]));
	}
	return segs.count();
}

/**
 */
gel::Segment *File::segment(int i) {
	return segs[i];
}


/**
 * Initialize the section part.
 */
void File::initSections(void) {

	// allocate memory
	t::uint32 size = h->e_shentsize * h->e_shnum;
	sec_buf = new t::uint8[size];
	array::set<uint8_t>(sec_buf, size, 0);

	// load sections
	readAt(h->e_shoff, sec_buf, size);

	// initialize sections
	sects.setLength(h->e_shnum);
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
		sects[i] = new Section(this, s);
		/*if(s->sh_flags & SHF_ALLOC)
			segs.add(sects[i]);*/
	}

}


/**
 * Get the sections of the file.
 * @return	File sections.
 * @throw gel::Exception 	If there is an error when file is read.
 */
Vector<Section *>& File::sections(void) throw(gel::Exception) {
	if(!sec_buf)
		initSections();
	return sects;
}


/**
 * @class Section
 * A section in an ELF file.
 * @ingroup elf
 */

/**
 * Empty builder.
 */
Section::Section(void): _file(0), _info(0), buf(0) {
}

/**
 * Builder.
 * @param file	Parent file.
 * @param entry	Section entry.
 */
Section::Section(elf::File *file, Elf32_Shdr *entry): _file(file), _info(entry), buf(0) {
}

Section::~Section(void) {
	if(buf)
		delete [] buf;
}

/**
 * Get the content of the section (if any).
 * @return	Section content.
 * @throw gel::Exception	If there is a file read error.
 */
Buffer Section::content(void) throw(gel::Exception) {
	if(!buf) {

		// read the data
		buf = new t::uint8[_info->sh_size];
		_file->readAt(_info->sh_offset, buf, _info->sh_size);

		// fix endianness according to the section type
		if(_info->sh_type == SHT_SYMTAB || _info->sh_type == SHT_DYNSYM) {
			if((_info->sh_size / _info->sh_entsize) * _info->sh_entsize != _info->sh_size)
				throw Exception(_ << "garbage found at end of symbol table " << name());
			Cursor c(Buffer(_file, buf, _info->sh_size));
			while(c.avail(_info->sh_entsize)) {
				Elf32_Sym *s = (Elf32_Sym *)c.here();
				c.decoder()->fix(s->st_name);
				c.decoder()->fix(s->st_value);
				c.decoder()->fix(s->st_size);
				c.decoder()->fix(s->st_shndx);
				c.skip(_info->sh_entsize);
			}
		}
	}
	return Buffer(_file, buf, _info->sh_size);
}

/**
 * Get the section name.
 * @return	Section name.
 * @throw gel::Exception	If there is an error during file read.
 */
cstring Section::name(void) throw(gel::Exception) {
	return _file->stringAt(_info->sh_name);
}

/**
 * @fn const Elf32_Shdr& Section::info(void);
 * Get section information.
 */

/**
 */
address_t Section::baseAddress(void) {
	return _info->sh_addr;
}

/**
 */
address_t Section::loadAddress(void) {
	return _info->sh_addr;
}

/**
 */
size_t Section::size(void) {
	return _info->sh_size;
}

/**
 */
size_t Section::alignment(void) {
	return _info->sh_addralign;
}

/**
 */
bool Section::isExecutable(void) {
	return _info->sh_flags & SHF_EXECINSTR;
}

/**
 */
bool Section::isWritable(void) {
	return _info->sh_flags & SHF_WRITE;
}

/**
 */
bool Section::hasContent(void) {
	return _info->sh_type == SHT_PROGBITS;
}

/**
 */
Buffer Section::buffer(void) throw(Exception)  {
	return content();
}


/**
 * @class ProgramHeader;
 * Represents an ELF program header, area of memory ready to be loaded
 * into the program image.
 * @ingroup elf
 */

/**
 */
ProgramHeader::ProgramHeader(void): _file(0), _info(0), _buf(0) {
}

/**
 */
ProgramHeader::ProgramHeader(File *file, Elf32_Phdr *info): _file(file), _info(info), _buf(0) {
}

/**
 */
ProgramHeader::~ProgramHeader(void) {
	if(_buf)
		delete [] _buf;
}

/**
 * @fn const Elf32_Phdr& ProgramHeaderinfo(void) const;
 * Get information on the program header.
 * @return	Program header information.
 */

/**
 * Get the content of the program header.
 * @return	Program header contant.
 * @throw gel::Exception	If there is an error at file read.
 */
Buffer ProgramHeader::content(void) throw(gel::Exception) {
	if(!_buf) {
		_buf = new t::uint8[_info->p_memsz];
		if(_info->p_filesz)
			_file->readAt(_info->p_offset, _buf, _info->p_filesz);
		if(_info->p_filesz < _info->p_memsz)
			array::set(_buf, _info->p_memsz - _info->p_filesz, t::uint8(0));
	}
	return Buffer(_file, _buf, _info->p_memsz);
}

/**
 * @fn bool ProgramHeader::contains(address_t a) const;
 * Test if the program contains the given address.
 * @param a		Address to test.
 * @return		True if the address is in the program header, false else.
 */


/**
 * @class  NoteIter
 * Iterator on the notes for a PT_NOTE program header.
 * @ingroup elf
 */

/**
 */
NoteIter::NoteIter(ProgramHeader& ph) throw(Exception): c(ph.content()) {
	ASSERT(ph.info().p_type == PT_NOTE);
	next();
}

/**
 * Read the next note.
 */
void NoteIter::next(void) throw(Exception) {

	// end reached
	if(c.ended()) {
		_desc = 0;
		return;
	}

	// read the note header
	if(!c.avail(sizeof(Elf32_Word) * 3))
		throw Exception("malformed note entry");
	Elf32_Word namesz;
	c.read(namesz);
	c.read(_descsz);
	c.read(_type);

	// get name and descriptor
	const t::uint8 *p;
	if(!c.read(namesz, p))
		throw Exception("malformed note entry");
	_name = cstring((const char *)p);
	if(!c.read(size_t(_descsz), p))
		throw Exception("malformed note entry");
	_desc = (Elf32_Word *)p;
}

/**
 * @fn bool NoteIter::ended(void);
 * Test if the note traversal is ended.
 * @return	True if the traversal is ended, false else.
 */

/**
 * @fn cstring NoteIter::name(void) const;
 * Get note name.
 * @return Note name.
 */

/**
 * @fn Elf32_Word NoteIter::descsz(void) const;
 * Get tdescription size.
 * @return	Description size (in bytes).
 */

/**
 * @fn const Elf32_Word *NoteIter::desc(void) const;
 * Get description of the note.
 * @return	Note description.
 */

/**
 * @fn Elf32_Word NoteIter::type(void) const;
 * Get the type of the note.
 * @return	Note type.
 */


/**
 * @class SymbolIter;
 * Iterator on the symbols contained in a section. Given sections must be of type
 * SHT_SYMTAB or SHT_DYNSYM.
 */

/**
 */
SymbolIter::SymbolIter(File& file, Section& section) throw(Exception): f(file), s(section), c(section.content()) {
	ASSERT(section.info().sh_type == SHT_SYMTAB || section.info().sh_type == SHT_DYNSYM);
}

/**
 * @fn bool SymbolIter::ended(void) const;
 * Test if the iterator is ended.
 */

/**
 * @fn const Elf32_Sym& SymbolIter::item(void) const;
 * Get the current symbol.
 */

/**
 * Move to next symbol.
 */
void SymbolIter::next(void) {
	c.skip(sizeof(Elf32_Sym));
}

/**
 * Get name of the symbol.
 */
cstring SymbolIter::name(void) throw(Exception) {
	if(names.isNull()) {

		// find string section number
		int sn;
		if(!s.info().sh_link)
			sn = f.info().e_shstrndx;
		else
			sn = s.info().sh_link;

		// get the section and the buffer
		if(sn == 0 || sn >= f.sections().length())
			throw Exception(_ << "bad number for symbol table string section in " << s.name());
		names = f.sections()[sn]->content();
	}
	cstring r;
	names.get(item().st_name, r);
	return r;
}

} }	// gel::file
