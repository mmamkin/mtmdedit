set MTMDPROJECTS=MtmdEdit.vcxproj

rem call "%VS100COMNTOOLS%vsvars32.bat"
call "%VS110COMNTOOLS%vsvars32.bat"
msbuild /p:Configuration="arx2015" /p:Platform="Win32;x64" /p:Projects="%MTMDPROJECTS%" /p:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V110" _MtmdBuild.proj
