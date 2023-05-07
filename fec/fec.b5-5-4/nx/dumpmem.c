/*****************************************************************************

Filename:	lib/nx/dumpmem.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/02 14:17:02 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/dumpmem.c,v 1.2.4.1 2011/09/02 14:17:02 hbray Exp $
 *
 $Log: dumpmem.c,v $
 Revision 1.2.4.1  2011/09/02 14:17:02  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:44  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: dumpmem.c,v 1.2.4.1 2011/09/02 14:17:02 hbray Exp $ "


/*****************************************************************************

Filename:	lib/nx/dumpmem.c

Purpose:	General purpose memory dump utility.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/02 14:17:02 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/dumpmem.c,v 1.2.4.1 2011/09/02 14:17:02 hbray Exp $
 *
 $Log: dumpmem.c,v $
 Revision 1.2.4.1  2011/09/02 14:17:02  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:44  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: dumpmem.c,v 1.2.4.1 2011/09/02 14:17:02 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/stringobj.h"



// local scope variables

DumpRadix_t	DumpRadix = DumpRadixHex;


void
DumpmemFull(void (*put) (int offset, char *dump, char *text, void *arg), void *putarg, void *mem, int len, int *offset)
{
	const unsigned char *memPtr = (const unsigned char *)mem;
	int ii, jj, ofs;
	char hexbuf[128];
	char textbuf[64];
	char *hb = hexbuf;

	if (len <= 0)
		return;					// done

	ofs = 0;
	if (offset)
		ofs = *offset;

	for (hexbuf[0] = *textbuf = ii = jj = 0; ii < len; ++ii )
	{
		if (0 == ii || 0 == (ii % 16))		// line break
		{
			if ( hexbuf[0] != '\0' )		// have line data
				(*put) (ofs, hexbuf, textbuf, putarg);
			hb = hexbuf;
			*hb = jj = 0;
			if (ii)
				ofs += 16;
		}
		hb += sprintf(hb, "%2.2x ", (int)memPtr[ii] & 0xff);

		if (isprint((int)memPtr[ii]))
			textbuf[jj] = memPtr[ii];
		else
			textbuf[jj] = '.';
		textbuf[++jj] = 0;
	}

	if (hexbuf[0])
	{
		(*put) (ofs, hexbuf, textbuf, putarg);
		ofs += 16;
	}
	if (offset)
		*offset = ofs;
}


typedef struct DumpLine_t
{
	void (*put) (char *line, void *arg);
	void *arg;
} DumpLine_t ;

static void
_DumpLine(int offset, char *dump, char *text, void *arg)
{
	char tmp[BUFSIZ];
	if ( DumpRadix == DumpRadixHex )
		sprintf(tmp, "%6x: %-48.48s %-16.16s\n", offset, dump, text);
	else if ( DumpRadix == DumpRadixOctal )
		sprintf(tmp, "%6o: %-48.48s %-16.16s\n", offset, dump, text);
	else	// use decimal
		sprintf(tmp, "%6d: %-48.48s %-16.16s\n", offset, dump, text);
	DumpLine_t *dlt = (DumpLine_t*)arg;
	(*dlt->put)(tmp, dlt->arg);
}


void
Dumpmem(void (*put) (char *line, void *arg), void *putarg, void *mem, int len, int *offset)
{
	DumpLine_t dlt;
	dlt.put = put;
	dlt.arg = putarg;
	DumpmemFull(_DumpLine, (void*)&dlt, mem, len, offset);
}
