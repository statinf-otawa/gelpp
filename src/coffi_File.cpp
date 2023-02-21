/*
 * GEL++ COFFI File implementation
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

#include <coffi/coffi.hpp>
#include <coffi/coffi_types.hpp>
#include <gel++/coffi/File.h>

namespace gel { namespace coffi {

/**
 * @defgroup coffi COFFI support
 * COFF file support using COFFI library: https://github.com/serge1/COFFI/.
 */

///
class Segment: public gel::Segment {
public:
	Segment(COFFI::section *sect): _sect(sect) {}
	cstring name() override {
		return _sect->get_name().c_str();
	}
	address_t baseAddress() override {
		return _sect->get_virtual_address();
	}
	address_t loadAddress() override {
		return _sect->get_physical_address();
	}
	size_t size() override {
		return _sect->get_virtual_size();
	}
	size_t alignment() override {
		return _sect->get_alignment();
	}
	bool isExecutable() override {
		return (_sect->get_flags() & IMAGE_SCN_MEM_EXECUTE) != 0;
	}
	bool isWritable() override {
		return (_sect->get_flags() & IMAGE_SCN_MEM_WRITE) != 0;
	}
	bool hasContent() override {
		return (_sect->get_flags() & IMAGE_SCN_MEM_DISCARDABLE) == 0;
	}
	Buffer buffer() override {

	}
private:
	COFFI::section *_sect;
};

/**
 * @class File
 * File for the COFF support.
 * @ingroup coffi
 */

///
File::File(
	Manager& manager,
	sys::Path path
): gel::File(manager, path), _reader(new COFFI::coffi)
{
	if(!_reader->load(path.toString().asSysString()))
		throw Exception(_ << "cannot open " << path);
	for(int i = 0; i < _reader->get_header()->get_sections_count(); i++)
		_segments.add(new Segment(_reader->get_sections()[i]));
}


///
File::~File() {
	delete _reader;
}

/**
 * Check if the magic number corresponds to COFFI support.
 * @param magic		Magic value to test.
 * @return			True if it matches, false else.
 */
bool File::matches(t::uint8 magic[4]) {
	return magic[0] == 0x4D && magic[1] == 0x5A;
}

///
gel::File::type_t File::type() {
	// TODO support more types
	return program;
}

///
bool File::isBigEndian(void) {
	// TODO probably to fix
	return false;
}

///
address_type_t File::addressType(void) {
	auto m = _reader->get_header()->get_machine();
	switch(m) {
	case IMAGE_FILE_MACHINE_AMD64:
	case IMAGE_FILE_MACHINE_ARM64:
		return address_64;
	case IMAGE_FILE_MACHINE_ARM:
	case IMAGE_FILE_MACHINE_ARMNT:
	case IMAGE_FILE_MACHINE_I386:
	default:
		return address_32;
	}
}

///
address_t File::entry(void) {
	// TODO
	return _reader->get_optional_header()->get_entry_point_address();
}

///
int File::count() {
	return _segments.length();
}

///
gel::Segment *File::segment(int i) {
	return _segments[i];
}

///
Image *File::make(const Parameter& params) {
	// TODO
	return nullptr;
}

///
const SymbolTable& File::symbols() {
}

///
string File::machine() const {
	auto m = _reader->get_header()->get_machine();
	switch(m) {
	case IMAGE_FILE_MACHINE_AM33:
		return "am33";
	case IMAGE_FILE_MACHINE_AMD64:
		return "amd64";
	case IMAGE_FILE_MACHINE_ARM:
		return "arm";
	case IMAGE_FILE_MACHINE_ARMNT:
		return "armnt";
	case IMAGE_FILE_MACHINE_ARM64:
		return "arm64";
	case IMAGE_FILE_MACHINE_I386:
		return "i386";

	default:
		return "unknown";
	}
}

///
string File::os() const {
	return "unknown";
}

}}	// gel::coffi
