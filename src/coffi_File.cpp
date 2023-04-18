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
#include <gel++/Image.h>

//*
#include <iostream>
#include <iomanip>
#include <map>
#include <list>
using COFFI::COFFI_ARCHITECTURE_TI;

#define DUMP_DEC_FORMAT(width) \
    std::setw(width) << std::setfill(' ') << std::dec << std::right
#define DUMP_HEX_FORMAT(width) \
    std::setw(width) << std::setfill('0') << std::hex << std::right
#define DUMP_STR_FORMAT(width) \
    std::setw(width) << std::setfill(' ') << std::hex << std::left
//*/

namespace gel { namespace coffi {

/**
 * @defgroup coffi COFFI support
 * COFF file support using COFFI library: https://github.com/serge1/COFFI/.
 */

///
class Symbol: public gel::Symbol {
public:
	Symbol(File& file, type_t type, bind_t bind, COFFI::symbol& sym)
		// : _file(file), _type(type), _bind(bind), _sym(sym) {}
		: _file(file), _type(type), _bind(bind), _value(sym.get_value()) {
			_name = std::string(sym.get_name().c_str());
		}
	cstring name() override { return _name.c_str(); }
	t::uint64 value() override { return _value; }
	// cstring name() override { return _sym.get_name().c_str(); }
	// t::uint64 value() override { return _sym.get_value(); }
	t::uint64 size() override { return 0; }
	type_t type() override { return _type; }
	bind_t bind() override { return _bind; }
private:
	File& _file;
	type_t _type;
	bind_t _bind;
	// COFFI::symbol& _sym;
	std::string _name; // TODO use elm data structures 
	t::uint32 _value;
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
		{ return (_sect->get_flags() & (IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_CNT_CODE)) != 0; }
	bool isWritable() override
		{ return true; }
	bool hasContent() override
		{ return (_sect->get_flags() & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0; }
	Buffer buffer() override {
		// ASSERT(_sect->get_data_size());
		// ASSERT(_sect->get_data());
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
		{ return (_sect->get_flags() & (IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_CNT_CODE)) != 0; }
	bool isWritable() override
		{ return true; }
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

#define DUMP_HEX_FORMAT(width) \
    std::setw(width) << std::setfill('0') << std::hex << std::right
#define I2X(value, signs) \
    DUMP_HEX_FORMAT(signs) << std::uppercase << static_cast<uint32_t>(value)

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
	//cerr << "DEBUG: base = " << io::hex(_base) << io::endl;

	// for(int i = 0; i < _reader->get_header()->get_sections_count(); i++) {
	for (auto sec : _reader->get_sections()) {
		// COFFI::section* s = _reader->get_sections()[i];
		_sections.add(new Section(*this, sec));
		//cerr << "DEBUG: section " << sec->get_name().c_str() << ": " << io::hex(sec->get_flags()) << ": " << io::hex(sec->get_data_size()) << io::endl;

		// FIXME: is this thing below needed for COFF files?
		//if((sec->get_flags() & IMAGE_SCN_MEM_DISCARDABLE) == 0) {
		if((sec->get_flags() & (IMAGE_SCN_CNT_CODE|IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_CNT_UNINITIALIZED_DATA)) != 0
		&& sec->get_data_size() != 0) {
			_segments.add(new Segment(*this, sec));
			//auto s = _segments.top();
			/*cerr << "DEBUG: segment " << s->name()
				<< " (" << io::hex(sec->get_flags()) << ")"
				 << " " << io::hex(s->baseAddress())
				 << ":" << io::hex(s->size())
				 << io::endl;*/
		}
		
		// std::cout << "  " << I2X(sec->get_index(), 3) << " "
		// 			<< DUMP_STR_FORMAT(9) << sec->get_name() << " \n";
		// std::cout << "    raw data offs:   " << I2X(sec->get_data_offset(), 8)
		// 			<< "  raw data size: " << I2X(sec->get_data_size(), 8) << "\n"
		// 			<< "    relocation offs: " << I2X(sec->get_reloc_offset(), 8)
		// 			<< "  relocations:   " << I2X(sec->get_reloc_count(), 8) << "\n";
	// }
		/*
		uint32_t    virt_size;
		std::string virt_size_label = "VirtSize: ";
		std::string memory_page     = "";
		if (_reader->get_architecture() == COFFI::COFFI_ARCHITECTURE_TI) {
			virt_size       = sec->get_physical_address();
			virt_size_label = "PhysAddr: ";
			memory_page =
				"(page " + std::to_string(sec->get_page_number()) + ")";
		}
		else {
			virt_size = sec->get_virtual_size();
		}
		std::cout << "  " << I2X(sec->get_index(), 3) << " "
					<< DUMP_STR_FORMAT(9) << sec->get_name() << " "
					<< virt_size_label << I2X(virt_size, 8) << "  "
					<< "VirtAddr:  " << I2X(sec->get_virtual_address(), 8)
					<< memory_page << "\n";
		std::cout << "    raw data offs:   " << I2X(sec->get_data_offset(), 8)
					<< "  raw data size: " << I2X(sec->get_data_size(), 8) << "\n"
					<< "    relocation offs: " << I2X(sec->get_reloc_offset(), 8)
					<< "  relocations:   " << I2X(sec->get_reloc_count(), 8) << "\n";
		if (_reader->get_architecture() != COFFI_ARCHITECTURE_TI) {
			std::cout << "    line # offs:     " << I2X(sec->get_line_num_offset(), 8)
						<< "  line #'s:      " << I2X(sec->get_line_num_count(), 8) << "\n";
		}
		std::cout << "    characteristics: " << I2X(sec->get_flags(), 8)
					<< "\n";
		std::cout << "     ";
		//*/

		// auto section_flags_list =
		// 	section_flags_per_architecture.at(_reader->get_architecture());
		// uint32_t alignment_mask =
		// 	alignment_mask_per_architecture.at(_reader->get_architecture());
		// uint32_t flags = sec->get_flags();
		// for (auto section_flags : *section_flags_list) {
		// 	if ((section_flags.flag & alignment_mask)) {
		// 		if ((flags & alignment_mask) == section_flags.flag) {
		// 			std::cout << " " << section_flags.descr;
		// 		}
		// 	}
		// 	else {
		// 		if ((flags & section_flags.flag) == section_flags.flag) {
		// 			std::cout << " " << section_flags.descr;
		// 		}
		// 	}
		// }
		// std::cout << "\n" << "\n";
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
	// return nullptr;
	SimpleBuilder builder(this, params);
	return builder.build();
}

///
const SymbolTable& File::symbols() {
	if(_symtab == nullptr) {
		_symtab = new SymbolTable;
		COFFI::sections& file_sections = _reader->get_sections();

		// find text and data
		// JORDY note: this doesn't work because there is no .data section. Instead, there are multiple sections tagged as "DATA". Use COFFI library tags instead.
		/* int _text = -1, _data = -1;
		for(int i = 0; i < (int)file_sections.size(); i++) {
			const std::string& name = file_sections[i]->get_name();
			if(name == ".text")
				_text = i;
			else if(name == ".data")
				_data = i;
		} */

		typedef struct section_flags_type
		{
			uint32_t    flag;
			std::string descr;
		} section_flags_type;

		std::list<section_flags_type> section_flags_ti{
			// {STYP_REG, "REG"} This flag has value 0, it is not printed in the output
			{STYP_DSECT, "DSECT"},
			{STYP_NOLOAD, "NOLOAD"},
			{STYP_GROUP, "GROUP"},
			{STYP_PAD, "PAD"},
			{STYP_COPY, "COPY"},
			{STYP_TEXT, "TEXT"},
			{STYP_DATA, "DATA"},
			{STYP_BSS, "BSS"},
			{STYP_BLOCK, "BLOCK"},
			{STYP_PASS, "PASS"},
			{STYP_CLINK, "CLINK"},
			{STYP_VECTOR, "VECTOR"},
			{STYP_PADDED, "PADDED"},
			{STYP_ALIGN_2, "STYP_ALIGN_2"},
			{STYP_ALIGN_4, "STYP_ALIGN_4"},
			{STYP_ALIGN_8, "STYP_ALIGN_8"},
			{STYP_ALIGN_16, "STYP_ALIGN_16"},
			{STYP_ALIGN_32, "STYP_ALIGN_32"},
			{STYP_ALIGN_64, "STYP_ALIGN_64"},
			{STYP_ALIGN_128, "STYP_ALIGN_128"},
			{STYP_ALIGN_256, "STYP_ALIGN_256"},
			{STYP_ALIGN_512, "STYP_ALIGN_512"},
			{STYP_ALIGN_1024, "STYP_ALIGN_1024"},
			{STYP_ALIGN_2048, "STYP_ALIGN_2048"},
			{STYP_ALIGN_4096, "STYP_ALIGN_4096"},
			{STYP_ALIGN_8192, "STYP_ALIGN_8192"},
			{STYP_ALIGN_16384, "STYP_ALIGN_16384"},
			{STYP_ALIGN_32768, "STYP_ALIGN_32768"},
		};
		t::uint32 alignment_mask_ti = 0x00000F00;
		ASSERT(_reader->get_architecture() == COFFI::coffi_architecture_t::COFFI_ARCHITECTURE_TI && "non-TI architecture not yet supported");

		// build symbols
		// Note: should we add functions that are in TEXT sections that are not .text? So far we do, let's see if it causes issues
		for (auto sym = _reader->get_symbols()->begin(); sym != _reader->get_symbols()->end(); sym++) {

			// std::cout << "\n[COFFDUMP]  " << I2X(sym->get_index(), 4) << " " << I2X(sym->get_value(), 8) << " " << I2X(sym->get_type(), 4) << " " << I2X(sym->get_storage_class(), 2) << "    " << sym->get_name() << "\n";
			//cerr << "DEBUG: symbol " << sym->get_name().c_str()
			//	 << ", type = " << io::hex(sym->get_type())
			//	 << ", storage = " << io::hex(sym->get_storage_class())
			//	 << io::endl;

			// get container seciton
			// cerr << "[GELPP/coffi] DEBUG: symbol " << sym->get_name().c_str() << " = " << io::hex(sym->get_value()) << io::endl;
			t::uint16 containing_section_index = sym->get_section_number(); // may be -1
			COFFI::section* containing_section = containing_section_index < file_sections.size() ? file_sections[sym->get_section_number() - 1] : nullptr;

			Symbol::type_t sym_type = Symbol::NO_TYPE; // should we use NO_TYPE or OTHER_TYPE? Looks like OTHER_TYPE is creating labels...
			if(containing_section != nullptr) {
				auto flags = containing_section->get_flags();
				if((flags & (STYP_DATA | STYP_BSS | STYP_COPY)) != 0) {
					sym_type = Symbol::DATA;
				}
				else if((flags & STYP_TEXT) != 0) {
					if(sym->get_type() == 0x0000) // label instead of function I think
						sym_type = Symbol::OTHER_TYPE; // I think OTHER_TYPE means label (in otawa-tms/tms.cpp:285)
					else
						sym_type = Symbol::FUNC; // is it always a function, or possibly a label?
				}
			}
			Symbol::bind_t bind = Symbol::bind_t::GLOBAL; // TODO!

			_symtab->put(sym->get_name().c_str(), new Symbol(*this, sym_type, bind, *sym));

			// do we need auxiliary symbols?
			// for (auto a = sym->get_auxiliary_symbols().begin();
			// 		a != sym->get_auxiliary_symbols().end(); a++) {
			// 	std::cout << "  AUX ";
			// 	for (size_t i = 0; i < sizeof(symbol_record); i++) {
			// 		std::cout << " "
			// 					<< I2X(static_cast<uint8_t>(a->value[i]), 2);
			// 	}
			// 	std::cout << std::endl;
			// }
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
