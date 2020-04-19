/*
 * GEL++ ArchPlugin class implementation
 * Copyright (c) 2018, IRIT- University of Toulouse
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
#include <elm/sys/Plugger.h>
#include <elm/sys/System.h>

namespace gel { namespace elf {

/**
 * @class ArchPlugin
 * Base class of plugin providing a specialization of the ELF format
 * for a particular architecture. To obtain the plugin for a machine,
 * use ArchPlugin::plug().
 *
 * @ingroup elf
 */

/**
 * Compute the paths for the plugger.
 * @return	Plugger paths.
 */
static string paths(void) {
	StringBuffer buf;
	sys::Path lib = sys::System::getUnitPath(&ArchPlugin::null).dirPart();
	buf << lib.toString();
	buf << ":";
	buf << (lib / "gel++").toString();
	return buf.toString();
}

/**
 * Find and plug the plug-in corresponding to the given machine code.
 * @param machine	Machine code in ELF file.
 * @return			Found plug-in or null.
 */
ArchPlugin *ArchPlugin::plug(Elf32_Half machine) {
	static sys::Plugger plugger(ELM_STRING(GEL_ELF_ARCH_HOOK), Version(1, 0 ,0), paths());
	string name = _ << "elf" << machine;
	sys::Plugin *p = plugger.plug(name);
	if(p == nullptr)
		return nullptr;
	else
		return static_cast<ArchPlugin *>(p);
}

/**
 * Build the plugin.
 * @param maker	Maker describing the plug-in.
 */
ArchPlugin::ArchPlugin(const make& maker): sys::Plugin(maker) {
}

/**
 * Output tag name for dynamic entry.
 * @param out	Stream to output to.
 * @param tag	Dynamic entry tag.
 */
void ArchPlugin::outputDynTag(io::Output& out, int tag) {
	out << format(address_32, tag);
}

/**
 * Output the value for dynamic entry.
 * @param out	Stream to output to.
 * @param tag	Dynamic entry tag.
 * @param val	Dynamic entry value.
 */
void ArchPlugin::outputDynValue(io::Output& out, int tag, t::uint64 val) {
	out << format(address_32, val);
}

/**
 * Null plugin.
 */
ArchPlugin ArchPlugin::null(make(ELM_STRING(GEL_ELF_ARCH_HOOK), GEL_ELF_ARCH_VERS));

} }	// gel::elf
