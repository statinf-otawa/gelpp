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
 * @var Array<sys::Path> UnixParameter::lib_paths;
 * The given paths are used to retrieve libraries in dynamic linking
 * resolution. They are put at the front of the list of looked directories.
 */

/**
 * @var sys::Path UnixParameter::sys_root;
 * If not null, it is used as prefix of all tested paths to retrieve
 * dynamic libraries. It may be used to embed in the current file system,
 * a file system corresponding to the linked executable and libraries.
 */

/**
 * @var bool UnixParameter::is_linux;
 * If set to true (default), consider that the executable lives in a Linux
 * system (/lib is also looked to resolve dynamic libraries, the RPATH
 * supports $ORIGIN, $LIB and $PLATFORM).
 */

/**
 * @var bool UnixParameter::no_default_path;
 * If set to true (default to false), no default directory (/lib, /usr/lib)
 * is involved in the dynamic library retrieval.
 */


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
Unit::Unit(sys::Path name): _name(name), _file(nullptr), _base(0), _dyn(nullptr)
{ }


/**
 * If required, retrieve the file from path list. And load it in the image.
 * @param builder	Current builder.
 * @param base		Base address.
 * @throw Exception	If there is an error in opening the library file.
 */
t::uint32 Unit::load(UnixBuilder& builder, t::uint32 base) {
	_base = base;
	address_t top = base;

	// build the image
	for(auto h: _file->programHeaders()) {
		switch(h->type()) {

		// load a segment
		case PT_LOAD: {
				ImageSegment::flags_t f = 0;
				if((h->flags() & PF_X) != 0)
					f |= ImageSegment::EXECUTABLE;
				if((h->flags() & PF_W) != 0)
					f |= ImageSegment::WRITABLE;
				if((h->flags() & PF_R) != 0)
					f |= ImageSegment::READABLE;
				ImageSegment *is = new ImageSegment(_file, h->content(), base + h->vaddr(), f);
				builder._im->add(is);
				top = max(top, _base + h->vaddr() + h->memsz());
			}
			break;

		// record for dynamic linking
		case PT_DYNAMIC:
			_dyn = h;
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
			builder.onError(level_warning, _ << "unknown program header " << format(address_32, h->type()));
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
				string path = getString(str, off);
				int i = path.indexOf(':');
				while(i >= 0) {
					_rpath.add(builder.expand(path.substring(0, i), this));
					path = path.substring(i + 1);
					i = path.indexOf(':');
				}
				_rpath.add(builder.expand(path, this));
			}
			break;

		case DT_NEEDED: {
				t::uint32 off = e->d_un.d_val;
				c.decoder()->fix(off);
				cstring name = getString(str, off);
				Unit *u = builder.resolve(name, this);
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

	// add LD_LIBRARY_PATHS
	cstring llp = _uparams->getenv("LD_LIBARY_PATH");
	int i = llp.indexOf(':');
	while(i >= 0) {
		lpaths.add(llp.substring(0, i));
		llp = llp.substring(i + 1);
		i = llp.indexOf(':');
	}
	lpaths.add(llp);

	// add parameter paths
	for(auto p: _uparams->lib_paths)
		lpaths.add(p);

	// add default paths
	if(!_uparams->no_default_path) {
		if(_uparams->is_linux)
			lpaths.add("/lib");
		lpaths.add("/usr/lib");
	}
}

/**
 * Try to load a unit using the given path.
 * @param p		Path of the unit.
 */
Unit *UnixBuilder::get(sys::Path p) {

	// look in current units
	p = p.absolute();
	Unit *u = _units.get(p, nullptr);
	if(u != nullptr)
		return u;

	// creates the unit
	u = new Unit(p);
	todo.add(u);
	_units.put(p, u);
	return u;

}


/**
 * Get a link unit by its name. If it is already involved in linking, return
 * the corresponding unit. Else, creates the unit and record it for
 * processing.
 * @param name	Unit name.
 * @param unit	Unit requiring the named unit.
 * @return		Unit.
 */
Unit *UnixBuilder::resolve(cstring name, Unit *unit) {

	// '/' in name => name = path
	// relative path: relative to what? CWD?
	if(name.indexOf('/') >= 0) {
		Unit *u = get(sys::Path(name));
		if(u != nullptr)
			return u;
	}

	// DT_RPATH
	for(auto p: unit->_rpath) {
		sys::Path pp = sys::Path(p) / name;
		Unit *u = get(pp);
		if(u != nullptr)
			return u;
	}

	// LD_LIBRARY_PATH + default
	for(auto p: lpaths) {
		sys::Path pp = sys::Path(p) / name;
		Unit *u = get(pp);
		if(u != nullptr)
			return u;
	}

	// not found
	return nullptr;
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
#	if 0
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
#	endif
	return nullptr;
}


/**
 * Expand the RPATH component s according to the given unit u.
 * @param s		RPATH component to expand.
 * @param u		Current unit.
 * @return		Expanded RPATH component.
 */
string UnixBuilder::expand(string s, Unit *u) {
	if(!_uparams->is_linux)
		return s;
	if(!s.startsWith("$"))
		return s;
	if(!s.startsWith("${")) {
		if(s.startsWith("$ORIGIN"))
			return u->origin() / s.substring(7);
		else if(s.startsWith("$LIB"))
			return sys::Path("lib") / s.substring(4);
		// !!TODO!!
		// else if(s.startsWith("$PLATFORM"))
		//	return platform / s.substring(9);
	}
	else {
		if(s.startsWith("${ORIGIN}"))
			return u->origin() / s.substring(9);
		else if(s.startsWith("${LIB}"))
			return sys::Path("lib") / s.substring(6);
		// !!TODO!!
		// else if(s.startsWith("${PLATFORM}"))
		//	return platform / s.substring(11);
	}

	onError(level_warning, _ << "cannot expand " << s);
	return s;
}


/**
 * Retrive a file corresponding to a library name.
 * @param name		Library name.
 * @return			Found file or null.
 * @throw Exception	If the file can not be obtained.
 */
gel::File *UnixBuilder::retrieve(sys::Path name) {
	if(!_uparams->sys_root.isEmpty())
		name = _uparams->sys_root / name.toString();
	if(!name.isFile())
		return nullptr;
	try {
		return gel::Manager::openELF(name);
	}
	catch(gel::Exception& e) {
		return nullptr;
	}
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
}

} }		// gel::elf
