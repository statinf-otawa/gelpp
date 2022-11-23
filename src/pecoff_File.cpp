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

#define IMARK	cerr << __FILE__ << ":" << __LINE__ << io::endl

namespace gel { namespace pecoff {

/**
 * @defgroup pecoff
 * The PE-COFF module is used to load executable and shared libraries from
 * the Windows OS.
 *
 * All details about this format can be found here:
 * * https://docs.microsoft.com/en-us/windows/win32/debug/pe-format
 * * https://en.wikipedia.org/wiki/Portable_Executable
 */

static const t::uint32 msdos_offset = 0x3C;
static char magic[4] = { 'P', 'E', '\0', '\0' };

/* something uglier than that?
 * 	offset			size			
 *	PE32	PE32+	PE32	PE32+	part
 * 	0		0		28		24		standard coff fields
 * 	28		24		68		88		windows specific fields
 *	96		112		variable		data directories
 */


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
	IMAGE_FILE_MACHINE_LOONGARCH32 = 0x6232,
	IMAGE_FILE_MACHINE_M32R = 0x9041,
	IMAGE_FILE_MACHINE_MIPS16 = 0x266,
	IMAGE_FILE_MACHINE_MIPSFPU = 0x366,
	IMAGE_FILE_MACHINE_MIPSFPU16 = 0x466,
	IMAGE_FILE_MACHINE_POWERPC = 0x1f0,
	IMAGE_FILE_MACHINE_POWERPCFP = 0x1f1,
	IMAGE_FILE_MACHINE_R4000 = 0x166,
	IMAGE_FILE_MACHINE_RISCV32 = 0x5032,
	IMAGE_FILE_MACHINE_RISCV64 = 0x5064,
	IMAGE_FILE_MACHINE_RISCV128 = 0x5128,
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
 * Test if the given magic number matches PE-COFF.
 * param magic	Magic number to be tested.
 * @return		True if magic number is PE-COFF, false else.
 */
bool File::matches(t::uint8 magic[4]) {
	return magic[0] == 0x4D && magic[1] == 0x5A;
}


/**
 * Open the given PE-COFF file.
 * @param manager	Current GEL manager.
 * @param path		Path of the file to open.
 * @throw Exception	If there is a format error.
 */
File::File(Manager& manager, sys::Path path, io::RandomAccessStream *stream_):
	gel::File(manager, path),
	stream(stream_),
	_data_directories(nullptr),
	_section_table(nullptr),
	_symbol_table(nullptr),
	_string_table(nullptr),
	_string_table_size(0)
{
	try {

		// read the offset
		t::uint32 offset;
		move(msdos_offset);
		read(&offset, sizeof(offset));
		swap(offset);

		// get the header
		move(offset);
		read(&_coff_header, sizeof(_coff_header));
		if(_coff_header.signature[0] != magic[0]
		|| _coff_header.signature[1] != magic[1]
		|| _coff_header.signature[2] != magic[2]
		|| _coff_header.signature[3] != magic[3])
			raise(_ << "not a PECOFF file: magic="
				<< escape(
					_coff_header.signature,
					sizeof(_coff_header.signature)));
		swap(_coff_header.machine);
		swap(_coff_header.number_of_sections);
		swap(_coff_header.time_date_stamp);
		swap(_coff_header.number_of_symbols);
		swap(_coff_header.size_of_optional_header);
		swap(_coff_header.characteristics);
		/*cerr << "DEBUG: _coff_header\n"
			 << io::hex(_coff_header.machine) << io::endl
			 << io::hex(_coff_header.number_of_sections) << io::endl
			 << io::hex(_coff_header.time_date_stamp) << io::endl
			 << io::hex(_coff_header.pointer_to_symbol_table) << io::endl
			 << io::hex(_coff_header.number_of_symbols) << io::endl
			 << io::hex(_coff_header.size_of_optional_header) << io::endl
			 << io::hex(_coff_header.characteristics) << io::endl;*/

		// read standard COFF fields
		read(
			&_standard_coff_fields,
			sizeof(_standard_coff_fields));
		swap(_standard_coff_fields.magic);
		switch(_standard_coff_fields.magic) {
		case PE32:
			break;
		case PE32P:
			break;
		default:
			raise(_
				<< "unknown PE type "
				<< io::hex(_standard_coff_fields.magic));
			break;
		}
		swap(_standard_coff_fields.size_of_code);
		swap(_standard_coff_fields.size_of_initialized_data);
		swap(_standard_coff_fields.size_of_unitialized_data);
		swap(_standard_coff_fields.address_of_entry_point);
		swap(_standard_coff_fields.base_of_code);
		swap(_standard_coff_fields.base_of_data);
		/*cerr << "DEBUG: _standard_coff_fields"
				<< io::hex(_standard_coff_fields.magic) << io::endl
				<< io::hex(_standard_coff_fields.major_linker_version) << io::endl
				<< io::hex(_standard_coff_fields.minor_linker_version) << io::endl
				<< io::hex(_standard_coff_fields.size_of_code) << io::endl
				<< io::hex(_standard_coff_fields.size_of_initialized_data) << io::endl
				<< io::hex(_standard_coff_fields.size_of_unitialized_data)  << io::endl
				<< io::hex(_standard_coff_fields.address_of_entry_point) << io::endl
				<< io::hex(_standard_coff_fields.base_of_code) << io::endl
				<< io::hex(_standard_coff_fields.base_of_data) << io::endl;
		*/

		// read windows specific field
		if(_standard_coff_fields.magic == PE32P) {
			stream->moveBackward(sizeof(t::uint32));
			read(
				&_windows_specific_fields,
				sizeof(_windows_specific_fields));
			swap(_windows_specific_fields.image_base);
			swap(_windows_specific_fields.size_of_stack_reserve);
			swap(_windows_specific_fields.size_of_stack_commit);
			swap(_windows_specific_fields.size_of_heap_reserve);
			swap(_windows_specific_fields.size_of_heap_commit);
		}
		else {
			windows_specific_fields_32_t w32;
			read(&w32, sizeof(w32));
			swap(w32.image_base);
			swap(w32.size_of_stack_reserve);
			swap(w32.size_of_stack_commit);
			swap(w32.size_of_heap_reserve);
			swap(w32.size_of_heap_commit);
			_windows_specific_fields.image_base = w32.image_base;
			_windows_specific_fields.section_alignment = w32.section_alignment;
			_windows_specific_fields.file_alignment = w32.file_alignment;
			_windows_specific_fields.major_operating_system_version = w32.major_operating_system_version;
			_windows_specific_fields.minor_operating_system_version = w32.minor_operating_system_version;
			_windows_specific_fields.major_image_version = w32.major_image_version;
			_windows_specific_fields.minor_image_version = w32.minor_image_version;
			_windows_specific_fields.major_subsystem_version = w32.major_subsystem_version;
			_windows_specific_fields.minor_subsystem_version = w32.minor_subsystem_version;
			_windows_specific_fields.win32_version_value = w32.win32_version_value;
			_windows_specific_fields.size_of_image = w32.size_of_image;
			_windows_specific_fields.size_of_headers = w32.size_of_headers;
			_windows_specific_fields.checksum = w32.checksum;
			_windows_specific_fields.subsystem = w32.subsystem;
			_windows_specific_fields.dll_characteristics = w32.dll_characteristics;
			_windows_specific_fields.size_of_stack_reserve = w32.size_of_stack_reserve;
			_windows_specific_fields.size_of_stack_commit = w32.size_of_stack_commit;
			_windows_specific_fields.size_of_heap_reserve = w32.size_of_heap_reserve;
			_windows_specific_fields.size_of_heap_commit = w32.size_of_heap_commit;
			_windows_specific_fields.loader_flags = w32.loader_flags;
			_windows_specific_fields.number_of_rva_and_sizes = w32.number_of_rva_and_sizes;
		}
		swap(_windows_specific_fields.section_alignment);
		swap(_windows_specific_fields.file_alignment);
		swap(_windows_specific_fields.major_operating_system_version);
		swap(_windows_specific_fields.minor_operating_system_version);
		swap(_windows_specific_fields.major_image_version);
		swap(_windows_specific_fields.minor_image_version);
		swap(_windows_specific_fields.major_subsystem_version);
		swap(_windows_specific_fields.minor_subsystem_version);
		swap(_windows_specific_fields.win32_version_value);
		swap(_windows_specific_fields.size_of_image);
		swap(_windows_specific_fields.size_of_headers);
		swap(_windows_specific_fields.checksum);
		swap(_windows_specific_fields.subsystem);
		swap(_windows_specific_fields.dll_characteristics);
		swap(_windows_specific_fields.loader_flags);
		swap(_windows_specific_fields.number_of_rva_and_sizes);
		/*cerr << "DEBUG: _windows_specific_fields\n"
			<< io::hex(_windows_specific_fields.image_base) << io::endl
			<< io::hex(_windows_specific_fields.section_alignment) << io::endl
			<< io::hex(_windows_specific_fields.file_alignment) << io::endl
			<< io::hex(_windows_specific_fields.major_operating_system_version) << io::endl
			<< io::hex(_windows_specific_fields.minor_operating_system_version) << io::endl
			<< io::hex(_windows_specific_fields.major_image_version) << io::endl
			<< io::hex(_windows_specific_fields.minor_image_version) << io::endl
			<< io::hex(_windows_specific_fields.major_subsystem_version) << io::endl
			<< io::hex(_windows_specific_fields.minor_subsystem_version) << io::endl
			<< io::hex(_windows_specific_fields.win32_version_value) << io::endl
			<< io::hex(_windows_specific_fields.size_of_image) << io::endl
			<< io::hex(_windows_specific_fields.size_of_headers) << io::endl
			<< io::hex(_windows_specific_fields.checksum) << io::endl
			<< io::hex(_windows_specific_fields.subsystem) << io::endl
			<< io::hex(_windows_specific_fields.dll_characteristics) << io::endl
			<< io::hex(_windows_specific_fields.size_of_stack_reserve) << io::endl
			<< io::hex(_windows_specific_fields.size_of_stack_commit) << io::endl
			<< io::hex(_windows_specific_fields.size_of_heap_reserve) << io::endl
			<< io::hex(_windows_specific_fields.size_of_heap_commit) << io::endl
			<< io::hex(_windows_specific_fields.loader_flags) << io::endl
			<< io::hex(_windows_specific_fields.number_of_rva_and_sizes) << io::endl;
		*/
		
		// read data directories
		_data_directories = new data_directory_t[_windows_specific_fields.number_of_rva_and_sizes];
		read(_data_directories, sizeof(data_directory_t) * _windows_specific_fields.number_of_rva_and_sizes);
		for(unsigned i = 0; i < _windows_specific_fields.number_of_rva_and_sizes; i++) {
			swap(_data_directories[i].size);
			swap(_data_directories[i].virtual_address);
			/*cerr << "DEBUG: " << i
				 << ": " << io::hex(_data_directories[i].virtual_address)
				 << ": " << io::hex(_data_directories[i].size) << io::endl;
			*/
		}

		// read the sections
		_section_table = new section_header_t[_coff_header.number_of_sections];
		read(_section_table, _coff_header.number_of_sections * sizeof(section_header_t));
		for(unsigned i = 0; i < _coff_header.number_of_sections; i++) {
			swap(_section_table[i].virtual_size);
			swap(_section_table[i].virtual_address);
			swap(_section_table[i].size_of_raw_data);
			swap(_section_table[i].pointer_to_raw_data);
			swap(_section_table[i].pointer_to_relocations);
			swap(_section_table[i].pointer_to_line_numbers);
			swap(_section_table[i].number_of_relocations);
			swap(_section_table[i].number_of_line_numbers);
			swap(_section_table[i].characteristics);
			sects.add(new Section(this, &_section_table[i]));
			/*cerr << "DEBUG: "
				 << sects.top()->name() << ' '
				 << io::hex(_section_table[i].virtual_address) << ":"
				 << io::hex(_section_table[i].virtual_size) << ' '
				 << io::hex(_section_table[i].pointer_to_raw_data) << ":"
				 << io::hex(_section_table[i].size_of_raw_data) << ' '
				 << io::hex(_section_table[i].pointer_to_relocations) << ":"
				 << io::hex(_section_table[i].number_of_relocations) << ' '
				 << io::hex(_section_table[i].pointer_to_line_numbers) << ":"
				 << io::hex(_section_table[i].number_of_line_numbers) << ' '
				 << io::hex(_section_table[i].characteristics) << io::endl;
			*/
		}
		
	}
	catch(sys::SystemException& e) {
		raise(e.message());
	}
}

///
File::~File() {
	if(stream != nullptr)
		delete stream;
	if(_data_directories != nullptr)
		delete [] _data_directories;
	if(_section_table != nullptr)
		delete [] _section_table;
	if(_symbol_table != nullptr)
		delete [] _symbol_table;
	if(_string_table != nullptr)
		delete [] _string_table;
	deleteAll(sects);
}

///
string File::machine() const {
	switch(_coff_header.machine) {
	case IMAGE_FILE_MACHINE_AM33:			return "Matsushita AM33";
	case IMAGE_FILE_MACHINE_ARM:			return "ARM";
	case IMAGE_FILE_MACHINE_ARM64:			return "ARM64";
	case IMAGE_FILE_MACHINE_ARMNT:			return "ARM Thumb-2";
	case IMAGE_FILE_MACHINE_EBC:			return "EFI byte-code";
	case IMAGE_FILE_MACHINE_I386:			return "Intel 386";
	case IMAGE_FILE_MACHINE_IA64:			return "Intel Itanium";
	case IMAGE_FILE_MACHINE_LOONGARCH32:	return "LoongArch";
	case IMAGE_FILE_MACHINE_M32R:			return "Mitsubishi M32R";
	case IMAGE_FILE_MACHINE_MIPS16:			return "MIPS16";
	case IMAGE_FILE_MACHINE_MIPSFPU:		return "MIPS with FPU";
	case IMAGE_FILE_MACHINE_MIPSFPU16:		return "MIPS16 with FPU";
	case IMAGE_FILE_MACHINE_POWERPC:		return "Power PC";
	case IMAGE_FILE_MACHINE_POWERPCFP:		return "Power PC with FPU";
	case IMAGE_FILE_MACHINE_R4000:			return "MIPS";
	case IMAGE_FILE_MACHINE_RISCV32:		return "RISC-V 32-bit";
	case IMAGE_FILE_MACHINE_RISCV64:		return "RISC-V 64-bit";
	case IMAGE_FILE_MACHINE_RISCV128:		return "RISC-V 128-bit";
	case IMAGE_FILE_MACHINE_SH3:			return "Hitachi SH3";
	case IMAGE_FILE_MACHINE_SH3DSP:			return "Hitachi SH3 DSP";
	case IMAGE_FILE_MACHINE_SH4:			return "Hitachi SH4";
	case IMAGE_FILE_MACHINE_SH5:			return "Hitachi SH5";
	case IMAGE_FILE_MACHINE_THUMB:			return "Thumb";
	case IMAGE_FILE_MACHINE_WCEMIPSV2:		return "MIPS WCET v2";
	case IMAGE_FILE_MACHINE_UNKNOWN:
	default:							return "unknown";
	}
}

///
string File::os() const {
	return "windows";
}

///
int File::elfMachine() const {
	switch(_coff_header.machine) {
	case IMAGE_FILE_MACHINE_AM33:			return 89;
	case IMAGE_FILE_MACHINE_ARM:			return 40;
	case IMAGE_FILE_MACHINE_ARM64:			return 183;
	case IMAGE_FILE_MACHINE_ARMNT:			return 40;
	case IMAGE_FILE_MACHINE_EBC:			return 0;
	case IMAGE_FILE_MACHINE_I386:			return 3;
	case IMAGE_FILE_MACHINE_IA64:			return 50;
	case IMAGE_FILE_MACHINE_LOONGARCH32:	return 258;
	case IMAGE_FILE_MACHINE_M32R:			return 88;
	case IMAGE_FILE_MACHINE_MIPS16:			return 8;
	case IMAGE_FILE_MACHINE_MIPSFPU:		return 8;
	case IMAGE_FILE_MACHINE_MIPSFPU16:		return 8;
	case IMAGE_FILE_MACHINE_POWERPC:		return 20;
	case IMAGE_FILE_MACHINE_POWERPCFP:		return 20;
	case IMAGE_FILE_MACHINE_R4000:			return 8;
	case IMAGE_FILE_MACHINE_RISCV32:		return 243;
	case IMAGE_FILE_MACHINE_RISCV64:		return 243;
	case IMAGE_FILE_MACHINE_RISCV128:		return 243;
	case IMAGE_FILE_MACHINE_SH3:			return 42;
	case IMAGE_FILE_MACHINE_SH3DSP:			return 42;
	case IMAGE_FILE_MACHINE_SH4:			return 42;
	case IMAGE_FILE_MACHINE_SH5:			return 42;
	case IMAGE_FILE_MACHINE_THUMB:			return 40;
	case IMAGE_FILE_MACHINE_WCEMIPSV2:		return 8;
	case IMAGE_FILE_MACHINE_UNKNOWN:
	default:								return 0;	
	}
}

///
int File::elfOS() const {
	return 0;
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
	if((_coff_header.characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) != 0)
		return program;
	else if((_coff_header.characteristics & IMAGE_FILE_DLL) != 0)
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
	if((_coff_header.characteristics & IMAGE_FILE_32BIT_MACHINE) != 0)
		return address_32;
	else
		return address_16;
}

///
address_t File::entry() {
	return _standard_coff_fields.address_of_entry_point + _windows_specific_fields.image_base;
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
 * Get the the string from the string table at the given offset.
 * @throw Exception	If there is an IO error.
 */
cstring File::getString(t::uint32 offset) {
	if(_string_table == nullptr) {
		// symbol_t is not padded! Size is 18 and not 20!
		stream->moveTo(_coff_header.pointer_to_symbol_table + 18 * _coff_header.number_of_symbols);
		read(&_string_table_size, sizeof(_string_table_size));
		swap(_string_table_size);
		_string_table_size -= 4;
		_string_table = new char[_string_table_size];
		read(_string_table, _string_table_size);
	}
	return _string_table + offset - 4;
}


// Decoder override
void File::fix(t::uint16& w) { swap(w); }
void File::fix(t::int16& w) { t::uint16 x = w; swap(x); w = x; }
void File::fix(t::uint32& w) { swap(w); }
void File::fix(t::int32& w) { t::uint32 x = w; swap(x); w = x; }
void File::fix(t::uint64& w) { swap(w); }
void File::fix(t::int64& w) { t::uint64 x = w; swap(x); w = x; }

void File::unfix(t::uint16& w) { fix(w); }
void File::unfix(t::int16& w) { fix(w); }
void File::unfix(t::uint32& w) { fix(w); }
void File::unfix(t::int32& w) { fix(w); }
void File::unfix(t::uint64& w) { fix(w); }
void File::unfix(t::int64& w) { fix(w); }


/**
 * @class Section
 * Segment from the GEL++ portable interface corresponds to sections of the
 * PE-COFF format.
 * @ingroup pecoff
 */

///
Section::Section(File *file, const section_header_t *header):
	hd(header), pec(file), _buf(nullptr), _flags(-1)
	{ }

///
Section::~Section() {
	if(_buf != nullptr)
		delete [] _buf;
}

/**
 * @fn const section_header_t& Section::header() const;
 * Get the section header itself.
 * @return	Section header.
 */

///
cstring Section::name(void) {
	if(!_name) {
		if(hd->name[0] == '/') {
			t::uint32 offset;
			string(hd->name + 1) >> offset;
			_name = pec->getString(offset);
		}
		else if(hd->name[7] == '\0')
			_name = hd->name;
		else
			_name = string(hd->name, 8);		
	}
	return _name.toCString();
}

///
address_t Section::baseAddress(void) {
	return hd->virtual_address;
}

///
address_t Section::loadAddress(void) {
	return hd->virtual_address;
}

///
size_t Section::size(void) {
	return hd->virtual_size;
}

///
size_t Section::alignment(void) {
	int msk = (hd->characteristics & IMAGE_SCN_ALIGN_1BYTES >> 20) & 0xf;
	return 1 << (msk - 1);
}

///
bool Section::isExecutable(void) {
	return (hd->characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
}

///
bool Section::isWritable(void) {
	return (hd->characteristics & IMAGE_SCN_MEM_WRITE) != 0;
}

///
bool Section::hasContent(void) {
	return !(hd->characteristics & IMAGE_SCN_MEM_DISCARDABLE);
}

///
Buffer Section::buffer(void) {
	if(_buf == nullptr) {
		_buf = new t::uint8[hd->virtual_size];
		if(hd->size_of_raw_data != 0) {
			if(!pec->stream->moveTo(hd->pointer_to_raw_data)) {
				delete [] _buf;
				_buf = nullptr;
				pec->raise(_ << "bad pointer_to_raw_data for section " << name());
			}
			pec->read(_buf, hd->size_of_raw_data);
		}
		if(hd->size_of_raw_data < hd->virtual_size)
			array::clear(
				_buf + hd->virtual_size,
				hd->virtual_size - hd->size_of_raw_data);
	}
	return Buffer(pec, _buf, hd->virtual_size);
}

///
size_t Section::offset() {
	return hd->pointer_to_raw_data;
}

///
size_t Section::fileSize() {
	return hd->size_of_raw_data;
}

///
flags_t Section::flags() {
	if(_flags == flags_t(-1)) {
		_flags = 0;
		if(hd->characteristics & IMAGE_SCN_MEM_EXECUTE)
			_flags |= IS_EXECUTABLE;
		if(hd->characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
			_flags |= HAS_CONTENT;
		if(!(hd->characteristics & IMAGE_SCN_LNK_REMOVE)
		&& !(hd->characteristics & IMAGE_SCN_MEM_DISCARDABLE))
			_flags |= IS_LOADABLE;
		if(hd->characteristics & IMAGE_SCN_MEM_READ)
			_flags |= IS_READABLE;
		if(hd->characteristics & IMAGE_SCN_MEM_WRITE)
			_flags |= IS_WRITABLE;
	}
	return _flags;
}


} } // gel::pecoff

