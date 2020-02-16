# Project:	Keycvt

.SUFFIXES:	.c .o .s
CC			= cc
CFLAGS		= -c -D__ARC__ -depend !Depend -IC:,@
ObjAsmflags	= -Stamp -NoCache -CloseExec -Quit
LINK		= link
LINKFLAGS	= -o $@
SQUEEZE		= squeeze
SQUEEZEFLAGS	= -o $@

FILES =	idea.o keycvt.o md5.o mdc.o

LIBS =	C:o.stubs

all:	keycvt

keycvt:	$(FILES)
		$(LINK) $(LINKFLAGS) $(FILES) $(LIBS)
		$(SQUEEZE) -f -v $@

.c.o:;	$(CC) $(CFLAGS) $< -o $@
.s.o:;	objasm $(ObjAsmflags) -from $< -to $@
