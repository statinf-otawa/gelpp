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
#include <gel++/LittleDecoder.h>

namespace gel { namespace coffi {

/**
 * @defgroup coffi COFFI support
 * COFF file support using COFFI library: https://github.com/serge1/COFFI/.
 */

///
class Symbol: public gel::Symbol {
public:
	Symbol(File& file, type_t type, bind_t bind, COFFI::symbol& sym)
		: _file(file), _type(type), _bind(bind), _sym(sym) {}
	cstring name() override { return _sym.get_name().c_str(); }
	t::uint64 value() override { return _sym.get_value(); }
	t::uint64 size() override { return 0; }
	type_t type() override { return _type; }
	bind_t bind() override { return _bind; }
private:
	File& _file;
	type_t _type;
	bind_t _bind;
	COFFI::symbol& _sym;
};


///
class Segment: public gel::Segment {
public:
	Segment(File& file, COFFI::section *sect)
		: _file(file), _sect(sect) {}
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
		return _sect->get_data_size();
	}
	size_t alignment() override {
		return _sect->get_alignment();
	}
	bool isExecutable() override
		{ return (_sect->get_flags() & IMAGE_SCN_MEM_EXECUTE) != 0; }
	bool isWritable() override
		{ return (_sect->get_flags() & IMAGE_SCN_MEM_WRITE) != 0; }
	bool hasContent() override
		{ return (_sect->get_flags() & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0; }
	Buffer buffer() override {
		return Buffer(
			&LittleDecoder::single,
			_sect->get_data(),
			_sect->get_data_size());
	}
private:
	File& _file;
	COFFI::section *_sect;
};


///
class Section: public gel::Section {
public:
	Section(File& file, COFFI::section *sect)
		: _file(file), _sect(sect) {}

	cstring name() override
		{ return _sect->get_name().c_str(); }
	address_t baseAddress() override
		{ return _sect->get_virtual_address(); }
	address_t loadAddress() override
		{ return _sect->get_physical_address(); }
	size_t size() override
		{ return _sect->get_data_size(); }
	size_t alignment() override
		{ return _sect->get_alignment(); }
	bool isExecutable() override
		{ return (_sect->get_flags() & IMAGE_SCN_MEM_EXECUTE) != 0; }
	bool isWritable() override
		{ return (_sect->get_flags() & IMAGE_SCN_MEM_WRITE) != 0; }
	bool hasContent() override
		{ return (_sect->get_flags() & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0; }
	size_t offset() override
		{ return _sect->get_data_offset(); }
	size_t fileSize() override
		{ return _sect->get_data_size(); }
	flags_t flags() override
		{ return 0; }

	Buffer buffer() override {
		return Buffer(
			&LittleDecoder::single,
			_sect->get_data(),
			_sect->get_data_size());
	}
private:
	File& _file;
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
): gel::File(manager, path), _reader(new COFFI::coffi), _symtab(nullptr)
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
			if(! _reader->get_optional_header())
				_base = 0;
			else
				_base = _reader->get_optional_header()->get_code_base();
			break;
		case COFFI::COFFI_ARCHITECTURE_NONE:
		default:
			throw Exception(_ << "Unknown architecture");
	}

	for(int i = 0; i < _reader->get_header()->get_sections_count(); i++) {
		auto s = _reader->get_sections()[i];
		_sections.add(new Section(*this, s));
		// FIXME: is this thing below needed for COFF files?
		if((s->get_flags() & IMAGE_SCN_MEM_DISCARDABLE) == 0)
			_segments.add(new Segment(*this, s));
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
	if(_symtab != nullptr) {
		for(auto sym: *_symtab)
			delete sym;
		delete _symtab;
	}
}

/**
 * Check if the magic number corresponds to COFFI support.
 * @param magic		Magic value to test.
 * @return			True if it matches, false else.
 */
bool File::matches(t::uint8 magic[4]) {
	// COFF-TI's magic number is encoded on two bytes;
	return (magic[0] == 0xc2 || magic[0] == 0xc1) && magic[1] == 0x00;
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
	return address_32;
	// TODO do this properly
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
	if(!_reader->get_optional_header())
		throw Exception(_ << "No optional header, unsure how to proceed");
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
}

///
const SymbolTable& File::symbols() {
	if(_symtab == nullptr) {
		_symtab = new SymbolTable;

		// find text and data
		auto _text = -1, _data = -1;
		for(int i = 0; i < _reader->get_sections().size(); i++) {
			auto name = _reader->get_sections()[i]->get_name();
			if(name == ".text")
				_text = i;
			else if(name == ".data")
				_data = i;
		}

		// build symbols
		for(auto s: _reader->get_sections()) {
			cerr << "DEBUG:" << s->get_name().c_str() << io::endl;
			// cerr << "DEBUG:" << s->value << io::endl;
		}
	}
	return *_symtab;
}

///
string File::machine() const {
	return "unknown"; // TODO implement this for COFF-TI support
	if(!_reader->get_header())
		return "unknown";
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
