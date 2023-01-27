/*
 * gel::elf::File class
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

#include <elm/array.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File.h>
#include <gel++/elf/UnixBuilder.h>
#include <gel++/elf/DebugLine.h>
#include <gel++/Image.h>

namespace gel { namespace elf {

/**
 * @defgroup elf ELF Loader
 *
 * This group includes all classes implementing the support for the ELF file format.
 * It is based on the following documents:
 * * Tool Interface Standard (TIS), Executable and Linking Format (ELF) Specification Version 1.2. TIS Committee. May 1995.
 *
 */

class Segment: public gel::Segment {
public:
	Segment(ProgramHeader *h): _head(h) {
		_name = defaultName(this);
	}

	cstring name() 			override { return _name; }
	address_t baseAddress()	override { return _head->vaddr(); }
	address_t loadAddress()	override { return _head->paddr(); }
	size_t size() 			override { return _head->memsz(); }
	size_t alignment() 		override { return _head->align(); }
	bool isExecutable() 	override { return _head->flags() & PF_X; }
	bool isWritable()		override { return _head->flags() & PF_W; }
	bool hasContent()		override { return true; }
	Buffer buffer()			override { return _head->content(); }

private:
	cstring _name;
	ProgramHeader *_head;
};


/**
 * @class File
 * Class handling executable file in ELF format (32-bits).
 * @ingroup elf
 */


void File::fix(t::uint16& i) 	{ i = ENDIAN2(id[EI_DATA], i); }
void File::fix(t::int16& i) 	{ i = ENDIAN2(id[EI_DATA], i); }
void File::fix(t::uint32& i)	{ i = ENDIAN4(id[EI_DATA], i); }
void File::fix(t::int32& i)		{ i = ENDIAN4(id[EI_DATA], i); }
void File::fix(t::uint64& i)	{ i = ENDIAN8(id[EI_DATA], i); }
void File::fix(t::int64& i)		{ i = ENDIAN8(id[EI_DATA], i); }

void File::unfix(t::uint16& i) 	{ i = UN_ENDIAN2(id[EI_DATA], i); }
void File::unfix(t::int16& i) 	{ i = UN_ENDIAN2(id[EI_DATA], i); }
void File::unfix(t::uint32& i)	{ i = UN_ENDIAN4(id[EI_DATA], i); }
void File::unfix(t::int32& i)	{ i = UN_ENDIAN4(id[EI_DATA], i); }
void File::unfix(t::uint64& i)	{ i = UN_ENDIAN8(id[EI_DATA], i); }
void File::unfix(t::int64& i)	{ i = UN_ENDIAN8(id[EI_DATA], i); }


/**
 * Test if the given magic number matches ELF.
 * param magic	Magic number to be tested.
 * @return		True if magic number is PE-COFF, false else.
 */
bool File::matches(t::uint8 magic[4]) {
	return magic[0] == ELFMAG0
		&& magic[1] == ELFMAG1
		&& magic[2] == ELFMAG2
		&& magic[3] == ELFMAG3;
}


/**
 * Constructor.
 * @param manager	Parent manager.
 * @param path		File path.
 * @param stream	Stream to read from.
 */
File::File(Manager& manager, sys::Path path, io::RandomAccessStream *stream)
:	gel::File(manager, path),
	s(stream),
	id(nullptr),
	ph_loaded(false),
	sects_loaded(false),
	str_tab(nullptr),
	syms(nullptr),
	segs_init(false),
	debug(nullptr)
{
}

/**
 */
File::~File(void) {
	delete s;
	if(syms != nullptr)
		delete syms;
	for(auto s: sects)
		delete s;
	for(auto p: phs)
		delete p;
	for(auto s: segs)
		delete s;
}

/**
 * Get the map of symbols of the file.
 * @return	Map of symbols.
 */
const gel::SymbolTable& File::symbols() {
	if(syms == nullptr) {
		syms = new SymbolTable();
		initSections();
		for(auto s: sects) {
			if(s->type() == SHT_SYMTAB || s->type() == SHT_DYNSYM)
				fillSymbolTable(*syms, s);
		}
	}
	return *syms;
}

/**
 * Get the program headers.
 * @return	Program headers.
 * @throw gel::Exception	If there is a file read error.
 */
Vector<ProgramHeader *>& File::programHeaders(void) {
	if(!ph_loaded) {
		loadProgramHeaders(phs);
		ph_loaded = true;
	}
	return phs;
}

/**
 * Get a string from the string table.
 * @param offset	Offset of the string.
 * @return			Found string.
 * @throw gel::Exception	If there is a file read error or offset is out of bound.
 */
cstring File::stringAt(t::uint64 offset) {
	return stringAt(offset, getStrTab());
}


/**
 * Get a string from the string table.
 * @param offset	Offset of the string.
 * @param sect		Section index to find the string in.
 * @return			Found string.
 * @throw gel::Exception	If there is a file read error or offset is out of bound.
 */
cstring File::stringAt(t::uint64 offset, int sect) {
	if(sect >= sections().length())
		throw gel::Exception(_ << "strtab index out of bound");
	cstring r;
	sects[sect]->content().get(offset, r);
	return r;
}


/**
 * Read from the file and throw an exception if there is an error.
 * @param buf	Buffer to fill in.
 * @param size	Size of the buffer.
 */
void File::read(void *buf, t::uint32 size) {
	if(t::uint32(s->read(buf, size)) != size)
		throw Exception(_ << "cannot read " << size << " bytes from " << path() << ": " << s->io::InStream::lastErrorMessage());
}


/**
 * Read a lock at a particular position.
 * @param pos	Position in file.
 * @param buf	Buffer to fill in.
 * @param size	Size of the buffer.
 */
void File::readAt(t::uint32 pos, void *buf, t::uint32 size) {
	if(!s->moveTo(pos))
		throw Exception(_ << "cannot move to position " << pos << " in " << path() << ": " << s->io::InStream::lastErrorMessage());
	read(buf, size);
}



/**
 */
File *File::toELF(void) {
	return this;
}


/**
 */
bool File::isBigEndian(void) {
	return id[EI_DATA] == ELFDATA2MSB;
}


/**
 */
Image *File::make(const Parameter& params) {
	//UnixBuilder builder(this, params);
	SimpleBuilder builder(this, params);
	return builder.build();
}


///
void File::initSegments() {
	if(segs_init)
		return;
	for(auto ph: programHeaders())
		if(ph->type() == PT_LOAD)
			segs.add(new Segment(ph));
	segs_init = true;
}


///
int File::count() {
	initSegments();
	return segs.count();
}


/**
 */
gel::Segment *File::segment(int i) {
	initSegments();
	return segs[i];
}


///
string File::machine() const {
	// Look here https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
	switch(elfMachine()) {
	case 0x00:	return "no machine";
	case 0x01:	return "we32100";
	case 0x02:	return "sparc";
	case 0x03:	return "i386";
	case 0x04:	return "m68k";
	case 0x05:	return "m88k";
	case 0x06:	return "iMCU";
	case 0x07:	return "i860";
	case 0x08:	return "r3000";
	case 0x09:	return "S370";
	case 0x0A:	return "r4000";
	case 0x0E:	return "pa-risc";
	case 0x11:	return "vpp500";
	case 0x12:	return "sparc32+";
	case 0x13:	return "i960";
	case 0x14:	return "ppc";
	case 0x15:	return "ppc64";
	case 0x16:	return "S390";
	case 0x17:	return "SPUC/SPC";
	case 0x24:	return "v800";
	case 0x25:	return "fr20";
	case 0x26:	return "rh32";
	case 0x27:	return "rce";
	case 0x28:	return "arm";
	case 0x29:	return "alpha";
	case 0x2A:	return "superh";
	case 0x2B:	return "sparcv9";
	case 0x2C:	return "tricore";
	case 0x2D:	return "arc";
	case 0x2E:	return "h8/300";
	case 0x2F:	return "h8/300h";
	case 0x30:	return "h8s";
	case 0x31:	return "h8/500";
	case 0x32:	return "ia-64";
	case 0x33:	return "mips-x";
	case 0x34:	return "coldfire";
	case 0x35:	return "m68hc12";
	case 0x36:	return "mma";
	case 0x37:	return "pcp";
	case 0x38:	return "ncpu";
	case 0x39:	return "ndr1";
	case 0x3A:	return "star*core";
	case 0x3B:	return "me16";
	case 0x3C:	return "st100";
	case 0x3D:	return "tinyj";
	case 0x3E:	return "x86-64";
	case 66:	return "fx663";
	case 67:	return "st9+";
	case 68:	return "st7";
	case 69:	return "68hc16";
	case 70:	return "68hc11";
	case 71:	return "68hc08";
	case 72:	return "68hc05";
	case 73:	return "svx";
	case 74:	return "st19";
	case 75:	return "vax";
	case 76:	return "cris";
	case 77:	return "javelin";
	case 78:	return "firepath";
	case 79:	return "zsp";
	case 80:	return "mmix";
	case 81:	return "huany";
	case 82:	return "prism";
	case 0x8C:	return "tms320c6000";
	case 0xB7:	return "arm64";
	case 0xF3:	return "riscv";
	case 0xF7:	return "bpf";
	case 0X101: return "wdc65c816";
	default:	return _ << "unknown (" << elfMachine() << ")";
	}
}


///
string File::os() const {
	switch(elfOS()) {
	case ELFOSABI_SYSV:			return "SysV";
	case ELFOSABI_HPUX: 		return "HPUX";
	case 0x02:					return "NetBSD";
	case 0x03:					return "Linux";
	case 0x04:					return "GNU-Hurd";
	case 0x06:					return "Solaris";
	case 0x07:					return "AIX";
	case 0x08:					return "IRIX";
	case 0x09:					return "FreeBSD";
	case 0x0A:					return "Tru64";
	case 0x0B:					return "Novel-Modesto";
	case 0x0C:					return "OpenBSD";
	case 0x0D:					return "OpenVMS";
	case 0x0E:					return "NonStop-Kernel";
	case 0x0F:					return "AROS";
	case 0x10:					return "Fenix-OS";
	case 0x11:					return "CloudABI";
	case 0X12:					return "OpenVOS";
	case ELFOSABI_STANDALONE:	return "standalone";
	default:					return gel::File::os();
	}
}


///
gel::DebugLine *File::debugLines() {
	if(debug == nullptr)
		debug = new DebugLine(this);
	return debug;
}


/**
 * Initialize the section part.
 */
void File::initSections(void) {
	if(!sects_loaded) {
		loadSections(sects);
		sects_loaded = true;
	}
}


/**
 * Get the sections of the file.
 * @return	File sections.
 * @throw gel::Exception 	If there is an error when file is read.
 */
Vector<Section *>& File::sections(void) {
	initSections();
	return sects;
}


/**
 * Find a section by its name.
 * @param name	Name of the looked section.
 * @return		Found section or null pointer.
 */
Section *File::findSection(cstring name) {
	for(auto s: sections())
		if(s->name() == name)
			return s;
	return nullptr;
}


/**
 * @class Section
 * A section in an ELF file.
 * @ingroup elf
 */

/**
 * Builder.
 * @param file	Parent file.
 * @param entry	Section entry.
 */
Section::Section(elf::File *file): _file(file), buf(0) {
}

Section::~Section(void) {
	if(buf)
		delete [] buf;
}

/**
 * Get the content of the section (if any).
 * @return	Section content.
 * @throw gel::Exception	If there is a file read error.
 */
Buffer Section::content() {
	if(!buf)
		buf = readBuf();
	return Buffer(_file, buf, size());
}


/**
 * @class ProgramHeader;
 * Represents an ELF program header, area of memory ready to be loaded
 * into the program image.
 * @ingroup elf
 */

/**
 */
ProgramHeader::ProgramHeader(elf::File *file): _file(file), _buf(nullptr) {
}

/**
 */
ProgramHeader::~ProgramHeader(void) {
	if(_buf)
		delete [] _buf;
}

/**
 * Get the content of the program header.
 * @return	Program header contant.
 * @throw gel::Exception	If there is an error at file read.
 */
Buffer ProgramHeader::content(void) {
	if(_buf == nullptr)
		_buf = readBuf();
	return Buffer(_file, _buf, memsz());
}

/**
 * @fn bool ProgramHeader::contains(address_t a) const;
 * Test if the program contains the given address.
 * @param a		Address to test.
 * @return		True if the address is in the program header, false else.
 */


/**
 * @class Symbol
 * A symbol for an ELF file (32- or 64-bit).
 * @ingroup elf
 */

/**
 * @fn Symbol::Symbol(File& file): _file(file);
 * Build a symbol from the given ELF file.
 * @param file	ELF file containing the symbol.
 */

///
Symbol::~Symbol() {
};

///
cstring Symbol::name() {
	return _name;
}


/**
 * @class SymbolTable
 * A table of symbols of an ELF file.
 * @ingroup elf
 */


/**
 * Build the symbol table.
 */


///
SymbolTable::~SymbolTable() {
	for(auto m: mems)
		delete [] m;
}


///
void SymbolTable::record(elm::t::uint8 *mem) {
	mems.add(mem);
}


/**
 * @class  NoteIter
 * Iterator on the notes for a PT_NOTE program header.
 * @ingroup elf
 */


/**
 */
NoteIter::NoteIter(ProgramHeader& ph): c(ph.content()) {
	ASSERT(ph.type() == PT_NOTE);
	next();
}

/**
 * Read the next note.
 */
void NoteIter::next(void) {

	// end reached
	if(c.ended()) {
		_desc = 0;
		return;
	}

	// read the note header
	if(!c.avail(sizeof(t::uint32) * 3))
		throw Exception("malformed note entry");
	t::uint32 namesz;
	c.read(namesz);
	c.read(_descsz);
	c.read(_type);

	// get name and descriptor
	const t::uint8 *p;
	if(!c.read(namesz, p))
		throw Exception("malformed note entry");
	_name = cstring((const char *)p);
	if(!c.read(size_t(_descsz), p))
		throw Exception("malformed note entry");
	_desc = reinterpret_cast<const t::uint32 *>(p);
}


/**
 * @fn bool NoteIter::ended(void);
 * Test if the note traversal is ended.
 * @return	True if the traversal is ended, false else.
 */


/**
 * @fn cstring NoteIter::name(void) const;
 * Get note name.
 * @return Note name.
 */

/**
 * @fn Elf32_Word NoteIter::descsz(void) const;
 * Get tdescription size.
 * @return	Description size (in bytes).
 */

/**
 * @fn const Elf32_Word *NoteIter::desc(void) const;
 * Get description of the note.
 * @return	Note description.
 */

/**
 * @fn Elf32_Word NoteIter::type(void) const;
 * Get the type of the note.
 * @return	Note type.
 */


/**
 * @class DynIter
 * Iterator on the dynamic entries of a section.
 */

///
File::DynIter::DynIter(File& file, Section *sec, bool ended): EntryIter(file, sec, ended) {
	if(ended)
		c.finish();
	else if(!c.ended())
		f.fetchDyn(c.here(), d);
}

Range<File::DynIter> File::dyns() {
	for(auto s: sects)
		if(s->type() == SHT_DYNAMIC)
			return dyns(s);
	ASSERT(false);
	return dyns(nullptr);
}

Range<File::DynIter> File::dyns(Section *sect) {
	return range(DynIter(*this, sect), DynIter(*this, sect, true));
}

} }	// gel::elf
