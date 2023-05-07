/*****************************************************************************

Filename:   include/proxy.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/include/proxy.h,v 1.3.4.9 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: proxy.h,v $
 Revision 1.3.4.9  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.8  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3.4.7  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/08/25 18:19:44  hbray
 *** empty log message ***

 Revision 1.3.4.4  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3.4.3  2011/08/18 19:28:53  hbray
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

#ident "@(#) $Id: proxy.h,v 1.3.4.9 2011/10/27 18:33:54 hbray Exp $ "


#ifndef _PROXY_H
#define _PROXY_H


typedef enum
{
	ProxyReqHostOnline			= 0,
	ProxyReqHostOffline			= 1,
	ProxyReqHostMsg				= 2
} ProxyReqType_t ;

extern char *ProxyReqTypeToString(ProxyReqType_t type);


typedef enum
{
	ProxyTTLNone				= 0
	// ProxyTTLValue			> 0
} ProxyTTL_t ;

extern char *ProxyTTLToString(ProxyTTL_t type);


typedef enum
{
	ProxyFlowNone				= 0,
	ProxyFlowSequential			= 1,
	ProxyFlowPersistent			= 2
} ProxyFlowType_t ;

extern char *ProxyFlowToString(ProxyFlowType_t type);




typedef struct ProxyRequest_t
{
	ProxyReqType_t		reqType;
	ProxyFlowType_t		flowType;
	int					replyTTL;				// =0 if no reply is expected, otherwise, time in seconds
	int					servicePort;
	NxUid_t				uid;

	int					pid;			// sender's pid

// might be less than sizeof hostReq
//     See ProxyRequestNewSized()
	int					maxDataSizeAllowed;

// hostReq is optional; present when reqType == ProxyReqHostMsg
	HostRequest_t		hostReq;
} ProxyRequest_t;

#define ProxyRequestNew() ProxyRequestNewSized(MaxHostRequestDataLen)
#define ProxyRequestNewSized(size) ObjectNewSized(ProxyRequest, (0-(MaxHostRequestDataLen))+(size))	// reduce to size
#define ProxyRequestVerify(var) ObjectVerify(ProxyRequest, var)
#define ProxyRequestDelete(var) ObjectDelete(ProxyRequest, var)
#define ProxyRequestLen(rq) ((sizeof(ProxyRequest_t)-sizeof((rq)->hostReq)) + HostRequestLen(&(rq)->hostReq))

extern ProxyRequest_t* ProxyRequestConstructor(ProxyRequest_t *this, char *file, int lno, int size);
extern void ProxyRequestDestructor(ProxyRequest_t *this, char *file, int lno);
extern char *ProxyReqTypeToString(ProxyReqType_t type);
extern BtNode_t* ProxyRequestNodeList;
extern struct Json_t* ProxyRequestSerialize(ProxyRequest_t *this, OutputType_t outputType);
extern char* ProxyRequestToString(ProxyRequest_t *this, OutputType_t outputType);
static void inline ProxyRequestClear(ProxyRequest_t *this) { ProxyRequestVerify(this); memset(this, 0, sizeof(*this));}


typedef struct ProxyQueue_t
{
	boolean				cleartosend;
	NxTime_t			lastProxySendTime;	// MS timer for detecting host send stall's
	NxTime_t			lastProxyRecvTime;	// MS time
	unsigned int		nbrInflights;
	ObjectList_t		*outputQueue;		// ObjectList_t* of ProxyRequest_t*
	ObjectList_t		*inputQueue;		// ObjectList_t* of ProxyRequest_t*

	int (*send)			(ProxyRequest_t *proxyReq);
} ProxyQueue_t ;

#define ProxyQueueNew() ObjectNew(ProxyQueue)
#define ProxyQueueVerify(var) ObjectVerify(ProxyQueue, var)
#define ProxyQueueDelete(var) ObjectDelete(ProxyQueue, var)

extern ProxyQueue_t* ProxyQueueConstructor(ProxyQueue_t *this, char *file, int lno);
extern void ProxyQueueDestructor(ProxyQueue_t *this, char *file, int lno);
extern int ProxyQueuePump(ProxyQueue_t *this);
extern int ProxyQueueClear(ProxyQueue_t *this);
extern int ProxyQueueAddInputRequest(ProxyQueue_t *this, ProxyRequest_t *pokeReq);
extern int ProxyQueueAddOutputRequest(ProxyQueue_t *this, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq, NxClient_t *client, int servicePort);
extern BtNode_t* ProxyQueueNodeList;
extern struct Json_t* ProxyQueueSerialize(ProxyQueue_t *this);
extern char* ProxyQueueToString(ProxyQueue_t *this);


// Proxy Context Implementation

typedef struct ProxyContextEntry_t
{
	struct ProxyContextMap_t	*map;
	NxUid_t						uid;
	long						ttl;				// in seconds
	NxTime_t					expiration;
	void						*vectors[];			// array of map->nbrVectors; at least one
} ProxyContextEntry_t ;

#define ProxyContextEntryVerify(var) ObjectVerify(ProxyContextEntry, var)
#define ProxyContextEntryDelete(var) ObjectDelete(ProxyContextEntry, var)
extern void ProxyContextEntryDestructor(ProxyContextEntry_t *this, char *file, int lno);
extern BtNode_t* ProxyContextEntryNodeList;
extern struct Json_t* ProxyContextEntrySerialize(ProxyContextEntry_t *this);
extern char* ProxyContextEntryToString(ProxyContextEntry_t *this);


typedef struct ProxyContextMap_t
{
	HashMap_t				*map; // a hash of ProxyContextEntry_t*; keyed by UID
	ObjectList_t			*timerList;		// Time orderlist list of ProxyContextEntry_t*'s in above map
	int						size;
	int						nbrVectors;
	void					(*collector)(ProxyContextEntry_t *entry);
	int						ageInterval;
	Timer_t					*ageTimer;
} ProxyContextMap_t ;

#define ProxyContextMapNew(size, ageInterval, nbrVectors, collector) ObjectNew(ProxyContextMap, size, ageInterval, nbrVectors, collector)
#define ProxyContextMapVerify(var) ObjectVerify(ProxyContextMap, var)
#define ProxyContextMapDelete(var) ObjectDelete(ProxyContextMap, var)

extern ProxyContextMap_t* ProxyContextMapConstructor(ProxyContextMap_t *this, char *file, int lno, int size, int ageInterval, int nbrVectors, void (*collector)(ProxyContextEntry_t *entry));
extern void ProxyContextMapDestructor(ProxyContextMap_t *this, char *file, int lno);
extern BtNode_t* ProxyContextMapNodeList;
extern struct Json_t* ProxyContextMapSerialize(ProxyContextMap_t *this);
extern char* ProxyContextMapToString(ProxyContextMap_t *this);

#define ProxyContextNew(this, ttl, uid) _ProxyContextNew(this, __FILE__, __LINE__, __FUNC__, ttl, uid)				// ttl in seconds
extern ProxyContextEntry_t* _ProxyContextNew(ProxyContextMap_t *this, char *file, int lno, char *func, long ttl, NxUid_t uid);	// ttl in seconds
extern void ProxyContextDelete(ProxyContextMap_t *this, ProxyContextEntry_t *entry);
extern void ProxyContextDeleteByVectorValue(ProxyContextMap_t *this, int vn, void *value);
extern ProxyContextEntry_t* ProxyContextFind(ProxyContextMap_t *this, NxUid_t uid);

#endif
