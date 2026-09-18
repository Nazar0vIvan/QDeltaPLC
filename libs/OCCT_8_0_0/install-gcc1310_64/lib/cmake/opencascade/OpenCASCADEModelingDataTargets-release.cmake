#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "TKG2d" for configuration "Release"
set_property(TARGET TKG2d APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TKG2d PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libTKG2d.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libTKG2d.dll"
  )

list(APPEND _cmake_import_check_targets TKG2d )
list(APPEND _cmake_import_check_files_for_TKG2d "${_IMPORT_PREFIX}/lib/libTKG2d.dll.a" "${_IMPORT_PREFIX}/bin/libTKG2d.dll" )

# Import target "TKG3d" for configuration "Release"
set_property(TARGET TKG3d APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TKG3d PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libTKG3d.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libTKG3d.dll"
  )

list(APPEND _cmake_import_check_targets TKG3d )
list(APPEND _cmake_import_check_files_for_TKG3d "${_IMPORT_PREFIX}/lib/libTKG3d.dll.a" "${_IMPORT_PREFIX}/bin/libTKG3d.dll" )

# Import target "TKGeomBase" for configuration "Release"
set_property(TARGET TKGeomBase APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TKGeomBase PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libTKGeomBase.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libTKGeomBase.dll"
  )

list(APPEND _cmake_import_check_targets TKGeomBase )
list(APPEND _cmake_import_check_files_for_TKGeomBase "${_IMPORT_PREFIX}/lib/libTKGeomBase.dll.a" "${_IMPORT_PREFIX}/bin/libTKGeomBase.dll" )

# Import target "TKBRep" for configuration "Release"
set_property(TARGET TKBRep APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TKBRep PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libTKBRep.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libTKBRep.dll"
  )

list(APPEND _cmake_import_check_targets TKBRep )
list(APPEND _cmake_import_check_files_for_TKBRep "${_IMPORT_PREFIX}/lib/libTKBRep.dll.a" "${_IMPORT_PREFIX}/bin/libTKBRep.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
