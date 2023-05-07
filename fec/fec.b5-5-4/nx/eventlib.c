/*****************************************************************************

Filename:   lib/nx/eventlib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/eventlib.c,v 1.3.4.7 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: eventlib.c,v $
 Revision 1.3.4.7  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/09/24 17:49:43  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/01 14:49:44  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/15 19:12:31  hbray
 5.5 revisions

 Revision 1.3.4.2  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.3.4.1  2011/07/29 16:10:15  hbray
 Commented out SysLog

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: eventlib.c,v 1.3.4.7 2011/10/27 18:33:56 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include <poll.h>



typedef struct EventList_t
{
	char					name[MaxNameLen];

	EventFilePriority_t		priority;				// 0 for high, 1 for low, 2...etc

	int						nbrFds;			// nbr currently occupying PollList
	boolean					pollListModified;

	struct					pollfd pollList[FD_SETSIZE];
	int						fdToPollMap[FD_SETSIZE];
	EventFile_t				*evfSet[FD_SETSIZE];
} EventList_t ;


#define EventListNew(priority, name, ...) ObjectNew(EventList, priority, name, ##__VA_ARGS__)
#define EventListVerify(var) ObjectVerify(EventList, var)
#define EventListDelete(var) ObjectDelete(EventList, var)

static void EventFileSetListHead(EventFile_t *this, EventFilePriority_t pri);

static EventList_t* EventListConstructor(EventList_t *this, char *file, int lno, EventFilePriority_t priority, char *name, ...);
static void EventListDestructor(EventList_t *this, char *file, int lno);
static BtNode_t* EventListNodeList = NULL;
static struct Json_t* EventListSerialize(EventList_t *this);
static char* EventListToString(EventList_t *this);

static int EventListEnable(EventList_t *this, EventFile_t *evf, EventPollMask_t pollMask);
static int EventListDisable(EventList_t *this, EventFile_t *evf);
static int EventListPoll(EventList_t *this, long defaultTimeout);
static void EventListEvict(EventList_t *this);

static const int EvNoFd = -1;



BtNode_t *EventGlobalNodeList = NULL;


EventGlobal_t*
EventGlobalConstructor(EventGlobal_t *this, char *file, int lno)
{
	this->hi = EventListNew(EventFileHighPri, "HighPri");
	this->lo = EventListNew(EventFileLowPri, "LowPri");
	this->joined = EventListNew(_EventFileJoinedPri, "Joined");
	return this;
}


void
EventGlobalDestructor(EventGlobal_t *this, char *file, int lno)
{
	EventListDelete(this->hi);
	EventListDelete(this->lo);
	EventListDelete(this->joined);
}


Json_t*
EventGlobalSerialize(EventGlobal_t *this)
{
	EventGlobalVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	Json_t *sub = JsonPushObject(root, this->joined->name);
	JsonAddString(sub, "Priority", EventPriorityToString(this->joined->priority));
	JsonAddNumber(sub, "NbrFds", this->joined->nbrFds);
	sub = JsonPushObject(root, this->hi->name);
	JsonAddString(sub, "Priority", EventPriorityToString(this->hi->priority));
	JsonAddNumber(sub, "NbrFds", this->hi->nbrFds);
	sub = JsonPushObject(root, this->lo->name);
	JsonAddString(sub, "Priority", EventPriorityToString(this->lo->priority));
	JsonAddNumber(sub, "NbrFds", this->lo->nbrFds);
	return root;
}


char*
EventGlobalToString(EventGlobal_t *this)
{
	Json_t *root = EventGlobalSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


int
EventGlobalJoinedFdCount(EventGlobal_t *this)
{
	EventGlobalVerify(this);
	return this->joined->nbrFds;			// nbr currently occupying PollList
}


char*
EventPriorityToString(EventFilePriority_t pri)
{
	char *text;

	if ( pri == EventFileHighPri )
		text = "High";
	else if ( pri == EventFileLowPri )
		text = "Low";
	else if ( pri == _EventFileJoinedPri )
		text = "Joined";
	else
		text = StringStaticSprintf("Priority_%d", (int)pri);

	return text;
}


static void
EventFileSetListHead(EventFile_t *this, EventFilePriority_t pri)
{
	EventFileVerify(this);

	this->priority = pri;
	if ( this->priority == EventFileHighPri )
		this->list = NxGlobal->efg->hi;
	else if ( this->priority == EventFileLowPri )
		this->list = NxGlobal->efg->lo;
	else
		SysLog(LogFatal, "%d is not a valid priority", this->priority);
}



BtNode_t *EventFileNodeList = NULL;


EventFile_t*
EventFileConstructor(EventFile_t *this, char *file, int lno)
{
	
	this->uid = NxUidNext();
	strcpy(this->uidString, NxUidToString(this->uid));

	// Finalize eventfile
	this->createdByPid = getpid();
	this->pollMask = EventNoMask;
	this->fd = EvNoFd;		// insure the fd is invalid...
	this->proc = NxCurrentProc;
	return this;
}


void
EventFileDestructor(EventFile_t *this, char *file, int lno)
{
	if (this->fd >= 0 && this->fd < FD_SETSIZE)
	{
		EventFileDisable(this);			// insure its no longer in the list
		NxGlobal->efg->evfSet[this->fd] = NULL;			// no longer in use
	}
}


int
EventFileSetNoWait(EventFile_t *this)
{

	EventFileVerify(this);

	int flags = fcntl(this->fd, F_GETFL);

	flags |= (O_NDELAY | O_NONBLOCK);
	if (fcntl(this->fd, F_SETFL, flags) != 0)
	{
		int error = errno;
		SysLog(LogError, "fcntl() error=%s: {%s}", ErrnoToString(error), EventFileToString(this));
		errno = error;
		return -1;
	}

	return 0;
}


int
EventFileEnable(EventFile_t *this, EventFilePriority_t pri, EventPollMask_t pollMask, EventHandler_t handler, void *harg)
{

	EventFileVerify(this);

	if ( EventFileDisable(this) != 0 )
		SysLog(LogError, "EventFileDisable failed on %s", EventFileToString(this));

	if ( EventFileSetNoWait(this) != 0 )
		SysLog(LogError, "EventFileSetNoWait failed on %s", EventFileToString(this));

	this->pollMask = pollMask;
	EventFileSetListHead(this, pri); // set new priority
	this->handler = handler;
	this->harg = harg;
	this->proc = NxCurrentProc;

	if ( EventListEnable(this->list, this, pollMask) != 0 )
	{
		SysLog(LogError, "EventListEnable failed: %s", EventFileToString(this));
		return -1;
	}
	if ( EventListEnable(NxGlobal->efg->joined, this, pollMask) != 0 )
	{
		SysLog(LogError, "EventListEnable failed: %s", EventFileToString(this));
		return -1;
	}

	return 0;
}


int
EventFileDisable(EventFile_t *this)
{

	EventFileVerify(this);

	if ( this->list != NULL )
	{
		if ( EventListDisable(this->list, this) != 0 )
		{
			SysLog(LogError, "EventListDisable failed: %s", EventFileToString(this));
			return -1;
		}
		this->list = NULL;
	}
	if ( EventListDisable(NxGlobal->efg->joined, this) != 0 )
	{
		SysLog(LogError, "EventListDisable failed: %s", EventFileToString(this));
		return -1;
	}

	return 0;
}


char *
EventPollMaskToString(EventPollMask_t pollMask)
{
	StringArrayStatic(sa, 16, 128);
	String_t *out = StringArrayNext(sa);
	StringClear(out);

	if (pollMask & EventReadMask)
	{
		pollMask &= (~EventReadMask);
		StringCat(out, "EventRead");
		if (pollMask != 0)
			StringCat(out, ",");
	}
	if (pollMask & EventWriteMask)
	{
		pollMask &= (~EventWriteMask);
		StringCat(out, "EventWrite");
		if (pollMask != 0)
			StringCat(out, ",");
	}
	if ( out->len <= 0 )
		StringCpy(out, "None");

	return out->str;
}


char*
EventPipeTypeToString(EventPipeType_t pipe)
{
	char *text;

	switch (pipe)
	{
		default:
			text = StringStaticSprintf("Pipe_%d", (int)pipe);
			break;

		case EventPipeTypeNo:
			text = "No";
			break;
		case EventPipeTypeLeft:
			text = "Left";
			break;
		case EventPipeTypeRight:
			text = "Right";
			break;
	}

	return text;
}


Json_t*
EventFileSerialize(EventFile_t *this)
{
	EventFileVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", this->name);
	JsonAddString(root, "Uid", this->uidString);
	JsonAddString(root, "Priority", EventPriorityToString(this->priority));
	JsonAddBoolean(root, "IsOpen", this->isOpen);

	if ( this->isOpen )
	{
		JsonAddNumber(root, "Fd", (double)this->fd);
		JsonAddString(root, "PollMask", EventPollMaskToString(this->pollMask));
		JsonAddString(root, "Domain", (this->domain==AF_INET)?"AF_INET":(this->domain==AF_UNIX)?"AF_UNIX":IntegerToString(this->domain, NULL));
		JsonAddString(root, "Type", (this->type==SOCK_DGRAM)?"SOCK_DGRAM":(this->type==SOCK_STREAM)?"SOCK_STREAM":IntegerToString(this->type, NULL));
		JsonAddBoolean(root, "IsConnected", this->isConnected);
		if ( this->isConnected )
		{
			if ( this->domain == AF_INET )
			{
				JsonAddString(root, "PeerIpAddr", this->peerIpAddrString);
				if ( ! this->isPipe )
					JsonAddString(root, "PeerPort", this->peerPortString);
			}
			if ( this->msTimeConnected )
				JsonAddString(root, "TimeConnected", MsTimeToStringShort(this->msTimeConnected, NULL));
		}
	}

	JsonAddBoolean(root, "IsShutdown", this->isShutdown);
	if ( this->isPipe )
		JsonAddString(root, "Pipe", EventPipeTypeToString(this->isPipe));
	JsonAddString(root, "ServiceIpAddr", this->serviceIpAddrString);
	if ( this->domain == AF_INET && ! this->isPipe )
		JsonAddString(root, "ServicePort", this->servicePortString);
	JsonAddNumber(root, "CreatedBy", (double)this->createdByPid);

// do counts
	if ( this->isOpen )
		JsonAddItem(root, "Counts", CountsSerialize(this->counts));

	return root;
}


char*
EventFileToString(EventFile_t *this)
{
	Json_t *root = EventFileSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}



static EventList_t*
EventListConstructor(EventList_t *this, char *file, int lno, EventFilePriority_t priority, char *name, ...)
{
	{
		va_list ap;
		va_start(ap, name);
		vsnprintf(this->name, sizeof(this->name), name, ap);
	}

	this->priority = priority;
	this->nbrFds = 0;
	this->pollListModified = true;
	memset(this->pollList, 0, sizeof(this->pollList));
	memset(this->evfSet, 0, sizeof(this->evfSet));

	for(int i = 0; i < FD_SETSIZE; ++i )
		this->fdToPollMap[i] = EvNoFd;

	return this;
}


static void
EventListDestructor(EventList_t *this, char *file, int lno)
{
}


static Json_t*
EventListSerialize(EventList_t *this)
{
	EventListVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", this->name);
	JsonAddString(root, "Priority", EventPriorityToString(this->priority));
	JsonAddNumber(root, "NbrFds", this->nbrFds);
	JsonAddString(root, "Modified", BooleanToString(this->pollListModified));
	if ( this->nbrFds > 0 )
	{
		Json_t *sub = JsonPushObject(root, "Fds");
		for (int i = 0; i < this->nbrFds; ++i)
		{
			Json_t *fd = JsonPushObject(sub, "Fd");
			JsonAddNumber(fd, "fd", this->pollList[i].fd);
			JsonAddString(fd, "events", "0x%08X", this->pollList[i].fd, this->pollList[i].events);
		}
	}
	return root;
}


static char*
EventListToString(EventList_t *this)
{
	Json_t *root = EventListSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


static int
EventListEnable(EventList_t *this, EventFile_t *evf, EventPollMask_t pollMask)
{

	EventListVerify(this);
	EventFileVerify(evf);

// Range check the fd

	if (evf->fd < 0 || evf->fd > FD_SETSIZE)
		SysLog(LogFatal, "%s: fd is out of range: {%s}", this->name, EventFileToString(evf));

	if (this->nbrFds < 0 || this->nbrFds >= FD_SETSIZE)
		SysLog(LogFatal, "%s: NbrFds %d is out of range", this->name, this->nbrFds);

// validate the event being requested

	if ((pollMask & EventReadMask) == 0 && (pollMask & EventWriteMask) == 0 && (pollMask != 0))
	{
		SysLog(LogError, "%s: %s is not a valid event mask: {%s}", this->name, EventPollMaskToString(pollMask), EventFileToString(evf));
		return -1;
	}

	SysLog(LogDebug, "%s: Enable %s for %s", this->name, EventPollMaskToString(pollMask), EventFileToString(evf));

	evf->pollMask = pollMask;

	if (this->fdToPollMap[evf->fd] == EvNoFd)		// a new one...
	{
		this->fdToPollMap[evf->fd] = this->nbrFds++;
		this->evfSet[evf->fd] = evf;
		this->pollListModified = true;
	}
	
	struct pollfd *pfd = &this->pollList[this->fdToPollMap[evf->fd]];	// get the poll entry

	memset(pfd, 0, sizeof(*pfd));
	pfd->fd = evf->fd;
	pfd->events = 0;
	pfd->revents = 0;

	if (evf->pollMask != EventNoMask)
	{
		if (evf->pollMask & EventReadMask)
			pfd->events = POLLIN | POLLPRI;
		if (evf->pollMask & EventWriteMask)
			pfd->events = POLLOUT;
	}

	return 0;
}


static int
EventListDisable(EventList_t *this, EventFile_t *evf)
{

	EventListVerify(this);
	EventFileVerify(evf);

	if ( evf->fd == EvNoFd )
		return 0;		// done

	if (evf->fd < 0 || evf->fd > FD_SETSIZE)
	{
		SysLog(LogError, "%s: fd is out of range: {%s}", this->name, EventFileToString(evf));
		return -1;
	}

	int pidx = this->fdToPollMap[evf->fd];

	if (pidx >= 0)				// in the poll list
	{
		SysLog(LogDebug, "%s: Disable poll %s", this->name, EventFileToString(evf));

		this->fdToPollMap[evf->fd] = EvNoFd;

		if (--this->nbrFds < 0)
			this->nbrFds = 0;

		// need to shift all the poll entries leftward...
		for (int n = pidx; n < this->nbrFds; ++n)
		{
			memcpy(&this->pollList[n], &this->pollList[n + 1], sizeof(this->pollList[n]));
			this->fdToPollMap[this->pollList[n].fd] = n;
		}

		this->pollListModified = true;
	}

	this->evfSet[evf->fd] = NULL;

	return 0;
}


static void
EventListEvict(EventList_t *this)
{

	EventListVerify(this);

// Build an eviction test poll list
	struct pollfd fds[FD_SETSIZE];
	int nfds;
	int fd;

	SysLog(LogError, "%s: Called for list %s", this->name, EventListToString(this));

	for (nfds = 0, fd = 0; fd < FD_SETSIZE; ++fd)
	{
		EventFile_t *evf = this->evfSet[fd];

		if (evf != NULL)
		{
			fds[nfds].fd = fd;
			fds[nfds].events = POLLIN | POLLPRI | POLLOUT | POLLERR | POLLHUP | POLLNVAL;
			++nfds;
		}
	}

	if (poll(fds, nfds, 0) < 0)
	{
		SysLog(LogError, "%s: poll failed; errno=%s", this->name, ErrnoToString(errno));
		return;
	}

	for (int i = 0; i < nfds; ++i)
	{
		if (fds[i].revents & POLLNVAL)
		{
			int fd = fds[i].fd;
			EventFile_t *evf = this->evfSet[fd];

			SysLog(LogError, "%s: I have what appears to be an orphaned file descriptor: %d; closing it...", this->name, fd);
			close(fd);		// force the issue...
			if ( evf != NULL )
			{
				SysLog(LogError, "%s: Disable poll %s", this->name, EventFileToString(evf));
				EventListDisable(this, evf);
			}
		}
	}
}


static int
EventListPoll(EventList_t *this, long defaultTimeout)
{

	EventListVerify(this);
	
	// SysLog(LogDebug, "%s: Called for list %s", this->name, EventListToString(this));

	NxTime_t timeout = defaultTimeout;

	if ( timeout > 0 )	// timeout requested
	{
		// Look at the timer list to determine max amount of time to remain in poll

		Timer_t *te;
		if ((te = _TimerFirstEntry()) != NULL)
		{
			TimerVerify(te);
			NxTime_t tod = GetMsTime();

			if ((timeout = (te->expirationTime - tod)) <= 0)	// time has elapsed, wait zero time...
				timeout = 0;	// this timer has expired...

			if (timeout > defaultTimeout)
				timeout = defaultTimeout;	// not time for this guy, wait the default
		}
	}

	_ProcHandleSignals(NxCurrentProc);

	{		// do poll
		// SysLog(LogDebug, "%-6.4f: %s", ((double)timeout)/1000.0, EventListToString(this));

		int npoll = poll(this->pollList, this->nbrFds, timeout);

		if (npoll < 0)
		{
			SysLog((errno == EINTR) ? LogWarn : LogError, "select error %d; errno=%s", errno, ErrnoToString(errno));
			if (errno == EBADF)	// bad file descriptor
				EventListEvict(this);	// try to remove it...
		}
	}

	// if we have a timed out event, call it
	Timer_t *te;
	if ((te = _TimerFirstEntry()) != NULL)
	{
		TimerVerify(te);
		NxTime_t eod = GetMsTime();

		if ( (te->expirationTime - eod) <= 0)	// time has elapsed, time to notify event
		{
			TimerHandler_t handler = te->expirationHandler;

			NxCurrentProc = te->proc;
			TimerCancel(te);

			SysLogPushLevel();
			if (handler != NULL)
				(*(handler)) (te);
			StackClear(SysLogGlobal->loglevelStack);
			StackClear(SysLogGlobal->prefixStack);
		}
	}

	int n = this->nbrFds;
	int nevents = 0;

	for (int pidx = 0; pidx < n; ++pidx)
	{
		struct pollfd *pfd = &this->pollList[pidx];

		if (pfd->revents)
		{						/* this one completed */
			EventFile_t *evf = this->evfSet[this->pollList[pidx].fd];

			if (evf == NULL)
				continue;		// probably got closed... ignore it

			EventFileVerify(evf);
			NxCurrentProc = evf->proc;

			SysLogPushLevel();

			if (NxIgnoreThisIp(evf->peerIpAddr))
				SysLogSetLevel(LogWarn);

			if (evf->handler != NULL)
			{
				EventPollMask_t pollMask = (pfd->revents & POLLOUT) ? EventWriteMask : 0;

				pollMask |= (pfd->revents & ~POLLOUT) ? EventReadMask : 0;
				(*evf->handler) (evf, pollMask, evf->harg);
				StackClear(SysLogGlobal->loglevelStack);
				StackClear(SysLogGlobal->prefixStack);
			}
			++nevents;
		}

		if (this->pollListModified)
			break;
	}

	this->pollListModified = false;

	return nevents;
}


int
_EventPoll(EventGlobal_t *this, long defaultTimeout)	// defaultTimeout in mseconds
{
	EventGlobalVerify(this);

	int npoll;

#if 0
	if ( (npoll = EventListPoll(this->joined, defaultTimeout)) < 0 )
		SysLog(LogError, "EventListPoll joined failed");
#else
	if ( (npoll = EventListPoll(this->hi, 0)) == 0 )
		if ( (npoll = EventListPoll(this->lo, 0)) == 0 )
			npoll = EventListPoll(this->joined, defaultTimeout);

	if ( poll < 0 )
		SysLog(LogError, "EventListPoll failed");
#endif

	return npoll;
}
