/*****************************************************************************

Filename:   include/server.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/


#ifndef _SERVER_H
#define _SERVER_H



// NxServer
//
typedef struct NxServer_t
{
	char				*addr;
	int					port;
	EventFile_t			*listen;
	struct Proc_t		*ownerProc;
	HashMap_t			*connectionList; // a hash of NxClient_t*

	char				*imagePath;			// shared memory file name
	char				*imageMemory;		// shared memory text image
	// Object_t			imageHdr;			// saved shared memory object header
} NxServer_t ;

#define NxServerNew() ObjectNew(NxServer)
#define NxServerClientNew(server) ObjectNew(NxClient, server)
#define NxServerVerify(var) ObjectVerify(NxServer, var)
#define NxServerDelete(var) ObjectDelete(NxServer, var)

extern NxServer_t* NxServerConstructor(NxServer_t *this, char *file, int lno);
extern void NxServerDestructor(NxServer_t *this, char *file, int lno);
extern BtNode_t* NxServerNodeList;
extern struct Json_t* NxServerSerialize(NxServer_t *this);
extern char* NxServerToString(NxServer_t *this);

// Methods
extern int NxServerListen(NxServer_t *this, int domain, int type, char *addr, int port, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler);
extern int NxServerUnlisten(NxServer_t *this, boolean disconnectClients);
extern struct NxClient_t* NxServerAccept(NxServer_t *this, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler);
extern int NxServerDupConnection(NxServer_t *this, NxServer_t *dup);
extern int NxServerClose(NxServer_t *this);
extern int NxServerDisconnectAll(NxServer_t *this);
extern int NxServerAddClient(NxServer_t *this, struct NxClient_t *client);
extern struct NxClient_t* NxServerFindClient(NxServer_t *this, NxUid_t uid);

static inline char *NxServerNameToString(NxServer_t *this) { NxServerVerify(this); EventFileVerify(this->listen); return this->listen->name; };
static inline void NxServerSetName(NxServer_t *this, char *name) { NxServerVerify(this); EventFileVerify(this->listen); strcpy(this->listen->name, name); };
static inline NxUid_t NxServerGetUid(NxServer_t *this) { NxServerVerify(this); EventFileVerify(this->listen); return this->listen->uid; };
static inline char *NxServerUidToString(NxServer_t *this) { NxServerVerify(this); EventFileVerify(this->listen); return this->listen->uidString; };


// NxClient
//
typedef struct NxClient_t
{
	NxServer_t			*server;		// NULL if this is the connecting side; otherwise, points to the 'listen'

	char				*addr;
	int					port;
	EventFile_t			*evf;
	struct Proc_t		*ownerProc;

	char				*imagePath;			// shared memory file name
	char				*imageMemory;		// shared memory text image
	// Object_t			imageHdr;			// saved shared memory object header
} NxClient_t ;

#define NxClientNew() ObjectNew(NxClient, NULL)
#define NxClientDelete(var) ObjectDelete(NxClient, var)
#define NxClientVerify(var) ObjectVerify(NxClient, var)

extern NxClient_t* NxClientConstructor(NxClient_t *this, char *file, int lno, NxServer_t *server);
extern void NxClientDestructor(NxClient_t *this, char *file, int lno);
extern BtNode_t* NxClientNodeList;
extern Json_t* NxClientSerialize(NxClient_t *this);
extern char* NxClientToString(NxClient_t *this);

// Methods
extern int NxClientConnect(NxClient_t *this, int domain, int type, char *addr, int port, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler);
extern int NxClientPair(NxClient_t *p1, NxClient_t *p2);
extern int NxClientDupConnection(NxClient_t *this, NxClient_t *dup);
extern int NxClientDisconnect(NxClient_t *this);
extern int NxClientSetEventMask(NxClient_t *this, EventFilePriority_t pri, EventPollMask_t mask, EventHandler_t handler, void *harg);
extern int NxClientSendConnection(NxClient_t *this, NxClient_t *to);
extern NxClient_t* NxClientRecvConnection(NxClient_t *from);
extern NxClient_t* NxClientSiezeConnection(NxClient_t *this);
extern int NxClientSendPkt(NxClient_t *this, void *pkt, int len);
extern int NxClientRecvPkt(NxClient_t *this, void *pkt, int len);
extern int NxClientSendRaw(NxClient_t *this, void *bfr, int len);
extern int NxClientRecvRaw(NxClient_t *this, void *bfr, int len);
extern int NxClientRecvRawExact(NxClient_t *this, void *bfr, int len, int ms);

static inline boolean NxClientIsOpen(NxClient_t *this) { NxClientVerify(this); EventFileVerify(this->evf); return this->evf->isOpen; };
static inline boolean NxClientIsConnected(NxClient_t *this) { NxClientVerify(this); return SockIsConnected(this->evf); };

static inline char *NxClientNameToString(NxClient_t *this) { NxClientVerify(this); EventFileVerify(this->evf); return this->evf->name; };
static inline void NxClientSetName(NxClient_t *this, char *name) { NxClientVerify(this); EventFileVerify(this->evf); strcpy(this->evf->name, name); };
static inline NxUid_t NxClientGetUid(NxClient_t *this) { NxClientVerify(this); EventFileVerify(this->evf); return this->evf->uid; };
static inline char *NxClientUidToString(NxClient_t *this) { NxClientVerify(this); EventFileVerify(this->evf); return this->evf->uidString; };

#endif
