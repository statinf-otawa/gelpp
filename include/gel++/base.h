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

typedef enum {
	address_8,
	address_16,
	address_32,
	address_64
} address_type_t;

io::IntFormat format(address_type_t t, address_t a);

}

#endif /* INCLUDE_GEL___BASE_H_ */
