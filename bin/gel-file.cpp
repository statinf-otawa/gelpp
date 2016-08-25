/*
 * gel-file command
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

#include <elm/assert.h>
#include <elm/options.h>
#include <elm/io.h>
#include <gel++.h>
#include <gel++/elf/File.h>

using namespace elm;
using namespace option;
using namespace gel;

class FileCommand: public option::Manager {
public:
	FileCommand(void)
	:	Manager(Manager::Make("gel-file", Version(2, 0))
			.copyright("Copyright (c) 2016	, université de Toulouse")
			.description("Provide basic information about a binary file")
			.free_argument("BINARY_FILE")
			.help()),
		show_all(SwitchOption::Make(*this).cmd("-a").description("display all information")),
		show_elf(SwitchOption::Make(*this).cmd("-e").description("display ELF information (if any)"))
	{
	}

	int run(int argc, char **argv) {

		// parse arguments
		try {
			parse(argc, argv);
			if(!args)
				throw OptionException("at least one executable file required!");
		}
		catch(OptionException& e) {
			displayHelp();
			cerr << "\nERROR: " << e.message() << io::endl;
			return 1;
		}

		// process files
		for(int i = 0; i < args.count(); i++) {
			try {
				File *f = gel::Manager::open(args[i]);

				// common part
				if(!show_elf || show_all) {
		 			cout << "file name = " << f->path() << io::endl;
					cout << "type = " << f->type() << io::endl;
					cout << "entry = " << f->format(f->entry()) << io::endl;
				}

				// ELF specialization
				elf::File *ef = f->toELF();
				if(ef && (show_elf || show_all)) {
					if(show_all)
						cout << "\nELF INFORMATION\n";
					io::IntFormat f16 = io::pad('0', io::right(io::width(4, 0)));
					const elf::Elf32_Ehdr& info = ef->info();
					cout << "type = " << get_type(info.e_type) << " (" << f16(info.e_type) << ")\n";
					cout << "machine = " << get_machine(info.e_machine) << " (" << f16(info.e_machine) << ")\n";
					cout << "version = " << info.e_version << io::endl;
					cout << "identification\n";
					display_block(info.e_ident, EI_NIDENT, 4);
					cout << "ident[EI_CLASS] = " << get_class(info.e_ident[EI_CLASS]) << " (" << info.e_ident[EI_CLASS] << ")\n";
					cout << "ident[EI_DATA] = " << get_data(info.e_ident[EI_DATA]) << " (" << info.e_ident[EI_DATA] << ")\n";
					cout << "ident[EI_OSABI] = " << info.e_ident[EI_OSABI] << "\n";
				}

				delete f;
			}
			catch(gel::Exception& e) {
				cerr << "ERROR: " << e.message() << io::endl;
			}
		}
	}

protected:

	virtual void process(String arg) {
		args.add(arg);
	}


private:

	const char *get_type(t::uint16 type) {
		static const char *names[] = {
			"ET_NONE",
			"ET_REL",
			"ET_EXEC",
			"ET_DYN",
			"ET_CORE"
		};
		if(type > sizeof(names) / sizeof(void *))
			return "UNKNOWN";
		else
			return names[type];
	}

	const char *get_machine(t::uint16 machine) {
		static struct {
			t::uint16 code;
			const char *name;
		} machines[] = {
			{ 0, "EM_NONE" },
			{ 1, "EM_M32" },
			{ 2, "EM_SPARC" },
			{ 3, "EM_386" },
			{ 4, "EM_68K" },
			{ 5, "EM_88K" },
			{ 6, "reserved" },
			{ 7, "EM_860" },
			{ 8, "EM_MIPS" },
			{ 9, "reserved" },
			{ 10, "EM_MIPS_RS4_BE" },
			{ 15, "EM_PARISC" },
			{ 17, "EM_VPP500" },
			{ 18, "EM_SPARC32PLUS" },
			{ 19, "EM_960" },
			{ 20, "EM_PPC" },
			{ 36, "EM_V800" },
			{ 37, "EM_FR20" },
			{ 38, "EM_RH32" },
			{ 39, "EM_RCE" },
			{ 40, "EM_ARM" },
			{ 41, "EM_ALPHA" },
			{ 42, "EM_SH" },
			{ 43, "EM_SPARCV9" },
			{ 44, "EM_TRICORE" },
			{ 45, "EM_ARC" },
			{ 46, "EM_H8_300" },
			{ 47, "EM_H8_300H" },
			{ 48, "EM_H8S" },
			{ 49, "EM_H8_500" },
			{ 50, "EM_IA_64" },
			{ 51, "EM_MIPS_X" },
			{ 52, "EM_COLDFIRE" },
			{ 53, "EM_68HC12" },
			{ 54, "EM_MMA" },
			{ 55, "EM_PCP" },
			{ 56, "EM_NCPU" },
			{ 57, "EM_NDR1" },
			{ 58, "EM_STARCORE" },
			{ 59, "EM_ME16" },
			{ 60, "EM_ST100" },
			{ 61, "EM_TINYJ" },
			{ 66, "EM_FX66" },
			{ 67, "EM_ST9PLUS" },
			{ 68, "EM_ST7" },
			{ 69, "EM_68HC16" },
			{ 70, "EM_68HC11" },
			{ 71, "EM_68HC08" },
			{ 72, "EM_68HC05" },
			{ 73, "EM_SVX" },
			{ 74, "EM_ST19" },
			{ 75, "EM_VAX" },
			{ 76, "EM_CRIS" },
			{ 77, "EM_JAVELIN" },
			{ 78, "EM_FIREPATH" },
			{ 79, "EM_ZSP" },
			{ 80, "EM_MMIX" },
			{ 81, "EM_HUANY" },
			{ 82, "EM_PRISM" },
			{ 0, 0 }
		};
		int i;

		for(i = 0; machines[i].name; i++)
			if(machines[i].code == machine)
				return machines[i].name;
		return "unknown";
	}


	void display_block(const t::uint8 *bytes, int size, int width) {
		int i, j;
		for(i = 0; i < size; i += width) {
			for(j = 0; j < width && i + j < size; j++)
				cout << io::hex(io::width(2, io::pad('0', bytes[i + j])));
			cout << '\t';
			for(j = 0; (j < width) && (i + j < size); j++) {
				if(bytes[i + j] >= ' ' && bytes[i + j] < 127)
					cout << static_cast<char>(bytes[i + j]);
				else
					cout << '.';
			}
			cout << io::endl;
		}
	}

	const char *get_class(char clazz) {
		switch(clazz) {
		case ELFCLASSNONE: return "ELFCLASSNONE";
		case ELFCLASS32: return "ELFCLASS32";
		case ELFCLASS64: return "EFLCLASS64";
		default: return "unknown";
		}
	}

	const char *get_data(char data) {
		switch(data) {
		case ELFDATANONE: return "ELFDATANONE";
		case ELFDATA2LSB: return "ELFDATA2LSB";
		case ELFDATA2MSB: return "ELFDATA2MSB";
		default: return "unknown";
		}
	}

	SwitchOption show_all, show_elf;
	genstruct::Vector<string> args;
};


int main(int argc, char **argv) {
	FileCommand cmd;
	cmd.run(argc, argv);
}
