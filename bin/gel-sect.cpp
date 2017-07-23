/*
 * gel-sect command
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

#include <elm/io.h>
#include <elm/options.h>
#include <gel++.h>
#include <gel++/elf/defs.h>
#include <gel++/elf/File.h>

using namespace elm;
using namespace elm::option;
using namespace gel;

io::IntFormat word_fmt = io::IntFormat().hex().width(8).right().pad('0');

class SectCommand: public option::Manager {
public:
	SectCommand(void):
		Manager(Manager::Make("gel-sect", Version(2, 0, 0))
			.description("list sections of an executable")
			.copyright("copyright (c) 2016, université de Toulouse")
			.free_argument("<file path>")
			.help()),
		find(Value<address_t>::Make(*this).cmd("-f").description("find the section containing this address").argDescription("ADDRESS").def(0))
	{ }

	int run(int argc, char **argv) {
		try {

			// parse arguments
			parse(argc, argv);
			if(!args) {
				displayHelp();
				cerr << "ERROR: a binary file is required !\n";
				return 1;
			}

			// process arguments
			for(int i = 0; i < args.count(); i++) {

				elf::File *f = gel::Manager::openELF(args[i]);
				Vector<elf::Section>& ss = f->sections();

				// find option
				if(find) {
					bool found = false;
					for(int j = 1; j < ss.count(); j++) {
						const elf::Elf32_Shdr& si = ss[j].info();
						if((si.sh_flags & SHF_ALLOC)
						&& si.sh_addr <= *find && *find < si.sh_addr + si.sh_size) {
							found = true;
							cout << "address " << word_fmt(*find) << " found in section " << ss[j].name() << io::endl;
							display_section(j, ss[j]);
						}
					}
					if(!found)
						cerr << "ERROR: no section containing address " << word_fmt(*find) << "\n";
					return 0;
				}

				// display sections
				else {
					cout << "INDEX TYPE         FLAGS VADDR    SIZE     OFFSET   LINK  NAME\n";
					for(int j = 0; j < ss.count(); j++)
						display_section(j, ss[j]);
				}

				delete f;
			}

		}
		catch(gel::Exception& e) {
			displayHelp();
			cerr << "\nERROR: " << e.message() << io::endl;
			return 1;
		}
		catch(OptionException& e) {
			displayHelp();
			cerr << "	W -- SHF_WRITE\n"
					"	A -- SHF_ALLOC\n"
					"	X -- SHF_EXECINSTR\n";
			cerr << "\nERROR: " << e.message() << io::endl;
			return 1;
		}
		return 0;
	}

protected:

	virtual void process(String arg) {
		args.add(arg);
	}

private:

	const string get_type(elf::Elf32_Word type) {
		static cstring label[] = {
			"SHT_NULL",
			"SHT_PROGBITS",
			"SHT_SYMTAB",
			"SHT_STRTAB",
			"SHT_RELA",
			"SHT_HASH",
			"SHT_DYNAMIC",
			"SHT_NOTE",
			"SHT_NOBITS",
			"SHT_REL",
			"SHT_SHLIB",
			"SHT_DYNSYM"
		};
		static char buf[10];
		if(type >= sizeof(label) / sizeof(const char *))
			return _ << io::pad('0', io::right(io::hex(io::width(8, type))));
		else
			return label[type];
	}

	void display_section(int i, elf::Section& sect) {
		const elf::Elf32_Shdr& s = sect.info();
		cout << io::fmt(i).width(5).right() << ' '
			 << io::fmt(get_type(s.sh_type)).width(12) << "  "
			 << (s.sh_flags & SHF_WRITE ? 'W' : '-')
			 << (s.sh_flags & SHF_ALLOC ? 'A' : '-')
			 << (s.sh_flags & SHF_EXECINSTR ? 'X' : '-') << "  "
			 << word_fmt(s.sh_addr) << ' '
			 << word_fmt(s.sh_size) << ' '
			 << word_fmt(s.sh_offset) << ' '
			 << io::fmt(s.sh_link).width(5).right() << ' '
			 << sect.name() << io::endl;
	}

	Vector<string> args;
	Value<address_t> find;
};

int main(int argc, char **argv) {
	SectCommand cmd;
	return cmd.run(argc, argv);
}

#if 0
/**
 * Command entry point.
 */
int main(int argc, char **argv) {
	const char *path;
	gel_file_t *file;
	int opt, i = 0;
	const char *find = 0;
	
	/* find the section if required */
	if(find) {

		/* convert address */
		uint32_t addr;
		addr = strtoul(find, 0, 16);

		/* perform the search */
		for(i = 0; i < file->sectnum; i++) {

			/* get the section information */
			gel_sect_info_t info;
			gel_sect_t *sect = gel_getsectbyidx(file, i);
			assert(sect);
			if(gel_sect_infos(sect, &info) < 0) {
				fprintf(stderr, "ERROR: %s\n", gel_strerror());
				return 3;
			}

			/* found ? */
			if(addr >= info.vaddr && addr < info.vaddr + info.size) {
		return 1;
	}
}
#endif
