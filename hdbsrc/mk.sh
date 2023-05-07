cc -c sha256.c
cc -c bt.c
cc -c util.c
cc -c cjson.c
cc -o cap.exe cap.c util.o
cc -o cat.exe cat.c util.o
cc -o creat.exe creat.c util.o
cc -o detab.exe detab.c util.o
cc -o finfo.exe finfo.c util.o
cc -o finger.exe finger.c util.o
cc -o flat.exe flat.c util.o
cc -o glob.exe glob.c util.o
cc -o grep.exe grep.c util.o
cc -o hex.exe hex.c util.o
cc -o lsf.exe lsf.c util.o
cc -o lz.exe lz.c util.o
cc -o od.exe od.c util.o
cc -o sleep.exe sleep.c util.o
cc -o sum.exe sum.c util.o
cc -o sx.exe sx.c util.o
cc -o remove.exe remove.c util.o
cc -o xlate.exe xlate.c util.o
