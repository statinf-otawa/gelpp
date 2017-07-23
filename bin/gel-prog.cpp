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
#include <gel++.h>
#include <gel++/elf/File.h>

using namespace elm;
using namespace elm::option;
using namespace gel;

io::IntFormat word_fmt = io::IntFormat().hex().width(8).right().pad('0');

class ProgCommand: public option::Manager {
public:
	ProgCommand(void):
		option::Manager(option::Manager::Make("gel-prog", Version(2, 0, 0))
			.description("Display program header of an ELF exceutable.")
			.copyright("Copyright (c) 2016, université de Toulouse")
			.free_argument("<file path>")
			.help()),
		note(SwitchOption::Make(*this).cmd("-n").description("display the content of the PT_NOTE segments"))
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
				elf::File *f = gel::Manager::openELF(args[i]);

				// display header if not notes
				if(!note)
					cout << "Index "
							"p_type     "
							"p_offset "
							"p_vaddr  "
							"p_paddr  "
							"p_filesz "
							"p_memsz  "
							"p_align  "
							"p_flags\n";

				// display program headers
				Vector<elf::ProgramHeader> phs = f->programHeaders();
				for(int j = 0; j < phs.count(); j++) {
					const elf::Elf32_Phdr &ph = phs[j].info();

					// display notes
					if(note) {
						if(ph.p_type == PT_NOTE)
							print_note(phs[j]);
					}

					// display header itself
					else {
						cout << io::fmt(j).width(5).right() << ' ';
						print_type(ph.p_type);
						cout << word_fmt(ph.p_offset) << ' ';
						cout << word_fmt(ph.p_vaddr) << ' ';
						cout << word_fmt(ph.p_paddr) << ' ';
						cout << word_fmt(ph.p_filesz) << ' ';
						cout << word_fmt(ph.p_memsz) << ' ';
						cout << word_fmt(ph.p_align) << ' ';
						print_flags(ph.p_flags);
					}
				}

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

	/**
	 * Print the flags.
	 * @param flags	Flags to display.
	 */
	void print_flags(const elf::Elf32_Word flags) {
		int one = 0;
		cout << word_fmt(flags) << ' ';
		if(flags & PF_X) {
			one = 1;
			cout << "PF_X";
		}
		if(flags & PF_W) {
			if(one)
				cout << ", ";
			else
				one = 1;
			cout << "PF_W";
		}
		if(flags & PF_R) {
			if(one)
				cout << ", ";
			else
				one = 1;
			cout << "PF_R";
		}
		cout << io::endl;
	}

	/**
	 * Display the program header type.
	 * @param type	Type to display.
	 */
	void print_type(elf::Elf32_Word type) {
		static cstring names[] = {
			"PT_NULL",
			"PT_LOAD",
			"PT_DYNAMIC",
			"PT_INTERP",
			"PT_NOTE",
			"PT_SHLIB",
			"PT_PHDR"
		};
		if(type <= PT_PHDR)
			cout << io::fmt(names[type]).width(10);
		else
			cout << io::fmt(type).width(10);
	}

	/**
	 * Display a note program header.
	 * @param phdr	Program header to process.
	 */
	void print_note(elf::ProgramHeader& ph) throw(gel::Exception) {
		for(elf::NoteIter note(ph); note; note++) {
			cout << "NOTE " << note.name() << ": " << note.type() << io::endl;
			Buffer buf(ph.decoder(), note.desc(), note.descsz());
			cout << buf;
			cout << io::endl;
		}
	}

	Vector<string> args;
	SwitchOption note;
};

int main(int argc, char **argv) {
	return ProgCommand().run(argc, argv);
}
