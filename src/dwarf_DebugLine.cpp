/*
 * GEL++ ELF DebugLine class implementation
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

#include <elm/data/util.h>

#include <gel++/elf/DebugLine.h>

#include <cstdlib>

namespace gel { namespace dwarf {

// #define DO_DEBUG
#define DEBUG_OUT(txt)	cerr << "DEBUG: " << txt << io::endl;
#ifdef DO_DEBUG
#	define DEBUG(txt)	DEBUG_OUT(txt)
#else
#	define DEBUG(txt)
#endif

// Standard opcodes
#define DW_LNS_copy					1
#define DW_LNS_advance_pc			2
#define DW_LNS_advance_line			3
#define DW_LNS_set_file				4
#define DW_LNS_set_column			5
#define DW_LNS_negate_stmt			6
#define DW_LNS_set_basic_block		7
#define DW_LNS_const_add_pc			8
#define DW_LNS_fixed_advance_pc		9
#define DW_LNS_set_prologue_end		10		/* DWARF-3 */
#define DW_LNS_set_epilogue_begin	11		/* DWARF-3 */
#define DW_LNS_set_isa				12		/* DWARF-3 */

// Extended opcodes
#define DW_LNE_end_sequence			1
#define DW_LNE_set_address			2
#define DW_LNE_define_file			3
#define DW_LNE_set_discriminator	4		/* DWARF-4 */

// Standard Content Description (DWARF-5, TODO)
#define DW_LNCT_path				0x1
#define DW_LNCT_directory_index		0x2
#define DW_LNCT_timestamp			0x3
#define DW_LNCT_size				0x4
#define DW_LNCT_MD5					0x5

// Attribute form
#define DW_FORM_addr			0x01
#define DW_FORM_block2			0x03
#define DW_FORM_block4			0x04
#define DW_FORM_data2			0x05
#define DW_FORM_data4			0x06
#define DW_FORM_data8			0x07			
#define DW_FORM_string			0x08
#define DW_FORM_block			0x09
#define DW_FORM_block1			0x0a
#define DW_FORM_data1			0x0b
#define DW_FORM_flag			0x0c
#define DW_FORM_sdata			0x0d
#define DW_FORM_strp			0x0e
#define DW_FORM_udata 0x0f
#define DW_FORM_ref_addr 0x10
#define DW_FORM_ref1 0x11
#define DW_FORM_ref2 0x12
#define DW_FORM_ref4 0x13
#define DW_FORM_ref8 0x14
#define DW_FORM_ref_udata 0x15
#define DW_FORM_indirect 0x16
#define DW_FORM_sec_offset 0x17
#define DW_FORM_exprloc 0x18
#define DW_FORM_flag_present 0x19
#define DW_FORM_strx 0x1a
#define DW_FORM_addrx 0x1b
#define DW_FORM_ref_sup4 0x1c
#define DW_FORM_strp_sup 0x1d
#define DW_FORM_data16 0x1e
#define DW_FORM_line_strp 0x1f
#define DW_FORM_ref_sig8 0x20
#define DW_FORM_implicit_const 0x21
#define DW_FORM_loclistx 0x22
#define DW_FORM_rnglistx 0x23
#define DW_FORM_ref_sup8 0x24
#define DW_FORM_strx1 0x25
#define DW_FORM_strx2 0x26
#define DW_FORM_strx3 0x27
#define DW_FORM_strx4 0x28
#define DW_FORM_addrx1 0x29
#define DW_FORM_addrx2 0x2a
#define DW_FORM_addrx3 0x2b
#define DW_FORM_addrx4 0x2c
	
	
/**
 * @class DebugLine::StateMachine
 * Only for internal use.
 */


/**
 * @class DebugLine
 * Provides access to debug source line information of an ELF file.
 */

/**
 * Build source line debug information for the given ELF file.
 * @param efile		ELF file to get information frome.
 */
DebugLine::DebugLine(elf::File *efile): gel::DebugLine(efile), is_64(false) {

	// get the buffer
	auto sect = efile->findSection(".debug_line");
	if(sect == nullptr)
		return;
	Cursor c(sect->buffer());

	auto sect_str = efile->findSection(".debug_str");
	if(sect_str != nullptr)
		str_sect_cursor = Cursor(sect_str->buffer());

	auto sect_line_str = efile->findSection(".debug_line_str");
	if(sect_line_str != nullptr)
		line_str_sect_cursor = Cursor(sect_line_str->buffer());

	// decode the content
	DEBUG("reading (size =" << c.size() << ")");
	while(!c.ended())
		readCU(c);
}

/**
 * Build source line debug information for the given ELF file.
 * @param file		ELF file to get information frome.
 * @param buf		Buffer to
 */
DebugLine::DebugLine(gel::File *file, Buffer buf): gel::DebugLine(file), is_64(false) {
	Cursor c(buf);
	DEBUG("reading (size =" << c.size() << ")");
	while(!c.ended())
		readCU(c);
}


/**
 * Build source line information from a buffer.
 */
/*DebugLine::DebugLine(Buffer& buf): gel::DebugLine(efile), is_64(false) {

}*/


/**
 * @fn const HashMap<sys::Path, File *>& DebugLine::files() const;
 * Get the list of source files involved in the current ELF file.
 * @return	List of source files.
 */

/**
 * @fn const FragTable<DebugLine::CompilationUnit *>& DebugLine::units() const;
 * Get the list of compilation units in the file.
 * @return	List of compilation units.
 */

void DebugLine::readCU(Cursor& c) {
	StateMachine sm;

	// start the compilation unit
	size_t unit_length = readUnitLength(c);
	size_t end_offset = c.offset() + unit_length;
	DEBUG("===> unit_length = " << unit_length
		 << ", end offset = " << end_offset);
	auto cu = new CompilationUnit();

	// parse the header
	try {
		readHeader(c, sm, cu);
		DEBUG("readHeader: file = " << sm.file);
		if(c.offset() < end_offset)
			runSM(c, sm, cu, end_offset);
	}
	catch(gel::Exception& e) {
		delete cu;
		throw e;
	}

	// finalize
	add(cu);
	c.move(end_offset);
}

void DebugLine::readHeader(Cursor& c, StateMachine& sm, CompilationUnit *cu) {

	// skip version
	error_if(!c.read(sm.version)); 
	DEBUG("version = " << sm.version);
	if(sm.version > 5)
		throw gel::Exception(_ << "DWARF version > 5 (" << sm.version << ")");

	if(sm.version >= 5) {
		c.read(sm.address_size);
		DEBUG("Address size = " << sm.address_size);
		c.read(sm.segment_selector_size);
		DEBUG("Segment selector size = " << sm.segment_selector_size);
	}
	
	// read header length
	size_t header_length = readHeaderLength(c);
	size_t lines = c.offset() + header_length;
	DEBUG("header length = " << header_length);

	// base information
	c.read(sm.minimum_instruction_length);
	DEBUG("Min inst length = " << sm.minimum_instruction_length);
	if(sm.version >= 4)
		c.read(sm.maximum_operations_per_instruction);
	else
		sm.maximum_operations_per_instruction = 1;
	DEBUG("Max op per insts = " << sm.maximum_operations_per_instruction);

	// SM initialization
	t::uint8 default_is_stmt;
	c.read(default_is_stmt);
	if(default_is_stmt)
		sm.set(LineNumber::IS_STMT);
	DEBUG("default_is_stmt = " << sm.bit(LineNumber::IS_STMT));
	c.read(sm.line_base);
	DEBUG("line base = " << sm.line_base);
	c.read(sm.line_range);
	DEBUG("line range = " << sm.line_range);
	c.read(sm.opcode_base);
	DEBUG("opcode base = " << sm.opcode_base);
	error_if(!c.avail(sm.opcode_base - 1));
	sm.standard_opcode_lengths = c.here();
#		ifdef DO_DEBUG
			cerr << "DEBUG: standard opcode lengths: ";
			for(int i = 0; i < sm.opcode_base - 1; i++)
				cerr << " " << sm.standard_opcode_lengths[i];
			cerr << io::endl;
#		endif
	c.skip(sm.opcode_base - 1);

	// include directories
	readDir(c, sm);
	// file names
	readFile(c, sm, cu);

	// ensure at end of header
	DEBUG("after header " << c.offset());
	c.move(lines);
}

void DebugLine::runSM(Cursor& c, StateMachine& sm, CompilationUnit *cu, size_t end) {
	while(!sm.end_sequence) {
		if(c.offset() >= end)
			throw gel::Exception("endless debug line opcode program");
		t::uint8 opcode;
		error_if(!c.read(opcode));
		DEBUG("@0x" << io::hex(c.offset()-1) << " " << opcode);

		// special
		if(opcode >= sm.opcode_base) {
			opcode -= sm.opcode_base;
			DEBUG("opcode=" << opcode << ", line_base=" << sm.line_base << ", line_range=" << sm.line_range);
			advanceLine(sm, sm.line_base + (opcode % sm.line_range));
			advancePC(sm, cu, opcode / sm.line_range);
			recordLine(sm, cu);
		}

		// standard and extended
		else
			switch(opcode) {
			case DW_LNS_copy:
				recordLine(sm, cu);
				DEBUG("Copy")
				break;
			case DW_LNS_advance_pc:
				advancePC(sm, cu, readLEB128U(c));
				break;
			case DW_LNS_advance_line: {
					auto l = readLEB128S(c);
					DEBUG("Advance line by " << l << " to " << (sm.line+l))
					advanceLine(sm, l);
					//DEBUG("advance line " << cu->_files[sm.file - 1]->path() << ":" << l << "@" << io::hex(sm.address) << " -> " << sm.line);
				}
				break;
			case DW_LNS_set_file:
				sm.file = readLEB128U(c);
				break;
			case DW_LNS_set_column:
				sm.column = readLEB128U(c);
				DEBUG("Set column to " << sm.column)
				break;
			case DW_LNS_negate_stmt:
				if(sm.bit(LineNumber::IS_STMT))
					sm.clear(LineNumber::IS_STMT);
				else
					sm.set(LineNumber::IS_STMT);
				break;
			case DW_LNS_set_basic_block:
				sm.set(LineNumber::BASIC_BLOCK);
				break;
			case DW_LNS_const_add_pc:
				advancePC(sm, cu, (255 - sm.opcode_base) / sm.line_range);
				break;
			case DW_LNS_fixed_advance_pc: {
					t::uint16 o;
					error_if(!c.read(o));
					sm.address += o;
					sm.op_index = 0;
				}
				break;
			case DW_LNS_set_prologue_end:
				sm.set(LineNumber::PROLOGUE_END);
				break;
			case DW_LNS_set_epilogue_begin:
				sm.set(LineNumber::EPILOGUE_BEGIN);
				break;
			case DW_LNS_set_isa:
				sm.isa = readLEB128U(c);
				break;

			case 0: {
					int offset = readLEB128U(c);
					offset += c.offset();
					error_if(!c.read(opcode));
					DEBUG("extended " << opcode);
					switch(opcode) {
					case DW_LNE_end_sequence:
						recordLine(sm, cu);
						sm.end_sequence = true;
						break;
					case DW_LNE_set_address:
						sm.address = readAddress(c);
						DEBUG("Set address to 0x" << io::hex(sm.address))
						break;
					case DW_LNE_define_file:
						readFile(c, sm, cu);
						break;
					case DW_LNE_set_discriminator:
						sm.discriminator = readLEB128U(c);
						break;
					default:
						throw gel::Exception("invalid debug line extended opcode");
					}
					c.move(offset);
				}
				break;

			default:
				throw gel::Exception("invalid debug line standard opcode");
			}
	}
}

void DebugLine::advancePC(StateMachine& sm, CompilationUnit *cu, t::uint64 adv) {
	if(sm.maximum_operations_per_instruction == 1)
		sm.address += sm.minimum_instruction_length * adv;
	else {
		sm.address += sm.minimum_instruction_length * ((sm.op_index + adv) / sm.maximum_operations_per_instruction);
		sm.op_index = (sm.op_index + adv) % sm.maximum_operations_per_instruction;
	}
}

void DebugLine::advanceLine(StateMachine& sm, t::int64 adv) {
	sm.line += adv;
}

void DebugLine::recordLine(StateMachine& sm, CompilationUnit *cu) {
	DEBUG("file = " << sm.file << " - addr: " << io::hex(sm.address) << " - line: " << sm.line << ": "<<sm.column);
	File *file;
	if(sm.version > 4)
		file = cu->files()[sm.file];
	else
		file = cu->files()[sm.file - 1];
	DEBUG("line "
		<< io::hex(sm.address) << " "
		<< file->path() << ":"
		<< sm.line << ":" << sm.column);

	// record the line
	cu->add(LineNumber(sm.address, file, sm.line,
			sm.column, sm.flags, sm.isa, sm.discriminator, sm.op_index));

	// update the SM
	sm.set(LineNumber::BASIC_BLOCK | LineNumber::PROLOGUE_END | LineNumber::EPILOGUE_BEGIN);
	sm.discriminator = 0;
}

void DebugLine::readFile(Cursor& c, StateMachine& sm, CompilationUnit *cu) {
	if(sm.version < 5) {
		cstring s;
		error_if(!c.read(s));
		while(s != "") {
			auto dir = readLEB128U(c);
			auto date = readLEB128U(c);
			auto size = readLEB128U(c);
			DEBUG("file = " << s << ", " << dir << ", " << date << ", " << size);
			sys::Path p;
			if(dir == 0) {
				p = sys::Path(".") / sys::Path(s);
				DEBUG("no dire: " << p );
			}
			else {
				DEBUG(dir << ":" << sm.include_directories[dir]);
				p = sys::Path(sm.include_directories[dir]) / s;
				DEBUG("p = " << p);
			}
			DEBUG("p = " << (void *)&p << ":");
			File *f = files().get(p, nullptr);
			if(f == nullptr) {
				f = new File(p, date, size);
				add(f);
			}
			cu->add(f);
			error_if(!c.read(s));
		}
	}
	else { //DWARF-5
		t::uint8 filename_entry_format_count = readLEB128U(c);
		DEBUG("Num file format: " << filename_entry_format_count)
		Vector<t::uint64> content_types;
		Vector<t::uint64> format_codes;
		for(t::uint8 i = 0 ; i < filename_entry_format_count ; ++i) {
			t::uint64 content_type = readLEB128U(c);
			t::uint64 format_code = readLEB128U(c);
			DEBUG("\tFormat: type=0x" << io::hex(content_type) << " - attr code=0x" << io::hex(format_code))
			content_types.add(content_type);
			format_codes.add(format_code);
		}


		t::uint64 file_names_count = readLEB128U(c);
		DEBUG("Num files: " << file_names_count)
		for(t::uint64 i = 0 ; i < file_names_count ; ++i) {
			cstring file_name;
			cstring dir_name = ".";
			t::uint64 date = 0;
			t::uint64 size = 0;
			for(t::uint8 iformat = 0 ; iformat < filename_entry_format_count ; ++iformat) {
				if(content_types[iformat] == DW_LNCT_path) {
					if(format_codes[iformat] == DW_FORM_string) {
						c.read(file_name);
					}
					else if(format_codes[iformat] == DW_FORM_line_strp) {
						size_t offset = readAddress(c);
						line_str_sect_cursor.move(offset);
						line_str_sect_cursor.read(file_name);
					}
					else if(format_codes[iformat] == DW_FORM_strp) {
						size_t offset = readAddress(c);
						str_sect_cursor.move(offset);
						str_sect_cursor.read(file_name);
					}
					else {
						DEBUG("--> Format code: " << format_codes[iformat]);
						throw gel::Exception("Don't know how to get the files.");
					}
				}
				else if(content_types[iformat] == DW_LNCT_directory_index) {
					t::uint64 index = readLEB128U(c);
					if(index < sm.include_directories.count())
						dir_name = sm.include_directories[index];
				}
				else if(content_types[iformat] == DW_LNCT_MD5) {
					//DW_LNCT_MD5 indicates that the value is a 16-byte MD5 digest of the file
					//contents. It is paired with form DW_FORM_data16.
					t::uint16 a;
					c.read(a);
				}
				else {
					DEBUG("--> Content type: " << content_types[iformat]);
					throw gel::Exception("Don't know how to get the files.");
				}
			}
			if(!file_name.isEmpty()) {
				DEBUG("File " << dir_name << "/" << file_name)
				sys::Path p;
				p = sys::Path(dir_name) / sys::Path(file_name);
				File *f = files().get(p, nullptr);
				if(f == nullptr) {
					f = new File(p, date, size);
					add(f);
				}
				cu->add(f);
			}
		}
	}
}

size_t DebugLine::readHeaderLength(Cursor& c) {
	if(!is_64) {
		t::uint32 l;
		c.read(l);
		return l;
	}
	else {
		t::uint64 l;
		c.read(l);
		return l;
	}
}

size_t DebugLine::readUnitLength(Cursor& c) {
	t::uint32 l;
	error_if(!c.read(l));
	if(l < 0xffffff00) {
		is_64 = false;
		return l;
	}
	t::uint64 ll;
	error_if(!c.read(ll));
	is_64 = true;
	return ll;
}

t::int64 DebugLine::readLEB128S(Cursor& c) {
	t::int64 r = 0;
	int p = 0;
	t::uint8 b;
	do {
		error_if(!c.read(b));
		r |= t::uint64(b & 0x7f) << p;
		p += 7;
	} while((b & 0x80) != 0);
	if(p > 0 && p < 64)
		r = (r << (64 - p) >> (64 - p));
	return r;
}

t::uint64 DebugLine::readLEB128U(Cursor& c) {
	t::uint64 r = 0;
	int p = 0;
	t::uint8 b;
	do {
		error_if(!c.read(b));
		r |= t::uint64(b & 0x7f) << p;
		p += 7;
	} while((b & 0x80) != 0);
	return r;
}

address_t DebugLine::readAddress(Cursor& c) {
	if(!is_64) {
		t::uint32 a;
		error_if(!c.read(a));
		return a;
	}
	else {
		t::uint64 a;
		error_if(!c.read(a));
		return a;
	}
}

void DebugLine::readDir(Cursor &c, StateMachine &sm) {
	if(sm.version < 5) {
		// include directories
		cstring s;
		do {
			error_if(!c.read(s));
			if(s) {
				sm.include_directories.add(s);
				DEBUG("include directory = " << s);
			}
		} while(s);
	}
	else { //version starting from DWARF-5
		t::uint8 directory_entry_format_count;
		c.read(directory_entry_format_count);
		if(directory_entry_format_count > 1)
			throw gel::Exception("Don't know how to do if there is more than one format.");
		if(directory_entry_format_count == 0)
			throw gel::Exception("Missing format to decode directories.");

		DEBUG("Num dir format: " << directory_entry_format_count)
		Vector<t::uint64> content_types;
		Vector<t::uint64> format_codes;
		for(t::uint8 i = 0 ; i < directory_entry_format_count ; ++i) {
			t::uint64 content_type = readLEB128U(c);
			t::uint64 format_code = readLEB128U(c);
			DEBUG("\tFormat: type=" << io::hex(content_type) << " - attr code=" << io::hex(format_code))
			content_types.add(content_type);
			format_codes.add(format_code);
		}

		t::uint64 directories_count = readLEB128U(c);
		DEBUG("Num dirs: " << directories_count)

		for(t::uint64 idir = 0 ; idir < directories_count ; ++idir) {
			cstring dir_name = "";
			for(t::uint64 iformat = 0 ; iformat < directory_entry_format_count ; ++iformat) {
				if(content_types[iformat] == DW_LNCT_path) {
					if(format_codes[iformat] == DW_FORM_string) {
						//All null terminated string are there at current c.offset()
						c.read(dir_name);
					}
					else if(format_codes[iformat] == DW_FORM_line_strp) {
						//check in .debug_line_str
						size_t offset = readAddress(c);
						line_str_sect_cursor.move(offset);
						line_str_sect_cursor.read(dir_name);
					}
					else if(format_codes[iformat] == DW_FORM_strp) {
						//check in .debug_str
						size_t offset = readAddress(c);
						str_sect_cursor.move(offset);
						str_sect_cursor.read(dir_name);
					}
					else {
						throw gel::Exception("Don't know how to get the include directory.");
					}
				} 
				else {
					throw gel::Exception("Don't know how to get the include directory.");
				}
			}
			sm.include_directories.add(dir_name);
		}
	}
}

} }	// gel::dwarf


