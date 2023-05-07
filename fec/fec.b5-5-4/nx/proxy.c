/*****************************************************************************

Filename:   lib/nx/proxy.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:58 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/proxy.c,v 1.3.4.10 2011/10/27 18:33:58 hbray Exp $
 *
 $Log: proxy.c,v $
 Revision 1.3.4.10  2011/10/27 18:33:58  hbray
 Revision 5.5

 Revision 1.3.4.9  2011/09/24 17:49:46  hbray
 Revision 5.5

 Revision 1.3.4.7  2011/09/01 14:49:46  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3.4.4  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3.4.3  2011/08/18 19:28:53  hbray
 *** empty log message ***

 Revision 1.3.4.2  2011/08/11 19:47:34  hbray
 Many changes

 Revision 1.3.4.1  2011/08/01 16:11:29  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: proxy.c,v 1.3.4.10 2011/10/27 18:33:58 hbray Exp $ "


#include "include/stdapp.h"
#include <include/libnx.h>

#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"




BtNode_t *ProxyRequestNodeList = NULL;


ProxyRequest_t*
ProxyRequestConstructor(ProxyRequest_t *this, char *file, int lno, int size)
{
	this->maxDataSizeAllowed = (MaxHostRequestDataLen)+size;	// size will always be zero or less
	return this;
}


void
ProxyRequestDestructor(ProxyRequest_t *this, char *file, int lno)
{
}


char*
ProxyReqTypeToString(ProxyReqType_t type)
{
	char *text;

	switch (type)
	{
		default:
			text = StringStaticSprintf("ProxyReqType_%d", (int)type);
			break;
		case ProxyReqHostOnline:
			text = "HostOnline";
			break;
		case ProxyReqHostOffline:
			text = "HostOffline";
			break;
		case ProxyReqHostMsg:
			text = "HostMsg";
			break;
	}
	return text;
}


char*
ProxyTTLToString(ProxyTTL_t ttl)
{
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%d", (int)ttl);
	return out->str;
}


char*
ProxyFlowToString(ProxyFlowType_t type)
{
	char *text;

	switch (type)
	{
		default:
			text = StringStaticSprintf("ProxyFlowType_%d", (int)type);
			break;
		case ProxyFlowNone:
			text = "None";
			break;
		case ProxyFlowSequential:
			text = "Sequential";
			break;
		case ProxyFlowPersistent:
			text = "Persistent";
			break;
	}
	return text;
}


Json_t*
ProxyRequestSerialize(ProxyRequest_t *this, OutputType_t outputType)
{
	ProxyRequestVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddString(root, "ReqType", ProxyReqTypeToString(this->reqType));
	JsonAddString(root, "SenderEcho", NxUidToString(this->uid));
	JsonAddNumber(root, "SenderPid", this->pid);

	if (this->reqType == ProxyReqHostMsg)
	{
		JsonAddString(root, "ReplyTTL", ProxyTTLToString(this->replyTTL));
		JsonAddString(root, "FlowType", ProxyFlowToString(this->flowType));
		JsonAddNumber(root, "ServicePort", this->servicePort);
		JsonAddItem(root, "HostRequest", HostRequestSerialize(&this->hostReq, outputType));
	}

	return root;
}


char*
ProxyRequestToString(ProxyRequest_t *this, OutputType_t outputType)
{
	Json_t *root = ProxyRequestSerialize(this, outputType);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}



BtNode_t *ProxyQueueNodeList = NULL;


ProxyQueue_t*
ProxyQueueConstructor(ProxyQueue_t *this, char *file, int lno)
{
	this->inputQueue = ObjectListNew(ObjectListVarType, "inputQueue");
	this->outputQueue = ObjectListNew(ObjectListVarType, "outputQueue");
	this->cleartosend = true;
	return this;
}


void
ProxyQueueDestructor(ProxyQueue_t *this, char *file, int lno)
{
	ProxyQueueClear(this);
	ObjectListDelete(this->inputQueue);
	ObjectListDelete(this->outputQueue);
}


int
ProxyQueueClear(ProxyQueue_t *this)
{
	ProxyQueueVerify(this);

	int result = 0;

	for (ProxyRequest_t *proxyReq; (proxyReq = (ProxyRequest_t *)ObjectListRemove(this->inputQueue, ObjectListFirstPosition)) != NULL; )
	{
		SysLog(LogWarn, "Discarding %s", ProxyRequestToString(proxyReq, DumpOutput));
		ProxyRequestDelete(proxyReq);
		result = 1;
	}

	for (ProxyRequest_t *proxyReq; (proxyReq = (ProxyRequest_t *)ObjectListRemove(this->outputQueue, ObjectListFirstPosition)) != NULL; )
	{
		SysLog(LogWarn, "Discarding %s", ProxyRequestToString(proxyReq, DumpOutput));
		ProxyRequestDelete(proxyReq);
		result = 1;
	}

	return result;
}


int
ProxyQueueAddInputRequest(ProxyQueue_t *this, ProxyRequest_t *pokeReq)
{
	ProxyQueueVerify(this);

	if (pokeReq == NULL)
		SysLog(LogFatal, "proxyReq is NULL");

	ProxyRequest_t *proxyReq = (ProxyRequest_t *)ProxyRequestNew();

	memcpy(proxyReq, pokeReq, sizeof(*proxyReq));	// copy the request

	if (proxyReq->hostReq.len > sizeof(proxyReq->hostReq.data))
	{
		SysLog(LogError, "HostRequest_t->hostReq.len of %d exceeds max allowable size %d; Truncating", pokeReq->hostReq.len, sizeof(proxyReq->hostReq.data));
		proxyReq->hostReq.len = sizeof(proxyReq->hostReq.data);
	}

	SysLog(LogDebug, "Enqueue Input HostRequest %s", ProxyRequestToString(proxyReq, NoOutput));

	if (ObjectListAdd(this->inputQueue, proxyReq, ObjectListLastPosition) == NULL)
	{
		SysLog(LogError, "ObjectListAdd of ProxyRequest_t failed");
		ProxyRequestDelete(proxyReq);
		return -1;
	}

	return 0;
}


int
ProxyQueueAddOutputRequest(ProxyQueue_t *this, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq, NxClient_t *client, int servicePort)
{

	ProxyQueueVerify(this);
	NxClientVerify(client);

// Verify the flow type

	if (flowType != ProxyFlowNone && flowType != ProxyFlowSequential && flowType != ProxyFlowPersistent)
	{
		SysLog(LogError, "flowType=%s is not valid", ProxyFlowToString(flowType));
		return -1;
	}

	ProxyRequest_t *proxyReq = (ProxyRequest_t *)ProxyRequestNew();

	proxyReq->reqType = ProxyReqHostMsg;
	proxyReq->flowType = flowType;
	proxyReq->replyTTL = replyTTL;
	proxyReq->servicePort = servicePort;
	proxyReq->pid = getpid();
	proxyReq->uid = client->evf->uid;
	memcpy(&proxyReq->hostReq, hostReq, sizeof(proxyReq->hostReq));
	memcpy(proxyReq->hostReq.hdr.peerIpAddr, client->evf->peerIpAddr, sizeof(proxyReq->hostReq.hdr.peerIpAddr));
	proxyReq->hostReq.hdr.peerIpType[0] = '4';		// ip4
	proxyReq->hostReq.hdr.peerUid = hostReq->hdr.peerUid;
	memcpy(proxyReq->hostReq.hdr.peerName, hostReq->hdr.peerName, sizeof(proxyReq->hostReq.hdr.peerName));
	proxyReq->hostReq.hdr.peerIpPort = client->evf->peerPort;

	if (proxyReq->hostReq.len > sizeof(proxyReq->hostReq.data))
	{
		SysLog(LogError, "HostRequest_t->hostReq.len of %d exceeds max allowable size %d; Truncating", proxyReq->hostReq.len, sizeof(proxyReq->hostReq.data));
		proxyReq->hostReq.len = sizeof(proxyReq->hostReq.data);
	}

// put the request on the outbound queue (the worker will send it)
// TODO: Here is where I need to 'spool' large numbers of requests... for those extra long transmissions...

	SysLog(LogDebug, "Enqueue Output HostRequest %s", ProxyRequestToString(proxyReq, NoOutput));

	if (ObjectListAdd(this->outputQueue, proxyReq, ObjectListLastPosition) == NULL)
	{
		SysLog(LogError, "ObjectListAdd of ProxyRequest_t failed");
		ProxyRequestDelete(proxyReq);
		return -1;
	}

// attempt a send

	if (ProxyQueuePump(this) < 0)
	{
		SysLog(LogWarn, "ProxyQueuePump failed");
		return -1;
	}

	return 0;
}


int
ProxyQueuePump(ProxyQueue_t *this)
{

	ProxyQueueVerify(this);

	if (this->cleartosend && ObjectListFirst(this->outputQueue) != NULL)
	{
		ProxyRequest_t *proxyReq = (ProxyRequest_t *)ObjectListRemove(this->outputQueue, ObjectListFirstPosition);

		if (proxyReq != NULL)	// have a packet
		{
			if ( this->send != NULL )
			{
				if ( (*this->send)(proxyReq) < 0 )
				{
					SysLog(LogWarn, "ProxyQueuePump failed: %s", ProxyQueueToString(this));
					ProxyRequestDelete(proxyReq);
					return -1;
				}
			}
			else
			{
				SysLog(LogWarn|SubLogDump, (char*)proxyReq, ProxyRequestLen(proxyReq), "No send method: %s", ProxyQueueToString(this));
				ProxyRequestDelete(proxyReq);
				return -1;
			}

			++this->nbrInflights;	// count as an inflight
			this->lastProxySendTime = GetMsTime();

			// If there are more packets to be sent to the host (multi-tran); then reset the clear-to-send
			// until we receive a response for this request
			if (proxyReq->hostReq.hdr.more)
			{
				SysLog(LogDebug, "set clear-to-send to false");
				this->cleartosend = false;
			}

			ProxyRequestDelete(proxyReq);
		}
	}
	else if (ObjectListFirst(this->outputQueue) != NULL)
	{
		SysLog(LogDebug, "Defer sending to host; clear-to-send is false: %s", ProxyQueueToString(this));
	}

	return 0;
}


Json_t*
ProxyQueueSerialize(ProxyQueue_t *this)
{
	ProxyQueueVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddBoolean(root, "ClearToSend", this->cleartosend);
	JsonAddString(root, "LastProxySendTime", MsTimeToStringShort(this->lastProxySendTime, NULL));
	JsonAddString(root, "LastProxyRecvTime", MsTimeToStringShort(this->lastProxyRecvTime, NULL));
	JsonAddNumber(root, "NbrInflights", this->nbrInflights);
	JsonAddNumber(root, "InputQueueSize", ObjectListNbrEntries(this->inputQueue));
	JsonAddNumber(root, "OutputQueueSize", ObjectListNbrEntries(this->outputQueue));
	return root;
}


char*
ProxyQueueToString(ProxyQueue_t *this)
{
	Json_t *root = ProxyQueueSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}



// Proxy Context Implementation

static void ProxyContextActivateTimer(ProxyContextMap_t *this);
static void ProxyContextHandleTimerEvent(Timer_t *this);


#define ProxyContextEntryNew(map, ttl, uid, file, lno, func) _ObjectNewSized(file,lno,func,ProxyContextEntry, sizeof(void**)*((map)->nbrVectors), map, ttl, uid)	// ttl in seconds


BtNode_t *ProxyContextEntryNodeList = NULL;


static ProxyContextEntry_t*
ProxyContextEntryConstructor(ProxyContextEntry_t *this, char *file, int lno, int size, ProxyContextMap_t *map, int ttl, NxUid_t uid)	// ttl in seconds
{
	ProxyContextMapVerify(map);
	this->map = map;
	if ( NxUidCompare(uid, NxUidNull) == 0 )		// no uid given, make one
		this->uid = NxUidNext();
	else
		this->uid = uid;
	this->ttl = ttl;	// ttl in seconds
	this->expiration = this->ttl + GetSecTime();
	return this;
}


void
ProxyContextEntryDestructor(ProxyContextEntry_t *this, char *file, int lno)
{
	ProxyContextMapVerify(this->map);
	if ( this->map->collector != NULL )
		(*this->map->collector) (this);
}


Json_t*
ProxyContextEntrySerialize(ProxyContextEntry_t *this)
{
	ProxyContextEntryVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Uid", NxUidToString(this->uid));
	JsonAddNumber(root, "TtlOrig", this->ttl);
	JsonAddNumber(root, "Ttl", (int)(this->expiration - GetSecTime()));
	JsonAddString(root, "Expiration", TimeToStringShort(this->expiration, NULL));
	JsonAddString(root, "CreatedBy=%s.%d", VarToObject(this)->file, VarToObject(this)->lno);
	JsonAddNumber(root, "NbrVectors", this->map->nbrVectors);
	Json_t *sub = JsonPushObject(root, "Vectors");
	for(int i = 0; i < this->map->nbrVectors; ++i)
	{
		char tmp[16];
		sprintf(tmp, "%d", i);
		JsonAddNumber(sub, tmp, (unsigned long)this->vectors[i]);
	}
	return root;
}


char*
ProxyContextEntryToString(ProxyContextEntry_t *this)
{
	Json_t *root = ProxyContextEntrySerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}




BtNode_t *ProxyContextMapNodeList = NULL;


ProxyContextMap_t*
ProxyContextMapConstructor(ProxyContextMap_t *this, char *file, int lno, int size, int ageInterval, int nbrVectors, void (*collector)(ProxyContextEntry_t *entry))
{
	this->size = size;
	this->ageInterval = ageInterval;
	this->nbrVectors = nbrVectors;
	this->collector = collector;
	this->map = HashMapNew(size, "ContextMap");
	this->timerList = ObjectListNew(ObjectListTimeType, "ActiveTimerList");

	this->ageTimer = TimerNew("AgeTimer");
	this->ageTimer->context = this;
	ProxyContextActivateTimer(this);
	return this;
}


void
ProxyContextMapDestructor(ProxyContextMap_t *this, char *file, int lno)
{
	for (ProxyContextEntry_t *ce; (ce = ObjectListRemove(this->timerList, ObjectListFirstPosition)) != NULL; )
		ProxyContextDelete(this, ce);
	HashMapDelete(this->map);
	ObjectListClear(this->timerList, false);
	ObjectListDelete(this->timerList);
	TimerDelete(this->ageTimer);
}


static void
ProxyContextActivateTimer(ProxyContextMap_t *this)
{
	ProxyContextMapVerify(this);
	if (TimerActivate(this->ageTimer, this->ageInterval*1000, ProxyContextHandleTimerEvent) != 0)
		SysLog(LogFatal, "TimerActivate failed");
}


Json_t*
ProxyContextMapSerialize(ProxyContextMap_t *this)
{
	ProxyContextMapVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Map", HashMapToString(this->map));
	JsonAddString(root, "CreatedBy", "%s.%d", VarToObject(this)->file, VarToObject(this)->lno);
	JsonAddString(root, "TimerList", ObjectListToString(this->timerList));
	JsonAddNumber(root, "Size", this->size);
	JsonAddNumber(root, "AgeInterval", this->ageInterval);
	JsonAddNumber(root, "NbrVectors", this->nbrVectors);
	JsonAddString(root, "AgeTimer", TimerToString(this->ageTimer));
	return root;
}


char*
ProxyContextMapToString(ProxyContextMap_t *this)
{
	Json_t *root = ProxyContextMapSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


ProxyContextEntry_t*
_ProxyContextNew(ProxyContextMap_t *this, char *file, int lno, char *func, long ttl, NxUid_t uid)	// ttl in seconds
{
	ProxyContextMapVerify(this);
	ProxyContextEntry_t *entry = ProxyContextEntryNew(this, ttl, uid, file, lno, func);

	if ( HashAddUid(this->map, entry->uid, entry) == NULL )
	{
		SysLog(LogError, "HashAddUid of %s failed", NxUidToString(entry->uid));
		ProxyContextEntryDelete(entry);
		entry = NULL;
	}
	if ( ObjectListAddOrdered(this->timerList, entry, &entry->expiration, 0) == NULL )
	{
		SysLog(LogError, "ObjectListAddOrdered of %s failed", NxUidToString(entry->uid));
		HashDeleteUid(this->map, entry->uid);
		ProxyContextEntryDelete(entry);
		entry = NULL;
	}
	return entry;
}


void
ProxyContextDelete(ProxyContextMap_t *this, ProxyContextEntry_t *entry)
{
	ProxyContextMapVerify(this);
	ProxyContextEntryVerify(entry);

	// remove from timerlist
	ObjectLink_t *link;
	if ( (link = ObjectListGetLink(this->timerList, entry)) != NULL )
		ObjectListYank(this->timerList, link);

	// then the hashmap
    HashEntry_t *he;
    if ( (he = HashFindUid(this->map, entry->uid)) != NULL )
        HashDeleteUid(this->map, he->key);	// remove value
	ProxyContextEntryDelete(entry);
}


void
ProxyContextDeleteByVectorValue(ProxyContextMap_t *this, int vn, void *value)
{
	ProxyContextMapVerify(this);
	if ( vn < 0 || vn >= this->nbrVectors )
		SysLog(LogFatal, "vector %d is out of range of 0 to %d", vn, this->nbrVectors-1);

	for (ObjectLink_t *ol = ObjectListFirst(this->timerList); ol != NULL; )
	{
		ProxyContextEntry_t *ce = (ProxyContextEntry_t*)ol->var;
		ProxyContextEntryVerify(ce);
		if ( ce->vectors[vn] == value )
		{
			ProxyContextDelete(this, ce);		// here's a reference
			ol = ObjectListFirst(this->timerList);	// we deleted this guy; start over
		}
		else
		{
			ol = ObjectListNext(ol);
		}
	}
}


ProxyContextEntry_t*
ProxyContextFind(ProxyContextMap_t *this, NxUid_t uid)
{
	ProxyContextMapVerify(this);

    HashEntry_t *he;
    if ( (he = HashFindUid(this->map, uid)) != NULL )
        return (ProxyContextEntry_t*)he->var;
	return NULL;
}


static void
ProxyContextHandleTimerEvent(Timer_t *this)
{
	ProxyContextMap_t *map = (ProxyContextMap_t*)this->context;
	ProxyContextMapVerify(map);

	for ( ObjectLink_t *link; (link = ObjectListFirst(map->timerList)) != NULL; )
	{
		ProxyContextEntry_t *ce = (ProxyContextEntry_t*)link->var;
		ProxyContextEntryVerify(ce);
		SysLog(LogDebug, "Checking age of %s", ProxyContextEntryToString(ce));

		if ( ce->expiration > GetSecTime() )
			break;			// not old

		SysLog(LogDebug, "%s expired", ProxyContextEntryToString(ce));
		ProxyContextDelete(map, ce);
	}

	ProxyContextActivateTimer(map);
}
