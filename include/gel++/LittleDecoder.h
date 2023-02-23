/*
 * GEL++ LittleDecoder class interface
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
#ifndef GELPP_LITTLE_DECODER_H
#define GELPP_LITTLE_DECODER_H

#include <gel++/base.h>

namespace gel {

class LittleDecoder: public Decoder {
public:
	static LittleDecoder single;

	void fix(t::uint16& w) override;
	void fix(t::int16& w) override;
	void fix(t::uint32& w) override;
	void fix(t::int32& w) override;
	void fix(t::uint64& w) override;
	void fix(t::int64& w) override;

	void unfix(t::uint16& w) override;
	void unfix(t::int16& w) override;
	void unfix(t::uint32& w) override;
	void unfix(t::int32& w) override;
	void unfix(t::uint64& w) override;
	void unfix(t::int64& w) override;
};

}	// gel

#endif	// GELPP_LITTLE_DECODER_H
