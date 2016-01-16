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
	h(new Elf32_Ehdr)
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
}


/**
 * Read from the file and throw an exception if there is an error.
 */
void File::read(void *buf, t::uint32 size) throw(Exception) {
	if(s->read(buf, size) != size)
		throw Exception(_ << "cannot read " << size << " bytes from " << path() << ": " << s->io::InStream::lastErrorMessage());
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
 * Get identification part of ELF header.
 * @return	Identification part.
 */
t::uint8 *File::ident(void) {
	return h->e_ident;
}


/**
 * Get machine information.
 * @return Machine information.
 */
t::uint16 File::machine(void) {
	return h->e_machine;
}


/**
 * Get version information.
 * @return	Version.
 */
t::uint32 File::version(void) {
	return h->e_version;
}


/**
 * Get flags.
 * @return	Flags.
 */
t::uint32 File::flags(void) {
	return h->e_flags;
}

} }	// gel::file
