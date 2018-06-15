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
 * @class UnixBuilder
 * Image builder mimicking the dynamic loader of Unix OS.
 * Only supports program of type ELF.
 */

/**
 */
UnixBuilder::UnixBuilder(File *prog, const Parameter& params) throw(gel::Exception)
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
 */
Image *UnixBuilder::build(void) throw(gel::Exception) {
	_im = new Image(_prog);

	// process each library in turn
	address_t base = 0;
	todo.put(_prog);
	while(todo) {
		File *file = todo.get();
		_im->add(file);
		address_t top = load(file, base);
		base = roundup(top, _uparams->page_size);
		//resolveLibs(file);
	}

	// build the stack
	buildStack();

	// clean up
	_libs.clear();
	return _im;
}


/**
 * Open the given file.
 * @param path	Path of file to open.
 * @return		Opened file or null (if it can't be opened or does not match).
 */
File *UnixBuilder::open(sys::Path path) {
	if(!path.isReadable())
		return 0;
	try {
		File *f = _prog->manager().openELF(path);
		if(f->info().e_machine == _prog->info().e_machine)
			return f;
		else {
			delete f;
			_prog->manager().getErrorHandler()->onError(level_warning, _ << "library " << path << ": bad machine");
			return 0;
		}
	}
	catch(Exception& e) {
		_prog->manager().getErrorHandler()->onError(level_warning, _ << "loading library " << path << ": " << e.message());
		return 0;
	}
}


/**
 */
gel::File *UnixBuilder::retrieve(string name) throw(Exception) {

	// already found?
	File *f = _libs.get(name, 0);
	if(f)
		return f;

	// contains any "/"?
	if(name.indexOf('/') >= 0)
		f = open(sys::Path(name));

	// lookup in the path
	else {
		for(int i = 0; !f && i < lpaths.count(); i++)
			f = open(lpaths[i] / name);
		for(int i = 0; !f && i < _params.paths.count(); i++)
			f = open(_params.paths[i] / name);
	}

	// record if found
	if(f)
		_libs[name] = f;
	return f;
}


/**
 * Build the content of the initial stack in the given segment.
 * @param s					Segment containing the stack.
 * @return					Allocated segment.
 * @throw gel::Exception	If the stack size is too small.
 */
ImageSegment *UnixBuilder::buildStack(void) throw(gel::Exception) {
	const t::uint32 zero(0);
	if(_uparams->stack_alloc)
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
		addr = _params.stack_addr - size;
	address_t sp = addr + size - isize;
	if(_params.sp)
		*_params.sp = sp;

	// create the segment
	Buffer buf(_prog, new t::uint8[size], size);
	ImageSegment *seg = new ImageSegment(buf, addr, ImageSegment::WRITABLE | ImageSegment::TO_FREE);
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
 * @throw gel::Exception	In case of error.
 */
void UnixBuilder::resolveLibs(File *file) throw(gel::Exception) {
	lpaths.clear();

	// look for SHT_DYNAMIC
	for(File::SecIter sec = file->sections(); sec; sec++)
		if(sec->info().sh_type == SHT_DYNAMIC) {

			// get string section
			if(int(sec->info().sh_link) >= file->sectionCount())
				throw gel::Exception(_ << "format error in " << sec->name());
			Section *strs = file->sectionAt(sec->info().sh_link);
			Buffer str = strs->buffer();

			// read the content
			for(Cursor c(sec->buffer()); c.avail(sizeof(Elf32_Dyn)); c.skip(sizeof(Elf32_Dyn))) {
				Elf32_Dyn *e = (Elf32_Dyn *)c.here();
				c.decoder()->fix(e->d_tag);
				switch(e->d_tag) {

				case DT_NULL:
					c.finish();
					break;

				case DT_RPATH: {
						t::uint32 off = e->d_un.d_val;
						c.decoder()->fix(off);
						if(off >= str.size())
							throw gel::Exception("bad offset in DT_RPATH entry");
						string name;
						str.get(off, name);
						lpaths.add(sys::Path(name));
					}
					break;

				case DT_NEEDED: {
						t::uint32 off = e->d_un.d_val;
						c.decoder()->fix(off);
						if(off >= str.size())
							throw gel::Exception("bad offset in DT_NEEDED entry");
						string name;
						str.get(off, name);
						gel::File *glib = retrieve(name);
						if(!glib)
							throw gel::Exception(_ << "cannot find dynamic library" << name);
						File *lib = glib->toELF();
						ASSERT(lib);
						bool found = false;
						for(Image::LinkIter l = _im->files(); !found && l; l++)
							if((*l).file == lib)
								found = true;
						if(!found) {
							_im->add(lib);
							todo.put(lib);
						}
					}
					break;

				case DT_PLTRELSZ:
				case DT_PLTGOT:
				case DT_HASH:
				case DT_STRTAB:
				case DT_SYMTAB:
				case DT_RELA:
				case DT_RELASZ:
				case DT_RELAENT:
				case DT_STRSZ:
				case DT_SYMENT:
				case DT_INIT:
				case DT_FINI:
				case DT_SONAME:
				case DT_SYMBOLIC:
				case DT_REL:
				case DT_RELSZ:
				case DT_RELENT:
				case DT_PLTREL:
				case DT_DEBUG:
				case DT_TEXTREL:
				case DT_JMPREL:
				case DT_BIND_NOW:
					getErrorHandler()->onError(level_warning, _ << "unknown dynamic entry: " << io::hex(e->d_tag));
					break;
				}
			}
		}
}


/**
 * Load a file at the given base address.
 * @param file	File to load.
 * @param base			Base address.
 * @return				Top used address.
 * @throw Exception		If there is an error during the load.
 */
address_t UnixBuilder::load(File *file, address_t base) throw(gel::Exception) {
	address_t top = base;
	for(int i = 0; i < file->count(); i++) {
		gel::Segment *s = file->segment(i);
		ImageSegment *is = new ImageSegment(file, s, base + s->baseAddress());
		_im->add(is);
		top = max(top, base + s->baseAddress() + s->size());
	}
	return top;
}

} }		// gel::elf
