# Project:	Hpack

.SUFFIXES:	.c .o .s
CC			= cc
CFLAGS		= -c -D__ARC__ -DARCHIVE_TYPE=3 -depend !Depend -IC:,@,ASN1,CRC,CRYPT,DATA,IO,LANGUAGE,LZA,STORE
ObjAsmflags	= -Stamp -NoCache -CloseExec -Quit
LINK		= link
LINKFLAGS	= -o $@
SQUEEZE		= squeeze
SQUEEZEFLAGS	= -o $@

FILES =	arcdir.o arcdirio.o archive.o cli.o error.o filesys.o frontend.o \
		script.o sql.o tags.o viewfile.o wildcard.o asn1.asn1.o \
		asn1.memmgr.o asn1.pemcode.o asn1.pemparse.o crc.crc16.o \
		crypt.crypt.o crypt.keymgmt.o crypt.md5.o crypt.mdc.o crypt.rsa.o \
		crypt.rsa_arc.o crypt.shs.o io.fastio.o io.display.o io.stream.o \
		language.language.o lza.lza.o lza.model.o lza.model2.o lza.model3.o \
		lza.model4.o lza.pack.o lza.unpack.o store.store.o system.arc.o

LIBS =	C:o.stubs

all:	hpack hpack_lang

hpack:	$(FILES)
		$(LINK) $(LINKFLAGS) $(FILES) $(LIBS)
		$(SQUEEZE) -f -v $@

.c.o:;	$(CC) $(CFLAGS) $< -o $@
.s.o:;	objasm $(ObjAsmflags) -from $< -to $@

hpack_lang:	language_d
			copy language_d hpack_lang
