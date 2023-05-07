/*****************************************************************************

Filename:   lib/nx/filefifo.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/filefifo.c,v 1.3.4.5 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: filefifo.c,v $
 Revision 1.3.4.5  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/24 17:49:44  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.1  2011/08/15 19:12:31  hbray
 5.5 revisions

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: filefifo.c,v 1.3.4.5 2011/10/27 18:33:56 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/filefifo.h"



BtNode_t *FileFifoNodeList = NULL;


FileFifo_t*
FileFifoConstructor(FileFifo_t *this, char *file, int lno, int contextLen)
{

	this->uid = NxUidNext();

// create a file name and a fifo file
	String_t *tmp = StringNew(32);
	StringSprintf(tmp, "%s/%s.fifo", ProcGetProcDir(NxCurrentProc), NxUidToString(this->uid));
	this->filePath = strdup(tmp->str);
	StringDelete(tmp);

// open the file
	FILE *f = fopen(this->filePath, "w");
	if ( f == NULL )
		SysLog(LogFatal, "Unable to open new fifo file %s; error %s", this->filePath, ErrnoToString(errno));
	fclose(f);		// then close it
	if (chmod(this->filePath, 0666) < 0)
		SysLog(LogWarn, "chmod() error=%s: {%s}", ErrnoToString(errno), this->filePath);

	if ( contextLen > 0 )
	{
		if ( (this->context = calloc(1, contextLen)) == NULL )
			SysLog(LogFatal, "Unable to allocate context contextlen of %d", contextLen);
	}
	return this;
}


void
FileFifoDestructor(FileFifo_t *this, char *file, int lno)
{
	if ( this->context != NULL )
		free(this->context);
	if ( unlink(this->filePath) != 0 )
		SysLog(LogError, "Unable to unlink %s; error=%s", this->filePath, ErrnoToString(errno));
	free(this->filePath);
}


unsigned int
FileFifoGetSize(FileFifo_t *this)
{
	FileFifoVerify(this);
	unsigned len = this->outputOffset - this->inputOffset;
	return len;
}


int
FileFifoRead(FileFifo_t *this, char *bfr, int len, boolean dump)
{
	FileFifoVerify(this);

	FILE *f = fopen(this->filePath, "r");
	if ( f == NULL )
		SysLog(LogFatal, "Unable to open fifo file %s; error %s", this->filePath, ErrnoToString(errno));

	if ( (this->inputOffset+len) > this->outputOffset )
	{
		SysLog(LogError, "fifo %s: length of %d (offset %ld) exceeds size of current file size %ld", this->filePath, len, (this->inputOffset+len), this->outputOffset);
		return -1;
	}

	unsigned long pos;
	if ( (pos = fseek(f, this->inputOffset, SEEK_SET)) != 0 )
		SysLog(LogFatal, "Unable to seek in fifo file %s; error %s; expected %ld, received %ld", this->filePath, ErrnoToString(errno), this->inputOffset, pos);

	if ( fread(bfr, len, 1, f) != 1 )
		SysLog(LogFatal, "Unable to write fifo file %s, len=%d; error %s", this->filePath, len, ErrnoToString(errno));
	
	SysLog(LogDebug, "Read %d bytes from %s", len, this->filePath);
	if ( dump )
		SysLog(LogDebug | SubLogDump, bfr, len, "");

	++this->counts.inPkts;
	this->counts.inChrs += len;

	this->inputOffset = ftell(f);
	fclose(f);		// then close it
	return len;		// done
}


int
FileFifoWrite(FileFifo_t *this, char *bfr, int len, boolean dump)
{
	FileFifoVerify(this);

	if ( dump )
		SysLog(LogDebug | SubLogDump, bfr, len, "");
	SysLog(LogDebug, "Adding %d bytes to %s", len, this->filePath);

	FILE *f = fopen(this->filePath, "a");
	if ( f == NULL )
		SysLog(LogFatal, "Unable to open fifo file %s; error %s", this->filePath, ErrnoToString(errno));

	if ( fwrite(bfr, len, 1, f) != 1 )
		SysLog(LogFatal, "Unable to write fifo file %s, len=%d; error %s", this->filePath, len, ErrnoToString(errno));

	++this->counts.outPkts;
	this->counts.outChrs += len;

	this->outputOffset = ftell(f);
	fclose(f);		// then close it
	return len;		// done
}


Json_t*
FileFifoSerialize(FileFifo_t *this)
{
	FileFifoVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Uid", NxUidToString(this->uid));
	JsonAddString(root, "FilePath", this->filePath);
	JsonAddNumber(root, "InputOffset", this->inputOffset);
	JsonAddNumber(root, "OutputOffset", this->outputOffset);
	JsonAddString(root, "Counts", CountsToString(this->counts));
	JsonAddNumber(root, "Context", (unsigned long)this->context);
	return root;
}


char*
FileFifoToString(FileFifo_t *this)
{
	Json_t *root = FileFifoSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}
