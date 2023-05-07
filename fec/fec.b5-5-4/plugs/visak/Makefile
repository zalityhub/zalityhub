PLUGDIR=../build
BLDDIR=build
LIB=${PLUGDIR}/libvisak.so

MKDIR=mkdir
RM=rm
CFLAGS+=-g -c -O0 -Wall -Wno-strict-aliasing -std=gnu99 -fPIC -I/usr/local/include -I../.. -I../../include -I. -MMD -MP 
CC=gcc ${CFLAGS}
LINK=gcc ${LFLAGS}
CP=cp

${BLDDIR}/%.o: %.c Makefile
	${MKDIR} -p ${BLDDIR}
	${RM} -f $@.d
	${CC} -MF $@.d -o $@ $<

#
# Make libvisak
#
OBJS= \
	${BLDDIR}/visakreadreq.o \
	${BLDDIR}/visaksendrsp.o \
	${BLDDIR}/beginsession.o \
	${BLDDIR}/data.o \
	${BLDDIR}/endsession.o \
	${BLDDIR}/load.o \
	${BLDDIR}/tostring.o \
	${BLDDIR}/unload.o

all: ${LIB}

clean:
	${RM} -rf ${BLDDIR} ${LIB}

${LIB}: ${OBJS} Makefile
	${MKDIR} -p ${PLUGDIR}
	${LINK} -shared -o $@ -fPIC ${OBJS}

DEPFILES=$(wildcard $(addsuffix .d, ${OBJS}))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
