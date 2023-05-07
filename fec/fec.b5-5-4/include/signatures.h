/*****************************************************************************

Filename:   include/signatures.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:12 $
 * $Header: /home/hbray/cvsroot/fec/include/signatures.h,v 1.3 2011/07/27 20:22:12 hbray Exp $
 *
 $Log: signatures.h,v $
 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: signatures.h,v 1.3 2011/07/27 20:22:12 hbray Exp $ "

#ifndef _SIGNATURES_H
#define _SIGNATURES_H

#define SigVersionId	0xdead
#define DefineSignature(id) (((SigVersionId)<<16) | (id))
#define SignatureIsValid(o, s) ( (o) != NULL && (o)->signature == s )

#endif
