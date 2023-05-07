/*****************************************************************************

Filename:	lib/nx/sdcdes.c

Purpose:	Marriott API Translator data encryption methods.

Advisory:	These methods use Sun's RPC secure DES algorithms that have
	`		been licensed for public usage.  Currently only block mode
			encryption is supported.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:19 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/sdcdes.c,v 1.2 2011/07/27 20:22:19 hbray Exp $
 *
 $Log: sdcdes.c,v $
 Revision 1.2  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: sdcdes.c,v 1.2 2011/07/27 20:22:19 hbray Exp $ "

#include <stdlib.h>
#include <string.h>

#include "include/sdcdes.h"



#define	perror(msg) xperror(__FILE__,__LINE__,msg)
extern void xperror(char *, int, const char *);

int
desecbStart(desecb_t * enc, char *key, int keysize)
{
	desecb_t crypt = 0;

	/* Validate arguments */
	if (!(enc) || !(key))
		return (-1);

	/* Allocate cryptography control */
	if (!(crypt = calloc(sizeof(crypt[0]) + 32, 1)))
		return (-1);

	/* Initialize the control */
	crypt->blkSize = 8;
	crypt->keySize = keysize;
	memcpy(crypt->key, key, crypt->keySize);

	/* Return the allocated cryptography control */
	*enc = crypt;

	return (0);
}								/* int desecbStart(desecb_t *enc,char *key) */

static char *
desEcb(desecb_t enc, int mode, char *iBuf, int iLen, int *oLen)
{
	char *oBuf = 0;
	int len = 0;

	/* Validate arguments */
	if (!(enc) || !(iBuf) || 0 >= iLen || !(oLen) || 0 > mode	/* decrypt */
		|| 1 < mode)			/* encrypt */
		return (NULL);

	/* Initialize the return encryption buffer length to failure */
	*oLen = 0;

	/* Calculate encryption buffer on blkSize boundary */
	if ((len = iLen % enc->blkSize))
		len = iLen + enc->blkSize - len;
	else
		len = iLen;

	/* Allocate encryption buffer */
	if (!(oBuf = calloc(1, len)))
		return (NULL);

	/* Copy cleartext into encryption buffer */
	memcpy(oBuf, iBuf, iLen);

	/* Encrypt the buffer */
	mode = (mode) ? DES_ENCRYPT : DES_DECRYPT;
	switch (DES_FAILED(Ecb_Crypt(enc->key, oBuf, len, mode)))
	{
	case DESERR_NONE:
	default:
		break;

	case DESERR_BADPARAM:
		free(oBuf);
		len = 0;
		oBuf = NULL;
		break;
	}							/* switch(DES_ENCRYPT(ecb_crypt(enc->key,oBuf,len,mode))) */

	/* Update the return encryption buffer length */
	*oLen = len;

	return (oBuf);
}								/* char *desEcb(desecb_t enc,int mode,char *iBuf,int iLen,int *oLen) */

char *
desecbEncrypt(desecb_t enc, char *buf, int iLen, int *oLen)
{
	return (desEcb(enc, 1, buf, iLen, oLen));
}								/* char *desecbEncrypt(desecb_t enc,char *buf,int iLen,int *oLen) */

char *
desecbDecrypt(desecb_t enc, char *buf, int iLen, int *oLen)
{
	return (desEcb(enc, 0, buf, iLen, oLen));
}								/* char *desecbDecrypt(desecb_t enc,char *buf,int iLen,int *oLen) */

void
desecbEnd(desecb_t * enc)
{
	if ((enc) && (*enc))
		free(*enc), *enc = 0;
}								/* void desecbEnt(desecb_t *enc) */
