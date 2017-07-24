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
#ifndef GELPP_ELF_FILE_H_
#define GELPP_ELF_FILE_H_

#include <elm/data/HashMap.h>
#include <elm/data/Vector.h>
#include <elm/io/RandomAccessStream.h>
#include "../Exception.h"
#include "../File.h"
#include "defs.h"

namespace gel { namespace elf {

class ProgramHeader {
public:
	ProgramHeader(void);
	ProgramHeader(File *file, Elf32_Phdr *info);
	~ProgramHeader(void);

	inline const Elf32_Phdr& info(void) const { return *_info; }
	Buffer content(void) throw(gel::Exception);
	inline bool contains(address_t a) const { return _info->p_vaddr <= a && a < _info->p_vaddr + _info->p_memsz; }
	inline Decoder *decoder(void) const;

private:
	elf::File *_file;
	Elf32_Phdr *_info;
	t::uint8 *_buf;
};

class Section: public Segment {
public:
	Section(void);
	Section(elf::File *file, Elf32_Shdr *entry);
	~Section(void);
	cstring name(void) throw(gel::Exception);
	const Elf32_Shdr& info(void) { return *_info; }
	Buffer content(void) throw(gel::Exception);
	inline bool contains(address_t a) const
		{ return (_info->sh_flags & SHF_ALLOC) && _info->sh_addr <= a && a < _info->sh_addr + _info->sh_size; }

	// Segment overload
	virtual address_t baseAddress(void);
	virtual address_t loadAddress(void);
	virtual size_t size(void);
	virtual size_t alignment(void);
	virtual bool isExecutable(void);
	virtual bool isWritable(void);
	virtual bool hasContent(void);
	virtual Buffer buffer(void) throw(gel::Exception);

private:
	elf::File *_file;
	Elf32_Shdr *_info;
	t::uint8 *buf;
};


class File: public gel::File, private Decoder {
	friend class ProgramHeader;
	friend class Section;
public:
	File(Manager& manager, sys::Path path, io::RandomAccessStream *stream) throw(Exception);
	virtual ~File(void);

	const Elf32_Ehdr& info(void) const { return *h; }
	typedef Vector<Section>::Iter SecIter;
	Vector<Section *>& sections(void) throw(gel::Exception);
	typedef Vector<ProgramHeader>::Iter ProgIter;
	Vector<ProgramHeader>& programHeaders(void) throw(gel::Exception);

	cstring stringAt(t::uint32 offset) throw(gel::Exception);

	typedef HashMap<cstring, const Elf32_Sym *> SymbolMap;
	const SymbolMap& symbols(void) throw(Exception);

	// gel::File overload
	virtual File *toELF(void);
	virtual type_t type(void);
	virtual bool isBigEndian(void);
	virtual address_type_t addressType(void);
	virtual address_t entry(void);
	virtual Image *make(const Parameter& params) throw(Exception);
	virtual int count(void);
	virtual Segment *segment(int i);

private:
	void initSections(void);
	void read(void *buf, t::uint32 size) throw(Exception);
	void readAt(t::uint32 pos, void *buf, t::uint32 size) throw(Exception);

	virtual void fix(t::uint16& i);
	virtual void fix(t::int16& i);
	virtual void fix(t::uint32& i);
	virtual void fix(t::int32& i);
	virtual void fix(t::uint64& i);
	virtual void fix(t::int64& i);

	Elf32_Ehdr *h;
	io::RandomAccessStream *s;
	t::uint8 *sec_buf;
	Section *str_tab;
	Vector<Section *> sects;
	t::uint8 *ph_buf;
	Vector<ProgramHeader> phs;
	SymbolMap *syms;
	Vector<Segment *> segs;
};

class NoteIter {
public:
	NoteIter(ProgramHeader& ph) throw(Exception);
	inline bool ended(void) const { return !_desc; }
	void next(void) throw(Exception);
	inline operator bool(void) const { return !ended(); }
	inline NoteIter& operator++(void) { next(); return *this; }
	inline NoteIter& operator++(int) { next(); return *this; }

	inline cstring name(void) const { return _name; }
	inline Elf32_Word descsz(void) const { return _descsz; }
	inline const Elf32_Word *desc(void) const { return _desc; }
	inline Elf32_Word type(void) const { return _type; }

private:
	Cursor c;
	cstring _name;
	Elf32_Word _descsz;
	const Elf32_Word *_desc;
	Elf32_Word _type;
};

inline Decoder *ProgramHeader::decoder(void) const { return _file; }

class SymbolIter: public PreIterator<SymbolIter, const Elf32_Sym& > {
public:
	SymbolIter(File& file, Section& section) throw(Exception);
	inline bool ended(void) const { return c.ended(); }
	inline const Elf32_Sym& item(void) const { return *(const Elf32_Sym *)c.here(); }
	void next(void);
	cstring name(void) throw(Exception);

private:
	File& f;
	Section& s;
	Cursor c;
	Buffer names;
};

} }	// gel::elf

#endif /* ELF_FILE_H_ */
