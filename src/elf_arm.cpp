/*
 * arm architecture plugin
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

#include <gel++/elf/ArchPlugin.h>

namespace gel { namespace elf {

class ArmPlugin: public ArchPlugin {
public:
	ArmPlugin(void): ArchPlugin(make("arm", GEL_ELF_ARCH_VERS)) { }

	void outputDynTag(io::Output& out, int tag) override {
		switch(tag) {
		case 0x7000000:	out << "DT_ARM_RESERVED1"; break;
		case 0x7000001:	out << "DT_ARM_SYMTABSZ"; break;
		case 0x7000002:	out << "DT_ARM_PREEMPTMAP"; break;
		case 0x7000003: out << "DT_ARM_RESERVED2"; break;
		default:		ArchPlugin::outputDynTag(out, tag); break;
		}
	}

	void outputDynValue(io::Output& out, int tag, t::uint64 val) override {
		switch(tag) {
		case 0x7000000:
			break;
		case 0x7000001:
			out << val;
			break;
		case 0x7000002:
			out << format(address_32, val);
			break;
		case 0x7000003:
			break;
		default:
			ArchPlugin::outputDynValue(out, tag, val); break;
		}
	}

};

} };	// gel::elf

static gel::elf::ArmPlugin arm_plugin;
ELM_PLUGIN(arm_plugin, GEL_ELF_ARCH_HOOK);

