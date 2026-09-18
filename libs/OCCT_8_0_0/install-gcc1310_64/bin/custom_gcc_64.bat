echo off

rem CASDEB comes as third argument

if /I "%VCVER%" == "gcc" (
  if "%ARCH%" == "64" (
    rem set environment variables used by OCCT
    set CSF_FPE=0

    set "TCL_DIR="
    set "TK_DIR="
    set "FREETYPE_DIR="
    set "FREEIMAGE_DIR="
    set "EGL_DIR="
    set "GLES2_DIR="
    set "TBB_DIR="
    set "VTK_DIR="
    set "FFMPEG_DIR="
    set "JEMALLOC_DIR="
    set "OPENVR_DIR="

    if not "" == "" (
      set "QTDIR="
    )
    set "TCL_VERSION_WITH_DOT="
    set "TK_VERSION_WITH_DOT="

    set "CSF_OCCTBinPath=%CASROOT%/bin%3"
    set "CSF_OCCTLibPath=%CASROOT%/lib%3"

    set "CSF_OCCTIncludePath=%CASROOT%/include/opencascade"
    set "CSF_OCCTResourcePath=%CASROOT%/share/opencascade/resources"
    set "CSF_OCCTDataPath=%CASROOT%/share/opencascade/data"
    set "CSF_OCCTTestsPath=%CASROOT%/share/opencascade/tests"
    set "CSF_OCCTDocPath=%CASROOT%/share/doc/opencascade"
  )
)

