set(CMAKE_INSTALL_RPATH "${ORIGIN}/../lib")
link_directories("${CMAKE_SOURCE_DIR}/src")

add_executable(gel-file "gel-file.cpp")
target_link_libraries(gel-file  "gel++" "${ELM_LIB}")

add_executable(gel-sect "gel-sect.cpp")
target_link_libraries(gel-sect  "gel++" "${ELM_LIB}")

add_executable(gel-prog "gel-prog.cpp")
target_link_libraries(gel-prog  "gel++" "${ELM_LIB}")

add_executable(gel-sym "gel-sym.cpp")
target_link_libraries(gel-sym  "gel++" "${ELM_LIB}")

add_executable(gel-im "gel-im.cpp")
target_link_libraries(gel-im  "gel++" "${ELM_LIB}")

add_executable(gel-dyn "gel-dyn.cpp")
target_link_libraries(gel-dyn  "gel++" "${ELM_LIB}")

add_executable(gel-line "gel-line.cpp")
target_link_libraries(gel-line  "gel++" "${ELM_LIB}")

add_executable(gel-seg "gel-seg.cpp")
target_link_libraries(gel-seg  "gel++" "${ELM_LIB}")

if(INSTALL_BIN)
	install(TARGETS gel-file DESTINATION bin)
	install(TARGETS gel-sect DESTINATION bin)
	install(TARGETS gel-prog DESTINATION bin)
	install(TARGETS gel-sym  DESTINATION bin)
	install(TARGETS gel-im   DESTINATION bin)
	install(TARGETS gel-dyn  DESTINATION bin)
	install(TARGETS gel-line DESTINATION bin)
	install(TARGETS gel-seg DESTINATION bin)
endif()
