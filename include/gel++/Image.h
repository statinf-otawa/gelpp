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

#include <elm/genstruct/DLList.h>
#include <gel++/base.h>
#include <gel++/Exception.h>

namespace gel {

using namespace elm;

class ImageSegment {
public:
	ImageSegment(File *file, address_t addr, Buffer buf);
	ImageSegment(File *file, address_t addr, Segment *segment);
	inline File *file(void) const { return _file; }
	inline Segment *segment(void) const { return _seg; }
	inline address_t base(void) const { return _base; }
	inline size_t size(void) const { return _buf.size(); }
	inline Buffer& buffer(void) { return _buf; }

private:
	File *_file;
	Segment *_seg;
	address_t _base;
	Buffer _buf;
};


class DynamicLinker {
public:
	virtual ~DynamicLinker(void);
	virtual void link(Image *image, File *file) throw(Exception) = 0;
	virtual void link(Image *image, const string& name) throw(Exception);
};

class Image {
public:
	static Image *make(File *program) throw(Exception);

	inline File *program(void) const { return prog; }

	typedef genstruct::DLList<File *>::Iterator file_iter;
	inline file_iter files(void) const { return file_iter(_files); }

	typedef genstruct::DLList<ImageSegment *>::Iterator seg_iter;
	inline seg_iter segments(void) const { return seg_iter(segs); }

	void add(ImageSegment *segment);
	ImageSegment *at(address_t address);

private:
	Image(File *program);
	File *prog;
	genstruct::DLList<File *> _files;
	genstruct::DLList<ImageSegment *> segs;
};

} // gel

#endif /* GELPP_IMAGE_H_ */
