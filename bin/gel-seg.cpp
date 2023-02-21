/*
 * gel-seg command
 * Copyright (c) 2023, IRIT- université de Toulouse
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

using namespace elm;
using namespace elm::option;
using namespace gel;

io::IntFormat word_fmt = io::IntFormat().hex().width(8).right().pad('0');

class SegCommand: public option::Manager {
public:
	SegCommand(void):
		Manager(Manager::Make("gel-seg", Version(1, 0, 0))
			.description("list segments of an executable")
			.copyright("copyright (c) 2023, université de Toulouse")
			.free_argument("<file path>")
			.help()),
		find(Value<address_t>::Make(*this).cmd("-f").description("find the section containing this address").argDescription("ADDRESS").def(0))
	{ }

	void processSegment(int i, Segment *s) {
		cout << io::fmt(i).width(5).right() << ' '
			<< (s->isWritable() ? 'W' : '-')
			<< (s->hasContent() ? 'A' : '-')
			<< (s->isExecutable() ? 'X' : '-') << "   "
			<< word_fmt(s->baseAddress()) << ' '
			<< word_fmt(s->size()) << ' '
			<< s->name() << io::endl;
	}

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
				auto f = gel::Manager::open(args[i]);
				cout << "INDEX FLAGS VADDR    SIZE     NAME\n";
				for(int i = 0; i < f->count(); i++)
					processSegment(i, f->segment(i));
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

	Vector<string> args;
	Value<address_t> find;
};

int main(int argc, char **argv) {
	SegCommand().run(argc, argv);
}

