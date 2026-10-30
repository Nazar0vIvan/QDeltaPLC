# The only STEP reader: converts resources/cad/*.stp to BREP during the build.
add_executable(cadstep2brep main.cpp)
target_link_libraries(cadstep2brep PRIVATE TKernel TKBRep TKDESTEP)
