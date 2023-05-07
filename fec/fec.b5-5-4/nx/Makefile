BLDDIR=build

RM=rm
MKDIR=mkdir
CFLAGS+=-g -c -O0 -Wall -Wno-strict-aliasing -std=gnu99 -I/usr/local/include/ -I.. -I../include -I. -MMD -MP -export-dynamic -rdynamic
CC=gcc ${CFLAGS}

#
# Make nx objects
#

NX_OBJS=bt.o \
	cJSON.o \
	command.o \
	counts.o \
	crc.o \
	datagram.o \
	des_crypt.o \
	des_impl.o \
	deskeys.o \
	dumpmem.o \
	eventlib.o \
	fecconfig.o \
	fifo.o \
	file.o \
	filefifo.o \
	fsmlib.o \
	fullxml.o \
	hashlib.o \
	hostio.o \
	hostrequest.o \
	http.o \
	iniparse.o \
	instrument.o \
	isoutil.o \
	itoa.o \
	json.o \
	leaklib.o \
	locklib.o \
	logwriter.o \
	lrc.o \
	memory.o \
	nx.o \
	nxregex.o \
	nxsignal.o \
	pair.o \
	objlib.o \
	parseobj.o \
	parsexml.o \
	pisession.o \
	proclib.o \
	proxy.o \
	random64.o \
	random.o \
	sdcdes.o \
	server.o \
	setproctitle.o \
	socklib.o \
	spin.o \
	sql.o \
	stack.o \
	stringobj.o \
	sysloglib.o \
	timerlib.o \
	utillib.o \
	writef.o
OBJS=$(addprefix ${BLDDIR}/,${NX_OBJS})
all: ${OBJS} Makefile


clean:
	${RM} -rf ${BLDDIR}


$(BLDDIR)/%.o: %.c Makefile
	${MKDIR} -p ${BLDDIR}
	${RM} -f $@.d
	${CC} -MF $@.d -o $@ $<


DEPFILES=$(wildcard $(addsuffix .d, ${OBJS}))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
