# cmake/FelgoHotReload.cmake
# include() this from the ROOT CMakeLists.txt, at root scope.
# Requires: QDELTA_HR_TARGET = name of the executable target.

if(NOT QDELTA_HR_TARGET)
  message(FATAL_ERROR "QDELTA_HR_TARGET not set before including FelgoHotReload.cmake")
endif()

# Felgo ships one client library per (Qt version, compiler) pair. Picking the
# wrong folder produces link errors or a client that crashes on connect.
if(MINGW)
  set(_fhr_compiler "mingw_64")
elseif(MSVC)
  set(_fhr_compiler "msvc2022_x64")
else()
  message(FATAL_ERROR "Unsupported compiler for Felgo Hot Reload on this platform")
endif()

# Override in Qt Creator: Projects > Build Settings > CMake > FELGO_HOT_RELOAD_PATH
if(NOT FELGO_HOT_RELOAD_PATH)
  foreach(_root
      "$ENV{FELGO_HOT_RELOAD_ROOT}"
      "C:/FelgoHotReload"
      "C:/Felgo/FelgoHotReload")
    set(_cand "${_root}/client/${Qt6_VERSION}/${_fhr_compiler}")
    if(EXISTS "${_cand}/lib/cmake/FelgoHotReload/FelgoHotReloadConfig.cmake")
      set(FELGO_HOT_RELOAD_PATH "${_cand}" CACHE PATH "Felgo Hot Reload client dir" FORCE)
      break()
    endif()
  endforeach()
endif()

if(NOT EXISTS "${FELGO_HOT_RELOAD_PATH}/lib/cmake/FelgoHotReload/FelgoHotReloadConfig.cmake")
  message(FATAL_ERROR
    "Felgo Hot Reload client not found.\n"
    "Expected: <install>/client/${Qt6_VERSION}/${_fhr_compiler}/lib/cmake/FelgoHotReload\n"
    "Set -DFELGO_HOT_RELOAD_PATH=<that folder> in Build Settings > CMake.")
endif()

# FORCE: the package config caches this; a stale value survives kit switches.
set(FelgoHotReload_DIR "${FELGO_HOT_RELOAD_PATH}/lib/cmake/FelgoHotReload"
    CACHE PATH "FelgoHotReload directory" FORCE)

find_package(FelgoHotReload CONFIG REQUIRED)

# Links every needed lib, adds missing Qt components, and defines
# USE_FELGO_HOT_RELOAD. Also writes felgo_hot_reload_cmake_targets_info.json
# next to the root CMakeLists, which the GUI app reads to auto-detect modules.
felgohotreload_configure_executable(${QDELTA_HR_TARGET})

message(STATUS "Felgo Hot Reload client: ${FELGO_HOT_RELOAD_PATH}")