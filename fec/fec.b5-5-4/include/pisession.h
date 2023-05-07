/*****************************************************************************

Filename:   include/piprofile.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/pisession.h,v 1.3.4.11 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: pisession.h,v $
 Revision 1.3.4.11  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.10  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3.4.8  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3.4.5  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.3  2011/08/17 17:58:57  hbray
 *** empty log message ***

 Revision 1.3.4.2  2011/08/11 19:47:32  hbray
 Many changes

 Revision 1.3.4.1  2011/08/01 16:11:28  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: pisession.h,v 1.3.4.11 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _PISESSION_H
#define _PISESSION_H

typedef struct PiContext_t
{
	struct PiSession_t	*sess;				// the owner
	NxUid_t				uid;				// A unique ID
	int					dataLen;
	void				*data;				// context area
} PiContext_t ;


typedef struct PiSession_t
{
	struct
	{
		NxTime_t		connectTime;		// MS

		PiProfile_t		*plugin;

		ProxyQueue_t	*t70Queue;
		ProxyQueue_t	*authQueue;
		ProxyQueue_t	*edcQueue;

		Fifo_t			*inputFifo;				// a Fifo_t*
		NxTime_t		lastPosRecvTime;	// MS timer for detecting POS recv stall's

		struct NxClient_t *client;			// The POS client

		PiApiResult_t	finalDisposition;
		ObjectList_t	*auditList;			// a ObjectList_t* of AuditXml_t*

		char			logPrefix[128];

		HashMap_t		*contextList;		// list of PiContext_t*
		HashMap_t		*batchList;			// list of FileFifo_t*
	} _priv;


	// Actual plugin members here:
	struct
	{
		char					*sessionDir;	// pointer to this sessions 'homedir'
		PiContext_t				*context;

		FecService_t			service;
		boolean					atEof;

		int						replyTTL;		// default value
	} pub;
} PiSession_t;


typedef struct PiPeekBfr_t
{
	boolean		atEof;
	int			len;
	char		*bfr;
} PiPeekBfr_t;


#define PiException(level, api, disposition, fmt, ...) _PiException(sess, level, api, disposition, true, __FILE__, __LINE__, __FUNC__, "%s: " fmt, PiSessionGetName(sess), ##__VA_ARGS__)
extern void _PiException(PiSession_t *sess, SysLogLevel lvl, char *api, PiApiResult_t disposition, boolean doLog, char *file, int lno, char *fnc, char *fmt, ...);


#if 0
static inline boolean PiAuditIsEnabled(int event)
{
	return (AuditIsEnabled(event) || NxGlobal->auditSessions);
};

#define PiAuditSendEvent(event, disposition, ...) PiAuditSendEventFull(event, __FILE__, __LINE__, __FUNC__, disposition, NULL, 0, ##__VA_ARGS__)
#define PiAuditSendEventData(event, disposition, data, len, ...) PiAuditSendEventFull((event), __FILE__, __LINE__, __FUNC__, disposition, data, len, ##__VA_ARGS__)
#define PiAuditSendEventFull(event, file, lno, fnc, disposition, data, len, ...) PiAuditIsEnabled(event)?_PiAuditSendEvent(sess, AuditGlobal, (event), file, lno, fnc, disposition, data, len, va_args_toarray(__VA_ARGS__)):(void)0
extern int _PiAuditSendEvent(PiSession_t*, Audit_t*, AuditEvent_t event, char *file, int lno, char *fnc, PiApiResult_t disposition, void *data, int len, void **str, int npairs);

#else
#define PiAuditIsEnabled(event) (false)
#define PiAuditSendEvent(event, disposition, ...)
#define PiAuditSendEventData(event, disposition, data, len, ...)
#define PiAuditSendEventFull(event, file, lno, fnc, disposition, data, len, ...)
#endif



#define PiSessionNew(service, client) ObjectNew(PiSession, service, client)
#define PiSessionVerify(var) ObjectVerify(PiSession, var)
#define PiSessionDelete(var) ObjectDelete(PiSession, var)

extern PiSession_t* PiSessionConstructor(PiSession_t *this, char *file, int lno, struct FecService_t *service, NxClient_t *client);
extern void PiSessionDestructor(PiSession_t *this, char *file, int lno);
extern BtNode_t* PiSessionNodeList;
extern struct Json_t* PiSessionSerialize(PiSession_t *this);
extern char* PiSessionToString(PiSession_t *this);


// TODO: Need abstraction... or something
// These functions are for the worker process; not plugins...
//
extern int PiPosPoke(PiSession_t *sess, char *bfr, int len);
extern PiApiResult_t PiBeginSession(PiSession_t *sess);
extern PiApiResult_t PiEndSession(PiSession_t *sess);
extern PiApiResult_t PiReadRequest(PiSession_t *sess);
extern PiApiResult_t PiSendResponse(PiSession_t *sess);



// Plugin Callable functions follow
//

extern void PiSessionSetLogPrefix(PiSession_t *this);
extern NxUid_t PiBatchOpen(PiSession_t *sess, int contextLen);
extern void* PiBatchGetContext(PiSession_t *sess, NxUid_t batchId);
extern int PiBatchPush(PiSession_t *sess, char *bfr, int len, NxUid_t batchId);
extern unsigned int PiBatchSize(PiSession_t *sess, NxUid_t batchId);
extern int PiBatchPosSend(PiSession_t *sess, NxUid_t batchId);
extern char* PiBatchToString(PiSession_t *sess, NxUid_t batchId);
extern int PiBatchClose(PiSession_t *sess, NxUid_t batchId);

extern int PiPosSend(PiSession_t *sess, char *bfr, int len);
extern int PiPosRecv(PiSession_t *sess, char *bfr, int len);
extern int PiPosUrlRecv(PiSession_t *sess, char *bfr, int len);
extern int PiPosEntityRecv(PiSession_t *sess, char *bfr, int len);
extern boolean PiPosPeek(PiSession_t *sess, PiPeekBfr_t *peekbfr);
extern int PiHostSend(PiSession_t *sess, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq);
extern int PiHostSendServicePort(PiSession_t *sess, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq, int servicePort);
extern int PiHostRecv(PiSession_t *sess, HostRequest_t *hostReq);
extern PiContext_t* PiCreateContext(PiSession_t *sess, int contextLen);
extern int PiDeleteContext(PiSession_t *sess, PiContext_t *context);
extern PiContext_t* PiGetContext(PiSession_t *sess, NxUid_t uid);
extern BtNode_t* PiContextNodeList;
extern struct Json_t* PiContextSerialize(PiContext_t *sess);
extern char* PiContextToString(PiContext_t *sess);
extern char* PiCreateSessionDir(PiSession_t *sess);

static inline PiSession_t* PiGetSession(NxClient_t *client) { NxClientVerify(client); return ((PiSession_t*)((PiSession_t*) (client->evf->context))); }
static inline FecService_t* PiGetService(PiSession_t *sess) { PiSessionVerify(sess); return ((FecService_t*) (&sess->pub.service)); }
static inline NxClient_t* PiGetClient(PiSession_t *sess) { PiSessionVerify(sess); return (sess->_priv.client); }

static inline void PiSessionSetEof(PiSession_t *sess, boolean value) { PiSessionVerify(sess); sess->pub.atEof = value; };
static inline boolean PiSessionAtEof(PiSession_t *sess) { PiSessionVerify(sess); return sess->pub.atEof; };
static inline char* PiSessionGetName(PiSession_t *sess) { return NxClientNameToString(PiGetClient(sess)); };

#define PiCallMethod(sess, method) ((*((sess)->_priv.plugin->api.methods.entry.method))(sess))


extern char* PiGetPropertyValue(PiSession_t *sess, char *propName, ...);
#define PiGetPropertyIntValue(propName, ...) CnvStringIntValue(PiGetPropertyValue(propName, ##__VA_ARGS__))
#define PiGetPropertyBooleanValue(propName, ...) CnvStringBooleanValue(PiGetPropertyValue(propName, ##__VA_ARGS__))

#endif
