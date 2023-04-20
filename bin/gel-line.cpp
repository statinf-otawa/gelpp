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

#include <math.h>
#include <elm/options.h>
#include <elm/data/FragTable.h>
#include <elm/data/HashMap.h>

#include <gel++/elf/DebugLine.h>

using namespace elm;
using namespace elm::option;
using namespace gel;

class LineCommand: public option::Manager {
public:
	LineCommand(void):
		option::Manager(option::Manager::Make("gel-line", Version(1, 0, 0))
			.description("Display debugging source line information for a binary file.")
			.copyright("Copyright (c) 2020, université de Toulouse")
			.free_argument("<file path>")
			.help()),
		list_files(option::Switch::Make(*this).cmd("-l").help("display file/line to code [default]")),
		list_code(option::Switch::Make(*this).cmd("-c").help("display code to file/line"))
	{ }

	void run() override {
		for(int i = 0; i < args.count(); i++)
			try {
				auto f = gel::Manager::open(args[i]);
				auto dl = f->debugLines();
				ASSERT(dl != nullptr);
				
				// set the address format
				if(f->addressType() == gel::address_32)
					addr_fmt.width(8);
				else
					addr_fmt.width(16);

				// perform the action
				if(list_code)
					listCode(dl);
				else
					listFiles(dl);

				delete f;
			}
			catch(gel::Exception& e) {
				cerr << "ERROR: when opening " << args[i] << ": " << e.message() << io::endl;
			}
	}

protected:
	virtual void process(String arg) {
		args.add(arg);
	}

private:

	void listFiles(DebugLine *dl) {
		for(auto file: dl->files()) {

			// collect addresses
			HashMap<int, List<Pair<address_t, address_t> > *> addrs;
			int min = type_info<int>::max, max = 0;
			for(auto cu: file->units()) {
				const auto& lines = cu->lines();
				for(int i = 0; i < lines.count() - 1; i++)
					if(file == lines[i].file()) {
						auto l = addrs.get(lines[i].line(), nullptr);
						if(l == nullptr) {
							l = new List<Pair<address_t, address_t> >();
							addrs.put(lines[i].line(), l);
						}
						l->add(pair(lines[i].addr(), lines[i + 1].addr()));
						if(lines[i].line() < min)
							min = lines[i].line();
						else if(lines[i].line() > max)
							max = lines[i].line();
					}
			}

			// display the addresses
			if(min <= max) {
				auto f = io::IntFormat().right().pad(' ').width(log10(max) + 1);
				for(int i = min; i <= max; i++)
					if(addrs.hasKey(i)) {
						auto l = addrs.get(i, nullptr);
						if(l != nullptr)
							for(auto a: *l)
								cout << file->path() << ":" << f(i)
									 << "\t" << addr_fmt(a.fst)
									 << "-"  << addr_fmt(a.snd) << io::endl;
					}
			}

			// clean all
			for(auto l: addrs)
				delete l;
		}
	}

	void listCode(DebugLine *dl) {
		for(auto cu: dl->units()) {
			const auto& lines = cu->lines();
			for(int i = 0; i < lines.count() - 1; i++)
				cout << addr_fmt(lines[i].addr()) << "\t" << lines[i].file()->path() << ":" << lines[i].line() << io::endl;
		}
	}

	io::IntFormat addr_fmt = io::IntFormat().pad('0').hex().right();
	option::Switch list_files, list_code;
	Vector<string> args;
};

ELM_RUN(LineCommand)
