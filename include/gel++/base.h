/*
 * GEL++ base definitions
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
#ifndef INCLUDE_GEL___BASE_H_
#define INCLUDE_GEL___BASE_H_

#include <elm/types.h>

namespace gel {

using namespace elm;

typedef t::uint64 address_t;
typedef t::uint64 size_t;
typedef t::uint64 offset_t;

typedef enum {
	address_8,
	address_16,
	address_32,
	address_64
} address_type_t;

io::IntFormat format(address_type_t t, address_t a);

class Decoder {
public:
	virtual ~Decoder(void);
	virtual void fix(t::uint16& w) = 0;
	virtual void fix(t::int16& w) = 0;
	virtual void fix(t::uint32& w) = 0;
	virtual void fix(t::int32& w) = 0;
	virtual void fix(t::uint64& w) = 0;
	virtual void fix(t::int64& w) = 0;
};

class Buffer {
public:
	inline Buffer(void): d(0), b(0), s(0) { }
	inline Buffer(Decoder *decoder, const t::uint8 *buffer, size_t size)
		: d(decoder), b(buffer), s(size) { }
	inline Buffer(Decoder *decoder, const void *buffer, size_t size)
		: d(decoder), b((t::uint8 *)buffer), s(size) { }

	inline const t::uint8 *buffer(void) const { return b; }
	inline size_t size(void) const { return s; }
	static Buffer null;
	inline bool isNull(void) const { return !b; }
	inline bool equals(const Buffer& buf) const { return b == buf.b && s == buf.s; }

	inline const t::uint8 *at(offset_t offset) const
		{ ASSERT(offset < s); return b + offset; }
	inline void get(offset_t off, t::uint8& r) const
		{ ASSERT(off + sizeof(t::uint8) <= s); r = *(t::uint8 *)(b + off); }
	inline void get(offset_t off, t::int8& r) const
		{ ASSERT(off + sizeof(t::int8) <= s); r = *(t::int8 *)(b + off); }
	inline void get(offset_t off, t::uint16& r) const
		{ ASSERT(off + sizeof(t::uint16) <= s); d->fix(r = *(t::uint16 *)(b + off)); }
	inline void get(offset_t off, t::int16& r) const
		{ ASSERT(off + sizeof(t::int16) <= s); d->fix(r = *(t::int16 *)(b + off)); }
	inline void get(offset_t off, t::uint32& r) const
		{ ASSERT(off + sizeof(t::uint32) <= s); d->fix(r = *(t::uint32 *)(b + off)); }
	inline void get(offset_t off, t::int32& r) const
		{ ASSERT(off + sizeof(t::int32) <= s); d->fix(r = *(t::int32 *)(b + off)); }
	inline void get(offset_t off, t::uint64& r) const
		{ ASSERT(off + sizeof(t::uint64) <= s); d->fix(r = *(t::uint64 *)(b + off)); }
	inline void get(offset_t off, t::int64& r) const
		{ ASSERT(off + sizeof(t::int64) <= s); d->fix(r = *(t::int64 *)(b + off)); }
	inline void get(offset_t off, cstring& s)
		{ ASSERT(off < s); s = cstring((const char *)(b + off)); }
	inline void get(offset_t off, string& s)
		{ ASSERT(off < s); s = string((const char *)(b + off)); }

	inline operator bool(void) const { return !isNull(); }
	inline bool operator==(const Buffer& b) const { return equals(b); }
	inline bool operator!=(const Buffer& b) const { return !equals(b); }

private:
	Decoder *d;
	const t::uint8 *b;
	size_t s;
};
io::Output& operator<<(io::Output& out, const Buffer& buf);

class Cursor {
public:
	inline Cursor(void): off(0) { }
	inline Cursor(const Buffer& b): buf(b), off(0) { }

	inline bool ended(void) const { return off >= buf.size(); }
	inline operator bool(void) const { return !ended(); }
	inline bool avail(size_t s) { return off + s <= buf.size(); }

	inline bool read(t::uint8& v)
		{ if(!avail(sizeof(t::uint8))) return false; buf.get(off, v); off += sizeof(t::uint8); return true; }
	inline bool read(t::int8& v)
		{ if(!avail(sizeof(t::int8))) return false; buf.get(off, v); off += sizeof(t::int8); return true; }
	inline bool read(t::uint16& v)
		{ if(!avail(sizeof(t::uint16))) return false; buf.get(off, v); off += sizeof(t::uint16); return true; }
	inline bool read(t::int16& v)
		{ if(!avail(sizeof(t::int16))) return false; buf.get(off, v); off += sizeof(t::int16); return true; }
	inline bool read(t::uint32& v)
		{ if(!avail(sizeof(t::uint32))) return false; buf.get(off, v); off += sizeof(t::uint32); return true; }
	inline bool read(t::int32& v)
		{ if(!avail(sizeof(t::int32))) return false; buf.get(off, v); off += sizeof(t::int32); return true; }
	inline bool read(t::uint64& v)
		{ if(!avail(sizeof(t::uint64))) return false; buf.get(off, v); off += sizeof(t::uint64); return true; }
	inline bool read(t::int64& v)
		{ if(!avail(sizeof(t::int64))) return false; buf.get(off, v); off += sizeof(t::int64); return true; }
	bool read(cstring& s);
	inline bool read(string& s) { cstring r; read(r); s = string(r); }
	bool read(size_t size, const t::uint8 *& buf);

private:
	offset_t off;
	Buffer buf;
};

} // gel

#endif /* INCLUDE_GEL___BASE_H_ */
