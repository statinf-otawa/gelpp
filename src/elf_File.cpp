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
		if(h->flags() & PF_X)
			_name = "code";
		else if(h->flags() & PF_W)
			_name = "data";
		else if(h->flags() & PF_R)
			_name = "rodata";
		else
			_name = "unknown";
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
	segs_init(false)
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
cstring File::machine() {
	switch(machineCode()) {
	case 0:		return "no machine";		// No machine.
	case 1:		return "we32100";		// AT&T WE 32100
	case 2:		return "sparc";			// SPARC
	case 3:		return "386";			// Intel 80386
	case 4:		return "m68k";			// Motorola 68000
	case 5:		return "m88k";			// Motorola 88000
	case 7:		return "860";			// Intel 80860
	case 8:		return "mips r3k";		// MIPS RS3000 Big-Endian
	case 10:	return "mips r4k";		// MIPS RS4000 Big-Endian
	case 15:	return "pa-risc";		// Hewlett-Packard PA-RISC
	case 17:	return "vpp500";		// Fujitsu VPP500
	case 18:	return "sparc32+";		// EM_SPARC32PLUS Enhanced instruction set SPARC
	case 19:	return "960";			// EM_960 Intel 80960
	case 20:	return "ppc";			// EM_PPC Power PC
	case 36:	return "v800";			// EM_V800 NEC V800
	case 37:	return "fr20";			// EM_FR20 Fujitsu FR20
	case 38:	return "rh32";			// EM_RH32 TRW RH-32
	case 39:	return "rce";			// EM_RCE Motorola RCE
	case 40:	return "arm";			// EM_ARM Advanced RISC Machines ARM
	case 41:	return "alpha";			// EM_ALPHA	Digital Alpha
	case 42:	return "sh";			// EM_SH Hitachi SH
	case 43:	return "sparcv9";		// EM_SPARCV9 SPARC Version 9
	case 44:	return "tricore";		// EM_TRICORE Siemens Tricore embedded processor
	case 45:	return "arc";			// EM_ARC Argonaut RISC Core, Argonaut Technologies Inc.
	case 46:	return "h8/300";		// EM_H8_300 Hitachi H8/300
	case 47:	return "h8/300h";		// EM_H8_300H Hitachi H8/300H
	case 48:	return "h8s";			// EM_H8S Hitachi H8S
	case 49:	return "h8/5003";		// EM_H8_500 Hitachi H8/500
	case 50:	return "ia-64";			// EM_IA_64 Intel MercedTM Processor
	case 51:	return "mps-x";			// EM_MIPS_X Stanford MIPS-X
	case 52:	return "coldfire";		// EM_COLDFIRE Motorola Coldfire
	case 53:	return "68hc12";		// EM_68HC12 Motorola M68HC12
	case 54:	return "mma";			// EM_MMA Fujitsu MMA Multimedia Accelerator
	case 55:	return "pcp";			// EM_PCP Siemens PCP
	case 56:	return "ncpu";			// EM_NCPU Sony nCPU embedded RISC processor
	case 57:	return "ndr1";			// EM_NDR1 Denso NDR1 microprocessor
	case 58:	return "starcore";		// EM_STARCORE Motorola Star*Core processor
	case 59:	return "me16";			// EM_ME16 Toyota ME16 processor
	case 60:	return "st100";			// EM_ST100 STMicroelectronics ST100 processor
	case 61:	return "tinyj";			// EM_TINYJ Advanced Logic Corp. TinyJ embedded processor family
	case 66:	return "fx663";			// EM_FX66 Siemens FX66 microcontroller
	case 67:	return "st9+";			// EM_ST9PLUS STMicroelectronics ST9+ 8/16 bit microcontroller
	case 68:	return "st7";			// EM_ST7 STMicroelectronics ST7 8-bit microcontroller
	case 69:	return "68hc16";		// EM_68HC16 Motorola MC68HC16 Microcontroller
	case 70:	return "68hc11";		// EM_68HC11 Motorola MC68HC11 Microcontroller
	case 71:	return "68hc08";		// EM_68HC08 Motorola MC68HC08 Microcontroller
	case 72:	return "68hc05";		// EM_68HC05 Motorola MC68HC05 Microcontroller
	case 73:	return "svx";			// EM_SVX Silicon Graphics SVx
	case 74:	return "st19";			// EM_ST19 STMicroelectronics ST19 8-bit microcontroller
	case 75:	return "vax";			// EM_VAX Digital VAX
	case 76:	return "cris";			// EM_CRIS Axis Communications 32-bit embedded processor
	case 77:	return "javelin";		// EM_JAVELIN Infineon Technologies 32-bit embedded processor
	case 78:	return "firepath";		// EM_FIREPATH Element 14 64-bit DSP Processor
	case 79:	return "zsp";			// EM_ZSP LSI Logic 16-bit DSP Processor
	case 80:	return "mmix";			// EM_MMIX Donald Knuth's educational 64-bit processor
	case 81:	return "huany";			// EM_HUANY Harvard University machine-independent object files
	case 82:	return "prism";			// EM_PRISM SiTera Prism
	default:	return gel::File::machine();
	}
}


///
cstring File::os() {
	switch(ident()[EI_OSABI]) {
	case ELFOSABI_SYSV:			return "SysV";
	case ELFOSABI_HPUX: 		return "HPUX";
	case ELFOSABI_STANDALONE:	return "standalone";
	default:					return gel::File::os();
	}
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
void SymbolTable::record(t::uint8 *mem) {
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
