/*****************************************************************************

Filename:   include/nxsignal.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/nxsignal.h,v 1.3.4.2 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: nxsignal.h,v $
 Revision 1.3.4.2  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: nxsignal.h,v 1.3.4.2 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _NXSIGNAL_H
#define _NXSIGNAL_H



typedef struct NxSignal_t
{
	siginfo_t	si;
} NxSignal_t ;


#define NxSignalNew(si) ObjectNew(NxSignal, si)
#define NxSignalVerify(var) ObjectVerify(NxSignal, var)
#define NxSignalDelete(var) ObjectDelete(NxSignal, var)


#define NxSigCrash		SIGUSR1		// used to crash the system

extern NxSignal_t* NxSignalConstructor(NxSignal_t *this, char *file, int lno, siginfo_t *si);
extern void NxSignalDestructor(NxSignal_t *this, char *file, int lno);
extern BtNode_t* NxSignalNodeList;
extern struct Json_t* NxSignalSerialize(NxSignal_t *this);
extern char* NxSignalToString(NxSignal_t *this);

extern char* NxSignalNbrToString(int signo);

#endif
