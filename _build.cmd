rem call "%VS100COMNTOOLS%vsvars32.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat" 
set > aaa
msbuild /p:Configuration=arx2019 /p:Platform=x64 MtmdEdit.vcxproj
pause
