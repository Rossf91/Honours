ifndef    PROG
PROG	  = ext_arith
endif

STARTUP   = vectors.o kernel.o
LD	  = arc-linux-uclibc-ld
AS	  = arc-linux-uclibc-as

LD_SCRIPT = link_script

AS_FLAGS  = -mA7 -mnorm -mswap -mmin_max -mEA
LD_FLAGS  = -T ${LD_SCRIPT} --print-map

${PROG}: ${PROG}.o ${STARTUP} ${LD_SCRIPT}
	${LD} ${LD_FLAGS} -o ${PROG} ${PROG}.o ${STARTUP} > ${PROG}.map

vectors.o: vectors.s
	${AS} ${AS_FLAGS} -o vectors.o vectors.s

kernel.o:  kernel.s
	${AS} ${AS_FLAGS} -o kernel.o kernel.s

${PROG}.o: ${PROG}.s
	${AS} ${AS_FLAGS} -o $@ $<

clean:
	rm -f ${PROG} *.o a.out *.map
