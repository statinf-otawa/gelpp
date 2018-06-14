add_library(gel++ SHARED
	elf_File.cpp
	elf_UnixBuilder.cpp
	gel_File.cpp
	gel_Image.cpp
	gel_Manager.cpp)

if(INSTALL_BIN)
	install(TARGETS gel++ DESTINATION ${LIBDIR})
endif()

include_directories("${CMAKE_SOURCE_DIR}/src")

