/*
 * GEL++ PE-COFF File interface
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

#ifndef GELPP_PECOFF_FILE_H_
#define GELPP_PECOFF_FILE_H_

#include <gel++/File.h>

namespace gel { namespace pecoff {

class File: public gel::File {
public:
	File(Manager& manager, sys::Path path);
	~File(void);

	type_t type(void) override;
	bool isBigEndian(void) override;
	address_type_t addressType(void) override;
	address_t entry(void) override;
	int count() override;
	Segment *segment(int i) override;
	Image *make(const Parameter& params) override;

	const SymbolTable& symbols() override;
};

} } // gel::pecoff

#endif /* GELPP_PECOFF_FILE_H_ */
