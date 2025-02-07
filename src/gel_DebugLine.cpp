/*
 * GEL++ DebugLine class implementation
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

#include <gel++/DebugLine.h>

namespace gel {

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
 * @class DebugLine::LineNumber
 * Represents a line of code information in the source line table.
 */
DebugLine::LineNumber::LineNumber(address_t addr, File *file, int line, int col,
t::uint32 flags, t::uint8 isa, t::uint8 desc, t::uint8 opi)
	: _file(file), _line(line), _col(col), _flags(flags),
	  _addr(addr), _isa(isa), _disc(desc), _opi(opi) { }

/**
 * @fn DebugLine::File *DebugLine::LineNumber::file() const;
 * Get the source file corresponding to the code.
 * @return	Code source file.
 */

/**
 * @fn int DebugLine::LineNumber::line() const;
 * Get the source line corresponding to this code.
 * @return	Source line of the corresponding code.
 */

/**
 * @fn int DebugLine::LineNumber::code() const;
 * Get the source column corresponding to this code.
 * @return	Source column of the corresponding code.
 */

/**
 * @fn t::uint32 DebugLine::LineNumber::flags() const;
 * Get flags about this code.
 * @return	Code flags (combination of IS_STMT, BASIC_BLOCK, PROLOGUE_END
 * 			and EPILOGUE_BEGIN).
 */

/**
 * @fn address_t DebugLine::LineNumber::addr() const;
 * Get the base address of this piece of code.
 * @return	Base address.
 */

/**
 * @fn t::uint8 DebugLine::LineNumber::isa() const;
 * Get the ISA of instructions used in this code.
 * @return	ISA code.
 */

/**
 * @fn t::uint8 DebugLine::LineNumber::discriminator();
 */

/**
 * @fn t::uint8 DebugLine::LineNumber::op_index() const;
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
 * Add a debug information line number to the compilation unit.
 * @param num	Line number information to add.
 */
void DebugLine::CompilationUnit::add(const LineNumber& num) {
	_lines.add(num);
}

/**
 * Add a source file to the compilation unit.
 * @param file	Added file.
 */
void DebugLine::CompilationUnit::add(File *file) {
	_files.add(file);
	file->_units.add(this);
}

/**
 * Get the base address of the compilation unit.
 * @return	Base address.
 */
address_t DebugLine::CompilationUnit::baseAddress() const {
	return _lines[0].addr();
}

/**
 * Get the top address of the compilation unit.
 * @return	Top address.
 */
address_t DebugLine::CompilationUnit::topAddress() const {
	return _lines[_lines.count() - 1].addr();
}

/**
 * @fn size_t DebugLine::CompilationUnit::size() const;
 * Compute the size of the compilation unit.
 * @return	Compilation unit size in bytes.
 */

/**
 * Find the line description corresponding to the given address.
 * @return	Found line number or null pointer.
 */
const DebugLine::LineNumber *DebugLine::CompilationUnit::lineAt(address_t addr) const {
	for(int i = 0; i < _lines.count() - 1; i++)
		if(_lines[i].addr() <= addr && addr < _lines[i+1].addr())
			return &_lines[i];
	return nullptr;
}


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
DebugLine::DebugLine(gel::File *efile): prog(*efile) {
}

///
DebugLine::~DebugLine() {
	deleteAll(_files);
	deleteAll(_cus);
}

/**
 * Find the line at the given address.
 * @return	Found line or null.
 */
const DebugLine::LineNumber *DebugLine::lineAt(address_t addr) const {
	for(auto unit: _cus)
		if(unit->hasDebugInfo() && unit->baseAddress() <= addr && addr < unit->topAddress())
			return unit->lineAt(addr);
	return nullptr;
}


/**
 * Add a compilation unit.
 * @param cu	Added compilation unit.
 */
void DebugLine::add(CompilationUnit *cu) {
	_cus.add(cu);
}

/**
 * Add a source file.
 * @param file	Source ile to add.
 */
void DebugLine::add(File *file) {
	_files.put(file->path(), file);
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

}	// gel


