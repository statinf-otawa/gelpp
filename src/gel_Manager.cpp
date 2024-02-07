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

#include "config.h"
#include <elm/compare.h>
#include <elm/sys/System.h>
#include <gel++.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File32.h>
#include <gel++/elf/File64.h>
#include <gel++/elf/common.h>
#include <gel++/pecoff/File.h>
#ifdef HAS_COFFI
#	include <gel++/coffi/File.h>
#endif

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

		// read first four bytes
		t::uint8 magic[4];
		t::size size = s->read(magic, sizeof(magic));
		s->resetPos();
		if(size < sizeof(magic))
			throw Exception("does not seem to be a binary!");
		
		// is it ELF?
		if(elf::File::matches(magic))
			return openELFFile(path, s);

		// is it COFF by COFFI?
#		ifdef HAS_COFFI
		else if(coffi::File::matches(magic))
				return new coffi::File(*this, path);
#		endif

		// is it PE-COFF?
		else if(pecoff::File::matches(magic))
			return openPECOFFFile(path, s);
			
		// else I don't know
		throw Exception(_
			<< "unknown executable format with magic: "
			<< io::hex(magic[0])
			<< io::hex(magic[1])
			<< io::hex(magic[2])
			<< io::hex(magic[3]));
	}
	catch(sys::SystemException& e) {
		throw Exception(e.message());
	}
	return nullptr;
}


/**
 * Open an ELF executable file. Caller is in charge of releasing
 * the obtained file.
 * @param path				Path to the file.
 * @return					Open file.
 * @throw gel::Exception	If there is an error.
 */
elf::File *Manager::openELFFile(sys::Path path) {
	io::RandomAccessStream *s = nullptr;
	try {
		s = sys::System::openRandomFile(path, sys::System::READ);
		return openELFFile(path, s);
	}
	catch(sys::SystemException& e) {
		throw Exception(e.message());
	}
}


/**
 * Open an ELF executable file. Caller is in charge of releasing
 * the obtained file.
 * @param path				Path to the file.
 * @param stream			Stream to read from.
 * @return					Open file.
 * @throw gel::Exception	If there is an error.
 */
elf::File *Manager::openELFFile(sys::Path path, io::RandomAccessStream *stream) {
	try {

		// lookup head
		t::uint8 buf[EI_NIDENT];
		int bufs = stream->read(buf, sizeof(buf));
		if(bufs < EI_NIDENT)
			throw Exception("not an ELF file");

		// is it ELF?
		if(!elf::File::matches(buf))
			throw Exception("bad header in ELF");

		// open the right ELF
		switch(buf[EI_CLASS]) {
		case ELFCLASS32:	return new elf::File32(*this, path, stream);
		case ELFCLASS64:	return new elf::File64(*this, path, stream);
		default:			throw Exception(_ << "unknown ELF class: " << io::hex(buf[EI_CLASS]));
		}
	}
	catch(sys::SystemException& e) {
		throw Exception(e.message());
	}
	return nullptr;
}


/**
 * Open a PE-COFF executable file. Caller is in charge of releasing
 * the obtained file.
 * @param path				Path to the file.
 * @param stream			Stream to read from.
 * @return					Open file.
 * @throw gel::Exception	If there is an error.
 */
pecoff::File *Manager::openPECOFFFile(sys::Path path, io::RandomAccessStream *stream) {
	return new pecoff::File(*this, path, stream);
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
