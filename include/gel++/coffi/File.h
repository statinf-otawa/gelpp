/*
 * GEL++ COFFI File interface
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
#ifndef GELPP_COFFI_GILE_H
#define GELPP_COFFI_GILE_H

#include <elm/data/Vector.h>
#include <gel++/File.h>

namespace COFFI {
	class coffi;
}	// COFFI

namespace gel { namespace coffi {

using namespace elm;

class Section;
class Segment;

class File: public gel::File {
public:
	File(Manager& manager, sys::Path path);
	~File();
	static bool matches(t::uint8 magic[4]);

 	type_t type() override;
	bool isBigEndian(void) override;
	address_type_t addressType(void) override;
	address_t entry(void) override;
	int count() override;
	gel::Segment *segment(int i) override;
	Image *make(const Parameter& params) override;
	const SymbolTable& symbols() override;
	string machine() const override;
	string os() const override;
	int countSections() override;
	gel::Section *section(int i) override;

private:
	COFFI::coffi *_reader;
	address_t _base;
	Vector<Section *> _sections;
	Vector<Segment *> _segments;
};

}}	// gel::coffi

#endif	// GELPP_COFFI_GILE_H
