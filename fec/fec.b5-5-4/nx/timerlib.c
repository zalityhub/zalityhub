/*****************************************************************************

Filename:   lib/nx/timerlib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:59 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/timerlib.c,v 1.3.4.4 2011/10/27 18:33:59 hbray Exp $
 *
 $Log: timerlib.c,v $
 Revision 1.3.4.4  2011/10/27 18:33:59  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:48  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/15 19:12:32  hbray
 5.5 revisions

 Revision 1.3.4.1  2011/08/11 19:47:34  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: timerlib.c,v 1.3.4.4 2011/10/27 18:33:59 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"


static ObjectList_t *TimerList;


// Static (Local) Functions
static int TimerInit(void);
static int _TimerActivate(Timer_t *this, long ms, TimerHandler_t expirationHandler);
static void _TimerInsert(Timer_t *this);
static void _TimerYank(Timer_t *this);
static void TimerQueueDump(void);



/*+******************************************************************
	Name:
		TimerInit - Init the timer tables.

	Synopsis:
		int TimerInit (void)

	Description:

	Diagnostics:
		None.
-*******************************************************************/

static int
TimerInit(void)
{
	TimerList = ObjectListNew(ObjectListTimeType, "ActiveTimerList"); // initialize timer lists
	return 0;
}

BtNode_t *TimerNodeList = NULL;

Timer_t*
TimerConstructor(Timer_t *this, char *file, int lno, char *name, ...)
{
	static boolean needInit = true;

	if ( needInit )
		TimerInit();

	needInit = false;

	{
		va_list ap;
		va_start(ap, name);
		vsnprintf(this->name, sizeof(this->name), name, ap);
	}

	this->proc = NxCurrentProc;
	return this;
}


void
TimerDestructor(Timer_t *this, char *file, int lno)
{
	ObjectLink_t *link;
	if ( (link = ObjectListGetLink(TimerList, this)) != NULL )
		ObjectListYank(TimerList, link);
}


/*+******************************************************************
	Name:
		TimerActivate - Activate timer event

	Synopsis:
		int
		TimerActivate (long ms, TimerHandler_t expirationHandler)

	Description:
		Activate a timeout request and associate the expirationHandler
		to this request.

		When the timeout period has elapsed, the expirationHandler function will be
		called in the form:
			expirationHandler (teptr)

		After the expirationHandler is completed, the timeout request will be removed
		from the timer queue and must be re-added to have the same timeout
		period repeated.

		A unique timer teptr is returned to the caller through the 'teptr' argument.
		This timer teptr may be used to subsequently cancel the active timer.
		If you are not interested, pass NULL.

		timeout is expressed in units of miliseconds.

	Diagnostics:
		int is returned with a non 0 and defines the
		particular error that occurred during timeout registration.
		In the event of an error, a NULL will be returned as the timer 'teptr'.
-*******************************************************************/
int
TimerActivate(Timer_t *this, long ms, TimerHandler_t expirationHandler)
{
	TimerVerify(this);
	return _TimerActivate(this, ms, expirationHandler);
}


static int
_TimerActivate(Timer_t *this, long ms, TimerHandler_t expirationHandler)
{
	this->expirationHandler = expirationHandler;
	this->ms = ms;
	this->proc = NxCurrentProc;
	_TimerInsert(this);
	return 0;
}


void
TimerCancelAll(void)
{
	if ( TimerList != NULL )
	{
		for (ObjectLink_t *link; (link = ObjectListFirst(TimerList)) != NULL; )
			TimerCancel((Timer_t*)link->var);
	}
}


/*+******************************************************************
	Name:
		TimerCancel - Cancel an active timer

	Synopsis:
		int TimerCancel (TimerId_t *this)

	Description:

		This expirationHandler allows an active timer to be removed from the
		timer queue thereby cancelling the timeout.

		If the expirationHandler pointer argument is non-null,
		the original expirationHandler and expirationHandler argument values that
		were specified in the TimerActivate expirationHandler will be returned to
		these pointer locations.

	Diagnostics:
		int is returned with a non 0 and defines the
		particular error that occurred during timeout cancellation.
-*******************************************************************/
int
TimerCancel(Timer_t *this)
{

	TimerVerify(this);

	_TimerYank(this);
	this->active = false;
	return 0;
}


int
TimerRefresh(Timer_t *this, long ms)
{
	TimerVerify(this);
	_TimerYank(this);
	this->ms = ms;
	_TimerInsert(this);
	return 0;					// done
}


/*+******************************************************************
	Name:
		_TimerFirstEntry - Get First Timer Entry

	Synopsis:
		Timer_t* _TimerFirstEntry ()

	Description:

	Diagnostics:
        If timer list is empty, returns NULL.
-*******************************************************************/
Timer_t *
_TimerFirstEntry()
{

	ObjectLink_t *link = ObjectListFirst(TimerList);
	if ( link != NULL )
			return (Timer_t*)link->var;
	return NULL;
}


static void
_TimerYank(Timer_t *this)
{

	if ( false )
		TimerQueueDump();

	ObjectLink_t *link;
	if ( (link = ObjectListGetLink(TimerList, this)) != NULL )
		ObjectListYank(TimerList, link);
}


static void
_TimerInsert(Timer_t *this)
{
// if already in timer list; remove it
	ObjectLink_t *link;
	if ( (link = ObjectListGetLink(TimerList, this)) != NULL )
		ObjectListYank(TimerList, link);

	this->expirationTime = this->ms + GetMsTime();	// future 'timeout' timeofday // calculate expiration time
	ObjectListAddOrdered(TimerList, this, &this->expirationTime, 0);
	this->active = true;
}


Json_t*
TimerSerialize(Timer_t *this)
{
	TimerVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddString(root, "Name", this->name);
	JsonAddBoolean(root, "Active", this->active);
	if ( this->active )
	{
		JsonAddString(root, "ExpirationTime", MsTimeToStringShort(this->expirationTime, NULL));
		JsonAddNumber(root, "MsTime", (double)(long)(this->expirationTime-(long)GetMsTime()));
		JsonAddNumber(root, "MsInterval", (double)this->ms);
	}

	return root;
}


char*
TimerToString(Timer_t *this)
{
	Json_t *root = TimerSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


static void
TimerQueueDump(void)
{

	char *tmp = alloca(ObjectListNbrEntries(TimerList)*512);
	char *out = tmp;
	*out = '\0';
	for (ObjectLink_t *link = ObjectListFirst(TimerList); link != NULL; link = ObjectListNext(link))
	{
		Timer_t *timer = (Timer_t*)link->var;
		out += sprintf(out, "%s%s", *tmp?"\n":"", TimerToString(timer));
	}
	SysLog(LogAny, "%d%s%s", ObjectListNbrEntries(TimerList), *tmp?"\n":"", tmp);
}
