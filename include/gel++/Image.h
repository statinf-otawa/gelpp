/*
 * GEL++ Image class interface
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
#ifndef GELPP_IMAGE_H_
#define GELPP_IMAGE_H_

#include <elm/data/BiDiList.h>
#include <gel++/base.h>
#include <gel++/File.h>

namespace gel {

using namespace elm;
class Image;

class ImageSegment {
public:
	ImageSegment(Buffer buf, address_t addr, bool _free = false);
	ImageSegment(File *file, Segment *segment, address_t addr);
	~ImageSegment(void);
	void clean(void);
	inline File *file(void) const { return _file; }
	inline Segment *segment(void) const { return _seg; }
	inline address_t base(void) const { return _base; }
	inline size_t size(void) const { return _buf.size(); }
	inline const Buffer& buffer(void) const { return _buf; }
	inline Buffer& buffer(void) { return _buf; }
	inline range_t range(void) const { return range_t(_base, _buf.size()); }

private:
	File *_file;
	Segment *_seg;
	address_t _base;
	Buffer _buf;
	bool _free;
};

class Image {
public:
	Image(File *program);
	~Image(void);
	inline File *program(void) const { return _prog; }
	void clean(void);

	typedef BiDiList<File *>::Iter FileIter;
	inline FileIter files(void) const { return FileIter(_files); }

	typedef BiDiList<ImageSegment *>::Iter SegIter;
	inline SegIter segments(void) const { return SegIter(segs); }

	void add(File *file);
	void add(ImageSegment *segment);
	ImageSegment *at(address_t address);

private:
	File *_prog;
	BiDiList<File *> _files;
	BiDiList<ImageSegment *> segs;
};

class Parameter {
public:
	static const cstring gen_abi, unix_abi;
	static const Parameter null;

	Parameter(void);
	virtual ~Parameter(void);
	virtual cstring abi(void) const;

	Array<cstring> arg;
	Array<cstring> env;
	bool stack_alloc, stack_at;
	address_t stack_addr;
	t::size stack_size;
	Array<sys::Path> paths;
};

class ImageBuilder {
public:
	virtual ~ImageBuilder(void);
	virtual Image *link(File *file, const Parameter& param = Parameter::null) throw(Exception) = 0;
	virtual File *retrieve(string name) throw(Exception) = 0;
};

class SimpleBuilder: public ImageBuilder {
public:
	virtual Image *link(File *file, const Parameter& param = Parameter::null) throw(Exception);
	virtual File *retrieve(string name) throw(Exception);
};

} // gel

#endif /* GELPP_IMAGE_H_ */
