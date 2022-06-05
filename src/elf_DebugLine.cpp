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

#include "../include/gel++/elf/DebugLine.h"

namespace gel { namespace elf {

#define DO_DEBUG
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
	
#define DW_FORM_line_strp
#define DW_FORM_strp_sup
#define DW_FORM_strx
#define DW_FORM_strx1
#define DW_FORM_strx2
#define DW_FORM_strx3
#define DW_FORM_strx4
	
	
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
 * @param efile
 */
DebugLine::DebugLine(elf::File *efile): gel::DebugLine(efile) {

	// customize according to the type of file
	if(efile->addressType() == address_32) {
		addr_size = 4;
		length_size = 4;
	}
	else if(efile->addressType() == address_64) {
		addr_size = 8;
		length_size = 12;
	}
	else
		throw gel::Exception("unsupported address type");

	// get the buffer
	elf::Section *sect = efile->findSection(".debug_line");
	if(sect == nullptr)
		return;
	Cursor c(sect->content());

	// decode the content
	DEBUG("reading (size =" << c.size() << ")");
	while(!c.ended())
		readCU(c);
}

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
	error_if(!c.avail(8 + length_size));

	// skip version
	t::uint16 version;
	c.read(version);
	static_cast<elf::File&>(prog).fix(version);
	if(version > 4)
		throw gel::Exception(_ << "DWARF version > 4 (" << version << ")");

	// DWARF-5 (TODO)
	// address_size (ubyte)
	// segment_selector_size (ubyte)
	
	// read header length
	size_t header_length = readHeaderLength(c);
	size_t lines = c.offset() + header_length;
	//DEBUG("lines = " << lines);

	// base information
	c.read(sm.minimum_instruction_length);
	if(version >= 4)
		c.read(sm.maximum_operations_per_instruction);
	else
		sm.maximum_operations_per_instruction = 1;

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
			cerr << "standard opcode lengths =";
			for(int i = 0; i < sm.opcode_base - 1; i++)
				cerr << " " << sm.standard_opcode_lengths[i];
			cerr << io::endl;
#		endif
	c.skip(sm.opcode_base - 1);

	// DWARF-5 new organisation (TODO)
	// directory_entry_format_count (ubyte)
	// directory_entry_format (ULEB128 pair list)
	// directories_count (ULEB128)
	// directories (string list)
	// filename_entry_format_count (ubyte)
	// file_name_entry_format (ULEB128 pair list)
	// file_names_count(ULEB128)
	// file_name(string list)
	
	// include directories
	cstring s;
	do {
		error_if(!c.read(s));
		if(s)
			sm.include_directories.add(s);
		DEBUG("include directory = " << s);
	} while(s);

	// file names
	while(readFile(c, sm, cu));

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
		DEBUG("@" << (c.offset() - 1) << " " << opcode);

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
				break;
			case DW_LNS_advance_pc:
				advancePC(sm, cu, readLEB128U(c));
				break;
			case DW_LNS_advance_line: {
					auto l = readLEB128S(c);
					advanceLine(sm, l);
					//DEBUG("advance line " << cu->_files[sm.file - 1]->path() << ":" << l << "@" << io::hex(sm.address) << " -> " << sm.line);
				}
				break;
			case DW_LNS_set_file:
				sm.file = readLEB128U(c);
				break;
			case DW_LNS_set_column:
				sm.column = readLEB128U(c);
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
					static_cast<elf::File&>(prog).fix(o);
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
	File *file = cu->files()[sm.file - 1];
	DEBUG("line " << io::hex(sm.address) << " "
		 << file->path() << ":" << sm.line << ":" << sm.column);

	// record the line
	cu->add(LineNumber(sm.address, file, sm.line,
			sm.column, sm.flags, sm.isa, sm.discriminator, sm.op_index));

	// update the SM
	sm.set(LineNumber::BASIC_BLOCK | LineNumber::PROLOGUE_END | LineNumber::EPILOGUE_BEGIN);
	sm.discriminator = 0;
}

bool DebugLine::readFile(Cursor& c, StateMachine& sm, CompilationUnit *cu) {
	cstring s;
	error_if(!c.read(s));
	if(s == "")
		return false;
	auto dir = readLEB128U(c);
	auto date = readLEB128U(c);
	auto size = readLEB128U(c);
	DEBUG("file = " << s << ", " << dir << ", " << date << ", " << size);
	sys::Path p;
	if(dir == 0) {
		p = sys::Path(".") / sys::Path(s);
		cerr << "DEBUG: no dire: " << p << io::endl;
	}
	else {
		cerr << "DEBUG: " << dir << ":" << sm.include_directories[dir] << io::endl;
		p = sys::Path(sm.include_directories[dir]) / s;
		cerr << "DEBUG: p = " << p << io::endl;
	}
	cerr << "DEBUG: p = " << (void *)&p << ":" << p << io::endl;
	File *f = files().get(p, nullptr);
	if(f == nullptr) {
		f = new File(p, date, size);
		add(f);
	}
	cu->add(f);
	return true;
}

size_t DebugLine::readHeaderLength(Cursor& c) {
	if(addr_size == 4) {
		t::uint32 l;
		c.read(l);
		static_cast<elf::File&>(prog).fix(l);
		return l;
	}
	else {
		t::uint64 l;
		c.read(l);
		static_cast<elf::File&>(prog).fix(l);
		return l;
	}
}

size_t DebugLine::readUnitLength(Cursor& c) {
	if(!c.avail(length_size))
		throw gel::Exception("bad length");
	t::uint32 l;
	error_if(!c.read(l));
	if(addr_size == 4) {
		static_cast<elf::File&>(prog).fix(l);
		return l;
	}
	if(l != 0xffffffffff)
		throw gel::Exception("bad 64-bit length");
	t::uint64 ll;
	error_if(!c.read(ll));
	static_cast<elf::File&>(prog).fix(ll);
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
	if(addr_size == 4) {
		t::uint32 a;
		error_if(!c.read(a));
		static_cast<elf::File&>(prog).fix(a);
		return a;
	}
	else {
		t::uint64 a;
		error_if(!c.read(a));
		static_cast<elf::File&>(prog).fix(a);
		return a;
	}
}

} }	// gel::elf


