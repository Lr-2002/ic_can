#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "IC_CAN::ic_can_core" for configuration "Release"
set_property(TARGET IC_CAN::ic_can_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(IC_CAN::ic_can_core PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libic_can_core.so.1.0.0"
  IMPORTED_SONAME_RELEASE "libic_can_core.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS IC_CAN::ic_can_core )
list(APPEND _IMPORT_CHECK_FILES_FOR_IC_CAN::ic_can_core "${_IMPORT_PREFIX}/lib/libic_can_core.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
