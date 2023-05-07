BLDDIR=build
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	$(error 64 Bit Not Supported)
endif

MKDIR=mkdir
RM=rm
CP=cp

#
# Make libs
#

all: ${BLDDIR}/libgmp.a ${BLDDIR}/libsqlite3.a ${BLDDIR}/libuuid.a ${BLDDIR}/libxml2.a

${BLDDIR}/libgmp.a: libgmp.${LBITS}.a
	${MKDIR} -p ${BLDDIR}
	${CP} $< $@

${BLDDIR}/libsqlite3.a: libsqlite3.${LBITS}.a
	${MKDIR} -p ${BLDDIR}
	${CP} $< $@

${BLDDIR}/libuuid.a: libuuid.${LBITS}.a
	${MKDIR} -p ${BLDDIR}
	${CP} $< $@

${BLDDIR}/libxml2.a: libxml2.${LBITS}.a
	${MKDIR} -p ${BLDDIR}
	${CP} $< $@

clean:
	${RM} -rf ${BLDDIR}
