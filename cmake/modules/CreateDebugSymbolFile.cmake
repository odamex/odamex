function(create_debug_symbol_file)
    set(options )
    set(oneValueArgs TARGET)
    set(multiValueArgs)
    cmake_parse_arguments(cds_arg "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

    if (MSVC)

        # Explicitly leaving this block empty because MSVC provides its own .pdb mechanism.

    elseif (UNIX AND NOT APPLE)

        # We use generator expressions to ensure that any generated multi-config build system
        # is able to switch back and forth between debug and non-debug at will and get the
        # desired results without having to re-generate.
        #
        # This is done on the suggestion of the CMake docs for add_custom_command.

        list(APPEND make_dbg_file
            objcopy --only-keep-debug ${cds_arg_TARGET} ${cds_arg_TARGET}.dbg)

        list(APPEND strip_exe_file
            objcopy --strip-debug ${cds_arg_TARGET})

        list(APPEND link_exe_to_dbg
            objcopy --add-gnu-debuglink=${cds_arg_TARGET}.dbg ${cds_arg_TARGET})

        add_custom_command(
            TARGET ${cds_arg_TARGET}
            POST_BUILD
            COMMAND "$<IF:$<CONFIG:Debug,RelWithDebInfo>,${make_dbg_file},>"
            COMMAND "$<IF:$<CONFIG:Debug,RelWithDebInfo>,${strip_exe_file},>"
            COMMAND "$<IF:$<CONFIG:Debug,RelWithDebInfo>,${link_exe_to_dbg},>"
            COMMAND_EXPAND_LISTS
            VERBATIM
            )
    endif()

endfunction()
