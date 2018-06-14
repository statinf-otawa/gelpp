/*
 * GEL++ UnixBuilder class interface
 * Copyright (c) 2016, IRIT- University of Toulouse
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
#ifndef GEL___ELF_UNIXBUILDER_H_
#define GEL___ELF_UNIXBUILDER_H_

#include <elm/data/HashMap.h>
#include <elm/data/VectorQueue.h>
#include <gel++/Image.h>

namespace gel { namespace elf {

class Auxiliary {
public:
	inline Auxiliary(t::uint32 t = 0, t::uint32 v = 0): type(t), val(v) { }
	t::uint32 type;
	t::uint32 val;
};

class UnixParameter: public Parameter {
public:
	UnixParameter(void);
	static UnixParameter null;
	size_t page_size;
	Array<Auxiliary> auxv;
};

class UnixBuilder: public ImageBuilder {
public:
	UnixBuilder(File *file, const Parameter& param = UnixParameter::null) throw(gel::Exception);
	virtual Image *build(void) throw(gel::Exception);
	virtual gel::File *retrieve(string name) throw(gel::Exception);
private:
	ImageSegment *buildStack(void) throw(gel::Exception);
	void resolveLibs(File *file) throw(gel::Exception);
	address_t load(File *file, address_t base) throw(gel::Exception);
	File *open(sys::Path path);

	elf::File *_prog;
	const UnixParameter *_uparams;
	HashMap<string, File *> _libs;
	Image *_im;
	VectorQueue<File *> todo;
	Vector<sys::Path> lpaths;
};

} }		// gel::elf

#endif /* GEL___ELF_UNIXBUILDER_H_ */
