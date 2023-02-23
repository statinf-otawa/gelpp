/*
 * GEL++ LittleDecoder class implementation
 * Copyright (c) 2023, IRIT- université de Toulouse
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

#include <elm/types.h>
#include <gel++/LittleDecoder.h>
#include "../config.h"

using namespace elm;

#ifdef BIGENDIAN
	static inline t::uint16 swap(t::uint16 x) { return (x << 8) | ((x >> 8) & 0xff); }
	static inline t::int16 swap(t::int16 x) { return swap(t::uint16(x); }
	static inline t::uint32 swap(t::uint32 x)
		{ return (swap(t::uint16(x)) << 16) | swap(t::uint16(x >> 16))); }
	static inline t::int32 swap(t::int32 x) { return swap(t::uint32(x); }
	static inline t::uint64 swap(t::uint64 x)
		{ return (swap(t::uint32(x)) << 32) | swap(t::uint32(x >> 32))); }
	static inline t::int64 swap(t::int64 x) { return swap(t::uint64(x); }
#else
	static inline t::uint16 swap(t::uint16 x) { return x; }
	static inline t::int16 swap(t::int16 x) { return x; }
	static inline t::uint32 swap(t::uint32 x) { return x; }
	static inline t::int32 swap(t::int32 x) { return x; }
	static inline t::uint64 swap(t::uint64 x) { return x; }
	static inline t::int64 swap(t::int64 x) { return x; }
#endif


namespace gel {

/**
 * @class LittleDecoder
 * Decoder to little endian.
 */

///
LittleDecoder LittleDecoder::single;

///
void LittleDecoder::fix(t::uint16& w) { w = swap(w); }

///
void LittleDecoder::fix(t::int16& w) { w = swap(w); }

///
void LittleDecoder::fix(t::uint32& w) { w = swap(w); }

///
void LittleDecoder::fix(t::int32& w) {  w = swap(w); }

///
void LittleDecoder::fix(t::uint64& w) { w = swap(w); }

///
void LittleDecoder::fix(t::int64& w) { w = swap(w); }

///
void LittleDecoder::unfix(t::uint16& w) { w = swap(w); }

///
void LittleDecoder::unfix(t::int16& w) { w = swap(w); }

///
void LittleDecoder::unfix(t::uint32& w) {  w = swap(w); }

///
void LittleDecoder::unfix(t::int32& w) {  w = swap(w); }

///
void LittleDecoder::unfix(t::uint64& w) { w = swap(w); }

///
void LittleDecoder::unfix(t::int64& w) { w = swap(w); }

}	// gel

