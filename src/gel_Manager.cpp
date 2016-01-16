/*
 * GEL++ Manager class
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

#include <elm/sys/System.h>
#include <gel++.h>
#include <gel++/elf/defs.h>

namespace gel {

/**
 * @class Manager
 * Top-level class of GEL++ library. It provides methods to
 * open an executable file.
 */


/**
 * Default implementation of GEL++ manager.
 * May be used to customize the output of errors.
 */
Manager Manager::DEFAULT;

/**
 * Open an executable file. Caller is in charge of releasing
 * the obtained file.
 * @param path				Path to the file.
 * @return					Open file.
 * @throw gel::Exception	If there is an error.
 */
File *Manager::openFile(sys::Path path) throw(gel::Exception) {
	try {
		io::RandomAccessStream *s = sys::System::openRandomFile(path, sys::System::READ);

		// lookup head
		t::uint8 buf[4];
		if(s->read(buf, sizeof(buf)) != sizeof(buf))
			throw Exception(_ << "can't even read the magic word: " << s->io::InStream::lastErrorMessage());

		// is it ELF?
		if(buf[0] == ELFMAG0 && buf[1] == ELFMAG1 && buf[2] == ELFMAG2 && buf[3] == ELFMAG3) {
			s->moveTo(0);
			return new elf::File(*this, path, s);
		}

		// else I don't know
		throw Exception(_ << "unknown executable format with magic: " << io::hex(buf[0]) << io::hex(buf[1]) << io::hex(buf[2]) << io::hex(buf[3]));
	}
	catch(sys::SystemException& e) {
		throw Exception(e.message());
	}
	return 0;
}


/**
 * Open an ELF executable file. Caller is in charge of releasing
 * the obtained file.
 * @param path				Path to the file.
 * @return					Open file.
 * @throw gel::Exception	If there is an error.
 */
elf::File *Manager::openELFFile(sys::Path path) throw(gel::Exception) {
	return 0;
}


/**
 * Format an address for output.
 * @param t	Type of address.
 * @param a	Address to format.
 * @return	Formatted address.
 */
io::IntFormat format(address_type_t t, address_t a) {
	switch(t) {
	case address_8 :	return io::right(io::hex(io::pad('0', io::width(2,a))));
	case address_16:	return io::right(io::hex(io::pad('0', io::width(4,a))));
	case address_32:	return io::right(io::hex(io::pad('0', io::width(8,a))));
	case address_64:	return io::right(io::hex(io::pad('0', io::width(16,a))));
	default:			ASSERT(false); return a;
	}
}

} // gel
