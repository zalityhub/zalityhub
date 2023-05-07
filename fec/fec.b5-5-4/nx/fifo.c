/*****************************************************************************

Filename:   lib/nx/fifo.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/fifo.c,v 1.3.4.3 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: fifo.c,v $
 Revision 1.3.4.3  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:44  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: fifo.c,v 1.3.4.3 2011/10/27 18:33:56 hbray Exp $ "

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/fifo.h"




static int FifoShift(Fifo_t *this, int len);

static const int InitialFifoSize = 128;

static const int FifoPadLen = 10;
static const int FifoMinimumIncrement = 8192;


static inline void
FifoPad(Fifo_t *this)
{
	FifoVerify(this);
	memset(&this->data[this->len], 0, FifoPadLen);	// additional terminators for safety
}


static inline void
ExpandFifo(Fifo_t *this, int size)
{

	if ( size < FifoMinimumIncrement )
		size = FifoMinimumIncrement;

	this->currentSize += size;
	if ( (this->data = (char *)realloc(this->data, this->currentSize + FifoPadLen)) == NULL )
		SysLog(LogFatal, "No memory for %d", this->currentSize + FifoPadLen);
}



BtNode_t *FifoNodeList = NULL;



Fifo_t*
FifoConstructor(Fifo_t *this, char *file, int lno, char *name, ...)
{

	{
		va_list ap;
		va_start(ap, name);
		StringNewStatic(tmp, 32);
		StringSprintfV(tmp, name, ap);
		this->name = strdup(tmp->str);		// make a copy
	}

	this->currentSize = InitialFifoSize;
	if ((this->data = calloc(1, this->currentSize + FifoPadLen)) == NULL)
		SysLog(LogFatal, "calloc of initial data failed");

	FifoClear(this);
	return this;
}


void
FifoDestructor(Fifo_t *this, char *file, int lno)
{
	FifoClear(this);
	free(this->name);
	free(this->data);
}


Json_t*
FifoSerialize(Fifo_t *this)
{
	FifoVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", this->name);
	JsonAddNumber(root, "CurrentSize", this->currentSize);
	JsonAddNumber(root, "Len", this->len);
	return root;
}


char*
FifoToString(Fifo_t *this)
{
	Json_t *root = FifoSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
FifoClear(Fifo_t *this)
{
	FifoVerify(this);
	this->len = 0;
}


int
FifoWrite(Fifo_t *this, char *bfr, int len)
{
	FifoVerify(this);

	if (len < 0)
	{
		SysLog(LogError, "%s len %d too small", this->name, len);
		return -1;
	}

	if ((this->len + len) > this->currentSize)	// need more bfr space
		ExpandFifo(this, len);

	memcpy(&this->data[this->len], bfr, len);
	this->len += len;
	FifoPad(this);
	return 0;
}


static int
FifoShift(Fifo_t *this, int len)
{
	if (len == 0)
		return 0;

	FifoVerify(this);

	if (len < 0 || (this->len - len) < 0)
	{
		SysLog(LogError, "%s shift len %d exceeds the message container len of %d; failed", this->name, len, this->len);
		return -1;
	}

	// remove the shifted data
	memcpy(this->data, &this->data[len], this->len - len);
	if ((this->len -= len) <= 0)
		this->len = 0;			// keep this safe...

	FifoPad(this);
	return len;
}


int
FifoRead(Fifo_t *this, char *bfr, int len)
{
	FifoVerify(this);

	if (len < 0 || len > this->len)
	{
		SysLog(LogError, "%s len %d causes underflow; fifo len is %d; failed", this->name, len, this->len);
		return -1;
	}

	if (len > 0)
	{
		memcpy(bfr, this->data, len);
		FifoShift(this, len); // remove the data just read
	}

	return len;
}


int
FifoEntityRead(Fifo_t *this, char *bfr, int len)
{
	FifoVerify(this);

	if (len < 0)
	{
		SysLog(LogError, "%s len %d too small", this->name, len);
		return -1;
	}

	char *ibfr = this->data;
	int ilen = this->len;
	char *obfr = bfr;
	int olen = len;

	while (olen > 0)
	{
		if (ilen <= 0)
		{
			FifoShift(this, (ibfr - this->data));
			SysLog(LogError, "%s len %d exceeds the message container len of %d; failed", this->name, olen, ilen);
			return -1;
		}
		char *ip = DecodeEntityCharacter(ibfr, obfr);
		int elen = (ip - ibfr);

		if (elen <= 0)
		{
			SysLog(LogFatal, "This is really, really bad..., len=%d,bfr=%p,this->len=%d,this->data=%p,ip=%p,elen=%d,obfr=%p",
				   len, bfr, this->len, this->data, ip, elen, obfr);
			return -1;
		}
		ibfr += elen;
		ilen -= elen;
		++obfr;
		--olen;
	}

	len = ibfr - this->data;
	FifoShift(this, len); // remove the data just read
	return len;
}


int
FifoUrlRead(Fifo_t *this, char *bfr, int len)
{
	FifoVerify(this);

	if (len < 0)
	{
		SysLog(LogError, "%s len %d too small", this->name, len);
		return -1;
	}

	char *ibfr = this->data;
	int ilen = this->len;
	char *obfr = bfr;
	int olen = len;

	while (olen > 0)
	{
		if (ilen <= 0)
		{
			FifoShift(this, (ibfr - this->data));
			SysLog(LogError, "%s len %d exceeds the message container len of %d; failed", this->name, olen, ilen);
			return -1;
		}
		char *ip = DecodeUrlCharacter(ibfr, obfr);
		int elen = (ip - ibfr);

		if (elen <= 0)
		{
			SysLog(LogFatal, "This is really, really bad..., len=%d,bfr=%p,this->len=%d,this->data=%p,ip=%p,elen=%d,obfr=%p",
				   len, bfr, this->len, this->data, ip, elen, obfr);
			return -1;
		}
		ibfr += elen;
		ilen -= elen;
		++obfr;
		--olen;
	}

	len = ibfr - this->data;
	FifoShift(this, len);  // remove the data just read
	return len;
}


char*
FifoLineRead(Fifo_t *this, char *bfr, int len)
{
	FifoVerify(this);

	if (len < 1)
	{
		if ( len > 0 )
			*bfr = '\0';
		SysLog(LogError, "%s len %d too small", this->name, len);
		return NULL;
	}

	char *lf = strchr(this->data, '\n');
	if ( lf == NULL )
		lf = &this->data[this->len];
	int llen = lf - this->data;		// length of line
	if ( llen >= len )
		llen = len-1;		// if too big; make it one less than desired (need to allow for null char)

	*bfr = '\0';
	if ( llen > 0 )
	{
		memcpy(bfr, this->data, llen);
		bfr[llen] = '\0';		// provide termination
		FifoShift(this, llen); // remove the data just read
	}

	return bfr;
}
