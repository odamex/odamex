### libADLMIDI ###

if(BUILD_CLIENT)
  if(USE_INTERNAL_LIBADLMIDI)
    lib_buildgen(LIBRARY libadlmidi PARAMS "-DCMAKE_DEBUG_POSTFIX=d")
    lib_build(LIBRARY libadlmidi)
  endif()

  find_package(libADLMIDI)
  if(TARGET libADLMIDI::ADLMIDI_static)
    set_target_properties(libADLMIDI::ADLMIDI_static PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
