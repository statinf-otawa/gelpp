/*
 * gel::File class
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

#include <gel++/File.h>
#include <gel++/Image.h>

namespace gel {

/**
 * @class Segment
 * A segment represents a unit of the execution image.
 */

/**
 */
Segment::~Segment(void) {
}

/**
 * @fn cstring Segment::name(void);
 * Get the name of the segment.
 * @return	Segment name.
 */

/**
 * @fn address_t Segment::baseAddress(void);
 * Get the running-time address of the segment, i.e. the address
 * where the segment will mapped as soon as the program is run.
 * @return	Running-time address.
 */

/**
 * @fn address_t Segment::loadAddress(void);
 * Get the address where the segment is loaded before the program
 * is launched. It may be different from baseAddress() if the segment
 * aimed to be load in ROM and then copied in RAM before use.
 * @return	Load-time address.
 */

/**
 * @fn size_t Segment::size(void);
 * Get the size of the segment.
 * @return Segment size (in bytes).
 */

/**
 * @fn size_t Segment::alignment(void);
 * Get the alignment constraint for allocation the segment.
 * The returned value must be a power of 2 and the segment will
 * be installedat address multiple of the alignment.
 * @return	Required alignment.
 */

/**
 * @fn bool Segment::isExecutable(void);
 * Test if the segment contains executable code.
 * @return	True if it contains code, false else.
 */

/**
 * @fn bool Segment::isWritable(void);
 * Test if the segment contains writable data.
 * @return	True if it contains writable data, false else.
 */

/**
 * @fn bool Segment::hasContent(void);
 * Test if the segment corresponds to data in the file.
 * @return	True if the segments has data in the file, false else.
 */

/**
 * @fn const Buffer& Segment::buffer(void);
 * Get a buffer on the content of the segment.
 */


/**
 * @class File
 * Interface of the files opened by GEL++.
 */


/**
 */
File::File(Manager& manager, sys::Path path): man(manager), _path(path) {
}


/**
 */
File::~File(void) {
}


/**
 * If the file is of type ELF 32-bit, return handler on it.
 * @return	ELF file handler or null.
 */
elf::File *File::toELF() {
	return nullptr;
}


/**
 * If the file is of type ELF 64-bit, return handler on it.
 * @return	ELF file handler or null.
 */
elf::File64 *File::toELF64() {
	return nullptr;
}


/**
 * @fn sys::Path File::path(void) const;
 * Get path of the file.
 * @return	File path.
 */


/**
 * @fn io::IntFormat File::format(address_t a);
 * Format the address according to the configuration of the file.
 * @param a		Address to format.
 * @return		Formatted address.
 */


/**
 * @fn type_t File::type(void);
 * Get the type of the file.
 */


/**
 * @fn bool File::isBigEndian(void);
 * Test if the file is encoded in big-endian or in little-endian.
 */


/**
 * @fn address_type_t File::addressType(void);
 * Get the type used to represent addresses.
 * @return	Address type.
 */


/**
 * @fn address_t File::entry(void);
 * Get the address of the entry of the program.
 * @return	Program entry address (only valid for program files).
 */


/**
 * @fn Image *File::make(const Parameter& params) throw(Exception);
 * Build an image considering the current file as the main program and
 * using the default image builder of the file type. For ELF file,
 * the Unix image builder is used.
 * @param params	Parameters to build the image.
 */


/**
 * Build an image considering the current file as the main program and
 * using the default image builder of the file type. For ELF file,
 * the Unix image builder is used.
 */
Image *File::make() {
	return make(Parameter::null);
}


/**
 * @fn Table<Segment *> File::segments(void);
 * Get the list of segments composing the file.
 * @return	List of segments.
 */



/**
 * @fn int File::count(void);
 * Count the number of segments. Notice that the segment 0
 * always corresponds to a null segment (not mapped in memory).
 * @return	Get the count of segments.
 */


/**
 * @fn Segment *File::segment(int i);
 * @param i	Index of the accessed segment.
 * @return	Corresponding segment.
 * @warning An asertion failure is raised if i does not match.
 */


/**
 */
io::Output& operator<<(io::Output& out, File::type_t t) {
	cstring labels[] = {
		"no_type",
		"program",
		"library"
	};
	out << labels[t];
	return out;
}

} // gel
