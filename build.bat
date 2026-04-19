@echo off
set MSVC_VER=14.51.36014
set SDK_VER=10.0.26100.0
set VS_BASE=C:\Program Files\Microsoft Visual Studio\18\Community
set SDK_BASE=C:\Program Files (x86)\Windows Kits\10

set CL_EXE=%VS_BASE%\VC\Tools\MSVC\%MSVC_VER%\bin\Hostx64\x64\cl.exe
set MSVC_INC=%VS_BASE%\VC\Tools\MSVC\%MSVC_VER%\include
set UC_INC=%SDK_BASE%\Include\%SDK_VER%\ucrt
set SH_INC=%SDK_BASE%\Include\%SDK_VER%\shared
set UM_INC=%SDK_BASE%\Include\%SDK_VER%\um
set MSVC_LIB=%VS_BASE%\VC\Tools\MSVC\%MSVC_VER%\lib\x64
set UCRT_LIB=%SDK_BASE%\Lib\%SDK_VER%\ucrt\x64
set UM_LIB=%SDK_BASE%\Lib\%SDK_VER%\um\x64

cd /d C:\University\Programmer\Compailer_for_Alex

"%CL_EXE%" /std:c++17 /EHsc /W3 ^
    /I"%MSVC_INC%" /I"%UC_INC%" /I"%SH_INC%" /I"%UM_INC%" ^
    main.cpp Lexer/lexer.cpp Parser/parser.cpp Semantic/semantic.cpp CodeGen/codegen.cpp ^
    /Fe:alexc.exe ^
    /link /LIBPATH:"%MSVC_LIB%" /LIBPATH:"%UCRT_LIB%" /LIBPATH:"%UM_LIB%"

echo Exit code: %ERRORLEVEL%
