/*****************************************************************************

Filename:	include/sdcdes.h

Purpose:	Marriott API Translator data encryption methods.

Advisory:	These methods use public mcrypt and mhash encryption libraries.
			Currently only block mode encryption is supported.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/08/23 19:53:58 $
 * $Header: /home/hbray/cvsroot/fec/include/sdcdes.h,v 1.2.4.1 2011/08/23 19:53:58 hbray Exp $
 *
 $Log: sdcdes.h,v $
 Revision 1.2.4.1  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.2  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: sdcdes.h,v 1.2.4.1 2011/08/23 19:53:58 hbray Exp $ "

#ifndef H_SDCDES
#define H_SDCDES

#include "des.h"
#include "des_crypt.h"

typedef struct desecb_t
{
	int		blkSize;
	int		keySize;
	char	key[1];
} *desecb_t;


extern int desecbStart(desecb_t *enc, char *key, int keysize);
extern char *desecbEncrypt(desecb_t enc, char *buf, int clrLen, int *encLen);
extern char *desecbDecrypt(desecb_t enc, char *buf, int encLen, int *clrLen);
extern void desecbEnd(desecb_t * enc);

#endif
