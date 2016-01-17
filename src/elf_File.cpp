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

#include <elm/util/array.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File.h>

namespace gel { namespace elf {

/**
 * @class File
 * Class handling executable file in ELF format (32-bits).
 */


void File::fix(t::uint16& i) 	{ i = ENDIAN2(h->e_ident[EI_DATA], i); }
void File::fix(t::int16& i) 	{ i = ENDIAN2(h->e_ident[EI_DATA], i); }
void File::fix(t::uint32& i)	{ i = ENDIAN4(h->e_ident[EI_DATA], i); }
void File::fix(t::int32& i)		{ i = ENDIAN4(h->e_ident[EI_DATA], i); }


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
	str_tab(0)
{
	read(h, sizeof(Elf32_Ehdr));
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
	if(sec_buf)
		delete [] sec_buf;
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
		str_tab = &sections()[h->e_shstrndx];
	}
	return str_tab->content().string(offset);
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
 * Get the sections of the file.
 * @return	File sections.
 * @throw gel::Exception 	If there is an error when file is read.
 */
genstruct::Vector<Section>& File::sections(void) throw(gel::Exception) {
	if(!sec_buf) {

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
			sects[i] = Section(this, s);
		}

	}
	return sects;
}


/**
 * @class Section
 * A section in an ELF file.
 */

/**
 * Empty builder.
 */
Section::Section(void): _file(0), e(0), buf(0) {
}

/**
 * Builder.
 * @param file	Parent file.
 * @param entry	Section entry.
 */
Section::Section(elf::File *file, Elf32_Shdr *entry): _file(file), e(entry), buf(0) {
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
		buf = new t::uint8[e->sh_size];
		if(!e->sh_offset)
			array::set(buf, e->sh_size, t::uint8(0));
		else
			_file->readAt(e->sh_offset, buf, e->sh_size);
	}
	return Buffer(buf, e->sh_size);
}

/**
 * Get the section name.
 * @return	Section name.
 * @throw gel::Exception	If there is an error during file read.
 */
cstring Section::name(void) throw(gel::Exception) {
	return _file->stringAt(e->sh_name);
}

/**
 * @fn const Elf32_Shdr& Section::info(void);
 * Get section information.
 */


} }	// gel::file
