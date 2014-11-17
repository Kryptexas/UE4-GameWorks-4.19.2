setlocal
set FLEX=win_flex.exe
set BISON=win_bison.exe
set SRCDIR=../../../Source/ThirdParty/hlslcc/hlslcc/src/hlslcc_lib/mesa

pushd ..\..\..\..\Extras\NotForLicensees\FlexAndBison\

%FLEX% --nounistd -o%SRCDIR%/glcpp-lex.c %SRCDIR%/glcpp-lex.l
%BISON% -v -o "%SRCDIR%/glcpp-parse.c" --defines=%SRCDIR%/glcpp-parse.h %SRCDIR%/glcpp-parse.y

%FLEX% --nounistd -o%SRCDIR%/hlsl_lexer.cpp %SRCDIR%/hlsl_lexer.ll
%BISON% -v -o "%SRCDIR%/hlsl_parser.cpp" -p "_mesa_hlsl_" --defines=%SRCDIR%/hlsl_parser.h %SRCDIR%/hlsl_parser.yy

pause

popd
endlocal