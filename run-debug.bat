@echo off
setlocal
set "PATH=C:\Felgo\Felgo\mingw_64\bin;C:\Felgo\Tools\mingw1310_64\bin;%PATH%"
set "QML_IMPORT_TRACE=1"
cd /d "E:\Qt\QtProjects\QDeltaPLC\build\Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug"
qdeltaplc.exe > import-trace.log 2>&1