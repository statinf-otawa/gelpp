/*
 * GEL++ UnixBuilder class implementation
 * Copyright (c) 2016, IRIT- University of Toulouse
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

#include <elm/data/VectorQueue.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File.h>
#include <gel++/elf/UnixBuilder.h>
#include <gel++.h>

namespace gel { namespace elf {


/**
 * @class Auxiliary
 * An auxiliary value in the Unix System V ABI defined as a pair
 * (type, value).
 */


/**
 * @class UnixParameter
 * Specialization of Parameter for the Unix image builder.
 */

/**
 * @var size_UnixParameter::page_size;
 * Size (in bytes) of OS pages (default 4KB).
 */

/**
 * @var Array<Auxiliary> auxv;
 * Vector of auxiliary values (see Unix System V ABI).
 */

/**
 */
UnixParameter::UnixParameter(void): page_size(4 * 1024) { }

/**
 * Default parameter configuration.
 */
UnixParameter UnixParameter::null;


/**
 * @class Unit
 * Program or library involved in the image building.
 * @ingroup elf
 */

/**
 */
Unit::Unit(File *file): _file(file), _base(0), _dyn(nullptr)
{ }

/**
 */
Unit::Unit(cstring name): _name(name), _file(nullptr), _base(0), _dyn(nullptr)
{ }

/**
 * If required, retrieve the file from path list. And load it in the image.
 * @param builder	Current builder.
 * @param base		Base address.
 * @throw Exception	If there is an error in opening the library file.
 */
t::uint32 Unit::load(UnixBuilder& builder, t::uint32 base) {
	_base = base;
	t::uint32 top = base;

	// build the image
	for(auto h: _file->programHeaders()) {
		switch(h.info().p_type) {

		// load a segment
		case PT_LOAD: {
				ImageSegment::flags_t f = 0;
				if((h.info().p_flags & PF_X) != 0)
					f |= ImageSegment::EXECUTABLE;
				if((h.info().p_flags & PF_W) != 0)
					f |= ImageSegment::WRITABLE;
				if((h.info().p_flags & PF_R) != 0)
					f |= ImageSegment::READABLE;
				ImageSegment *is = new ImageSegment(_file, h.content(), base + h.info().p_vaddr, f);
				builder._im->add(is);
				top = max(top, _base + h.info().p_vaddr + h.info().p_memsz);
			}
			break;

		// record for dynamic linking
		case PT_DYNAMIC:
			_dyn = &h;
			break;

		// simply ignored
		case PT_INTERP:
			break;
		case PT_NOTE:
			break;
		case PT_SHLIB:
			break;
		case PT_PHDR:
			break;
		default:
			builder.onError(level_warning, _ << "unknown program header " << format(address_32, h.info().p_type));
			break;
		}
	}

	return top;
}


/**
 * Read the dynamic part and link with libraries, if required.
 *
 */
void Unit::link(UnixBuilder& builder) {

	// relocate in back order
	if(_dyn == nullptr)
		return;

	// first collect static information
	// TODO should be improved to use only loaded segments
	for(Cursor c(_dyn->content()); c.avail(sizeof(Elf32_Dyn)); c.skip(sizeof(Elf32_Dyn))) {
		Elf32_Dyn *e = (Elf32_Dyn *)c.here();
		Elf32_Sword tag = e->d_tag;
		c.decoder()->fix(tag);
		switch(e->d_tag) {
		case DT_NULL:		c.finish(); break;
		case DT_NEEDED:		break;
		case DT_PLTRELSZ:	c.decoder()->fix(e->d_un.d_val); pltrelsz = e->d_un.d_val; break;
		case DT_PLTGOT:		c.decoder()->fix(e->d_un.d_ptr); pltgot = e->d_un.d_ptr; break;
		case DT_HASH:		c.decoder()->fix(e->d_un.d_ptr); hash = e->d_un.d_ptr; break;
		case DT_STRTAB:		c.decoder()->fix(e->d_un.d_ptr); strtab = e->d_un.d_ptr; break;
		case DT_SYMTAB:		c.decoder()->fix(e->d_un.d_ptr); symtab = e->d_un.d_ptr; break;
		case DT_RELA:		break;
		case DT_RELASZ:		break;
		case DT_RELAENT:	break;
		case DT_STRSZ:		c.decoder()->fix(e->d_un.d_val); strsz = e->d_un.d_val; break;
		case DT_SYMENT:		c.decoder()->fix(e->d_un.d_val); syment = e->d_un.d_val; break;
		case DT_INIT:		c.decoder()->fix(e->d_un.d_ptr); init = e->d_un.d_ptr; break;
		case DT_FINI:		c.decoder()->fix(e->d_un.d_ptr); fini = e->d_un.d_ptr; break;
		case DT_SONAME:		/* TODO */; break;
		case DT_RPATH:		break;
		case DT_SYMBOLIC:	flags |= SYMBOLIC; break;
		case DT_REL:		break;
		case DT_RELSZ:		break;
		case DT_RELENT:		break;
		case DT_PLTREL:		break;
		case DT_DEBUG:		c.decoder()->fix(e->d_un.d_ptr); debug = e->d_un.d_ptr; break;
		case DT_TEXTREL:	flags |= TEXTREL; break;
		case DT_JMPREL:		break;
		case DT_BIND_NOW:	flags |= BIND_NOW; break;
		default:
			builder.onError(level_warning, _ << "unknown dynamic entry: " << io::hex(e->d_tag));
			break;
		}
	}

	// obtain the segment containing the string table
	ImageSegment *str = builder._im->at(strtab);
	if(str == nullptr)
		throw Exception("STRTAB address not in loaded segments!");

	// perform the link itself
	Vector<sys::Path> lpaths;
	for(Cursor c(_dyn->content()); c.avail(sizeof(Elf32_Dyn)); c.skip(sizeof(Elf32_Dyn))) {
		Elf32_Dyn *e = (Elf32_Dyn *)c.here();
		Elf32_Sword tag = e->d_tag;
		c.decoder()->fix(tag);
		switch(e->d_tag) {

		case DT_NULL:
			c.finish();
			break;

		case DT_RPATH: {
				t::uint32 off = e->d_un.d_val;
				c.decoder()->fix(off);
				cstring path = getString(str, off);
				lpaths.add(sys::Path(path));
			}
			break;

		case DT_NEEDED: {
				t::uint32 off = e->d_un.d_val;
				c.decoder()->fix(off);
				cstring name = getString(str, off);
				Unit *u = builder.get(name);
				_needed.add(u);
			}
			break;

		}
	}
}


/**
 * Get the C string at the given offset in the STRTAB.
 * @param s		Segment containing the STRTAB.
 * @param off	Offset of the C string.
 * @return		Found C string.
 */
cstring Unit::getString(ImageSegment *s, t::uint32 off) {
	if(off >= strsz)
		throw gel::Exception("bad offset in DT_RPATH entry");
	cstring name;
	s->buffer().get(off - s->base(), name);
	return name;
}


/**
 * @class UnixBuilder
 * Image builder mimicking the dynamic loader of Unix OS.
 * Only supports program of type ELF.
 */

/**
 */
UnixBuilder::UnixBuilder(File *prog, const Parameter& params)
:	ImageBuilder(prog, params),
	_prog(0),
	_uparams(&UnixParameter::null),
	_im(0)
{
	if(params.abi() == Parameter::unix_abi)
		_uparams = static_cast<const UnixParameter *>(&params);
	_prog = prog->toELF();
	ASSERTP(_prog, "UnixBuilder builder supports only ELF files!");
	ASSERTP(_prog->type() == File::program, "file must be a program!");
}

/**
 * Get a link unit by its name. If it is already involved in linking, return
 * the corresponding unit. Else, creates the unit and record it for
 * processing.
 * @param name	Unit name.
 * @return		Unit.
 */
Unit *UnixBuilder::get(cstring name) {

	// look in current units
	Unit *u = _units.get(name, nullptr);
	if(u != nullptr)
		return u;

	// creates the unit
	u = new Unit(name);
	todo.add(u);
	_units.put(name, u);
	return u;
}

/**
 */
Image *UnixBuilder::build(void) {
	_im = new Image(_prog);

	// create initial unit
	Unit *u = new Unit(_prog);
	todo.add(u);

	// process each library in turn
	address_t base = 0;
	int i = 0;
	while(i < todo.count()) {
		u = todo[i++];
		address_t top = u->load(*this, base);
		// perform link
		base = roundup(top, _uparams->page_size);
	}

	// build the stack
	buildStack();


	// clean up
	return _im;
}


/**
 * Open the given file.
 * @param path	Path of file to open.
 * @return		Opened file or null (if it can't be opened or does not match).
 */
File *UnixBuilder::open(sys::Path path) {
	if(!path.isReadable())
		return nullptr;
	try {
		File *f = _prog->manager().openELF(path);
		if(f->info().e_machine == _prog->info().e_machine)
			return f;
		else {
			delete f;
			_prog->manager().getErrorHandler()->onError(level_warning, _ << "library " << path << ": bad machine");
			return nullptr;
		}
	}
	catch(Exception& e) {
		_prog->manager().getErrorHandler()->onError(level_warning, _ << "loading library " << path << ": " << e.message());
		return nullptr;
	}
}


/**
 * Add an RPath content.
 * @param paths	Colon-separated paths to retrieve a dependency.
 */
void UnixBuilder::addRPath(string paths) {

}


/**
 * Retrive a file corresponding to a library name.
 * @param name		Library name.
 * @return			Found file or null.
 * @throw Exception	If the file can not be obtained.
 */
gel::File *UnixBuilder::retrieve(string name) {
	File *f = nullptr;

	// contains any "/"?
	if(name.indexOf('/') >= 0)
		f = open(sys::Path(name));

	// lookup in the path
	else {
		for(int i = 0; f == nullptr && i < lpaths.count(); i++)
			f = open(lpaths[i] / name);
		for(int i = 0; f == nullptr && i < _params.paths.count(); i++)
			f = open(_params.paths[i] / name);
	}

	// return found file
	return f;
}


/**
 * Build the content of the initial stack in the given segment.
 * @param s					Segment containing the stack.
 * @return					Allocated segment.
 * @throw gel::Exception	If the stack size is too small.
 */
ImageSegment *UnixBuilder::buildStack(void) {
	const t::uint32 zero(0);
	if(!_uparams->stack_alloc)
		return nullptr;

	// compute the sizes
	size_t s = 0;
	s += 3 * sizeof(t::uint32);									// argc, argv, envp
	size_t arg_a = s;
	s += (_params.arg.count() + 1) * sizeof(t::uint32);			// argv[argc + 1]
	size_t env_a = s;
	s += (_params.env.count() + 1) * sizeof(t::uint32);			// envp[envc + 1]
	s += (_uparams->auxv.count() * 2 + 1) * sizeof(t::uint32);	// auxv
	size_t arg_s = s;
	for(int i = 0; i < _params.arg.count(); i++)
		s += _params.arg[i].length() + 1;
	size_t env_s = s;
	for(int i = 0; i < _params.env.count(); i++)
		s += _params.env[i].length() + 1;
	size_t isize = elm::roundup(s, sizeof(t::uint32));

	// check the size
	size_t size = _params.stack_size;
	if(size < isize)
		throw gel::Exception("stack size too small");

	// initial address
	address_t addr = 0x80000000;
	if(_params.stack_at)
		addr = _params.stack_addr;
	addr -= size;
	address_t sp = addr + size - isize;
	if(_params.sp)
		*_params.sp = sp;

	// create the segment
	Buffer buf(_prog, new t::uint8[size], size);
	ImageSegment *seg = new ImageSegment(buf, addr, ImageSegment::WRITABLE | ImageSegment::TO_FREE, "stack");
	if(_params.sp_segment)
		*_params.sp_segment = seg;
	_im->add(seg);

	// put the main arguments
	Cursor c(buf);
	c.skip(size - isize);
	c.write(t::uint32(_params.arg.count()));
	c.write(t::uint32(sp + arg_a));
	c.write(t::uint32(sp + env_a));

	// put the argument array
	t::uint32 p = sp + arg_s;
	for(int i = 0; i < _params.arg.count(); i++) {
		c.write(p);
		p += _params.arg[i].length() + 1;
	}
	c.write(zero);

	// put the environment array
	p = sp + env_s;
	for(int i = 0; i < _params.env.count(); i++) {
		c.write(p);
		p += _params.env[i].length() + 1;
	}
	c.write(zero);

	// put the auxiliary vector
	for(int i = 0; i < _uparams->auxv.count(); i++) {
		c.write(_uparams->auxv[i].type);
		c.write(_uparams->auxv[i].val);
	}
	c.write(zero);

	// put the argument strings
	for(int i = 0; i < _params.arg.count(); i++)
		c.write(_params.arg[i]);

	// put the environment strings
	for(int i = 0; i < _params.env.count(); i++)
		c.write(_params.env[i]);

	return seg;
}


/**
 * Resolve the dynamic libraries for the gien file (program or library).
 * @param file				File to resolve for.
 * @param base				Base address of the unit.
 * @throw gel::Exception	In case of error.
 */
void UnixBuilder::link(File *file, address_t base) {
	// TODO
#if 0
	}
#endif
}

} }		// gel::elf
