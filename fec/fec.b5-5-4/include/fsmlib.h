/*****************************************************************************

Filename:   include/fsmlib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/fsmlib.h,v 1.3.4.5 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: fsmlib.h,v $
 Revision 1.3.4.5  2011/10/27 18:33:52  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:42  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/11 19:47:32  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:34  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: fsmlib.h,v 1.3.4.5 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _FSMLIB_H
#define _FSMLIB_H

typedef struct Fsm_t
{
	char			name[MaxNameLen];

	void			(*eventHandler) (struct Fsm_t *fsm, int evt, void * efarg);
	char			*(*stateToString) (struct Fsm_t *fsm, int state);
	char			*(*eventToString) (struct Fsm_t *fsm, int evt);

	int				deadmanValue;
	Timer_t			*deadmanTimer;
	
	int				currentState;
	int				lastEvent;
} Fsm_t;


// Helper Macros (We use these so the current __FILE__, __LINE__ nbrs are used in logging)
//

#define FsmSetNewState(this, newState) \
        { \
            FsmVerify(this); \
            if ( (this)->currentState != newState ) \
            { \
                SysLog(LogDebug, "Change state from %s to %s", (*(this)->stateToString)(this, (this)->currentState), (*(this)->stateToString)(this, newState)); \
                _FsmSetNewState(this, newState); \
            } \
			return; \
        }

#define FsmDeclareEvent(this, event, efarg) \
        { \
            FsmVerify(this); \
            SysLog(LogDebug, "Event %s", (*(this)->eventToString)(this, event)); \
            _FsmDeclareEvent(this, event, efarg); \
			return; \
        }


// Used to fire random events
#define FsmDeadmanEvent (-1)



// External Functions
//

#define FsmNew(eventHandler, stateToString, eventToString, initialState, name, ...) ObjectNew(Fsm, eventHandler, stateToString, eventToString, initialState, name, ##__VA_ARGS__)
#define FsmVerify(var) ObjectVerify(Fsm, var)
#define FsmDelete(var) ObjectDelete(Fsm, var)


extern Fsm_t* FsmConstructor(Fsm_t *this, char *file, int lno,
				void (*eventHandler)(Fsm_t *this, int evt, void * efarg),
				char *(*stateToString)(Fsm_t *this, int state),
				char *(*eventToString)(Fsm_t *this, int evt),
				int initialState,
				char *name, ...);
extern void FsmDestructor(Fsm_t *this, char *file, int lno);
extern BtNode_t* FsmNodeList;
extern Json_t* FsmSerialize(Fsm_t *this);
extern char* FsmToString(Fsm_t *this);

extern void _FsmSetNewState(Fsm_t *this, int state);
extern void _FsmDeclareEvent(Fsm_t *this, int event, void * efarg);

#endif
