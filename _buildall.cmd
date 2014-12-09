set MTMDPROJECTS=MtmdEdit.vcxproj

call "%VS100COMNTOOLS%vsvars32.bat"
rem msbuild /p:Configuration="arx2012;arx2013;brx13;brx14;brx15" /p:Platform="Win32;x64" /p:Projects="%MTMDPROJECTS%" _MtmdBuild.proj
msbuild /p:Configuration="brx15" /p:Platform="Win32;x64" /p:Projects="%MTMDPROJECTS%" _MtmdBuild.proj
