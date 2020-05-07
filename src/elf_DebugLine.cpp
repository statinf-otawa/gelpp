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

namespace gel { namespace elf {

//#define DO_DEBUG
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

/**
 * @class DebugLine::File
 * This class represents a source file in the debugging source line/address map
 * provided by DebugLine.
 */

/**
 * @fn const sys::Path& DebugLine::File::path() const;
 * Get the path of the source file (relative to the compilation location).
 * @return	Source file path.
 */

/**
 * @fn t::uint64 DebugLine::File::date() const;
 * Get the last modification date of the file (for verification).
 * @return	Modification date.
 */

/**
 * @fn size_t DebugLine::File::size() const;
 * Get the size of the source file (for verification).
 * @return	Source file size.
 */

/**
 * @fn const List<DebugLine::CompilationUnit *>& DebugLine::File::units() const;
 * Get the compilation units using code from this source file.
 * @return	Compilation units using the source file.
 */

/**
 * Find the address ranges corresponding to the given line number in the current file.
 * @param line	Looked line in the current source file.
 * @param addrs	Used to return the code ranges corresponding to the line.
 */
void DebugLine::File::find(int line, Vector<Pair<address_t, address_t> >& addrs) const {
	for(auto cu: _units) {
		const auto& lines = cu->lines();
		for(int i = 0; i < lines.count() - 1; i++)
			if(lines[i].file() == this && lines[i].line() == line)
				addrs.add(pair(lines[i].addr(), lines[i+1].addr()));
	}
}


/**
 * @class DebugLine::Line
 * Represents a line of code information in the source line table.
 */
DebugLine::Line::Line(address_t addr, File *file, int line, int col,
t::uint32 flags, t::uint8 isa, t::uint8 desc, t::uint8 opi)
	: _file(file), _line(line), _col(col), _flags(flags),
	  _addr(addr), _isa(isa), _disc(desc), _opi(opi) { }

/**
 * @fn DebugLine::File *DebugLine::Line::file() const;
 * Get the source file corresponding to the code.
 * @return	Code source file.
 */

/**
 * @fn int DebugLine::Line::line() const;
 * Get the source line corresponding to this code.
 * @return	Source line of the corresponding code.
 */

/**
 * @fn int DebugLine::Line::code() const;
 * Get the source column corresponding to this code.
 * @return	Source column of the corresponding code.
 */

/**
 * @fn t::uint32 DebugLine::Line::flags() const;
 * Get flags about this code.
 * @return	Code flags (combination of IS_STMT, BASIC_BLOCK, PROLOGUE_END
 * 			and EPILOGUE_BEGIN).
 */

/**
 * @fn address_t DebugLine::Line::addr() const;
 * Get the base address of this piece of code.
 * @return	Base address.
 */

/**
 * @fn t::uint8 DebugLine::Line::isa() const;
 * Get the ISA of instructions used in this code.
 * @return	ISA code.
 */

/**
 * @fn t::uint8 DebugLine::Line::discriminator();
 */

/**
 * @fn t::uint8 DebugLine::Line::op_index() const;
 * Get the operation index of the instruction starting this code
 * (only meaningful for VLIW architecture).
 * @return	Operation index.
 */


/**
 * @class CompilationUnit
 * Represents a compilation unit involved in the build of an executable or
 * of a dynamic library.
 */

/**
 * @fn const FragTable<DebugLine::Line>& DebugLine::CompilationUnit::lines() const;
 * Get the array of lines in the compilation unit. Notice that the last entry
 * of this array does not represent an actual line but provides the top address of
 * the previous line.
 * @return	Array of lines.
 */

/**
 * @fn const Vector<DebugLine::File *>& DebugLine::CompilationUnit::files() const;
 * Get the list of source files involved in this compilation unit.
 * @return	List of compilation units.
 */


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
DebugLine::DebugLine(elf::File *efile): prog(*efile) {

	// customize according to the type of file
	if(prog.addressType() == address_32) {
		addr_size = 4;
		length_size = 4;
	}
	else if(prog.addressType() == address_64) {
		addr_size = 8;
		length_size = 12;
	}
	else
		throw gel::Exception("unsupported address type");

	// get the buffer
	elf::Section *sect = prog.findSection(".debug_line");
	if(sect == nullptr)
		return;
	Cursor c(sect->content());

	// decode the content
	DEBUG("reading (size =" << c.size() << ")");
	while(!c.ended())
		readCU(c);
}

///
DebugLine::~DebugLine() {
	deleteAll(_files);
	deleteAll(_cus);
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
	_cus.add(cu);
	c.move(end_offset);
}

void DebugLine::readHeader(Cursor& c, StateMachine& sm, CompilationUnit *cu) {
	error_if(!c.avail(8 + length_size));

	// skip version
	t::uint16 version;
	c.read(version);
	prog.fix(version);
	DEBUG("version = " << version);
	if(version > 4)
		throw gel::Exception("DWARF version > 4");

	// read header length
	size_t header_length = readHeaderLength(c);
	size_t lines = c.offset() + header_length;
	DEBUG("lines = " << lines);

	// base information
	c.read(sm.minimum_instruction_length);
	DEBUG("min inst length = " << cu->minimum_instruction_length);
	if(version >= 4) {
		c.read(sm.maximum_operations_per_instruction);
		DEBUG("max op per inst = " << cu->maximum_operations_per_instruction);
	}
	else
		sm.maximum_operations_per_instruction = 1;

	// SM initialization
	t::uint8 default_is_stmt;
	c.read(default_is_stmt);
	if(default_is_stmt)
		sm.set(Line::IS_STMT);
	DEBUG("default_is_stmt = " << sm.bit(Line::IS_STMT));
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
					DEBUG("advance line " << cu->_files[sm.file - 1]->path() << ":" << l << "@" << io::hex(sm.address) << " -> " << sm.line);
				}
				break;
			case DW_LNS_set_file:
				sm.file = readLEB128U(c);
				break;
			case DW_LNS_set_column:
				sm.column = readLEB128U(c);
				break;
			case DW_LNS_negate_stmt:
				if(sm.bit(Line::IS_STMT))
					sm.clear(Line::IS_STMT);
				else
					sm.set(Line::IS_STMT);
				break;
			case DW_LNS_set_basic_block:
				sm.set(Line::BASIC_BLOCK);
				break;
			case DW_LNS_const_add_pc:
				advancePC(sm, cu, (255 - sm.opcode_base) / sm.line_range);
				break;
			case DW_LNS_fixed_advance_pc: {
					t::uint16 o;
					error_if(!c.read(o));
					prog.fix(o);
					sm.address += o;
					sm.op_index = 0;
				}
				break;
			case DW_LNS_set_prologue_end:
				sm.set(Line::PROLOGUE_END);
				break;
			case DW_LNS_set_epilogue_begin:
				sm.set(Line::EPILOGUE_BEGIN);
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
	File *file = cu->_files[sm.file - 1];
	DEBUG("line " << io::hex(sm.address) << " "
		 << file->path() << ":" << sm.line << ":" << sm.column);

	// record the line
	cu->_lines.add(Line(sm.address, file, sm.line,
		sm.column, sm.flags, sm.isa, sm.discriminator, sm.op_index));

	// update the SM
	sm.set(Line::BASIC_BLOCK | Line::PROLOGUE_END | Line::EPILOGUE_BEGIN);
	sm.discriminator = 0;

	// record the line number
	if(!file->_units.contains(cu))
		file->_units.add(cu);
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
	if(dir == 0)
		p = s;
	else
		p = sys::Path(sm.include_directories[dir]) / s;
	File *f = _files.get(p, nullptr);
	if(f == nullptr) {
		f = new File(p, date, size);
		_files[p] = f;
	}
	cu->_files.add(f);
	f->_units.add(cu);
	return true;
}

size_t DebugLine::readHeaderLength(Cursor& c) {
	if(addr_size == 4) {
		t::uint32 l;
		c.read(l);
		prog.fix(l);
		return l;
	}
	else {
		t::uint64 l;
		c.read(l);
		prog.fix(l);
		return l;
	}
}

size_t DebugLine::readUnitLength(Cursor& c) {
	if(!c.avail(length_size))
		throw gel::Exception("bad length");
	t::uint32 l;
	error_if(!c.read(l));
	if(addr_size == 4) {
		prog.fix(l);
		return l;
	}
	if(l != 0xffffffffff)
		throw gel::Exception("bad 64-bit length");
	t::uint64 ll;
	error_if(!c.read(ll));
	prog.fix(ll);
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
		prog.fix(a);
		return a;
	}
	else {
		t::uint64 a;
		error_if(!c.read(a));
		prog.fix(a);
		return a;
	}
}

} }	// gel::elf


