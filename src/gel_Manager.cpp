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

#include <elm/compare.h>
#include <elm/sys/System.h>
#include <gel++.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File.h>

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
File *Manager::openFile(sys::Path path) {
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
elf::File *Manager::openELFFile(sys::Path path) {
	try {
		io::RandomAccessStream *s = sys::System::openRandomFile(path, sys::System::READ);
		return new elf::File(*this, path, s);
	}
	catch(sys::SystemException& e) {
		throw Exception(e.message());
	}
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


/**
 * @class Decoder
 * Decoders are used to convert data find in executable files,
 * like integers, into data from the native environment.
 * Typically, they support  endianness conversions.
 */

/**
 */
Decoder::~Decoder(void) {
}

/**
 * @fn void Decoder::fix(t::uint16& w);
 * Called to convert w from executable endianness to native endianness.
 */

/**
 * @fn void Decoder::fix(t::int16& w);
 * Called to convert w from executable endianness to native endianness.
 */

/**
 * @fn void Decoder::fix(t::uint32& w);
 * Called to convert w from executable endianness to native endianness.
 */

/**
 * @fn void Decoder::fix(t::int32& w);
 * Called to convert w from executable endianness to native endianness.
 */

/**
 * @fn void Decoder::fix(t::uint64& w);
 * Called to convert w from executable endianness to native endianness.
 */

/**
 * @fn void Decoder::fix(t::int64& w);
 * Called to convert w from executable endianness to native endianness.
 */


/**
 * @class Buffer
 * Class representing a content of an executable file. It provides
 * facilities to access data in the buffer and prevent out-of-buffer
 * accesses.
 */

/**
 * Null buffer.
 */
Buffer Buffer::null;

/**
 */
io::Output& operator<<(io::Output& out, const Buffer& buf) {
	for(size_t i = 0; i < buf.size(); i += 8) {
		for(size_t j = 0; j < 8; j++) {
			if(i + j < buf.size()) {
				t::uint8 b;
				buf.get(i + j, b);
				out << io::fmt(b).hex().width(2).pad('0');
			}
			else
				out << "  ";
		}
		out << ' ';
		for(size_t j = 0; j < 8; j++) {
			if(i + j < buf.size() ) {
				t::uint8 c;
				buf.get(i + j, c);
				if(c >= ' ' && c < 127)
					out << char(c);
				else
					out << '.';
			}
			else
				out << ' ';
		}
		out << io::endl;
	}
	return out;
}


/**
 * A cursor allows reading a buffer like an output stream.
 */

/**
 * Read a C string and return it. Skip characters
 * until the final null character.
 * @param s	C string resulting of the read.
 * @return	True for success (null-ended string), false else.
 */
bool Cursor::read(cstring& s) {
	if(!avail(sizeof(t::uint8)))
		return false;
	buf.get(off, s);
	while(avail(sizeof(t::uint8))) {
		t::uint8 b;
		buf.get(off, b);
		off++;
		if(b == '\0')
			return true;
	}
	return false;
}


/**
 * Read a memory block and skip corresponding bytes.
 * @param size	Block size.
 * @param buf	Pointer of first byte.
 * @return		True for success (enough bytes in the buffer), false else.
 */
bool Cursor::read(size_t size, const t::uint8 *& buf) {
	if(!avail(size))
		return false;
	buf = this->buf.at(off);
	off += size;
	return true;
}

} // gel
