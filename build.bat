cls

del *.exe

del *.obj

del *.txt

cl.exe /c /EHsc /Iheader /ID:\Astromedicomp\RTR\01_OpenGL\02_PP\01_Windows\include /IC:\ProgramEnvirounment\include src/*.cpp

rc.exe /i header resource\OGL.rc

link.exe main.obj resource\*.res /SUBSYSTEM:WINDOWS /LIBPATH:D:\Astromedicomp\RTR\01_OpenGL\02_PP\01_Windows\lib\Release\x64 /LIBPATH:C:\ProgramEnvirounment\lib user32.lib gdi32.lib kernel32.lib
