/*
 * GEL++ PE-COFF File implementation
 * Copyright (c) 2020, IRIT- université de Toulouse
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

#include <elm/io/RandomAccessStream.h>
#include <elm/sys/System.h>

#include <gel++/pecoff/File.h>

#define IMARK	//cerr << __FILE__ << ":" << __LINE__ << io::endl

namespace gel { namespace pecoff {

/**
 * @defgroup pecoff
 * The PE-COFF module is used to load executable and shared libraries from
 * the Windows OS.
 *
 * All details about this format can be found here: https://docs.microsoft.com/en-us/windows/win32/debug/pe-format .
 */

static const t::uint32 msdos_offset = 0x3C;
static char magic[4] = { 'P', 'E', '\0', '\0' };

typedef enum {
	IMAGE_FILE_MACHINE_UNKNOWN = 0x0,
	IMAGE_FILE_MACHINE_AM33 = 0x1d3,
	IMAGE_FILE_MACHINE_AMD64 = 0x8664,
	IMAGE_FILE_MACHINE_ARM = 0x1c0,
	IMAGE_FILE_MACHINE_ARMNT = 0x1c4,
	IMAGE_FILE_MACHINE_ARM64 = 0xaa64,
	IMAGE_FILE_MACHINE_EBC = 0xebc,
	IMAGE_FILE_MACHINE_I386 = 0x14c,
	IMAGE_FILE_MACHINE_IA64 = 0x200,
	IMAGE_FILE_MACHINE_M32R = 0x9041,
	IMAGE_FILE_MACHINE_MIPS16 = 0x266,
	IMAGE_FILE_MACHINE_MIPSFPU = 0x366,
	IMAGE_FILE_MACHINE_MIPSFPU16 = 0x466,
	IMAGE_FILE_MACHINE_POWERPC = 0x1f0,
	IMAGE_FILE_MACHINE_POWERPCFP = 0x1f1,
	IMAGE_FILE_MACHINE_R4000 = 0x166,
	IMAGE_FILE_MACHINE_SH3 = 0x1a2,
	IMAGE_FILE_MACHINE_SH3DSP = 0x1a3,
	IMAGE_FILE_MACHINE_SH4 = 0x1a6,
	IMAGE_FILE_MACHINE_SH5 = 0x1a8,
	IMAGE_FILE_MACHINE_THUMB = 0x1c2,
	IMAGE_FILE_MACHINE_WCEMIPSV2 = 0x169,
} machine_type_t;

typedef enum {
	IMAGE_FILE_RELOCS_STRIPPED = 0x0001,
	IMAGE_FILE_EXECUTABLE_IMAGE = 0x0002,
	IMAGE_FILE_LINE_NUMS_STRIPPED = 0x0004,
	IMAGE_FILE_LOCAL_SYMS_STRIPPED = 0x0008,
	IMAGE_FILE_AGGRESSIVE_WS_TRIM = 0x0010,
	IMAGE_FILE_LARGE_ADDRESS_AWARE = 0x0020,
	IMAGE_FILE_BYTES_REVERSED_LO = 0x0080,
	IMAGE_FILE_32BIT_MACHINE = 0x0100,
	IMAGE_FILE_DEBUG_STRIPPED = 0x0200,
	IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400,
	IMAGE_FILE_NET_RUN_FROM_SWAP = 0x0800,
	IMAGE_FILE_SYSTEM = 0x1000,
	IMAGE_FILE_DLL = 0x2000,
	IMAGE_FILE_UP_SYSTEM_ONLY = 0x4000,
	IMAGE_FILE_BYTES_REVERSED_HI = 0x8000
} characteristics_t;

typedef enum {
	IMAGE_SUBSYSTEM_UNKNOWN = 0,
	IMAGE_SUBSYSTEM_NATIVE = 1,
	IMAGE_SUBSYSTEM_WINDOWS_GUI = 2,
	IMAGE_SUBSYSTEM_WINDOWS_CUI = 3,
	IMAGE_SUBSYSTEM_POSIX_CUI = 7,
	IMAGE_SUBSYSTEM_WINDOWS_CE_GUI = 9,
	IMAGE_SUBSYSTEM_EFI_APPLICATION = 10,
	IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER = 11,
	IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER = 12,
	IMAGE_SUBSYSTEM_EFI_ROM = 13,
	IMAGE_SUBSYSTEM_XBOX = 14
} windows_subsystem_t;

typedef enum {
	IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE = 0x0040,
	IMAGE_DLL_CHARACTERISTICS_FORCE_INTEGRITY = 0x0080,
	IMAGE_DLL_CHARACTERISTICS_NX_COMPAT = 0x0100,
	IMAGE_DLL_CHARACTERISTICS_NO_ISOLATION = 0x0200,
	IMAGE_DLL_CHARACTERISTICS_NO_SEH = 0x0400,
	IMAGE_DLL_CHARACTERISTICS_NO_BIND = 0x0800,
	IMAGE_DLL_CHARACTERISTICS_WDM_DRIVER = 0x2000,
	IMAGE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE = 0x8000
} dll_characteristics_t;

typedef enum {
	IMAGE_SCN_TYPE_NO_PAD = 0x00000008,
	IMAGE_SCN_CNT_CODE = 0x00000020,
	IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040,
	IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080,
	IMAGE_SCN_LNK_OTHER = 0x00000100,
	IMAGE_SCN_LNK_INFO = 0x00000200,
	IMAGE_SCN_LNK_REMOVE = 0x00000800,
	IMAGE_SCN_LNK_COMDAT = 0x00001000,
	IMAGE_SCN_GPREL = 0x00008000,
	IMAGE_SCN_MEM_PURGEABLE = 0x00020000,
	IMAGE_SCN_MEM_16BIT = 0x00020000,
	IMAGE_SCN_MEM_LOCKED = 0x00040000,
	IMAGE_SCN_MEM_PRELOAD = 0x00080000,
	IMAGE_SCN_ALIGN_1BYTES = 0x00100000,
	IMAGE_SCN_ALIGN_2BYTES = 0x00200000,
	IMAGE_SCN_ALIGN_4BYTES = 0x00300000,
	IMAGE_SCN_ALIGN_8BYTES = 0x00400000,
	IMAGE_SCN_ALIGN_16BYTES = 0x00500000,
	IMAGE_SCN_ALIGN_32BYTES = 0x00600000,
	IMAGE_SCN_ALIGN_64BYTES = 0x00700000,
	IMAGE_SCN_ALIGN_128BYTES = 0x00800000,
	IMAGE_SCN_ALIGN_256BYTES = 0x00900000,
	IMAGE_SCN_ALIGN_512BYTES = 0x00A00000,
	IMAGE_SCN_ALIGN_1024BYTES = 0x00B00000,
	IMAGE_SCN_ALIGN_2048BYTES = 0x00C00000,
	IMAGE_SCN_ALIGN_4096BYTES = 0x00D00000,
	IMAGE_SCN_ALIGN_8192BYTES = 0x00E00000,
	IMAGE_SCN_LNK_NRELOC_OVFL = 0x01000000,
	IMAGE_SCN_MEM_DISCARDABLE = 0x02000000,
	IMAGE_SCN_MEM_NOT_CACHED = 0x04000000,
	IMAGE_SCN_MEM_NOT_PAGED = 0x08000000,
	IMAGE_SCN_MEM_SHARED = 0x10000000,
	IMAGE_SCN_MEM_EXECUTE = 0x20000000,
	IMAGE_SCN_MEM_READ = 0x40000000,
	IMAGE_SCN_MEM_WRITE = 0x80000000
} characteristics_flags_t;

typedef enum {
	IMAGE_SYM_UNDEFINED = 0,
	IMAGE_SYM_ABSOLUTE = -1,
	IMAGE_SYM_DEBUG = -2
} section_number_t;

typedef enum {
	IMAGE_SYM_TYPE_NULL = 0,
	IMAGE_SYM_TYPE_VOID = 1,
	IMAGE_SYM_TYPE_CHAR = 2,
	IMAGE_SYM_TYPE_SHORT = 3,
	IMAGE_SYM_TYPE_INT = 4,
	IMAGE_SYM_TYPE_LONG = 5,
	IMAGE_SYM_TYPE_FLOAT = 6,
	IMAGE_SYM_TYPE_DOUBLE = 7,
	IMAGE_SYM_TYPE_STRUCT = 8,
	IMAGE_SYM_TYPE_UNION = 9,
	IMAGE_SYM_TYPE_ENUM = 10,
	IMAGE_SYM_TYPE_MOE = 11,
	IMAGE_SYM_TYPE_BYTE = 12,
	IMAGE_SYM_TYPE_WORD = 13,
	IMAGE_SYM_TYPE_UINT = 14,
	IMAGE_SYM_TYPE_DWORD = 15
} lsb_type_t;

typedef enum {
	IMAGE_SYM_DTYPE_NULL = 0,
	IMAGE_SYM_DTYPE_POINTER = 1,
	IMAGE_SYM_DTYPE_FUNCTION = 2,
	IMAGE_SYM_DTYPE_ARRAY = 3
} msb_type_t;

typedef enum {
	IMAGE_SYM_CLASS_END_OF_FUNCTION = 0xFF,
	IMAGE_SYM_CLASS_NULL = 0,
	IMAGE_SYM_CLASS_AUTOMATIC = 1,
	IMAGE_SYM_CLASS_EXTERNAL = 2,
	IMAGE_SYM_CLASS_STATIC = 3,
	IMAGE_SYM_CLASS_REGISTER = 4,
	IMAGE_SYM_CLASS_EXTERNAL_DEF = 5,
	IMAGE_SYM_CLASS_LABEL = 6,
	IMAGE_SYM_CLASS_UNDEFINED_LABEL = 7,
	IMAGE_SYM_CLASS_MEMBER_OF_STRUCT = 8,
	IMAGE_SYM_CLASS_ARGUMENT = 9,
	IMAGE_SYM_CLASS_STRUCT_TAG = 10,
	IMAGE_SYM_CLASS_MEMBER_OF_UNION = 11,
	IMAGE_SYM_CLASS_UNION_TAG = 12,
	IMAGE_SYM_CLASS_TYPE_DEFINITION = 13,
	IMAGE_SYM_CLASS_UNDEFINED_STATIC = 14,
	IMAGE_SYM_CLASS_ENUM_TAG = 15,
	IMAGE_SYM_CLASS_MEMBER_OF_ENUM = 16,
	IMAGE_SYM_CLASS_REGISTER_PARAM = 17,
	IMAGE_SYM_CLASS_BIT_FIELD = 18,
	IMAGE_SYM_CLASS_BLOCK = 100,
	IMAGE_SYM_CLASS_FUNCTION = 101,
	IMAGE_SYM_CLASS_END_OF_STRUCT = 102,
	IMAGE_SYM_CLASS_FILE = 103,
	IMAGE_SYM_CLASS_SECTION = 104,
	IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105,
	IMAGE_SYM_CLASS_CLR_TOKEN = 107
} storage_class_t;


static inline void swap(t::uint64& swap) {
#	ifdef ENDIANNESS_BIG
		swap =	((swap & 0x00000000000000ffULL) << 56)
			 |	((swap & 0x000000000000ff00ULL) << 40)
			 |	((swap & 0x0000000000ff0000ULL) << 16)
			 |	((swap & 0x00000000ff000000ULL) << 8);
			 |  ((swap & 0x000000ff00000000ULL) >> 8)
			 |	((swap & 0x0000ff0000000000ULL) >> 16)
			 |	((swap & 0x00ff000000000000ULL) >> 40)
			 |	((swap & 0xff00000000000000ULL) >> 56);
#	endif
}

static inline void swap(t::uint32& swap) {
#	ifdef ENDIANNESS_BIG
		swap =	((swap & 0x000000ff) << 24)
			 |	((swap & 0x0000ff00) << 8)
			 |	((swap & 0x00ff0000) >> 8)
			 |	((swap & 0xff000000) >> 24);
#	endif
}

static inline void swap(t::uint16& swap) {
#	ifdef ENDIANNESS_BIG
		swap =	((swap & 0x00ff) << 8)
			 |	((swap & 0xff00) >> 8);
#	endif
}


/**
 * Escape a text as a string.
 * @param buf	Buffer containing the text.
 * @param size	Size of th text.
 * @return		Text with escaped characers.
 */
static string escape(const char *buf, int size) {
	StringBuffer out;
	for(int i = 0; i < size; i++) {
		t::uint8 c = buf[i];
		if(c >= 32 && c < 128)
			out << char(c);
		else
			switch(c) {
			case '\0': out << "\\0"; break;
			case '\n': out << "\\n"; break;
			case '\t': out << "\\t"; break;
			case '\r': out << "\\r"; break;
			default:   out << "\\x" << io::hex(c); break;
			}
	}
	return out.toString();
}


/**
 * @class File
 * Representation of a binary file in PE-COFF format.
 * @ingroup pecoff
 */

/**
 * Open the given PE-COFF file.
 * @param manager	Current GEL manager.
 * @param path		Path of the file to open.
 * @throw Exception	If there is a format error.
 */
File::File(Manager& manager, sys::Path path):
	gel::File(manager, path),
	stream(nullptr),
	rvas(nullptr),
	hds(nullptr),
	str_tab(nullptr),
	str_size(0)
{
	try {
		stream = sys::System::openRandomFile(path);

		// read the offset
		t::uint32 offset;
		move(msdos_offset);
		read(&offset, sizeof(offset));
		swap(offset);
		IMARK;

		// read the magic
		char m[4];
		move(offset);
		read(m, sizeof(m));
		if(m[0] != magic[0] || m[1] != magic[1] || m[2] != magic[2] || m[3] != magic[3])
			raise(_ << "not a PECOFF file: magic=" << escape(m, sizeof(m)));
		IMARK;

		// read the PE header
		read(&_header, sizeof(_header));
		swap(_header.machine);
		swap(_header.number_of_sections);
		swap(_header.time_date_stamp);
		swap(_header.pointer_to_symbol_table);
		swap(_header.number_of_symbols);
		swap(_header.size_of_optional_header);
		swap(_header.characteristics);
		IMARK;

		// read the type
		t::uint16 t;
		read(&t, sizeof(t));
		swap(t);
		if(t != PE32 && t != PE32P)
			raise(_ << "unknown PE type " << io::hex(t));
		IMARK;

		// compute size of standard and windows-specific files
		int standard_fields_size;
		int windows_specific_fields_size;
		switch(t) {
		case PE32:
			standard_fields_size = sizeof(standard_fields_t);
			windows_specific_fields_size = sizeof(pe32_windows_specific_fields_t);
			break;
		case PE32P:
			standard_fields_size = sizeof(standard_fields_t) - sizeof(t::uint32);
			windows_specific_fields_size = sizeof(windows_specific_fields_t);
			break;
		}
		array::clear(&standard_fields, 1);
		if(_header.size_of_optional_header < standard_fields_size + windows_specific_fields_size + sizeof(t::uint16))
			raise("format error (3)");
		IMARK;

		// read standard field
		read(&standard_fields.major_linker_version, standard_fields_size - sizeof(standard_fields.magic));
		standard_fields.magic = t;
		swap(standard_fields.size_of_code);
		swap(standard_fields.size_of_initialized_data);
		swap(standard_fields.address_of_entry_point);
		swap(standard_fields.base_of_code);
		swap(standard_fields.base_of_data);
		IMARK;

		// read the windows specific field
		if(t == PE32) {
			static pe32_windows_specific_fields_t f;
			read(&f, sizeof(f));
			swap(f.image_base);
			swap(f.section_alignment);
			swap(f.file_alignment);
			swap(f.major_operating_system_version);
			swap(f.minor_operating_system_version);
			swap(f.major_image_version);
			swap(f.minor_image_version);
			swap(f.major_subsystem_version);
			swap(f.minor_subsystem_version);
			swap(f.win32_version_value);
			swap(f.size_of_image);
			swap(f.size_of_headers);
			swap(f.checksum);
			swap(f.subsystem);
			swap(f.dll_characteristics);
			swap(f.size_of_stack_reserve);
			swap(f.size_of_stack_commit);
			swap(f.size_of_heap_reserve);
			swap(f.size_of_heap_commit);
			swap(f.loader_flags);
			swap(f.number_of_rva_and_sizes);
			windows_specific_fields.image_base = f.image_base;
			windows_specific_fields.section_alignment = f.section_alignment;
			windows_specific_fields.file_alignment = f.file_alignment;
			windows_specific_fields.major_operating_system_version = f.major_operating_system_version;
			windows_specific_fields.minor_operating_system_version = f.minor_operating_system_version;
			windows_specific_fields.major_image_version = f.major_image_version;
			windows_specific_fields.minor_image_version = f.minor_image_version;
			windows_specific_fields.major_subsystem_version = f.major_subsystem_version;
			windows_specific_fields.minor_subsystem_version = f.minor_subsystem_version;
			windows_specific_fields.win32_version_value = f.win32_version_value;
			windows_specific_fields.size_of_image = f.size_of_image;
			windows_specific_fields.size_of_headers = f.size_of_headers;
			windows_specific_fields.checksum = f.checksum;
			windows_specific_fields.subsystem = f.subsystem;
			windows_specific_fields.dll_characteristics = f.dll_characteristics;
			windows_specific_fields.size_of_stack_reserve = f.size_of_stack_reserve;
			windows_specific_fields.size_of_stack_commit = f.size_of_stack_commit;
			windows_specific_fields.size_of_heap_reserve = f.size_of_heap_reserve;
			windows_specific_fields.size_of_heap_commit = f.size_of_heap_commit;
			windows_specific_fields.loader_flags = f.loader_flags;
			windows_specific_fields.number_of_rva_and_sizes = f.number_of_rva_and_sizes;
		}
		else {
			read(&windows_specific_fields, sizeof(windows_specific_fields));
			swap(windows_specific_fields.image_base);
			swap(windows_specific_fields.section_alignment);
			swap(windows_specific_fields.file_alignment);
			swap(windows_specific_fields.major_operating_system_version);
			swap(windows_specific_fields.minor_operating_system_version);
			swap(windows_specific_fields.major_image_version);
			swap(windows_specific_fields.minor_image_version);
			swap(windows_specific_fields.major_subsystem_version);
			swap(windows_specific_fields.minor_subsystem_version);
			swap(windows_specific_fields.win32_version_value);
			swap(windows_specific_fields.size_of_image);
			swap(windows_specific_fields.size_of_headers);
			swap(windows_specific_fields.checksum);
			swap(windows_specific_fields.subsystem);
			swap(windows_specific_fields.dll_characteristics);
			swap(windows_specific_fields.size_of_stack_reserve);
			swap(windows_specific_fields.size_of_stack_commit);
			swap(windows_specific_fields.size_of_heap_reserve);
			swap(windows_specific_fields.size_of_heap_commit);
			swap(windows_specific_fields.loader_flags);
			swap(windows_specific_fields.number_of_rva_and_sizes);
		}
		IMARK;

		// read RVAs
		t::uint32 rva_size = _header.size_of_optional_header - standard_fields_size - windows_specific_fields_size;
		if(rva_size / sizeof(image_data_directory_t) != windows_specific_fields.number_of_rva_and_sizes)
			raise("inconsistency in RVA size");
		rvas = new image_data_directory_t[windows_specific_fields.number_of_rva_and_sizes];
		read(rvas, sizeof(image_data_directory_t) * windows_specific_fields.number_of_rva_and_sizes);
		IMARK;

		// read the section headers
		hds = new section_header_t[_header.number_of_sections];
		read(hds, sizeof(section_header_t) * _header.number_of_sections);
		for(int i = 0; i < _header.number_of_sections; i++)
			sects.add(new Section(this, &hds[i]));
		IMARK;
	}
	catch(sys::SystemException& e) {
		raise(e.message());
	}
}

///
File::~File() {
	if(stream != nullptr)
		delete stream;
	if(rvas != nullptr)
		delete [] rvas;
	if(hds != nullptr)
		delete [] hds;
	deleteAll(sects);
}


/**
 * Raise an exception with the given message.
 * @param msg		Message to raise exception with.
 * @throw Exception	Raised exception.
 */
void File::raise(const string& msg) {
	throw Exception(msg);
}


/**
 * Read from the file and takes into account carefully
 * the errors.
 * @param buf		Buffer to write to.
 * @param len		Length of the buffer.
 */
void File::read(void *buf, int len) {
	int r = stream->read(buf, len);
	if(r < 0)
		raise(_ << "IO error: " << stream->io::InStream::lastErrorMessage());
	else if(r != len)
		raise(_ << "format error, requested " << len << " bytes, got " << r << " bytes");
}


/**
 * Move the stream to the given location.
 * @param offset	Absolute offset in the file.
 * @throw Exception	If there is an IO error.
 */
void File::move(offset_t offset) {
	if(!stream->moveTo(offset))
		throw Exception(_ << "IO error: " << stream->io::InStream::lastErrorMessage());
}


///
File::type_t File::type() {
	if((_header.characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) != 0)
		return program;
	else if((_header.characteristics & IMAGE_FILE_DLL) != 0)
		return library;
	else
		return no_type;
}

///
bool File::isBigEndian() {
	return false;
}

///
address_type_t File::addressType() {
	if((_header.characteristics & IMAGE_FILE_32BIT_MACHINE) != 0)
		return address_32;
	else
		return address_16;
}

///
address_t File::entry() {
	return standard_fields.address_of_entry_point;
}

///
int File::count() {
	return sects.count();
}

///
Segment *File::segment(int i) {
	return sects[i];
}

///
Image *File::make(const Parameter& params) {
	return nullptr;
}

///
const SymbolTable& File::symbols() {

}


/**
 * @class Section
 * Segment from the GEL++ portable interface corresponds to sections of the
 * PE-COFF format.
 * @ingroup pecoff
 */

///
Section::Section(File *file, const section_header_t *header):
	hd(header), pec(file)
	{ }

/**
 * @fn const section_header_t& Section::header() const;
 * Get the section header itself.
 * @return	Section header.
 */

///
cstring Section::name(void) {
	return hd->name;
}

address_t Section::baseAddress(void) {
	return hd->virtual_address;
}

address_t Section::loadAddress(void) {
	return hd->virtual_size;
}

size_t Section::size(void) {
	return hd->virtual_size;
}

size_t Section::alignment(void) {
	int msk = (hd->characteristics & IMAGE_SCN_ALIGN_1BYTES >> 20) & 0xf;
	return 1 << (msk - 1);
}

bool Section::isExecutable(void) {
	return (hd->characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
}

bool Section::isWritable(void) {
	return (hd->characteristics & IMAGE_SCN_MEM_WRITE) != 0;
}

bool Section::hasContent(void) {
}

Buffer Section::buffer(void) {
}

} } // gel::pecoff

