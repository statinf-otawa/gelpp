/*
 * GEL++ PE-COFF File implementation
 * Copyright (c) 2020, IRIT- université de Toulouse
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

#include <gel++/pecoff/File.h>

namespace gel { namespace pecoff {

/**
 * @class File
 * Representation of a binary file in PE-COFF format.
 */

///
File::File(Manager& manager, sys::Path path): gel::File(manager, path) {

}

///
File::~File() {

}

///
File::type_t File::type() {

}

///
bool File::isBigEndian() {

}

///
address_type_t File::addressType() {

}

///
address_t File::entry() {

}

///
int File::count() {

}

///
Segment *File::segment(int i) {

}

///
Image *File::make(const Parameter& params) {

}

///
const SymbolTable& File::symbols() {

}

} } // gel::pecoff

