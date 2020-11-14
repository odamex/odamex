global_compile_options()

# Master
file(GLOB MASTER_HEADERS *.h)
file(GLOB MASTER_SOURCES *.cpp)

# Platform definitions
define_platform()

# Master target
add_executable(odamast ${MASTER_SOURCES} ${MASTER_HEADERS})
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
