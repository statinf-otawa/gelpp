/*
 * GEL++ File interface
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

#ifndef GELPP_FILE_H_
#define GELPP_FILE_H_

#include <elm/data/Array.h>
#include <elm/data/HashMap.h>
#include <elm/sys/Path.h>
#include <gel++/base.h>

namespace gel {

using namespace elm;
class Image;
class Manager;
class Parameter;
namespace elf {
	class File;
	class File64;
}

class Segment {
public:
	virtual ~Segment(void);
	virtual cstring name(void) = 0;
	virtual address_t baseAddress(void) = 0;
	virtual address_t loadAddress(void) = 0;
	virtual size_t size(void) = 0;
	virtual size_t alignment(void) = 0;
	virtual bool isExecutable(void) = 0;
	virtual bool isWritable(void) = 0;
	virtual bool hasContent(void) = 0;
	virtual Buffer buffer(void) = 0;
};

class Symbol {
public:
	enum type_t {
		NO_TYPE = 0,
		OTHER_TYPE = 1,
		FUNC = 2,
		DATA = 3
	};

	enum bind_t {
		NO_BIND = 0,
		OTHER_BIND = 1,
		LOCAL = 2,
		GLOBAL = 3,
		WEAK = 4
	};

	virtual ~Symbol();
	virtual cstring name() = 0;
	virtual t::uint64 value() = 0;
	virtual t::uint64 size() = 0;
	virtual type_t type() = 0;
	virtual bind_t bind() = 0;
};


class SymbolTable: public HashMap<cstring, Symbol *> {
public:
	virtual ~SymbolTable();
};

class DebugLine;

class File {
public:
	typedef enum {
		no_type,
		program,
		library
	} type_t;

	File(Manager& manager, sys::Path path);
	virtual ~File(void);

	inline sys::Path path(void) const { return _path; }
	inline io::IntFormat format(address_t a) { return gel::format(addressType(), a); }
	inline Manager& manager(void) const { return man; }

	virtual elf::File *toELF();
	virtual elf::File64 *toELF64();

	virtual type_t type(void) = 0;
	virtual bool isBigEndian(void) = 0;
	virtual address_type_t addressType(void) = 0;
	virtual address_t entry(void) = 0;
	virtual int count() = 0;
	virtual Segment *segment(int i) = 0;
	virtual Image *make();
	virtual Image *make(const Parameter& params) = 0;

	virtual const SymbolTable& symbols() = 0;
	virtual DebugLine *debugLines();
	virtual cstring machine();
	virtual cstring os();

protected:
	Manager& man;
private:
	sys::Path _path;
};

io::Output& operator<<(io::Output& out, File::type_t t);

} // gel

#endif /* GELPP_FILE_H_ */
