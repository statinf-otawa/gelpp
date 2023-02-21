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
	Segment(address_t base, COFFI::section *sect)
		: _base(base), _sect(sect) {}
	cstring name() override {
		return _sect->get_name().c_str();
	}
	address_t baseAddress() override {
		return _base + _sect->get_virtual_address();
	}
	address_t loadAddress() override {
		return _base + _sect->get_physical_address();
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
	address_t _base;
	COFFI::section *_sect;
};


///
class Section: public gel::Section {
public:
	Section(address_t base, COFFI::section *sect)
		: _base(base), _sect(sect) {}

	cstring name() override
		{ return _sect->get_name().c_str(); }
	address_t baseAddress() override
		{ return _base + _sect->get_virtual_address(); }
	address_t loadAddress() override
		{ return _base + _sect->get_physical_address(); }
	size_t size() override
		{ return _sect->get_virtual_size(); }
	size_t alignment() override
		{ return _sect->get_alignment(); }
	bool isExecutable() override
		{ return (_sect->get_flags() & IMAGE_SCN_MEM_EXECUTE) != 0; }
	bool isWritable() override
		{ return (_sect->get_flags() & IMAGE_SCN_MEM_WRITE) != 0; }
	size_t offset() override
		{ return _sect->get_data_offset(); }
	size_t fileSize() override
		{ return _sect->get_data_size(); }
	flags_t flags() override
		{ return 0; }

	Buffer buffer() override {

	}

	bool hasContent() override {
		auto f = _sect->get_flags();
		return (f & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0;
	}
private:
	address_t _base;
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

	// get the image base 	
	switch (_reader->get_architecture()) {
		case COFFI::COFFI_ARCHITECTURE_PE:
			if(! _reader->get_win_header())
				throw Exception(_ << "No Windows header for " << path);
			_base = _reader->get_win_header()->get_image_base();
			break;
		case COFFI::COFFI_ARCHITECTURE_CEVA:
		case COFFI::COFFI_ARCHITECTURE_TI:
			_base = _reader->get_optional_header()->get_code_base();
			break;
	}

	for(int i = 0; i < _reader->get_header()->get_sections_count(); i++) {
		auto s = _reader->get_sections()[i];
		_sections.add(new Section(_base, s));
		if((s->get_flags() & IMAGE_SCN_MEM_DISCARDABLE) == 0)
			_segments.add(new Segment(_base, s));
	}
	/*cerr << "DEBUG: code "
		 << io::hex(_reader->get_optional_header()->get_code_base())
		 << ":" << io::hex(_reader->get_optional_header()->get_code_size())
		 << io::endl;
	cerr << "DEBUG: data "
		 << io::hex(_reader->get_optional_header()->get_data_base())
		 << ":" << io::hex(_reader->get_optional_header()->get_initialized_data_size())
		 << ":" << io::hex(_reader->get_optional_header()->get_uninitialized_data_size())
		 << io::endl;
	cerr << "DEBUG: image "
		 << io::hex(_reader->get_win_header()->get_image_base())
		 << io::endl;*/

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

///
int File::countSections() {
	return _sections.length();
}

///
gel::Section *File::section(int i) {
	return _sections[i];
}

}}	// gel::coffi
