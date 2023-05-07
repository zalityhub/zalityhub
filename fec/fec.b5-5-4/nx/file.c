/*****************************************************************************

Filename:   lib/nx/file.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/file.c,v 1.3.4.2 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: file.c,v $
 Revision 1.3.4.2  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:44  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: file.c,v 1.3.4.2 2011/10/27 18:33:56 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/file.h"
#include "include/fifo.h"



BtNode_t *FileNodeList = NULL;


File_t*
FileConstructor(File_t *this, char *file, int lno)
{
	this->name = calloc(1, 1);
	this->mode = calloc(1, 1);
	return this;
}


void
FileDestructor(File_t *this, char *file, int lno)
{
	free(this->name);
	free(this->mode);

	if ( this->handle != NULL )
		fclose(this->handle);
}


Json_t*
FileSerialize(File_t *this)
{
	FileVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", this->name);
	JsonAddString(root, "Mode", this->mode);
	JsonAddBoolean(root, "IsOpen", (this->handle!=NULL));
	return root;
}


char*
FileToString(File_t *this)
{
	Json_t *root = FileSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


int
FileOpen(File_t *this, char *name, char *mode)
{

	FileVerify(this);

	if ( this->handle != NULL )		// a file is already open
	{
		SysLog(LogWarn, "%s is already open; closing", this->name);
		FileClose(this);
	}
	
	if ( (this->handle = fopen(name, mode)) == NULL )
	{
		SysLog(LogError, "Unable to open %s with mode %s; error %s", name, mode, ErrnoToString(errno));
		return -1;
	}

	this->name = strdup(name);
	this->mode = strdup(mode);
	return 0;
}


int
FileClose(File_t *this)
{

	FileVerify(this);

	if ( this->handle == NULL )
		SysLog(LogWarn, "%s is not open", this->name);
	else
		fclose(this->handle);

	this->handle = NULL;
	return 0;
}


int
FileRead(File_t *this, char *bfr, int len)
{

	FileVerify(this);

	if ( this->handle == NULL )
	{
		SysLog(LogError, "%s is not open", this->name);
		return -1;
	}

	int rlen = read(fileno(this->handle), bfr, len);
	if ( rlen < 0 )
		SysLog(LogError, "read of %d error %s: %s", len, ErrnoToString(errno), FileToString(this));

	return rlen;
}


struct Fifo_t*
FileFdReadContents(int fd)
{

	Fifo_t *fifo = FifoNew("%s.%d", __FUNC__, fd);

	for(;;)
	{
		char bfr[8192];
		int rlen = read(fd, bfr, sizeof(bfr));
		if ( rlen < 0 )
		{
			SysLog(LogError, "read of %d error %s", sizeof(bfr), ErrnoToString(errno));
			FifoDelete(fifo);
			break;
		}
		if ( rlen == 0 )
			break;		// done
		FifoWrite(fifo, bfr, rlen);
	}

	return fifo;
}


Fifo_t*
FileReadContents(char *name, ...)
{

	File_t *f = FileNew();

	StringNewStatic(tmp, 32);
	{
		va_list ap;
		va_start(ap, name);
		StringSprintfV(tmp, name, ap);
	}

	Fifo_t *fifo = FifoNew("%s", tmp->str);

	if ( FileOpen(f, tmp->str, "r") < 0 )
	{
		SysLog(LogError, "FileOpen failed: %s", tmp->str);
		FifoDelete(fifo);
		FileDelete(f);
		return NULL;
	}

	char bfr[8192];
	for(int rlen = 0; (rlen = FileRead(f, bfr, sizeof(bfr))) > 0;)
		FifoWrite(fifo, bfr, rlen);

	FileDelete(f);
	return fifo;
}
