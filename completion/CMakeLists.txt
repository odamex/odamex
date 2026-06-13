if(UNIX AND NOT APPLE)
	set(INSTALL_BASH_COMPLETION_DEFAULT ON)
else()
	set(INSTALL_BASH_COMPLETION_DEFAULT OFF)
endif()

option(
	INSTALL_BASH_COMPLETION
	"Install bash completion scripts"
	${INSTALL_BASH_COMPLETION_DEFAULT}
)

if(WIN32)
  set(INSTALL_BASH_COMPLETION OFF CACHE BOOL "" FORCE)
endif()

if(INSTALL_BASH_COMPLETION)

	if(APPLE)
		message(WARNING "Bash completion uses associative arrays (Bash 4+ required). macOS system Bash is 3.2.")
	endif()

	if(BUILD_CLIENT)
		install(
			FILES "${CMAKE_CURRENT_SOURCE_DIR}/odamex.bash"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/bash-completion/completions"
			COMPONENT client
		)
	endif()

	if(BUILD_SERVER)
		install(
			FILES
			"${CMAKE_CURRENT_SOURCE_DIR}/odamex.bash"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/bash-completion/completions"
			RENAME "odasrv.bash"
			COMPONENT server
		)
	endif()

endif()