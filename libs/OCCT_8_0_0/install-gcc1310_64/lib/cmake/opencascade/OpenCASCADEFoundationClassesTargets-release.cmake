#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "TKernel" for configuration "Release"
set_property(TARGET TKernel APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TKernel PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libTKernel.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libTKernel.dll"
  )

list(APPEND _cmake_import_check_targets TKernel )
list(APPEND _cmake_import_check_files_for_TKernel "${_IMPORT_PREFIX}/lib/libTKernel.dll.a" "${_IMPORT_PREFIX}/bin/libTKernel.dll" )

# Import target "TKMath" for configuration "Release"
set_property(TARGET TKMath APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TKMath PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libTKMath.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libTKMath.dll"
  )

list(APPEND _cmake_import_check_targets TKMath )
list(APPEND _cmake_import_check_files_for_TKMath "${_IMPORT_PREFIX}/lib/libTKMath.dll.a" "${_IMPORT_PREFIX}/bin/libTKMath.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
