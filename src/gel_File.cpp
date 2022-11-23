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
 * @class Symbol
 * This class represents a symbol found in an executable file.
 * A symbol is defined by its name, its value and other properties
 * like the size (for a memory object), the type (type of object)
 * and the binding (visibility of the symbol).
 */

/**
 * @enum Symbol::type_t
 * Describes the type of the symbol.
 *
 * @var Symbol::type_t Symbol::NO_TYPE
 * No symbol type.
 *
 * @var Symbol::type_t Symbol::OTHER_TYPE
 * Any unknown symbol type.
 *
 * @var Symbol::type_t Symbol::FUNC
 * Function type.
 *
 * @var Symbol::type_t Symbol::DATA
 * Data type.
 */

/**
 * @enum Symbol::bind_t
 * Describes the binding (visibility) of the symbol.
 *
 * @var Symbol::bind_t Symbol::NO_BIND
 * No binding for this symbol.
 *
 * @var Symbol::bind_t Symbol::OTHER_BIND
 * Other type of binding.
 *
 * @var Symbol::bind_t Symbol::LOCAL
 * Binding local to the unit.
 *
 * @var Symbol::bind_t Symbol::GLOBAL
 * Binding global over the current execution image.
 *
 * @var Symbol::bind_t Symbol::WEAK
 * Like global binding but the symbol is hidden if there is another
 * symbol with same name.
 */

///
Symbol::~Symbol() {
}

/**
 * @fn cstring Symbol::name();
 * @return	Name of the symbol.
 */

/**
 * @fn t::uint64 Symbol::value();
 * @return	Value of the symbol (address for type FUNC and DATA).
 */

/**
 * @fn t::uint64 Symbol::size();
 * @return	Size of the symbol (if any).
 */

/**
 * @fn Symbol::type_t Symbol::type();
 * @return	Type of the symbol.
 */

/**
 * @fn Symbol::bind_t Symbol::bind();
 * @return	Binding of the symbol.
 */


/**
 * @class SymbolTable
 * Represents the table of symbols for an executable file.
 * This class is basically a hash map with the symbol name as keys.
 * All facilities of an ELM class map are provided.
 */

///
SymbolTable::~SymbolTable() {
}


/**
 * @class Segment
 * A segment represents a unit of the execution image.
 */

/**
 */
Segment::~Segment(void) {
}

/**
 * Compute a name for the given segment according to its properties.
 * @param seg	Segment to compute name for.
 * @return		Name fot the segment.
 */
cstring Segment::defaultName(Segment *seg) {
	if(seg->isExecutable())
		return "code";
	else if(seg->isWritable())
		return "data";
	else if(seg->hasContent())
		return "rodata";
	else
		return "unknown";
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
 * @class Section
 * A section is a logical division of a binary file. A section can be used to
 * represent image parts of a binary as well as convenient part as debugging
 * information, symbol table, relocation table, etc.
 * 
 * Often, a segment, that is a part of the image, is made of one or several
 * sections.
 */

///
Section::~Section() { }

/**
 * @fn size_t Section::offset();
 * Get the offset in the file.
 * @return	Offset in the file (in bytes).
 */

/**
 * size_t Section::fileSize();
 * Get the size in the file of the section (may be different from the size
 * in the image).
 * @return	File size in bytes.
 */

/**
 * @fn flags_t Section::flags();
 * Get flags describing the section.
 * @return Section flags (OR'ed combination of @ref flag_values_t).
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
 * const SymbolTable& File::symbols();
 * Get the table of symbols found from the current file. This function has to be implemented
 * by the class actually implemented a File.
 *
 * @return	Symbol table for the file.
 */


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
 * Get the count opf sections.
 * @return	Number of sections. 0 if the section concept is not supported
 * 			by the format.
 */
int File::countSections() {
	return 0;
}


/**
 * Get the section at index i.
 * @param i		Section index (between 0 and File::countSections()).
 * @return		Section at the given index.
 */
Section *File::section(int i) {
	ASSERTP(false, "sections not supported");
	return nullptr;
}


/**
 * If available, returns debugging information about the map
 * between the code and the source lines. Notice that the ownership of then
 * returned object is still the GEL file (it cannot be deleted).
 * @return	Debug source line information if any, nullptr else.
 */
DebugLine *File::debugLines() {
	return nullptr;
}


/**
 * Get the name of the machine this binary is run on.
 * @return	Host machine name.
 */
string File::machine() const {
	return "unknown machine";
}


/**
 * Get the name of the OS this binary is run on.
 * @return	Host OS name.
 */
string File::os() const {
	return "unknown OS";
}


/**
 * Get the machine using ELF code.
 * @return	ELF machine code or -1 if it can be determined.
 */
int File::elfMachine() const {
	return -1;
}

/**
 * Get the OS using ELF code.
 * @return	ELF OS code or -1 if it can be determined.
 */
int File::elfOS() const {
	return -1;
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
