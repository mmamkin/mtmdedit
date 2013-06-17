call "%VS100COMNTOOLS%vsvars32.bat"
msbuild /p:Configuration=arx2012 /p:Platform=x64 MtmdEdit.vcxproj
pause
