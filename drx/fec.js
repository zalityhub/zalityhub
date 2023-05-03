/*****************************************************************************
Copyright(c) 1991-2009 - Elavon Gateway Payment Solutions All Rights Reserved

Filename:   fec.js

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2016.01.01 harold bray          Created release 1.0
 *****************************************************************************/




// The header exchanged between the FEC and the Host
// NOTE: the following structure must be on byte bounds.
// NOTE: use this to structure to encode/decode Host headers
#pragma pack(1)
typedef struct
{
	unsigned short		len;			// The value of len is not self-inclusive
	char				msgType[2];			// see below
	unsigned short		servicePort;	// the peer (pos) service port
	unsigned char		svcType[1];		// see below
	unsigned char		peerIpAddr[8];
	unsigned char		peerIpType[1];
	unsigned short		peerIpPort;
	char				sysid[3];
	char				more[1];
	NxEcho_t			proxyEcho;
	NxEcho_t			clientEcho;
	NxEcho_t			contextEcho;
} HostHeader_t;					// Memory size: 53 + sizeof(short)

#pragma pack()


// Host Message Types:
#define HostConfigReqestMsgType     "00"
#define HostRequestMsgType          "01"
#define HostResponseMsgType         "02"

// Host Service Types:
#define HostConfigSvcType           '0'
#define HostAuthSvcType             '1'
#define HostEdcSvcType              '2'
#define HostMultiEdcSvcType         '3'

#define MAX_HOST_PACKET MaxSockPacketLen
#define MAX_HOST_PAYLOAD ((MAX_HOST_PACKET)-sizeof(HostHeader_t))	// Maximum Host transmission size is 8kb

typedef struct
{
	HostHeader_t	hdr;
	unsigned char	payload[MAX_HOST_PAYLOAD];
} HostFrame_t;

#pragma pack()


#define HostFrameHdrLen() (sizeof(HostHeader_t)-sizeof(unsigned short))

#define HostFrameLen(frame) ( sizeof((frame).hdr) + HostPayloadLen(frame) )	// length of the entire frame
#define HostPayloadLenHdr(hdr) ( ((hdr).len) - (HostFrameHdrLen()) )	// length of the payload
#define HostPayloadLen(frame) ( HostPayloadLenHdr((frame).hdr) )


// External Functions

extern void HostBuildHeader(HostHeader_t *hhdr, char *msgType, int servicePort, char svcType, char *sysid, char *peerIpAddr, char peerIpType, unsigned short peerIpPort, char more, char *proxyEcho, char *clientEcho, char *contextEcho);
extern char *HostHeaderToString(HostHeader_t *hhdr);
extern int HostInit(void);
extern int HostRecvFrame(EventFile_t *utf, HostFrame_t *frame);
extern int HostSendFrame(EventFile_t *utf, HostFrame_t *frame);
extern void HostSetFrameLength(HostFrame_t *frame, int len);
extern int HostSetPayload(HostFrame_t *frame, unsigned char *payload, int len);
extern int HostTerm(void);




// Host Request Response Types
//
typedef enum
{
	eReqConfig			= 0,
	eReqAuth			= 1,
	eReqEDC				= 2,
	eReqEDCMulti		= 3
} HostReqType_t;


typedef struct
{
	struct
	{
		char				peerName[MaxNameLen];
		char				peerIpAddr[8];
		char				peerIpType[1];
		unsigned short		peerIpPort;
	} _priv ;		// internal area...

	HostReqType_t		reqType;
	boolean				more;					// true for additional packets in this sequence
	NxEcho_t			contextEcho;		// will be returned in response
	int					len;					// length of data[] in bytes
	char				data[MaxSockPacketLen];	// request data packet
} HostRequest_t;

#define HostRequestLen(pi) ((sizeof(HostRequest_t)-sizeof((pi)->data)) + (pi)->len)


// Types of Proxy Requests
//
typedef enum
{
	StratusProxyType	= 0,
	T70ProxyType		= 1
} ProxyType_t ;

extern char* ProxyTypeToString(ProxyType_t type);


typedef enum
{
	eHostOnline			= 0,
	eHostOffline		= 1,
	eHostReq			= 2
} ProxyReqType_t ;

extern char *ProxyReqTypeToString(ProxyReqType_t type);


typedef struct ProxyRequest_t
{
	ProxyType_t			proxyType;
	ProxyReqType_t		reqType;
	int					servicePort;
	NxEcho_t			clientEcho;

// Payload is optional; present when type == eHostReq
	HostRequest_t		hostReq;
} ProxyRequest_t;

#define ProxyRequestNew() ObjectNew(ProxyRequest)
#define ProxyRequestVerify(var) ObjectVerify(ProxyRequest, var)
#define ProxyRequestDelete(var) ObjectDelete(ProxyRequest, var)
#define ProxyRequestLen(rq) ((sizeof(ProxyRequest_t)-sizeof((rq)->hostReq)) + HostRequestLen(&(rq)->hostReq))

extern ProxyRequest_t* ProxyRequestConstructor(ProxyRequest_t *this);
extern void ProxyRequestDestructor(ProxyRequest_t *this);
extern char* ProxyRequestToString(ProxyRequest_t *this, ...);


typedef struct ProxyQueue_t
{
	ProxyType_t			proxyType;
	boolean				cleartosend;
	NxTime_t			lastProxySendTime;	// MS timer for detecting host send stall's
	NxTime_t			lastProxyRecvTime;	// MS time
	unsigned int		nbrInflights;
	ObjectList_t		*outboundQueue;	// a ObjectList_t* of ProxyRequest_t*

	struct Proc_t		*proc;
	struct PiSession_t	*sess;
	int (*send)	(struct Proc_t *proc, struct PiSession_t *sess, struct ProxyRequest_t *proxyReq);
} ProxyQueue_t ;

#define ProxyQueueNew(proc, sess, proxyType) ObjectNew(ProxyQueue, proc, sess, proxyType)
#define ProxyQueueVerify(var) ObjectVerify(ProxyQueue, var)
#define ProxyQueueDelete(var) ObjectDelete(ProxyQueue, var)

extern ProxyQueue_t* ProxyQueueConstructor(ProxyQueue_t *this, struct Proc_t *proc, struct PiSession_t *sess, ProxyType_t proxyType);
extern void ProxyQueueDestructor(ProxyQueue_t *this);
extern int ProxyQueuePump(ProxyQueue_t *this);
extern int ProxyQueueReset(ProxyQueue_t *this);
extern int ProxyQueueAddRequest(ProxyQueue_t *this, HostRequest_t *hostReq, EventFile_t *peer, int servicePort);
extern char* ProxyQueueToString(ProxyQueue_t *this);

#endif
