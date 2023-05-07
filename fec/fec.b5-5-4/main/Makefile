LN=ln
MKDIR=mkdir
RM=rm
CFLAGS+=-g -c -O0 -Wall -Wno-strict-aliasing -std=gnu99 -I/usr/local/include/ -I.. -I. -MMD -MP -export-dynamic -rdynamic
CHMOD=chmod
TR=tr
MV=mv
CC=gcc ${CFLAGS}
LINK=gcc -export-dynamic -rdynamic -lpthread
CP=cp
CD=cd
CUT=cut

BLDDIR=build
DISTDIR=dist/LinuxDebug/GNU-Linux-x86
NXDIR=../nx
PLUGDIR=../plugs
LIBDIR=../lib
PLUGDIST=plugs
LOGDIR=logs

LIBSOPTIONS=-ldl -lm -Wl,-rpath ${PLUGDIST}

LINK_TARGETS=./fec \
			./gi \
			./ki \
			./li \
			./si \
			./rs \
			./er \
			./fcmd \
			./dumpin \
			./sockstat \
			./jts \
			./rtrace \
			./splitapi \
			./tellapi

DIST_TARGETS=${DISTDIR}/fec \
			${DISTDIR}/sqlite \
			${DISTDIR}/sockstat \
			${DISTDIR}/jts \
			${DISTDIR}/fcmd \
			${DISTDIR}/dumpin \
			${DISTDIR}/rtrace \
			${DISTDIR}/splitapi \
			${DISTDIR}/fec.sh \
			${DISTDIR}/awkit.sh \
			${DISTDIR}/fec.xml \
			${DISTDIR}/nx.xml \
			${DISTDIR}/version.seq

all: ${DIST_TARGETS} ${LINK_TARGETS}

clean: dist_clean link_clean


#
# Make Dist
#

# Etc...
#
dist_clean: fec_clean
	${RM} -rf ${DISTDIR} ${BLDDIR}

${DISTDIR}/version.seq: version.seq Makefile
	${MKDIR} -p ${DISTDIR}
	${CUT} -d= -f2 version.seq > $@

${DISTDIR}/fec.xml: fec.xml
	${MKDIR} -p ${DISTDIR}
	${TR} -d \\r < $< > $@

${DISTDIR}/nx.xml: nx.xml
	${MKDIR} -p ${DISTDIR}
	${TR} -d \\r < $< > $@

${DISTDIR}/awkit.sh: awkit.sh
	${MKDIR} -p ${DISTDIR}
	${TR} -d \\r < $< > $@
	${CHMOD} +x $@

${DISTDIR}/fec.sh: fec.sh
	${MKDIR} -p ${DISTDIR}
	${TR} -d \\r < $< > $@
	${CHMOD} +x $@

# Executables
#
# Make sqlite
#
${DISTDIR}/sqlite: sqlite
	${MKDIR} -p ${DISTDIR}
	${CP} -pf $< $@

# Make fcmd
#
FCMD_OBJS=${BLDDIR}/fcmd.o

${DISTDIR}/fcmd: ${FCMD_OBJS}
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${FCMD_OBJS}

# Make sockstat
#
SOCKSTAT_OBJS=${BLDDIR}/sockstat.o

${DISTDIR}/sockstat: ${SOCKSTAT_OBJS}
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${SOCKSTAT_OBJS}

# Make Json To String
#
JTS_OBJS=${BLDDIR}/jts.o

${DISTDIR}/jts: ${JTS_OBJS} ${NXDIR}/${BLDDIR}/cJSON.o
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${JTS_OBJS} ${NXDIR}/${BLDDIR}/cJSON.o


#
# Make rtrace
#
RTRACE_OBJS=${BLDDIR}/rtrace.o

${DISTDIR}/rtrace: ${RTRACE_OBJS}
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${RTRACE_OBJS}


#
# Make dumpin
#
DUMPIN_OBJS=${BLDDIR}/dumpin.o

${DISTDIR}/dumpin: ${DUMPIN_OBJS}
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${DUMPIN_OBJS}


#
# Make splitapi
#
SPLITAPI_OBJS=${BLDDIR}/splitapi.o

${DISTDIR}/splitapi: ${SPLITAPI_OBJS}
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${SPLITAPI_OBJS}


#
# Make tellapi
#
TELLAPI_OBJS=${BLDDIR}/tellapi.o

${DISTDIR}/tellapi: ${TELLAPI_OBJS}
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${TELLAPI_OBJS}


#
# Make fec
#
%.o: %.c Makefile
	${MKDIR} -p $(BLDDIR)
	${RM} -f $@.d
	${CC} -DNX_VERSION=\"`cat version.seq`\" -MF $@.d -o $@ $<

${BLDDIR}/%.o: %.c Makefile
	${MKDIR} -p $(BLDDIR)/machine
	${RM} -f $@.d
	${CC} -DNX_VERSION=\"`cat version.seq`\" -MF $@.d -o $@ $<

FEC_LIBS=-lgmp -lsqlite3 -lxml2 -luuid
FEC_OBJS=${BLDDIR}/machine/root.o \
	${BLDDIR}/machine/stratusproxy.o \
	${BLDDIR}/machine/hostsim.o \
	${BLDDIR}/machine/service.o \
	${BLDDIR}/machine/worker.o \
	${BLDDIR}/fec.o

fec_clean:
	${RM} -f fec.history fec.log gmon.out* core.* core -rf ${PLUGDIST} ${LOGDIR}
	${CD} ${PLUGDIR} && ${MAKE} -f Makefile clean
	${CD} ${LIBDIR} && ${MAKE} -f Makefile clean
	${CD} ${NXDIR} && ${MAKE} -f Makefile clean

${DISTDIR}/fec: ${FEC_OBJS} plugs nx lib
	${MKDIR} -p ${DISTDIR}
	${LINK} ${LIBSOPTIONS} -o $@ ${FEC_OBJS} ${NXDIR}/${BLDDIR}/*.o -L${LIBDIR}/${BLDDIR} ${FEC_LIBS}


plugs:
	${CD} ${PLUGDIR} && ${MAKE} -f Makefile
	${MKDIR} -p ${PLUGDIST}
	${MKDIR} -p ${DISTDIR}/${PLUGDIST}
	${CP} -cpf ${PLUGDIR}/${BLDDIR}/* ${DISTDIR}/${PLUGDIST}
	${CD} ${PLUGDIST} && ${LN} -s ../${DISTDIR}/${PLUGDIST}/* .

nx:
	${CD} ${NXDIR} && ${MAKE} -f Makefile

lib:
	${CD} ${LIBDIR} && ${MAKE} -f Makefile


#
# Make links
#
link_clean:
	${RM} -f ${LINK_TARGETS}

./fcmd: ${DISTDIR}/fcmd Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./sockstat: ${DISTDIR}/sockstat Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./jts: ${DISTDIR}/jts Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./rtrace: ${DISTDIR}/rtrace Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./dumpin: ${DISTDIR}/dumpin Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./splitapi: ${DISTDIR}/splitapi Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./tellapi: ${DISTDIR}/splitapi Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./fec: ${DISTDIR}/fec Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./si: ${DISTDIR}/fec.sh Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./rs: ${DISTDIR}/fec.sh Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./er: ${DISTDIR}/fec.sh Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./ki: ${DISTDIR}/fec.sh Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./gi: ${DISTDIR}/fec.sh Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@

./li: ${DISTDIR}/fec.sh Makefile
	${MKDIR} -p ${DISTDIR}
	${RM} -f $@
	${LN} -s $< $@


DEPFILES=$(wildcard $(addsuffix .d, ${FEC_OBJS} ${FCMD_OBJS} ${DUMPIN_OBJS} ${SPLITAPI_OBJS} ${TELLAPI_OBJS}))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
