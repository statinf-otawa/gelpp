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
#include <elm/data/List.h>
#include <elm/data/Vector.h>
#include <elm/io/RandomAccessStream.h>
#include "../Exception.h"
#include "../File.h"
#include "defs.h"

namespace gel { namespace elf {

class ProgramHeader {
public:
	ProgramHeader(elf::File *file);
	virtual ~ProgramHeader();
	Buffer content();
	inline bool contains(address_t a) const { return vaddr() <= a && a < vaddr() + memsz(); }
	inline Decoder *decoder(void) const;

	virtual t::uint32 flags() const = 0;
	virtual address_t vaddr() const = 0;
	virtual address_t paddr() const = 0;
	virtual size_t memsz() const = 0;
	virtual size_t align() const = 0;
	virtual int type() const = 0;
	virtual size_t filesz() const = 0;
	virtual offset_t offset() const = 0;

protected:
	virtual t::uint8 *readBuf() = 0;
	inline void readAt(t::uint32 pos, void *buf, t::uint32 size);

private:
	elf::File *_file;
	t::uint8 *_buf;
};


class Section {
public:
	Section(void);
	Section(elf::File *file);
	virtual ~Section(void);
	Buffer content(void);
	inline bool contains(address_t a)
		{ return ((flags() & SHF_ALLOC) != 0) && addr() <= a && a < addr() + size(); }

	virtual cstring name() = 0;
	virtual t::uint32 flags() const = 0;
	virtual int type() const = 0;
	virtual t::uint32 link() const = 0;
	virtual t::uint64 offset() const = 0;
	virtual address_t addr() const = 0;
	virtual size_t size() const = 0;
	virtual size_t entsize() const = 0;
	virtual void read(t::uint8 *buf) = 0;

protected:
	virtual t::uint8 *readBuf() = 0;
	inline void readAt(t::uint32 pos, void *buf, t::uint32 size);
	inline elf::File *file() const { return _file; }

private:
	elf::File *_file;
	t::uint8 *buf;
};

class Symbol {
public:
	inline Symbol(cstring name): _name(name) { }
	virtual ~Symbol();
	inline cstring name() const { return _name; };
	virtual t::uint64 value() = 0;
	virtual t::uint64 size() = 0;
	virtual t::uint8 bind() = 0;
	virtual t::uint8 type()  = 0;
	virtual int shndx() = 0;

protected:
	cstring _name;
};


class SymbolTable: public HashMap<cstring, Symbol *> {
public:
	~SymbolTable();
	void record(t::uint8 *mem);
private:
	List<t::uint8 *> mems;
};


class Segment;


class File: public gel::File, public Decoder {
	friend class ProgramHeader;
	friend class Section;
	friend class Segment;
public:
	File(Manager& manager, sys::Path path, io::RandomAccessStream *stream);
	virtual ~File(void);

	virtual int elfType() = 0;
	virtual t::uint16 machine() = 0;
	virtual t::uint16 version() = 0;
	virtual const t::uint8 *ident() = 0;

	typedef Vector<Section *>::Iter SecIter;
	Vector<Section *>& sections(void);
	inline Section *sectionAt(int i) const { return sects[i]; }
	inline int sectionCount(void) const { return sects.count(); }

	typedef Vector<ProgramHeader *>::Iter ProgIter;
	Vector<ProgramHeader *>& programHeaders(void);
	inline ProgramHeader *headerAt(int i) const { return phs[i]; }
	inline int headerCount(void) const { return phs.count(); }

	cstring stringAt(t::uint64 offset);
	cstring stringAt(t::uint64 offset, int sect);

	const SymbolTable& symbols();
	virtual void fillSymbolTable(SymbolTable& symtab, Section *sect) = 0;

	// gel::File overload
	File *toELF() override;
	bool isBigEndian() override;
	Image *make(const Parameter& params) override;
	int count() override;
	gel::Segment *segment(int i) override;

	// Decoder override
	void fix(t::uint16& i) override;
	void fix(t::int16& i) override;
	void fix(t::uint32& i) override;
	void fix(t::int32& i) override;
	void fix(t::uint64& i) override;
	void fix(t::int64& i) override;

	void unfix(t::uint16& w) override;
	void unfix(t::int16& w) override;
	void unfix(t::uint32& w) override;
	void unfix(t::int32& w) override;
	void unfix(t::uint64& w) override;
	void unfix(t::int64& w) override;


protected:
	inline void setIdent(t::uint8 *i) { id = i; }
	virtual void loadProgramHeaders(Vector<ProgramHeader *>& headers) = 0;
	virtual void loadSections(Vector<Section *>& segments) = 0;
	virtual int getStrTab() = 0;
	void read(void *buf, t::uint32 size);
	void readAt(t::uint32 pos, void *buf, t::uint32 size);

private:
	void initSections();
	void initSegments();

	io::RandomAccessStream *s;
	t::uint8 *id;
	bool ph_loaded;
	Vector<ProgramHeader *> phs;
	bool sects_loaded;
	Vector<Section *> sects;
	Section *str_tab;
	SymbolTable *syms;
	Vector<Segment *> segs;
	bool segs_init;
};

class NoteIter {
public:
	NoteIter(ProgramHeader& ph);
	inline bool ended(void) const { return !_desc; }
	void next(void);
	inline operator bool(void) const { return !ended(); }
	inline NoteIter& operator++(void) { next(); return *this; }
	inline NoteIter& operator++(int) { next(); return *this; }

	inline cstring name(void) const { return _name; }
	inline t::uint32 descsz(void) const { return _descsz; }
	inline const t::uint32 *desc(void) const { return _desc; }
	inline t::uint32 type(void) const { return _type; }

private:
	Cursor c;
	cstring _name;
	t::uint32 _descsz;
	const t::uint32 *_desc;
	t::uint32 _type;
};

inline Decoder *ProgramHeader::decoder(void) const { return _file; }
inline void ProgramHeader::readAt(t::uint32 pos, void *buf, t::uint32 size)
	{ _file->readAt(pos, buf, size); }
inline void Section::readAt(t::uint32 pos, void *buf, t::uint32 size)
	{ _file->readAt(pos, buf, size); }

} }	// gel::elf

#endif /* ELF_FILE_H_ */
