/*
 * GEL++ ELF DebugLine class interface
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
#ifndef GELPP_DWARF_DEBUG_LINE_H
#define GELPP_DWARF_DEBUG_LINE_H

#include <elm/data/FragTable.h>
#include <elm/data/HashMap.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File.h>
#include <gel++.h>
#include <gel++/elf/ArchPlugin.h>
#include <gel++/DebugLine.h>

namespace gel { namespace dwarf {

using namespace elm;

class DebugLine: public gel::DebugLine {
public:

	class StateMachine {
	public:
		inline StateMachine() { include_directories.add("."); }
		t::uint16 version;
		address_t address = 0;
		t::uint32
			op_index = 0,
			file = 1,
			line = 1,
			column = 0,
			isa = 0,
			discriminator = 0;
		bool end_sequence = false;
		t::uint8 flags = 0;
		inline void set(t::uint8 m) { flags |= m; }
		inline void clear(t::uint8 m) { flags &= ~m; }
		inline bool bit(t::uint8 m) { return (flags & m) != 0; }
		t::int8 line_base = 0;
		t::uint8
			line_range = 0,
			opcode_base = 0;
		const t::uint8 *standard_opcode_lengths = nullptr;
		t::uint8
			minimum_instruction_length = 0,
			maximum_operations_per_instruction = 0;
		t::uint8
			address_size = 0,
			segment_selector_size = 0,
			directory_entry_format_count = 0,
			filename_entry_format_count = 0;
		
		Vector<cstring> include_directories, files;
		// initialize from program header 
		bool basic_block = false;		// DWARF-5? (TODO)
		bool prologue_end = false;		// DWARF-5 (TODO)
		bool epilogue_begin = false;	// DWARF-5 (TODO)
	};

	DebugLine(elf::File *efile);
	DebugLine(gel::File *file, Buffer buf);

private:
	void readCU(Cursor& c);
	void readHeader(Cursor& c, StateMachine& sm, CompilationUnit *cu);
	void runSM(Cursor& c, StateMachine& sm, CompilationUnit *cu, size_t end);
	void advancePC(StateMachine& sm, CompilationUnit *cu, t::uint64 adv);
	void advanceLine(StateMachine& sm, t::int64 adv);
	void recordLine(StateMachine& sm, CompilationUnit *cu);
	void readDir(Cursor &c, StateMachine &sm);
	void readFile(Cursor& c, StateMachine& sm, CompilationUnit *cu);
	size_t readHeaderLength(Cursor& c);
	size_t readUnitLength(Cursor& c);
	t::int64 readLEB128S(Cursor& c);
	t::uint64 readLEB128U(Cursor& c);
	inline static void error_if(bool cond)
		{ if(cond) throw gel::Exception("debug line error"); }
	address_t readAddress(Cursor& c);

	bool is_64;
	Cursor str_sect_cursor;
	Cursor line_str_sect_cursor;
};

} }	// gel::dwarf

#endif	// GELPP_DWARF_DEBUG_LINE_H



