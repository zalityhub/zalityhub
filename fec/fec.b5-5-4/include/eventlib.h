/*****************************************************************************

Filename:   include/eventlib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/eventlib.h,v 1.3.4.3 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: eventlib.h,v $
 Revision 1.3.4.3  2011/10/27 18:33:52  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:35  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/15 19:12:31  hbray
 5.5 revisions

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:34  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: eventlib.h,v 1.3.4.3 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _EVENTLIB_H
#define _EVENTLIB_H


typedef enum
{
	_EventFileJoinedPri		= -1,		// do not use; internal use only
	EventFileHighPri		= 0,
	EventFileLowPri			= 1
} EventFilePriority_t ;


typedef enum
{
	EventNoMask		= 0,
	EventReadMask	= 1,
	EventWriteMask	= 2
} EventPollMask_t;

typedef enum
{
	EventPipeTypeNo		= 0,
	EventPipeTypeLeft	= 1,
	EventPipeTypeRight	= 2
} EventPipeType_t ;



struct EventFile_t;
typedef void (*EventHandler_t)(struct EventFile_t *evf, EventPollMask_t mask, void *harg);

typedef struct EventFile_t
{
	NxUid_t					uid;					// A unique ID
	char					uidString[33];			// string version

	EventFilePriority_t		priority;				// 0 for high, 1 for low, 2...etc

	struct EventList_t		*list;

	// NxTime_t				timeCreated;
	int						createdByPid;			// Pid (useful when passing sockets around)

	boolean					isOpen;
	boolean					isConnected;
	boolean					isShutdown;
	NxTime_t				msTimeConnected;

	int						fd;
	char					name[MaxNameLen];

	EventPollMask_t			pollMask;
	EventHandler_t			handler;
	void					*harg;

	struct Proc_t			*proc;
	void					*context;				// Misc external (not eventlib) use

	int						domain;
	int						type;

	EventPipeType_t			isPipe;

	int						servicePort;
	char					servicePortString[64];
	unsigned char			serviceIpAddr[8];
	char					serviceIpAddrString[64];

	int						peerPort;
	char					peerPortString[64];
	unsigned char			peerIpAddr[8];
	char					peerIpAddrString[64];

	// I/O Stats
	Counts_t				counts;
} EventFile_t;


// Globals
typedef struct EventGlobal_t
{
	struct EventList_t		*joined;
	struct EventList_t		*hi;
	struct EventList_t		*lo;
	EventFile_t				*evfSet[FD_SETSIZE];		// global FD set
} EventGlobal_t ;


#define EventGlobalNew() ObjectNew(EventGlobal)
#define EventGlobalVerify(var) ObjectVerify(EventGlobal, var)
#define EventGlobalDelete(var) ObjectDelete(EventGlobal, var)

extern EventGlobal_t* EventGlobalConstructor(EventGlobal_t *this, char *file, int lno);
extern void EventGlobalDestructor(EventGlobal_t *this, char *file, int lno);
extern BtNode_t* EventGlobalNodeList;
extern struct Json_t* EventGlobalSerialize(EventGlobal_t *this);
extern char* EventGlobalToString(EventGlobal_t *this);
extern int EventGlobalJoinedFdCount(EventGlobal_t *this);





// Object Functions

#define EventFileNew() ObjectNew(EventFile)
#define EventFileVerify(var) ObjectVerify(EventFile, var)
#define EventFileDelete(var) ObjectDelete(EventFile, var)

extern EventFile_t* EventFileConstructor(EventFile_t *this, char *file, int lno);
extern void EventFileDestructor(EventFile_t *this, char *file, int lno);
extern BtNode_t* EventFileNodeList;
extern Json_t* EventFileSerialize(EventFile_t *this);
extern char* EventFileToString(EventFile_t *this);

static inline char *EventFileUidString(EventFile_t *this) { EventFileVerify(this); return this->uidString; };


// External Functions

#define EventPoll(timeout) _EventPoll(NxGlobal->efg, timeout)
extern int _EventPoll(EventGlobal_t *this, long defaultTimeout);
extern char *EventPollMaskToString(EventPollMask_t pollMask);
extern char *EventPipeTypeToString(EventPipeType_t pipe);
extern int EventFileDisable(EventFile_t *this);
extern int EventFileSetNoWait(EventFile_t *this);
extern int EventFileEnable(EventFile_t *this, EventFilePriority_t pri, EventPollMask_t pollMask, EventHandler_t handler, void *harg);
extern char* EventPriorityToString(EventFilePriority_t pri);

#endif
