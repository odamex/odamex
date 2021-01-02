include(OdamexTargetSettings)

# Master
file(GLOB MASTER_SOURCES *.cpp *.h)

# Master target
add_executable(odamast ${MASTER_SOURCES})
odamex_target_settings(odamast)

if(WIN32)
  target_link_libraries(odamast wsock32)
elseif(SOLARIS)
  target_link_libraries(odamast socket nsl)
endif()

if(UNIX)
	install( TARGETS odamast DESTINATION ${CMAKE_INSTALL_BINDIR} )
endif()

if(BUILD_OR_FAIL AND NOT TARGET odamast)
  message(FATAL_ERROR "Odamex metaserver target could not be generated")
endif()
