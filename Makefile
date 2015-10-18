ffff.exe : ffff.obj ffff.msg ffff.mb
	cl /AL ffff.obj ffff.def
	msgbind ffff.mb
	bind ffff doscalls.lib

ffff.msg : ffff.txt
	mkmsgf ffff.txt ffff.msg

ffff.obj : ffff.c
	cl /W3 /AL /G0 /Ox /c ffff.c

ffff.mb : ffff.txt
	makemb ffff.exe ffff.msg <ffff.txt >ffff.mb

clean:
	del *.mb *.msg *.obj
