/*
 * GEL++ Image class implementation
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

#include <gel++/Image.h>

namespace gel {

/**
 * @class ImageSegment
 * This class represents a file segment mapped inside an image.
 * It provides back references to the owner file and segment
 * and the new base address of the segment (after relocation).
 *
 * Some image segment may not match any file segment if they
 * represent memory allocated by the system. For example,
 * the stack memory.
 */

/**
 * Build an image segment from an allocated memory (usually
 * not coming from a file).
 * @param buf	Buffer containing data for the segment.
 * @param addr	Map address of the buffer.
 * @param flags	Flags of the segment (OR combination of @ref WRITABLE, @ref EXECUTABLE and @ref TO_FREE).
 * @param name	Symbolic name of the segment.
 */
ImageSegment::ImageSegment(Buffer buf, address_t addr, flags_t flags, cstring name)
:	_name(name),
	_file(0),
	_seg(0),
	_base(addr),
	_buf(buf),
	_flags(flags)
{
	if(!_name)
		_name = defaultName(flags);
}

/**
 * Build an image segment from a file and a buffer.
 * @param file		File containing the segment.
 * @param buf		Buffer providing content of the segment.
 * @param addr		Address in the image of the segment.
 * @param flags		Flags of the segment.
 * @parma name		Optional symbolic name.
 */
ImageSegment::ImageSegment(File *file, Buffer buf, address_t addr, flags_t flags, cstring name)
	:	_name(name),
		_file(file),
		_seg(0),
		_base(addr),
		_buf(buf),
		_flags(flags)
{
	if(!_name)
		_name = defaultName(flags);
}


/**
 * Build an image segment from a file segment.
 * @param file		Owner file.
 * @param segment	Segment to build image.
 * @param addr		Address to install the segment to.
 * @param name	Symbolic name of the segment.
 */
ImageSegment::ImageSegment(File *file, Segment *segment, address_t addr, cstring name)
:	_name(name),
	_file(file),
	_seg(segment),
	_base(addr),
	_flags(TO_FREE)
{
	if(segment->isWritable())
		_flags |= WRITABLE;
	if(segment->isExecutable())
		_flags |= EXECUTABLE;
	Buffer sbuf = segment->buffer();
	_buf = Buffer(sbuf.decoder(), new t::uint8[segment->size()], segment->size());
	size_t size = 0;
	if(segment->hasContent()) {
		array::copy(_buf.bytes(), sbuf.bytes(), sbuf.size());
		size = sbuf.size();
	}
	if(size < segment->size())
		array::clear(_buf.bytes() + size, segment->size() - size);
	if(!_name)
		name = defaultName(_flags);
}

/**
 */
ImageSegment::~ImageSegment(void) {
	if(_flags & TO_FREE)
		delete [] _buf.bytes();
}

/**
 * Clean up any link with the original file
 * (for memory save).
 */
void ImageSegment::clean(void) {
	_file = 0;
	_seg = 0;
}

/**
 * Compute default segment name from its flags.
 * @param flags	Segment flags.
 * @return		Corresponding name.
 */
cstring ImageSegment::defaultName(flags_t flags) {
	if(flags & EXECUTABLE)
		return "code";
	else if(flags & WRITABLE)
		return "data";
	else
		return "rodata";
}


/**
 * @fn File *ImageSegment::file(void) const;
 * Get the file (if any) containing the segment.
 * @return	Container file or null.
 */

/**
 * @fn Segment *ImageSegment::segment(void) const;
 * Get the corresponding file segment.
 * @return	Corresponding file segment.
 */

/**
 * @fn address_t ImageSegment::base(void) const;
 * Get the base address of the segment in the image.
 * @return	Base address.
 */

/**
 * @fn size_t ImageSegment::size(void) const;
 * Get the size of the segment.
 * @return	Segment size.
 */

/**
 * @fn const Buffer& ImageSegment::buffer(void) const;
 * Get the buffer to access segment data.
 * @return	Segment buffer.
 */

/**
 * @fn Buffer& ImageSegment::buffer(void);
 * Get the buffer to access segment data.
 * @return	Segment buffer.
 */


/**
 * @class Parameter
 * Set of parameters to build a program image.
 * This class may be specialized according to the different
 * linker and the actual type of a parameter can be checked
 * using the @ref abi() function: either the parameters
 * are compatible with the linker and it can be down-casted
 * to its type, or the linker can only use the common part
 * of the parameters.
 */

/*** Default identifier for the common part of the parameters. */
const cstring Parameter::gen_abi = "gen";

/*** Identifier for the unix parameters. */
const cstring Parameter::unix_abi = "unix";

/*** Default null parameters. */
const Parameter Parameter::null;

/**
 */
Parameter::Parameter(void)
:	stack_alloc(true),
	stack_at(false),
	stack_addr(0),
	stack_size(1 << 12),
	sp(0),
	sp_segment(0)
{ }

/**
 */
Parameter::~Parameter(void) {
}

/**
 * Get the actual ABI of the parameters. Each parameter
 * passed to linker must must extend the Parameter class but
 * with a different ABI. If the actual ABI corresponds to
 * a known parameter, the parameters can be down-casted to
 * the actual parameter type.
 * @return	Actual ABI name (for now, one of @ref gen_abi or @ref unix_abi).
 */
cstring Parameter::abi(void) const {
	return gen_abi;
}


/**
 * Get an environment variable value from the current parameters.
 * @param name	Name of the variable.
 */
cstring Parameter::getenv(cstring name) const {
	int l = name.length();
	for(auto e: env)
		if(e.startsWith(name)
		&& e.length() >= l + 1
		&& e[l] == '=')
			return e.substring(l + 1);
	return "";
}


/**
 * @var Array<cstring> Parameter::arg;
 * The list of command line arguments passed to the program,
 * one for each array entry. Default is an empty array.
 */

/**
 * @var Array<cstring> Parameter::env;
 * The list of environment variables passed to the program,
 * from its laucnh environment. Each array entry must of the
 * form VAR`=`VAL where VAR is the variable identifier and
 * VAL is the variable value. As a default, this array is empty.
 */

/**
 * @var bool Parameter::stack_alloc;
 * If true (default), an initial stack is built for the image.
 */

/**
 * @var bool Parameter::stack_at;
 * If true (default false), the stack is not allocated according the ABI stack
 * allocation policy but at the given @ref stack_addr.
 */

/**
 * @var address_t Parameter::stack_addr;
 * Address where to allocate the initial stack. This parameter is only used
 * when @ref stack_at is true.
 */

/**
 * @var t::size Parameter::stack_size;
 * Size of the stack. In the common parameters, the default is 4KB.
 * Only used if @ref stack_alloc is true.
 */

/**
 * @var Array<sys::Path> Parameter::paths;
 * List of directory paths to retrieve the dynamic libraries
 * (default empty).
 */

/**
 * @var address_t *sp;
 * Pointer to an address to get back the initial value of the stack.
 */

/**
 * @var ImageSegment *sp_segment;
 * Pointer to a segment pointer to get back the segment containing the stack.
 */


/**
 * @class Image
 * Image of a running program, that is a collection of code and data
 * segments containing a program and, if required, dynamically linked
 * libraries.
 *
 * An image is obtained using an @ref ImageBuilder.
 */


/**
 * Build an image using the given file as the program.
 * @param program	Program to use (it is to the user to free it).
 */
Image::Image(File *program): _prog(program) {
	add(program);
}

/**
 */
Image::~Image(void) {
	clean();
}

/**
 * @fn File *Image::program(void) const;
 * Get the image program.
 * @return 	Image program.
 */

/**
 * @fn FileIter Image::files(void) const;
 * Get an iterator on the file involved in the image.
 * @return	Iterator on files.
 */

/**
 * @fn SegIter Image::segments(void) const;
 * Get an iterator on the segment representing the image.
 * @return	Iterator on segments.
 */

/**
 * Add a file to the image.
 * @param file	File to add.
 * @param base	Base address of the file.
 */
void Image::add(File *file, address_t base) {
	_links.addLast(link_t(file, base));
}

/**
 * Add a segment to the image.
 * @param segment	Added segment.
 */
void Image::add(ImageSegment *segment) {
	segs.addLast(segment);
}

/**
 * Find the segment at the given address.
 * @param address	Looked address.
 * @return			Found segment or null.
 */
ImageSegment *Image::at(address_t address) {
	for(SegIter s = segments(); s(); s++)
		if(s->range().contains(address))
			return *s;
	return null<ImageSegment>();
}

/**
 * Get rid of the additional files (usually dynamic libraries)
 * to save memory.
 */
void Image::clean(void) {

	// remove files
	for(LinkIter l = files(); l(); l++)
		if((*l).file != _prog)
			delete (*l).file;
	_links.clear();

	// clean segments
	for(SegIter s = segments(); s(); s++)
		if(s->file() != _prog)
			s->clean();
}


/**
 * @class ImageBuilder
 * Interface shared by all image builders. It is used to build an image
 * using a set of parameters. The parameter class can be specialized according
 * to the image builder to use.
 *
 * Known implementation: @ref gel::SimpleLoader, @ref gel::elf::UnixBuilder.
 */

/**
 * Construct the image builder.
 * @param file				File used as the program.
 * @param params			Parameters to use.
 * @throw gel::Exception	In case of error.
 */
ImageBuilder::ImageBuilder(File *file, const Parameter& params)
: _prog(file), _params(params) {

}

/**
 */
ImageBuilder::~ImageBuilder(void) {
}

/**
 * @fn Image *ImageBuilder::build(void) throw(gel::Exception);
 * Build an image for the give file and parameters.
 * @param file				Program to build the image for.
 * @param param				Parameter to use (default to Parameter::null).
 * @return					Built image.
 * @throw gel::Exception	In case of error.
 */

/**
 * @fn File *ImageBuilder::retrieve(sys::Path name) throw(gel::Exception);
 * Retrie a dynamic library by its name.
 * @param name				Name of looked library.
 * @return					Corresponding file.
 * @throw gel::Exception	In case of error (during load or if the library cannot be found).
 */


/**
 * @class SimpleBuilder
 * Very simple image builder that build the memory for the given program but
 * (a) does not perform dynamic linking, (b) does not perform relocation and
 * (c) does not allocate and initialize the stack.
 */

/**
 * Basic constructor.
 * @param file				File used as a program.
 * @param params			Image parameters.
 * @throw gel::Excpetion	In case of error.
 */
SimpleBuilder::SimpleBuilder(File *file, const Parameter& params)
: ImageBuilder(file, params) {
}

/**
 */
Image *SimpleBuilder::build(void) {
	Image *im = new Image(_prog);
	for(int i = 0; i < _prog->count(); i++) {
		Segment *seg = _prog->segment(i);
		im->add(new ImageSegment(_prog, seg, seg->loadAddress()));
	}
	return im;
}

/**
 */
File *SimpleBuilder::retrieve(sys::Path name) {
	throw MessageException(_ << "cannot find " << name);
}

}	// gel
