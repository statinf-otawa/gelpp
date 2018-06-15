/*
 * gel-prog command
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

#include <elm/options.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File.h>
#include <gel++.h>
#include <gel++/elf/ArchPlugin.h>

using namespace elm;
using namespace elm::option;
using namespace gel;

io::IntFormat word_fmt = io::IntFormat().hex().width(8).right().pad('0');

class DynCommand: public option::Manager {
public:
	DynCommand(void):
		option::Manager(option::Manager::Make("gel-dyn", Version(1, 0, 0))
			.description("Display dynamic linking information for the file.")
			.copyright("Copyright (c) 2017, université de Toulouse")
			.free_argument("<file path>")
			.help())
	{ }

	int run(int argc, char **argv) {

		// parse arguments
		try {
			parse(argc, argv);
		}
		catch(OptionException& e) {
			displayHelp();
			cerr << "\nERROR: " << e.message() << io::endl;
			return 1;
		}

		// process arguments
		for(int i = 0; i < args.count(); i++)
			try {
				File *f = gel::Manager::open(args[i]);

				// ELF processing
				if(args.count() > 1)
					cout << "FILE: " << args[i] << io::endl;
				elf::File *ef = f->toELF();
				if(ef)
					processELF(ef);
				if(args.count() > 1)
					cout << io::endl;

				delete f;
			}
			catch(gel::Exception& e) {
				cerr << "ERROR: when opening " << argv[i] << ": " << e.message() << io::endl;
			}
		return 0;
	}

protected:
	virtual void process(String arg) {
		args.add(arg);
	}

private:

	void processELF(elf::File *file) throw(gel::Exception) {
		static const int width = 16;
		static cstring labels[] = {
			"NULL",
			"NEEDED",
			"PLTRELSZ",
			"PLTGOT",
			"DT_HASH",
			"STRTAB",
			"SYMTAB",
			"RELA",
			"RELASZ",
			"RELAENT",
			"STRSZ",
			"SYMENT",
			"INIT",
			"FINI",
			"SONAME",
			"RPATH",
			"SYMBOLIC",
			"REL",
			"RELSZ",
			"RELENT",
			"PLTREL",
			"DEBUG",
			"TEXTREL",
			"JMPREL",
			"BIND_NOW",
			"INIT_ARRAY",
			"FINI_ARRAY",
			"INIT_ARRAYSZ",
			"FINI_ARRAYSZ",
			"RUNPATH",
			"FLAGS",
			"",
			"PREINIT_ARRAY",
			"PREINIT_ARRAYSZ",
			"SYMTAB_SHNDX"
		};

		// get the plugin
		elf::ArchPlugin *plug = elf::ArchPlugin::plug(file->info().e_machine);

		// look all sections
		for(elf::File::SecIter s = file->sections(); s; s++)

			// display the dynamics
			if(s->info().sh_type == SHT_DYNAMIC) {

				// prepare data
				Buffer buf = s->buffer();
				if(int(s->info().sh_link) >= file->sectionCount())
					throw gel::Exception("bad string table in dynamic");
				Buffer sbuf = file->sectionAt(s->info().sh_link)->buffer();

				// traverse the entries
				for(offset_t off = 0; off + s->info().sh_entsize <= buf.size(); off += s->info().sh_entsize) {
					const elf::Elf32_Dyn *e = (const elf::Elf32_Dyn *)buf.at(off);
					if(e->d_tag == DT_NULL)
						break;
					if(e->d_tag >= DT_COUNT) {
						if(plug != nullptr) {
							StringBuffer sbuf;
							plug->outputDynTag(sbuf, *e);
							cout << io::fmt(sbuf.toString()).width(width);
							cout << ": ";
							plug->outputDynValue(cout, *e, buf);
							cout << io::endl;
						}
						else
							cout << file->format(e->d_tag) << ": " << file->format(e->d_un.d_val) << io::endl;
					}
					else {
						cout << io::fmt(labels[e->d_tag]).width(width) << ": ";
						switch(e->d_tag) {
						case DT_NULL:
						case DT_SYMBOLIC:
						case DT_TEXTREL:
						case DT_BIND_NOW:
							break;
						case DT_NEEDED:
						case DT_SONAME:
						case DT_RPATH:
						case DT_RUNPATH:
							if(e->d_un.d_val >= sbuf.size())
								throw gel::Exception(_ << "bad string offset in " << e->d_tag);
							else {
								cstring s;
								sbuf.get(e->d_un.d_val, s);
								cout << s;
							}
							break;
						case DT_PLTRELSZ:
						case DT_RELASZ:
						case DT_RELAENT:
						case DT_STRSZ:
						case DT_SYMENT:
						case DT_RELSZ:
						case DT_RELENT:
						case DT_PLTREL:
						case DT_INIT_ARRAYSZ:
						case DT_FINI_ARRAYSZ:
						case DT_PREINIT_ARRAYSZ:
							cout << e->d_un.d_val;
							break;
						case DT_PLTGOT:
						case DT_HASH:
						case DT_STRTAB:
						case DT_SYMTAB:
						case DT_RELA:
						case DT_INIT:
						case DT_FINI:
						case DT_REL:
						case DT_DEBUG:
						case DT_JMPREL:
						case DT_INIT_ARRAY:
						case DT_FINI_ARRAY:
						case DT_PREINIT_ARRAY:
						case DT_SYMTAB_SHNDX:
							cout << file->format(e->d_un.d_ptr);
							break;
						case DT_FLAGS:
							if((e->d_un.d_val & DF_ORIGIN)		!= 0)	cout << "ORIGIN ";
							if((e->d_un.d_val & DF_SYMBOLIC)	!= 0)	cout << "SYMBOLIC ";
							if((e->d_un.d_val & DF_TEXTREL) 	!= 0)	cout << "TEXTREL ";
							if((e->d_un.d_val & DF_BIND_NOW)	!= 0)	cout << "BIND_NOW ";
							if((e->d_un.d_val & DF_STATIC_TLS)	!= 0)	cout << "STATIC_TLS ";
							break;
						}
						cout << io::endl;
					}
				}

			}

		// release the plugin
		if(plug != nullptr)
			plug->unplug();
	}

	Vector<string> args;
};

int main(int argc, char **argv) {
	return DynCommand().run(argc, argv);
}
