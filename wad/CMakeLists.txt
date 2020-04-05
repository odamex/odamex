find_program(DEUTEX deutex)

# If DeuTex is available, use it to build the WAD.
if(DEUTEX)
	message("Found DeuTex: ${DEUTEX}")

	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/odamex.wad
		COMMAND ${DEUTEX} -rgb 0 255 255 -doom2 bootstrap -build wadinfo.txt ${CMAKE_CURRENT_BINARY_DIR}/odamex.wad
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/odamex.wad
		VERBATIM)

	add_custom_target(odawad ALL
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/odamex.wad)

	if(WIN32)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/odamex.wad
			DESTINATION .
			COMPONENT common)
	else()
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/odamex.wad
			DESTINATION ${CMAKE_INSTALL_DATADIR}/odamex
			COMPONENT common)
	endif()
else()
	if(WIN32)
		install(FILES odamex.wad
			DESTINATION .
			COMPONENT common)
	else()
		install(FILES odamex.wad
			DESTINATION ${CMAKE_INSTALL_DATADIR}/odamex
			COMPONENT common)
	endif()
endif()
