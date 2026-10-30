# Script mode: converts SOURCE (STEP) to OUTPUT (BREP) with CONVERTER.
# RUNTIME_DIRS holds the converter's DLL directories separated by "|", so no list
# separator crosses the build command line; OCCT_RESOURCES is the OCCT resource directory.
string(REPLACE "|" ";" runtime_dirs "${RUNTIME_DIRS}")
string(REPLACE "/" "\\" runtime_dirs "${runtime_dirs}")
set(ENV{PATH} "${runtime_dirs};$ENV{PATH}")
set(ENV{CSF_STEPDefaults} "${OCCT_RESOURCES}/XSTEPResource")

# An interrupted or failed conversion must not leave an output the build treats as current.
set(temporary "${OUTPUT}.tmp")
file(REMOVE "${temporary}")
execute_process(COMMAND "${CONVERTER}" "${SOURCE}" "${temporary}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  file(REMOVE "${temporary}")
  message(FATAL_ERROR "STEP conversion failed (${result}): ${SOURCE}")
endif()
file(RENAME "${temporary}" "${OUTPUT}")
