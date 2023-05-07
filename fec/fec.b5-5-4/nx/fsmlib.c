/*****************************************************************************

Filename:   lib/nx/fsmlib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/fsmlib.c,v 1.3.4.3 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: fsmlib.c,v $
 Revision 1.3.4.3  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:44  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: fsmlib.c,v 1.3.4.3 2011/10/27 18:33:56 hbray Exp $ "


#include <sys/types.h>
#include <sys/stat.h>

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/random.h"
#include "include/fsmlib.h"


// Static functions
static void HandleDeadmanEvent(Timer_t *tid);



BtNode_t *FsmNodeList = NULL;


Fsm_t*
FsmConstructor(Fsm_t *this, char *file, int lno,
				void (*eventHandler)(Fsm_t *this, int evt, void * efarg),
				char *(*stateToString)(Fsm_t *this, int state),
				char *(*eventToString)(Fsm_t *this, int evt),
				int initialState,
				char *name, ...)
{

	
	{
		va_list ap;
		va_start(ap, name);
		vsnprintf(this->name, sizeof(this->name), name, ap);
	}

	this->eventHandler = eventHandler;
	this->stateToString = stateToString;
	this->eventToString = eventToString;
	this->currentState = initialState;

	this->deadmanTimer = TimerNew("%sDeadmanTimer", NxCurrentProc->name);

// If a Deadman timer is requested; start it

	{
		char *prop = ProcGetPropertyValue(NxCurrentProc, "Deadman");

		if (prop != NULL)
		{
			if (isdigit(*prop) && atoi(prop) > 0)
			{
				this->deadmanValue = atoi(prop);
				int r = RandomRange(1, this->deadmanValue);
				SysLog(LogDebug, "Arming Deadman timer for %d seconds", r);
				this->deadmanTimer->context = this;
				TimerActivate(this->deadmanTimer, r * 1000, HandleDeadmanEvent);
			}
		}
	}

	return this;
}


void
FsmDestructor(Fsm_t *this, char *file, int lno)
{

	TimerDelete(this->deadmanTimer);
}


// Fires Random events to the fsm implentation; used for debugging
static void
HandleDeadmanEvent(Timer_t *tid)
{

	Fsm_t *this = (Fsm_t *)tid->context;
	FsmVerify(this);

	SysLog(LogWarn, "Event %s", (*(this)->eventToString) (this, (int)FsmDeadmanEvent));
	_FsmDeclareEvent(this, FsmDeadmanEvent, tid);

	int r = RandomRange(1, this->deadmanValue);

	SysLog(LogDebug, "Arming Deadman timer for %d seconds", r);
	TimerActivate(tid, r * 1000, HandleDeadmanEvent);
}


Json_t*
FsmSerialize(Fsm_t *this)
{
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", this->name);
	if ( this->deadmanValue != 0 && this->deadmanTimer != NULL )
		JsonAddItem(root, "Deadman", TimerSerialize(this->deadmanTimer));
	JsonAddString(root, "CurrentState", (*(this)->stateToString) (this, (this)->currentState));
	JsonAddString(root, "LastEvent", (*(this)->eventToString) (this, (this)->lastEvent));
	return root;
}


char*
FsmToString(Fsm_t *this)
{
	Json_t *root = FsmSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
_FsmSetNewState(Fsm_t *this, int state)
{
	FsmVerify(this);
	this->currentState = state;
}


void
_FsmDeclareEvent(Fsm_t *this, int event, void * efarg)
{

	FsmVerify(this);

	if (this->eventHandler == NULL)
		SysLog(LogFatal, "Fsm_t %s does not have an eventHandler", this->name);

	this->lastEvent = event;
	(*(this->eventHandler)) (this, event, efarg);
}
