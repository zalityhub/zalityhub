/*****************************************************************************

Filename:   include/auditlib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:35 $
 * $Header: /home/hbray/cvsroot/fec/include/auditlib.h,v 1.3.4.4 2011/09/24 17:49:35 hbray Exp $
 *
 $Log: auditlib.h,v $
 Revision 1.3.4.4  2011/09/24 17:49:35  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/23 19:53:57  hbray
 eliminate fecplugin.h

 Revision 1.3.4.2  2011/08/23 12:03:13  hbray
 revision 5.5

 Revision 1.3.4.1  2011/08/15 19:12:31  hbray
 5.5 revisions

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:33  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: auditlib.h,v 1.3.4.4 2011/09/24 17:49:35 hbray Exp $ "


#ifndef _AUDITLIB_H
#define _AUDITLIB_H

#include <regex.h>


typedef enum
{
	AuditFirst,

	AuditSystemStarted,
	AuditSystemStopped,
	AuditSystemWarning,
	AuditSystemError,

	AuditCmdInput,
	AuditCmdResponse,
	
	AuditNxStarted,

	AuditProcStarted,
	AuditProcStopped,

	AuditProcCreated,
	AuditProcNewState,
	AuditFsmEvent,

	AuditPiException,
	AuditPiApiCall,
	AuditPiPosSend,
	AuditPiPosRecv,
	AuditPiHostSend,
	AuditPiHostRecv,
	AuditPiPosReadEnabled,
	AuditPiPosReadDisabled,

	AuditWorkerSend,
	AuditWorkerRecv,

	AuditStratusConnect,
	AuditStratusDisconnect,
	AuditStratusProxyOnline,
	AuditStratusProxyOffline,
	AuditStratusProxySend,
	AuditStratusProxyRecv,

	AuditT70Connect,
	AuditT70Disconnect,
	AuditT70ProxyOnline,
	AuditT70ProxyOffline,
	AuditT70ProxySend,
	AuditT70ProxyRecv,

	AuditHostConnect,
	AuditHostDisconnect,
	AuditHostConfigStart,
	AuditHostConfigComplete,
	AuditHostSend,
	AuditHostRecv,
	AuditHostPingRecv,

	AuditPosConnect,
	AuditPosForwardSend,
	AuditPosForwardReceived,
	AuditPosSend,
	AuditPosRecv,
	AuditPosDisconnect,

	AuditStartService,

	AuditSignalSent,
	AuditSignalRecv,

	AuditLast
} AuditEvent_t;

#if 0

typedef struct AuditFilter_t
{
	struct AuditConnection_t	*connection;
	boolean						compiled;
	regex_t						regex;
} AuditFilter_t;


typedef struct AuditConnection_t
{
	int						enableList[AuditLast + 1];

	struct Proc_t			*proc;
	struct NxClient_t		*client;

	// Hash list of audit filters
	// List of AuditEventFilter_t* keyed by audit level name (string)
	HashMap_t				*auditFilterList;
} AuditConnection_t;


#define AuditGlobal NxGlobal->audit


// Audit Counts

typedef struct AuditCounts_t
{
	NxCount_t	counts[AuditLast + 1];
} AuditCounts_t ;

#define AuditCountsNew() ObjectNew(AuditCounts)
#define AuditCountsVerify(var) ObjectVerify(AuditCounts, var)
#define AuditCountsDelete(var) ObjectDelete(AuditCounts, var)

extern AuditCounts_t* AuditCountsConstructor(AuditCounts_t *this, char *file, int lno);
extern void AuditCountsDestructor(AuditCounts_t *this, char *file, int lno);
extern char* AuditCountsToString(AuditCounts_t *this);
extern struct Json_t* AuditCountsSerialize(AuditCounts_t *this);

static inline void AuditCountInc(AuditEvent_t event) {if ( (event) > AuditFirst && (event) < AuditLast ) ++NxGlobal->auditCounts->counts[event];};
static inline void AuditCountDec(AuditEvent_t event) {if ( (event) > AuditFirst && (event) < AuditLast ) --NxGlobal->auditCounts->counts[event];};


// Audit XML
typedef struct AuditXml_t
{
	time_t			dateCreated;
	AuditEvent_t	event;
	int				disposition;
	String_t		*xml;
} AuditXml_t ;

#define AuditXmlNew() ObjectNew(AuditXml)
#define AuditXmlVerify(var) ObjectVerify(AuditXml, var)
#define AuditXmlDelete(var) ObjectDelete(AuditXml, var)

extern AuditXml_t* AuditXmlConstructor(AuditXml_t *this, char *file, int lno);
extern void AuditXmlDestructor(AuditXml_t *this, char *file, int lno);
extern struct Json_t* AuditXmlSerialize(AuditXml_t *this);
extern char* AuditXmlToString(AuditXml_t *this);



// Audit

typedef struct Audit_t
{
	int				auditDay;			// for session exceptions
	int				auditFd;			// for session exceptions
	struct stat		auditStat;			// audit file stats

	HashMap_t		*auditConnections;	// A hash of AuditConnection_t*
	int				enableList[AuditLast + 1];	// composite of all enableList in auditConnections
} Audit_t;


#define AuditNew() ObjectNew(Audit)
#define AuditVerify(var) ObjectVerify(Audit, var)
#define AuditDelete(var) ObjectDelete(Audit, var)

extern Audit_t* AuditConstructor(Audit_t *this, char *file, int lno);
extern void AuditDestructor(Audit_t *this, char *file, int lno);
extern char* AuditToString(Audit_t *this);
extern struct Json_t* AuditSerialize(Audit_t *this);


// External Functions

#define AuditEnableEventMonitor(parser, connection, resonse) _AuditEnableEventMonitor(AuditGlobal, parser, connection, response)
extern AuditConnection_t *_AuditEnableEventMonitor(Audit_t *this, Parser_t *parser, NxClient_t *client, String_t *response);



static inline boolean AuditIsEnabled(AuditEvent_t event)
{
	AuditCountInc(event);
	return ( (event>AuditFirst && event<AuditLast && AuditGlobal != NULL && AuditGlobal->enableList[event]) );
};

#define AuditSendEvent(event, ...) AuditSendEventFull((event), __FILE__, __LINE__, __FUNC__, NULL, 0, ##__VA_ARGS__)
#define AuditSendEventData(event, data, len, ...) AuditSendEventFull((event), __FILE__, __LINE__, __FUNC__, data, len, ##__VA_ARGS__)

#define AuditSendEventFull(event, file, lno, fnc, data, len, ...) AuditIsEnabled(event)?_AuditSendEvent(AuditGlobal, (event), file, lno, fnc, data, len, va_args_toarray(__VA_ARGS__)):(void)0
extern int _AuditSendEvent(Audit_t *, AuditEvent_t event, char *file, int lno, const char *fnc, void *data, int len, void **nv, int npairs);
extern int _AuditWriteSessionError(Audit_t *this, AuditXml_t *auditXml);
extern AuditXml_t* _AuditFormatEvent(Audit_t *this, AuditEvent_t event, char *file, int lno, const char *fnc, void *data, int len, void **nv, int npairs);

extern int _AuditSendXml(Audit_t *this, AuditXml_t *xml);

extern AuditEvent_t AuditEventToVal(char *event);
extern char *AuditEventToString(AuditEvent_t event);
extern struct Json_t *AuditEventSerialize(AuditEvent_t event);

#else

#endif

typedef struct Audit_t
{
	int				dummy;
} Audit_t;

typedef struct AuditCounts_t
{
	int				dummy;
} AuditCounts_t ;

typedef struct AuditXml_t
{
	time_t			dateCreated;
	AuditEvent_t	event;
	int				disposition;
	String_t		*xml;
} AuditXml_t ;

#define AuditGlobal
#define AuditCountsNew() (NULL)
#define AuditCountsVerify(var)
#define AuditCountsDelete(var)
#define AuditXmlNew() (NULL)
#define AuditXmlVerify(var)
#define AuditXmlDelete(var)
#define AuditNew() (NULL)
#define AuditVerify(var)
#define AuditDelete(var)
#define AuditEnableEventMonitor(parser, connection, resonse) (false)
#define AuditSendEvent(event, ...)
#define AuditSendEventData(event, data, len, ...)
#define AuditSendEventFull(event, file, lno, fnc, data, len, ...)
#define AuditIsEnabled(event) (false)

#define _AuditFormatEvent(this, event, file, lno, fnc, data, len, nv, npairs) (NULL)
#define AuditEventToString(event) (NULL)
#define AuditXmlToString(this) (NULL)
#define _AuditSendXml(this, xml)
#define AuditCountDec(event)

#endif
