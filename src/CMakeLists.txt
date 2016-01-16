add_library(gel++ SHARED
	elf_File.cpp
	gel_File.cpp
	gel_Manager.cpp)

if(INSTALL_BIN)
	install(TARGETS gel++ DESTINATION ${LIBDIR})
endif()

include_directories("${CMAKE_SOURCE_DIR}/src")

