/*
 * gel-sym command
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
#include <gel++.h>
#include <gel++/elf/File.h>

using namespace elm;
using namespace elm::option;
using namespace gel;

io::IntFormat word_fmt = io::IntFormat().hex().width(8).right().pad('0');

class SymCommand: public option::Manager {
public:
	SymCommand(void):
		option::Manager(option::Manager::Make("gel-sym", Version(2, 0, 0))
			.description("Display symbols of an ELF exceutable.")
			.copyright("Copyright (c) 2016, université de Toulouse")
			.free_argument("<file path>")
			.help())
	{ }

	int run(int argc, char **argv) {
		try {
			parse(argc, argv);
			if(!args) {
				displayHelp();
				cerr << "\nERROR: no executable given.\n";
				return 1;
			}
		}
		catch(OptionException& e) {
			displayHelp();
			cerr << "\nERROR: " << e.message() << io::endl;
			return 1;
		}

		// process each executable
		for(int i = 0; i < args.count(); i++)
			try {
				elf::File *f = gel::Manager::openELF(args[i]);
				for(int i = 0; i < f->sections().length(); i++) {
					elf::Section *sect = f->sections()[i];
					if(sect->type() == SHT_SYMTAB || sect->type() == SHT_DYNAMIC) {
						cout << "SECTION " << sect->name() << io::endl;
						cout << "st_value st_size  binding type    st_shndx         name\n";
						for(auto s: f->symbols()) {
							auto sym = static_cast<elf::Symbol *>(s);
							cout <<	word_fmt(sym->value())						<< ' '
								 << word_fmt(sym->size()) 						<< ' '
								 << io::fmt(sym->size()).width(7)				<< ' '
								 << io::fmt(sym->elfType()).width(7)			<< ' '
								 << io::fmt(get_section_index(f, *sym)).width(16)	<< ' '
								 << sym->name()										<< io::endl;
						}
					}
				}
				delete f;
			}
			catch(gel::Exception& e) {
				cerr << "ERROR: during opening of " << args[i] << ": " << e.message() << io::endl;
				return 2;
			}

		return 0;
	}

protected:
	virtual void process(String arg) {
		args.add(arg);
	}

private:

	/**
	 * Get the binding of a symbol.
	 * @param infos		Information about the symbol.
	 * @return			Binding as a string.
	 */
	string get_binding(const elf::Elf32_Sym& infos) {
		switch(ELF32_ST_BIND(infos.st_info)) {
		case STB_LOCAL: 	return "local";
		case STB_GLOBAL: 	return "global";
		case STB_WEAK: 		return "weak";
		default:			return _ << ELF32_ST_BIND(infos.st_info);
		}
	}

	/**
	 * Get the type of a symbol.
	 * @param infos		Information about the symbol.
	 * @return			Type as a string.
	 */
	string get_type(const elf::Elf32_Sym& infos) {
		switch(ELF32_ST_TYPE(infos.st_info)) {
		case STT_NOTYPE: 	return "notype";
		case STT_OBJECT: 	return "object";
		case STT_FUNC: 		return "func";
		case STT_SECTION:	return "section";
		case STT_FILE: 		return "file";
		default:			return _ << ELF32_ST_TYPE(infos.st_info);
		}
	}

	/**
	 * Get a section as a text.
	 * @param file		Current file.
	 * @param infos		Information about the symbol.
	 * @return			Section as a string.
	 */
	string get_section_index(elf::File *file, elf::Symbol& sym) {
		switch(sym.shndx()) {
		case SHN_UNDEF: 	return "undef";
		case SHN_ABS: 		return "abs";
		case SHN_COMMON:	return "common";
		default: {
			if(file->sections().length() <= sym.shndx())
				return _ << sym.shndx();
			else
				return file->sections()[sym.shndx()]->name();
			}
		}
	}

	Vector<string> args;
};

int main(int argc, char **argv) {
	return SymCommand().run(argc, argv);
}
