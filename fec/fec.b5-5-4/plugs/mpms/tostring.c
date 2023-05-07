/*****************************************************************************

Filename:	lib/mpms/tostring.c

Purpose:
			The load library tostring method.
			Called to return a string describing anything interesting about the plugin.

			Return a string

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:17 $
 * $Header: /home/hbray/cvsroot/fec/lib/mpms/tostring.c,v 1.2 2011/07/27 20:22:17 hbray Exp $
 *
 $Log: tostring.c,v $
 Revision 1.2  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:44  hbray
 Added cvs headers

 *
$History: $
2010.09.29 harold bray          Initial
*****************************************************************************/

#ident "@(#) $Id: tostring.c,v 1.2 2011/07/27 20:22:17 hbray Exp $ "

// Application plugin data/method header declarations

#include "data.h"


char*
ToString(PiSession_t *sess)
{

	if ( sess != NULL )
		PiSessionVerify(sess);

	return "mpms";
}
