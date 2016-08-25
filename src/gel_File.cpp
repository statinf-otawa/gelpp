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

namespace gel {

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
 * @fn ImageBuilder *File::builder(void);
 * Return an ImageBuilder object adapted for building the image with
 * the current file as program. The caller is in charge of releasing
 * the returned object.
 * @return	Adapted image builder.
 */


/**
 * If the file is of type ELF, return handler on it.
 * @return	ELF file handler or null.
 */
elf::File *File::toELF(void) {
	return 0;
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
 * @fn Image *File::make(DynamicLinker *linker = 0) throw(Exception);
 * Build an image considering the current file as the main program.
 * @param linker	(Optional) Dynamic linker to use.
 */


/**
 * @fn void File::relocate(Image *image) throw(Exception);
 * This function is called by the dynamic linker to let a file
 * involved in an image building to apply relocation to its own
 * segments (depending closely on the type of the file).
 *
 * When this call is performed, the file is ensured that its
 * segments has been mapped in the image.
 *
 * @param image		Currently built image.
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
