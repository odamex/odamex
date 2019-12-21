find_program(DEUTEX deutex)

# If DeuTex is available, use it to build the WAD.
if(DEUTEX)
	message("Found DeuTex: ${DEUTEX}")

	file(REMOVE odamex.wad)
	add_custom_command(OUTPUT odamex.wad
		COMMAND ${DEUTEX} -rgb 0 255 255 -doom2 bootstrap -build wadinfo.txt odamex.wad
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM)

	add_custom_target(odawad DEPENDS odamex.wad)

	if(BUILD_CLIENT)
	  add_dependencies(odamex odawad)
	endif()
	if(BUILD_SERVER)
	  add_dependencies(odasrv odawad)
	endif()
endif()

if(WIN32)
	install(FILES odamex.wad
		DESTINATION .
		COMPONENT common)
else()
	install(FILES odamex.wad
		DESTINATION ${CMAKE_INSTALL_DATADIR}/odamex
		COMPONENT common)
endif()
