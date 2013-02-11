#                          Copyright Notice
#
#    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
#  The University Court of the University of Edinburgh. All Rights Reserved.
#
# =============================================================================

ifndef PROG
PROG := $(basename $(wildcard *.s))
endif

ELF2HEX = elf2hex

PBIN  = $(addsuffix .x, ${PROG})
BOBJ  = ../bare/vector.o ../bare/bare.o

LD    = arc-elf32-ld
AS    = arc-elf32-as

LD_SCRIPT = ../bare/link_bare

AS_FLAGS  = -mA7 -mmin_max -mEA
LD_FLAGS  = -static -T ${LD_SCRIPT} --print-map

all: ${BOBJ} ${LD_SCRIPT} ${PBIN}

%.x: %.o ${BOBJ}
	${LD} ${LD_FLAGS} -o $@ $< ${BOBJ} > $@.map

%.o: %.s
	${AS} ${AS_FLAGS} -o $@ $<

clean:
	rm -f ${PBIN} *.o ${BOBJ} a.out *.map

disp:
	echo ${PROG}
