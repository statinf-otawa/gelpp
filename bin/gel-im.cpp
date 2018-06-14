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

using namespace elm;
using namespace option;
using namespace gel;

class ImageCommand: public option::Manager {
public:
	ImageCommand(void)
	:	Manager(Manager::Make("gel-im", Version(2, 0))
			.copyright("Copyright (c) 2016, University of Toulouse")
			.description("Build the execution images corresponding to the given file as programs.")
			.free_argument("BINARY_FILE")
			.help()),
		no_stack(SwitchOption::Make(*this).cmd("-s").cmd("--no-stack").description("do not initialize any stack")),
		no_content(SwitchOption::Make(*this).cmd("-c").cmd("--no-content").description("do not display the content of blocks"))
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

				// build the image
				Parameter params;
				params.stack_alloc = !no_stack;
				Image *im = f->make(params);

				// display all segments
				for(Image::SegIter seg = im->segments(); seg; seg++) {
					cout << "BLOCK " << seg->name() << " @ " << f->format(seg->base())
						 << " (" << io::hex(seg->size()) << ")";
					if(seg->isWritable())
						cout << " WRITE";
					if(seg->isExecutable())
						cout << " EXEC";

					if(!no_content) {
						int p = 15;
						address_t a = seg->base();
						for(Cursor c(seg->buffer()); c.avail(1); ) {

							// display address
							if(p < 15)
								p++;
							else {
								cout << "\n" << f->format(a + c.offset());
								p = 0;
							}

							// display byte
							t::uint8 b;
							c.read(b);
							cout << ' ' << io::byte(b);
						}
					}
					cout << "\n\n";

				}

				delete im;
				delete f;
			}
			catch(gel::Exception& e) {
				cerr << "ERROR: " << e.message() << io::endl;
				return 1;
			}
		}
		return 0;
	}

protected:

	virtual void process(String arg) {
		args.add(arg);
	}


private:
	SwitchOption no_stack, no_content;
	Vector<string> args;
};

int main(int argc, char **argv) {
	ImageCommand cmd;
	cmd.run(argc, argv);
}
