/*****************************************************************************

Filename:   main/machine/stratusproxy.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/


#include <libgen.h>

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/server.h"



static int NxServerRemoveClient(NxServer_t *this, NxClient_t *client);
static void NxServerCreateImageFile(NxServer_t *this, char *addr, int port);
static void NxServerDestroyImageFile(NxServer_t *this);
static void NxClientCreateImageFile(NxClient_t *this, int port);
static void NxClientDestroyImageFile(NxClient_t *this);



BtNode_t *NxServerNodeList = NULL;


NxServer_t*
NxServerConstructor(NxServer_t *this, char *file, int lno)
{
	this->ownerProc = NxCurrentProc;
	this->listen = EventFileNew();
	this->connectionList = HashMapNew(FD_SETSIZE, "ConnectionList");
	return this;
}


void
NxServerDestructor(NxServer_t *this, char *file, int lno)
{
	EventFileVerify(this->listen);
	NxServerUnlisten(this, true);
	HashMapDelete(this->connectionList);
	EventFileDelete(this->listen);
}


Json_t*
NxServerSerialize(NxServer_t *this)
{
	NxServerVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddNumber(root, "NbrConnections", HashMapLength(this->connectionList));
	JsonAddItem(root, "Listen", EventFileSerialize(this->listen));
	return root;
}


char*
NxServerToString(NxServer_t *this)
{
	Json_t *root = NxServerSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


static void
NxServerCreateImageFile(NxServer_t *this, char *addr, int port)
{
	NxServerVerify(this);

	EventFile_t *evf = this->listen;
	EventFileVerify(evf);

return;

	char path[1024];
	if ( evf->domain == AF_UNIX )
		sprintf(path, "%s/listen/%s", this->ownerProc->sessionDir, basename(addr));
	else
		sprintf(path, "%s/listen/%d", this->ownerProc->sessionDir, port);

	if ( MkDirPath(path) != 0 )
		SysLog(LogFatal, "Unable to create %s; error %s", path, ErrnoToString(errno));
	strcat(path, "/");
	strcat(path, evf->uidString);

	this->imagePath = strdup(path);
	Memory_t *image = MemoryNewShared(this->imagePath, NxGlobal->pagesize-1);
	this->imageMemory = ((char*)VarToObject(image));		// align the pointer to the page boundry
	// TODO: memcpy(&this->imageHdr, this->imageMemory, sizeof(this->imageHdr));		// save hdr
	(void)NxServerToString(this);
}


static void
NxServerDestroyImageFile(NxServer_t *this)
{
	NxServerVerify(this);

	if ( this->imagePath != NULL )
	{
		// TODO: memcpy(this->imageMemory, &this->imageHdr, sizeof(this->imageHdr));		// restore hdr
		this->imageMemory = ((char*)ObjectToVar(this->imageMemory));
		MemoryDelete(this->imageMemory);
		if ( unlink(this->imagePath) != 0 )
		{
			AuditSendEvent(AuditSystemError, "connid", NxServerUidToString(this), "fnc", "unlink", "error", ErrnoToString(errno));
			SysLog(LogError, "Unable to delete %s; error %s", this->imagePath, ErrnoToString(errno));
		}
		free(this->imagePath);
		this->imagePath = NULL;
	}
}


int
NxServerListen(NxServer_t *this, int domain, int type, char *addr, int port, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler)
{
	NxServerVerify(this);

	EventFile_t *evf = this->listen;
	EventFileVerify(evf);

	if (evf->isOpen)
		NxServerClose(this);

	if (SockListen(evf, addr, port, domain, type) != 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxServerUidToString(this), "fnc", "SockListen");
		SysLog(LogError, "SockListen failed: %s.%d", addr, port);
		NxServerClose(this);
		return -1;
	}

	if ( handler != NULL )
	{
		if ( EventFileEnable(evf, pri, mask, handler, this) != 0 )
		{
			AuditSendEvent(AuditSystemError, "connid", NxServerUidToString(this), "fnc", "EventFileEnable");
			SysLog(LogError, "EventFileEnable failed: %s", EventFileToString(evf));
			NxServerClose(this);
			return -1;
		}
	}

	NxServerCreateImageFile(this, addr, port);
	return 0;
}


int
NxServerUnlisten(NxServer_t *this, boolean disconnectClients)
{
	NxServerVerify(this);

	if ( disconnectClients )
		NxServerDisconnectAll(this);

	if ( NxServerClose(this) < 0 )
	{
		AuditSendEvent(AuditSystemError, "connid", NxServerUidToString(this), "fnc", "NxServerClose");
		SysLog(LogError, "NxServerClose failed");
	}

	return 0;
}


int
NxServerDupConnection(NxServer_t *this, NxServer_t *dup)
{
	NxServerVerify(this);
	NxServerVerify(dup);
	EventFileVerify(this->listen);
	EventFileVerify(dup->listen);

	// dup connection
	if (SockDupConnection(this->listen, dup->listen) < 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxServerUidToString(this), "fnc", "SockDupConnection");
		SysLog(LogError, "SockDupConnection failed");
		return -1;
	}

// Now clone the image area
	NxServerDestroyImageFile(dup);		// remove any previous
	if ( this->imagePath != NULL )		// original has one
	{
		dup->imagePath = strdup(this->imagePath);
		Memory_t *image = MemoryNewShared(dup->imagePath, NxGlobal->pagesize-1);
		dup->imageMemory = ((char*)VarToObject(image));        // align the pointer to the page boundry
		// TODO: memcpy(&dup->imageHdr, dup->imageMemory, sizeof(dup->imageHdr));		// save hdr
		(void)NxServerToString(dup);
	}

	return 0;
}


int
NxServerClose(NxServer_t *this)
{
	NxServerVerify(this);

	EventFile_t *evf = this->listen;
	EventFileVerify(evf);

	SockClose(evf);

	// if owner get rid of image area
	if ( ObjectIsOwner(this) )
		NxServerDestroyImageFile(this);
	return 0;
}


NxClient_t*
NxServerAccept(NxServer_t *this, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler)
{
	NxServerVerify(this);

	NxClient_t *client = NxServerClientNew(this);	// new
	EventFile_t *evf = client->evf;
	EventFileVerify(evf);
	EventFileVerify(this->listen);

	if (SockAccept(this->listen, evf) != 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxServerUidToString(this), "fnc", "SockAccept");
		SysLog(LogError, "SockAccept failed: %s", NxServerNameToString(this));
		NxClientDelete(client);
		return NULL;
	}

	if (! NxClientIsConnected(client) )
	{
		SysLog(LogError, "Failed. Not Connected: %s", NxClientToString(client));
		NxClientDelete(client);
		return NULL;
	}

	if ( handler != NULL )
	{
		if ( NxClientSetEventMask(client, pri, mask, handler, client) != 0 )
		{
			AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(client), "fnc", "NxClientSetEventMask");
			SysLog(LogError, "NxClientSetEventMask failed: %s", NxClientToString(client));
			NxClientDelete(client);
			return NULL;
		}
	}

	SysLog(LogDebug, "Connected to client %s", NxClientToString(client));

	NxClientCreateImageFile(client, evf->servicePort);
	(void)NxClientToString(client);

	return client;
}


int
NxServerAddClient(NxServer_t *this, NxClient_t *client)
{
	NxServerVerify(this);
	NxClientVerify(client);
	EventFile_t *evf = client->evf;
	EventFileVerify(evf);

	if (HashAddUid(this->connectionList, evf->uid, client) == NULL)
	{
		AuditSendEvent(AuditSystemError, "connid", NxServerUidToString(this), "fnc", "HashAddUid");
		SysLog(LogError, "HashAddUid of %s failed", NxClientToString(client));
		NxClientDelete(client);
		return -1;
	}

	client->server = this;
	return 0;
}


NxClient_t*
NxServerFindClient(NxServer_t *this, NxUid_t uid)
{
	NxServerVerify(this);
	return (NxClient_t*)HashFindUidVar(this->connectionList, uid);
}


static int
NxServerRemoveClient(NxServer_t *this, NxClient_t *client)
{
	NxServerVerify(this);
	NxClientVerify(client);
	EventFile_t *evf = client->evf;
	EventFileVerify(evf);

	if ( NxServerFindClient(this, evf->uid) != NULL )
		HashDeleteUid(this->connectionList, evf->uid);
	client->server = NULL;
	return 0;
}


int
NxServerDisconnectAll(NxServer_t *this)
{
	NxServerVerify(this);
	EventFileVerify(this->listen);

	for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(this->connectionList, NULL)) != NULL;)
	{
		NxClient_t *client = (NxClient_t *)entry->var;
		NxClientVerify(client);
		NxClientDelete(client);
	}

	return 0;
}


// Client Functions


BtNode_t *NxClientNodeList = NULL;


NxClient_t*
NxClientConstructor(NxClient_t *this, char *file, int lno, NxServer_t *server)
{
	this->ownerProc = NxCurrentProc;
	this->evf = EventFileNew();

	if ( server != NULL )
	{
		if ( NxServerAddClient(server, this) != 0 )
			SysLog(LogFatal, "NxServerAddClient of %s failed", NxClientToString(this));
	}

	return this;
}


void
NxClientDestructor(NxClient_t *this, char *file, int lno)
{
	if ( this->server )
	{
		if ( NxServerRemoveClient(this->server, this) != 0 )
			SysLog(LogFatal, "NxServerRemoveClient of %s failed", NxClientToString(this));
	}
	if ( this->evf != NULL )
	{
		NxClientDisconnect(this);
		EventFileDelete(this->evf);
	}
}


Json_t*
NxClientSerialize(NxClient_t *this)
{
	NxClientVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	if ( this->server != NULL )
		JsonAddString(root, "Server", NxServerNameToString(this->server));
	JsonAddItem(root, "Session", EventFileSerialize(this->evf));
	return root;
}


char*
NxClientToString(NxClient_t *this)
{
	Json_t *root = NxClientSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


static void
NxClientCreateImageFile(NxClient_t *this, int port)
{
	NxClientVerify(this);

	EventFile_t *evf = this->evf;
	EventFileVerify(evf);

return;

	char path[1024];
	if ( evf->domain == AF_UNIX )
	{
		char *taddr = strdup(evf->serviceIpAddrString);
		char *base = NULL;
		char *ptr = NULL;
		if ( (ptr = strrchr(taddr, '/')) != NULL && ptr > taddr && ((*ptr = '\0') == 0) )
			base = strrchr(taddr, '/');
		if ( ptr != NULL )
			*ptr = '/';
		if ( base == NULL )
			base = taddr;
		sprintf(path, "%s/client/%s", this->ownerProc->sessionDir, base);
		free(taddr);
	}
	else
	{
		sprintf(path, "%s/client/%d/%s", this->ownerProc->sessionDir, port, evf->peerIpAddrString);
	}
	if ( MkDirPath(path) != 0 )
		SysLog(LogFatal, "Unable to create %s; error %s", path, ErrnoToString(errno));
	strcat(path, "/");
	strcat(path, evf->uidString);

	this->imagePath = strdup(path);
	Memory_t *image = MemoryNewShared(this->imagePath, NxGlobal->pagesize-1);
	this->imageMemory = ((char*)VarToObject(image));        // align the pointer to the page boundry
	// TODO: memcpy(&this->imageHdr, this->imageMemory, sizeof(this->imageHdr));		// save hdr
}


static void
NxClientDestroyImageFile(NxClient_t *this)
{
	NxClientVerify(this);

	if ( NxCurrentProc && this->imagePath != NULL )
	{
		// TODO: memcpy(this->imageMemory, &this->imageHdr, sizeof(this->imageHdr));		// restore hdr
		this->imageMemory = ((char*)ObjectToVar(this->imageMemory));
		MemoryDelete(this->imageMemory);
		if ( unlink(this->imagePath) != 0 )
		{
			AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "unlink", "error", ErrnoToString(errno));
			SysLog(LogError, "Unable to delete %s; error %s", this->imagePath, ErrnoToString(errno));
		}
		free(this->imagePath);
		this->imagePath = NULL;
	}
}


int
NxClientConnect(NxClient_t *this, int domain, int type, char *addr, int port, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler)
{
	NxClientVerify(this);

	EventFile_t *evf = this->evf;
	EventFileVerify(evf);

	int status;
	if ( (status = SockConnect(evf, addr, port, domain, type)) < 0 )
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockConnect");
		SysLog(LogError, "SockConnect failed: %s", EventFileToString(evf));
		NxClientDisconnect(this);
		return -1;
	}

	if ( handler != NULL )
	{
		if ( NxClientSetEventMask(this, pri, mask, handler, this) != 0 )
		{
			AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "NxClientSetEventMask");
			SysLog(LogError, "NxClientSetEventMask failed: %s", NxClientToString(this));
			NxClientDisconnect(this);
			return -1;
		}
	}

	NxClientCreateImageFile(this, port);
	(void)NxClientToString(this);

	return status;
}


int
NxClientDisconnect(NxClient_t *this)
{
	NxClientVerify(this);

	EventFile_t *evf = this->evf;
	EventFileVerify(evf);

	SockClose(evf);

	// if owner and there's a image area; get rid of it
	if ( ObjectIsOwner(this) )
		NxClientDestroyImageFile(this);
	return 0;
}


int
NxClientPair(NxClient_t *p1, NxClient_t *p2)
{
	NxClientVerify(p1);
	NxClientVerify(p2);
	EventFileVerify(p1->evf);
	EventFileVerify(p2->evf);

	// Open a socket pair
	if (SockSocketPair(p1->evf, p2->evf, SOCK_STREAM) < 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(p1), "fnc", "SockSocketPair");
		SysLog(LogError, "SockSocketPair failed");
		return -1;
	}

	NxClientCreateImageFile(p1, p1->evf->fd);
	NxClientCreateImageFile(p2, p2->evf->fd);

	return 0;
}


int
NxClientDupConnection(NxClient_t *this, NxClient_t *dup)
{
	NxClientVerify(this);
	NxClientVerify(dup);
	EventFileVerify(this->evf);
	EventFileVerify(dup->evf);

	// dup connection
	if (SockDupConnection(this->evf, dup->evf) < 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockDupConnection");
		SysLog(LogError, "SockDupConnection failed");
		return -1;
	}

// Now clone the image area
	NxClientDestroyImageFile(dup);		// remove any previous
	if ( this->imagePath != NULL )		// original has one
	{
		dup->imagePath = strdup(this->imagePath);
		Memory_t *image = MemoryNewShared(dup->imagePath, NxGlobal->pagesize-1);
		dup->imageMemory = ((char*)VarToObject(image));        // align the pointer to the page boundry
		// TODO: memcpy(&dup->imageHdr, dup->imageMemory, sizeof(dup->imageHdr));		// save hdr
		(void)NxClientToString(dup);
	}

	return 0;
}


int
NxClientSetEventMask(NxClient_t *this, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler, void *harg)
{
	NxClientVerify(this);

	EventFile_t *evf = this->evf;
	EventFileVerify(evf);

	if ( EventFileEnable(evf, pri, EventReadMask, handler, harg) != 0 )
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "EventFileEnable");
		SysLog(LogError, "EventFileEnable failed: %s", EventFileToString(evf));
		return -1;
	}

	return 0;
}


NxClient_t*
NxClientSiezeConnection(NxClient_t *this)
{
	NxClientVerify(this);

	if ( this->server )
		NxServerRemoveClient(this->server, this);

	NxClientSetEventMask(this, EventFileLowPri, EventNoMask, NULL, NULL);
	return this;
}


int
NxClientSendConnection(NxClient_t *this, NxClient_t *to)
{
	NxClientVerify(this);

	EventFile_t *evf = this->evf;
	EventFileVerify(evf);

	NxClientVerify(to);
	EventFileVerify(to->evf);

	SysLog(LogDebug, "Sending connection %s to %s", NxClientNameToString(this), NxClientNameToString(to));

	if (SockSendConnection(to->evf, evf) != 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(to), "fnc", "SockSendConnection");
		SysLog(LogError, "SockSendConnection failed: %s", EventFileToString(to->evf));
		return -1;
	}

	NxClientDelete(this);
	return 0;
}


NxClient_t*
NxClientRecvConnection(NxClient_t *this)
{
	NxClientVerify(this);

	EventFile_t *evf = this->evf;
	EventFileVerify(evf);

	NxClient_t *peer = NxClientNew();
	int status;
	if ( (status = SockRecvConnection(evf, peer->evf)) != 0)
	{
		int err = errno;
		if ( status > 0 )
		{
			if (peer->evf->isOpen)
				NxClientDisconnect(peer);
			NxClientDelete(peer);
			errno = err;
			return NULL;		// nothing available
		}
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockRecvConnection");
		SysLog(LogError, "SockRecvConnection failed on %s: %s", NxClientToString(this), ErrnoToString(err));
		if (peer->evf->isOpen)
			NxClientDisconnect(peer);
		NxClientDelete(peer);
		errno = err;
		return (NxClient_t*)-1;		// hard error
	}

// See if we're open

	if ((!peer->evf->isOpen) || (!peer->evf->isConnected))
	{
		SysLog(LogWarn, "SockRecvConnection returned no connection: %s", NxClientToString(this));
		if (peer->evf->isOpen)
			NxClientDisconnect(peer);
		NxClientDelete(peer);
		errno = 0;
		return NULL;		// nothing available
	}

	return peer;
}


int
NxClientRecvPkt(NxClient_t *this, void *pkt, int len)
{
	NxClientVerify(this);

	EventFileVerify(this->evf);

	int rlen = SockRecvPkt(this->evf, (char*)pkt, len, 0);

	if (rlen <= 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockRecvPkt");
		if ( errno != 0 )
		{
			AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockRecvPkt");
			SysLog(LogError, "SockRecvPkt failed: %s. Probably a disconnect on %s", ErrnoToString(errno), EventFileToString(this->evf));
		}
		return -1;
	}

	return rlen;
}


int
NxClientSendPkt(NxClient_t *this, void *pkt, int len)
{
	NxClientVerify(this);

	EventFileVerify(this->evf);

	if (SockSendPkt(this->evf, (char*)pkt, len, 0) < 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockSendPkt");
		SysLog(LogError, "SockSendPkt failed: %s", EventFileToString(this->evf));
		return -1;
	}

	return len;
}


int
NxClientRecvRaw(NxClient_t *this, void *bfr, int len)
{
	NxClientVerify(this);

	EventFileVerify(this->evf);

	int rlen = SockRecvRaw(this->evf, (char*)bfr, len, 0);

	if (rlen <= 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockRecvRaw");
		if ( errno != 0 )
		{
			if ( rlen == 0 )
				SysLog(LogDebug, "SockRecvRaw failed. Probably a disconnect: %s", EventFileToString(this->evf));
			else
				SysLog(LogError, "SockRecvRaw failed: %s", EventFileToString(this->evf));
		}
		return -1;
	}

	return rlen;
}


int
NxClientRecvRawExact(NxClient_t *this, void *bfr, int len, int ms)
{
	NxClientVerify(this);

	EventFileVerify(this->evf);

	int rlen = SockRecvRawExact(this->evf, (char*)bfr, len, 0, ms);

	if (rlen <= 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockRecvRawExact");
		if ( errno != 0 )
		{
			AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockRecvRawExact");
			SysLog(LogError, "SockRecvRaw failed. Probably a disconnect: %s", EventFileToString(this->evf));
		}
		return -1;
	}

	return rlen;
}


int
NxClientSendRaw(NxClient_t *this, void *bfr, int len)
{
	NxClientVerify(this);
	EventFileVerify(this->evf);

	if (SockSendRaw(this->evf, (char*)bfr, len, 0) < 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(this), "fnc", "SockSendRaw");
		SysLog(LogError, "SockSendRaw failed: %s", EventFileToString(this->evf));
		return -1;
	}

	return len;
}
