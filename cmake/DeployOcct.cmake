# Configure deployment here; execute the same script after linking and installing.
# file(GET_RUNTIME_DEPENDENCIES) is available at our CMake 3.16 minimum.
if(NOT DEFINED ROBOCRAP_DEPLOY_CONFIG)
  function(robocrap_deploy_occt target occt_prefix)
    get_filename_component(compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(OCCT_RUNTIME_SEARCH_DIRS "${compiler_bin}" CACHE STRING
        "Additional directories containing OCCT third-party runtime DLLs")
    set(deploy_config "${CMAKE_CURRENT_BINARY_DIR}/occt-deploy-$<CONFIG>.cmake")
    file(GENERATE OUTPUT "${deploy_config}" CONTENT
"set(occt_prefix [==[${occt_prefix}]==])
set(cad_source [==[${PROJECT_SOURCE_DIR}/resources/cad/kr10]==])
set(runtime_search [==[${OCCT_RUNTIME_SEARCH_DIRS}]==])
set(objdump [==[${CMAKE_OBJDUMP}]==])
set(runtime_roots
  [==[$<TARGET_FILE:TKernel>]==]
  [==[$<TARGET_FILE:TKMath>]==]
  [==[$<TARGET_FILE:TKG3d>]==]
  [==[$<TARGET_FILE:TKBRep>]==]
  [==[$<TARGET_FILE:TKService>]==]
  [==[$<TARGET_FILE:TKV3d>]==]
  [==[$<TARGET_FILE:TKOpenGl>]==]
  [==[$<TARGET_FILE:TKDESTEP>]==]
)
")
    set(deploy_script "${PROJECT_SOURCE_DIR}/cmake/DeployOcct.cmake")
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_COMMAND}"
        "-DROBOCRAP_DEPLOY_CONFIG=${deploy_config}"
        "-DROBOCRAP_DEPLOY_DEST=$<TARGET_FILE_DIR:${target}>"
        -P "${deploy_script}"
      COMMENT "Deploying OCCT libraries, resources, and KR10 CAD assets"
      VERBATIM
    )
    install(CODE "
      set(ROBOCRAP_DEPLOY_CONFIG \"${CMAKE_CURRENT_BINARY_DIR}/occt-deploy-\${CMAKE_INSTALL_CONFIG_NAME}.cmake\")
      set(ROBOCRAP_DEPLOY_DEST \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
      include(\"${deploy_script}\")
    ")
  endfunction()
  return()
endif()

include("${ROBOCRAP_DEPLOY_CONFIG}")
if(NOT IS_DIRECTORY "${cad_source}" OR NOT IS_DIRECTORY "${occt_prefix}/share/opencascade/resources")
  message(FATAL_ERROR "The KR10 CAD or OCCT resource source directory is missing.")
endif()
if(NOT EXISTS "${objdump}")
  message(FATAL_ERROR "CMAKE_OBJDUMP must identify the selected MinGW kit's objdump for OCCT deployment.")
endif()

set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${objdump}")
file(GET_RUNTIME_DEPENDENCIES
  LIBRARIES ${runtime_roots}
  DIRECTORIES "${occt_prefix}/bin" ${runtime_search}
  RESOLVED_DEPENDENCIES_VAR resolved
  UNRESOLVED_DEPENDENCIES_VAR unresolved
  PRE_EXCLUDE_REGEXES "^[aA][pP][iI]-[mM][sS]-" "^[eE][xX][tT]-[mM][sS]-"
  POST_EXCLUDE_REGEXES "^[A-Za-z]:[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\]"
)
if(unresolved)
  message(FATAL_ERROR "Missing OCCT runtime dependencies: ${unresolved}. Set OCCT_RUNTIME_SEARCH_DIRS to their locations.")
endif()

file(MAKE_DIRECTORY "${ROBOCRAP_DEPLOY_DEST}")
file(COPY ${runtime_roots} ${resolved} DESTINATION "${ROBOCRAP_DEPLOY_DEST}")
file(MAKE_DIRECTORY "${ROBOCRAP_DEPLOY_DEST}/resources/cad/kr10")
file(COPY "${cad_source}/" DESTINATION "${ROBOCRAP_DEPLOY_DEST}/resources/cad/kr10"
  FILES_MATCHING PATTERN "*.stp" PATTERN "*.brep" PATTERN "*.m3d")
file(MAKE_DIRECTORY "${ROBOCRAP_DEPLOY_DEST}/resources/occt")
file(COPY "${occt_prefix}/share/opencascade/resources/"
  DESTINATION "${ROBOCRAP_DEPLOY_DEST}/resources/occt")
file(COPY "${occt_prefix}/share/doc/opencascade/LICENSE_LGPL_21.txt"
  DESTINATION "${ROBOCRAP_DEPLOY_DEST}/licenses/occt")
