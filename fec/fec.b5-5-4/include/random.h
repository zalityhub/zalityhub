/*****************************************************************************

Filename:   include/random.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:12 $
 * $Header: /home/hbray/cvsroot/fec/include/random.h,v 1.2 2011/07/27 20:22:12 hbray Exp $
 *
 $Log: random.h,v $
 Revision 1.2  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: random.h,v 1.2 2011/07/27 20:22:12 hbray Exp $ "

#ifndef _RANDOM_H
#define	_RANDOM_H

extern int RandomNbr(void);
extern int RandomRange(int min, int max);
extern int RandomBoolean(int percentage);


#endif /* _RANDOM_H */
