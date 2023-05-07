/*****************************************************************************

Filename:   include/timerlib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/include/timerlib.h,v 1.3.4.2 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: timerlib.h,v $
 Revision 1.3.4.2  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:39  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: timerlib.h,v 1.3.4.2 2011/10/27 18:33:54 hbray Exp $ "


#ifndef _TIMERLIB_H
#define _TIMERLIB_H

struct Timer_t;
typedef void (*TimerHandler_t)(struct Timer_t *tim);

typedef struct Timer_t
{
	char			name[MaxNameLen];
	boolean			active;
	long			ms;			// the original requested interval
	NxTime_t		expirationTime;	// the time when this is to be expired

	TimerHandler_t	expirationHandler;

	void			*context;
	struct Proc_t	*proc;
} Timer_t;


#define TimerNew(name, ...) ObjectNew(Timer, name, ##__VA_ARGS__)
#define TimerVerify(var) ObjectVerify(Timer, var)
#define TimerDelete(var) ObjectDelete(Timer, var)

extern Timer_t* TimerConstructor(Timer_t *this, char *file, int lno, char *name, ...);
extern void TimerDestructor(Timer_t *this, char *file, int lno);
extern BtNode_t* TimerNodeList;
extern struct Json_t* TimerSerialize(Timer_t *this);
extern char* TimerToString(Timer_t *this);


// External Functions

extern int TimerActivate(Timer_t *this, long ms, TimerHandler_t expirationHandler);
extern void TimerCancelAll(void);
extern int TimerCancel(Timer_t *this);
extern int TimerRefresh(Timer_t *this, long ms);
extern Timer_t *_TimerFirstEntry();

#endif
