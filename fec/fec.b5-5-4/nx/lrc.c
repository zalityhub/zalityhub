/*****************************************************************************

Filename:	lib/nx/lrc.c

Purpose:	Longitudinal Redundancy Check (LRC) calculator

			Returns the eight bit LRC of size bytes pointed to by data.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:18 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/lrc.c,v 1.2 2011/07/27 20:22:18 hbray Exp $
 *
 $Log: lrc.c,v $
 Revision 1.2  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: lrc.c,v 1.2 2011/07/27 20:22:18 hbray Exp $ "

int
CalcLrc(char *data, int size)
{
	int lrc = 0;
	char *bp = data;

	for (; (size--); bp++)
	{
		lrc = lrc ^ *bp;
	}

	return (lrc);
}
