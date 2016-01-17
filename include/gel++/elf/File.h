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
#ifndef GELPP_ELF_FILE_H_
#define GELPP_ELF_FILE_H_

#include <elm/io/RandomAccessStream.h>
#include "../Exception.h"
#include "../File.h"

namespace gel { namespace elf {

struct Elf32_Ehdr;

class File: public gel::File {
public:
	File(Manager& manager, sys::Path path, io::RandomAccessStream *stream) throw(Exception);
	virtual ~File(void);

	virtual File *toELF(void);
	virtual type_t type(void);
	virtual bool isBigEndian(void);
	virtual address_type_t addressType(void);
	virtual address_t entry(void);

	t::uint16 elfType(void);
	t::uint8 *ident(void);
	t::uint16 machine(void);
	t::uint32 version(void);
	t::uint32 flags(void);

private:
	void read(void *buf, t::uint32 size) throw(Exception);
	void fix(t::uint16& i);
	void fix(t::int16& i);
	void fix(t::uint32& i);
	void fix(t::int32& i);

	struct elf::Elf32_Ehdr *h;
	io::RandomAccessStream *s;
};

} }	// gel::elf

#endif /* ELF_FILE_H_ */
