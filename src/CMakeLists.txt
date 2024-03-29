set(CMAKE_INSTALL_RPATH "${ORIGIN}/..")
set(CMAKE_MACOSX_RPATH true)

# prepare sources
set(SOURCES
	"dwarf_DebugLine.cpp"
	"elf_ArchPlugin.cpp"
	"elf_File.cpp"
	"elf_File32.cpp"
	"elf_File64.cpp"
	"elf_UnixBuilder.cpp"
	"gel_DebugLine.cpp"
	"gel_File.cpp"
	"gel_Image.cpp"
	"gel_LittleDecoder.cpp"
	"gel_Manager.cpp"
	"pecoff_File.cpp")
if(HAS_COFFI)
	list(APPEND SOURCES "coffi_File.cpp")
endif()

# main library
add_library(gel++ SHARED ${SOURCES})

target_link_libraries("gel++" "${ELM_LIB}")
set_target_properties(gel++ PROPERTIES
	INSTALL_RPATH "\$ORIGIN")
if(INSTALL_BIN)
	install(TARGETS gel++ DESTINATION "${LIBDIR}")
endif()

include_directories("${CMAKE_SOURCE_DIR}/src")

# arm architecture plugin
add_library(elf40 SHARED elf_arm.cpp)
set_property(TARGET elf40 PROPERTY PREFIX "")
set_property(TARGET elf40 PROPERTY SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
target_link_libraries(elf40 "gel++" "${ELM_LIB}")
install(TARGETS elf40 DESTINATION "${LIBDIR}/gel++")
