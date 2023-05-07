/*****************************************************************************

Filename:   lib/nx/http.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/http.c,v 1.3.4.6 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: http.c,v $
 Revision 1.3.4.6  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:45  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/01 14:49:45  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/18 18:26:17  hbray
 release 5.5

 Revision 1.3.4.1  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: http.c,v 1.3.4.6 2011/10/27 18:33:57 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/http.h"
#include "include/fifo.h"


static Fifo_t* HttpRecvFifo(EventFile_t *this, int msTimeout, int flags);


char*
HttpGetTime(time_t tm, char *gmt)
{
	StringArrayStatic(sa, 16, 32);
	time_t ttm;
	struct tm gmtTm = { 0 };

	if ( (ttm = tm) == 0 )
		ttm = time(NULL);

	if ( gmt == NULL )
		gmt = StringArrayNext(sa)->str;

	// Get the GMT time for tm
	memcpy(&gmtTm, gmtime(&ttm), sizeof(gmtTm));

	// Format the timestamp, example below
	strftime(gmt, 2 + sizeof("Tue, 25-Nov-97 10:44:53 GMT"), "%a, %d-%b-%g %H:%M:%S GMT", &gmtTm);
	return (gmt);
}



BtNode_t *HttpRequestNodeList = NULL;


HttpRequest_t*
HttpRequestConstructor(HttpRequest_t *this, char *file, int lno)
{
	this->errorText = StringNew(32);
	this->methodText = StringNew(32);
	this->url = StringNew(32);
	this->path = StringNew(32);
	this->protocol = StringNew(32);
	this->args = HashMapNew(32, "HttpRequest UrlArgs");

	this->headers = HashMapNew(32, "HttpRequest Headers");
	
	this->content = FifoNew("HttpRequest Fifo");

	StringCpy(this->protocol, "HTTP/1.1");
	return this;
}


void
HttpRequestDestructor(HttpRequest_t *this, char *file, int lno)
{
	StringDelete(this->errorText);
	StringDelete(this->methodText);
	StringDelete(this->path);
	StringDelete(this->url);
	StringDelete(this->protocol);

	FifoDelete(this->content);

	HashClear(this->args, false);	// delete all args
	HashMapDelete(this->args);

	HashClear(this->headers, false);	// delete all headers
	HashMapDelete(this->headers);
}


Json_t*
HttpRequestSerialize(HttpRequest_t *this)
{
	HttpRequestVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	return root;
}


char*
HttpRequestToString(HttpRequest_t *this)
{
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);

	HttpRequestVerify(this);

	StringSprintf(out, "isValid=%s", this->isValid?"true":"false");
	if ( ! this->isValid )
		StringSprintfCat(out, ", errorText=%s", this->errorText->str);
	StringSprintfCat(out, ", method=%s", this->methodText->str);
	StringSprintfCat(out, ", url=%s", this->url->str);
	StringSprintfCat(out, ", path=%s", this->path->str);
	StringSprintfCat(out, ", protocol=%s", this->protocol->str);
	StringSprintfCat(out, ", content.len=%d", this->content->len);
	
	{ // all args
		char *sep = "";
		StringCat(out, ", args{");
		ObjectList_t* list = HashGetOrderedList(this->args, ObjectListStringType);
		for ( HashEntry_t *entry = NULL; (entry = (HashEntry_t*)ObjectListRemove(list, ObjectListFirstPosition)); )
		{
			StringSprintfCat(out, "%s%s=%s", sep, entry->string, (char*)entry->var);
			sep = ",";
			HashEntryDelete(entry);
		}
		StringCat(out, "}");
		ObjectListDelete(list);
	}

	{ // all headers
		char *sep = "";
		StringCat(out, ", hdrs{");
		ObjectList_t* list = HashGetOrderedList(this->headers, ObjectListStringType);
		for ( HashEntry_t *entry = NULL; (entry = (HashEntry_t*)ObjectListRemove(list, ObjectListFirstPosition)); )
		{
			StringSprintfCat(out, "%s%s=%s", sep, entry->string, (char*)entry->var);
			sep = ",";
			HashEntryDelete(entry);
		}
		StringCat(out, "}");
		ObjectListDelete(list);
	}

	return out->str;
}


static char*
HttpStripMaliciousChars(char *text)
{
	char *ch;
	while ( (ch = strchr(text, '%')) != NULL )
		strcpy(ch, ch+1);
	return text;
}


void
HttpRequestPutHeader(HttpRequest_t *this, char *hdr, ...)
{

	HttpRequestVerify(this);

	String_t *text = StringNew(32);
	{
		va_list ap;
		va_start(ap, hdr);
		StringSprintfV(text, hdr, ap);
	}

	Parser_t *tokens = ParserNew(strlen(text->str)+10);
	ParserSetInputData(tokens, text->str, strlen(text->str));

	char *name = FTrim(ParserGetNextToken(tokens, ":"), " \r\n");
	char *value = LTrim(ParserGetString(tokens), " ");

	if ( HashUpdateString(this->headers, name, StringNewCpy(value)) == NULL )
		SysLog(LogFatal, "Fatal error adding header %s", text->str);

	StringDelete(text);
	ParserDelete(tokens);
}


char*
HttpRequestGetHeader(HttpRequest_t *this, char *name)
{
	HttpRequestVerify(this);

	HashEntry_t *e;
	if ( (e = HashFindString(this->headers, name)) != NULL )
		return ((String_t*)(e->var))->str;
	return NULL;
}


void
HttpRequestClearContent(HttpRequest_t *this)
{
	
	HttpRequestVerify(this);
	FifoClear(this->content);
}


void
HttpRequestPutContent(HttpRequest_t *this, char *content)
{
	HttpRequestVerify(this);
	FifoWrite(this->content, content, strlen(content)+1);	// add plus the null terminator
}


int
HttpRequestSetMethodType(HttpRequest_t *this, HttpMethodType_t type)
{

	HttpRequestVerify(this);

	switch(type)
	{
		default:
			SysLog(LogError, "%d is an invalid HttpMethodType", type);
			return -1;
			break;

		case HttpMethodGet:
			StringCpy(this->methodText, "GET");
			break;

		case HttpMethodPost:
			StringCpy(this->methodText, "POST");
			break;
	}

	this->methodType = type;
	return 0;
}


char*
HttpRequestGetArg(HttpRequest_t *this, char *name)
{
	HttpRequestVerify(this);

	HashEntry_t *a;
	if ( (a = HashFindString(this->args, name)) != NULL )
		return ((String_t*)(a->var))->str;
	return NULL;
}


void
HttpRequestPutArg(HttpRequest_t *this, char *name, char *value)
{

	HttpRequestVerify(this);

	if ( HashUpdateString(this->args, name, StringNewCpy(value)) == NULL )
		SysLog(LogFatal, "Fatal error adding arg %s=%s", name, value);
}


char*
HttpRequestGetArgList(HttpRequest_t *this)
{

	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);

	HttpRequestVerify(this);

	StringClear(out);

	{ // all args
		char *sep = "?";
		ObjectList_t* list = HashGetOrderedList(this->args, ObjectListStringType);
		for ( HashEntry_t *entry = NULL; (entry = (HashEntry_t*)ObjectListRemove(list, ObjectListFirstPosition)); )
		{
			StringSprintfCat(out, "%s%s=%s", sep, EncodeUrlCharacters(entry->string, strlen(entry->string)), EncodeUrlCharacters((char*)entry->var, strlen((char*)entry->var)));
			sep = "&";
			HashEntryDelete(entry);
		}
		ObjectListDelete(list);
	}

	return out->str;
}


BtNode_t *HttpResponseNodeList = NULL;


HttpResponse_t*
HttpResponseConstructor(HttpResponse_t *this, char *file, int lno)
{
	this->errorText = StringNew(32);
	this->statusCode = StringNew(32);
	this->statusCodeText = StringNew(32);
	this->protocol = StringNew(32);

	this->headers = HashMapNew(32, "HttpResponse Headers");
	
	this->content = FifoNew("HttpResponse Fifo");

	return this;
}


void
HttpResponseDestructor(HttpResponse_t *this, char *file, int lno)
{
	StringDelete(this->errorText);
	StringDelete(this->statusCode);
	StringDelete(this->statusCodeText);
	StringDelete(this->protocol);

	FifoDelete(this->content);

	HashClear(this->headers, false);	// delete all headers
	HashMapDelete(this->headers);
}


Json_t*
HttpResponseSerialize(HttpResponse_t *this)
{
	HttpResponseVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	return root;
}


char*
HttpResponseToString(HttpResponse_t *this)
{
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);

	HttpResponseVerify(this);

	StringSprintf(out, "isValid=%s", this->isValid?"true":"false");
	if ( ! this->isValid )
		StringSprintfCat(out, ", errorText=%s", this->errorText->str);
	StringSprintfCat(out, ", statusCode=%s", this->statusCode->str);
	StringSprintfCat(out, ", statusCodeText=%s", this->statusCodeText->str);
	StringSprintfCat(out, ", protocol=%s", this->protocol->str);
	StringSprintfCat(out, ", content.len=%d", this->content->len);
	
	{ // all headers
		char *sep = "";
		StringCat(out, ", hdrs{");
		ObjectList_t* list = HashGetOrderedList(this->headers, ObjectListStringType);
		for ( HashEntry_t *entry = NULL; (entry = (HashEntry_t*)ObjectListRemove(list, ObjectListFirstPosition)); )
		{
			StringSprintfCat(out, "%s%s=%s", sep, entry->string, (char*)entry->var);
			sep = ",";
			HashEntryDelete(entry);
		}
		StringCat(out, "}");
		ObjectListDelete(list);
	}

	return out->str;
}


void
HttpResponsePutHeader(HttpResponse_t *this, char *hdr, ...)
{

	HttpResponseVerify(this);

	String_t *text = StringNew(32);
	{
		va_list ap;
		va_start(ap, hdr);
		StringSprintfV(text, hdr, ap);
	}

	Parser_t *tokens = ParserNew(strlen(text->str)+10);
	ParserSetInputData(tokens, text->str, strlen(text->str));

	char *name = FTrim(ParserGetNextToken(tokens, ":"), " \r\n");
	char *value = LTrim(ParserGetString(tokens), " ");

	if ( HashUpdateString(this->headers, name, StringNewCpy(value)) == NULL )
		SysLog(LogFatal, "Fatal error adding header %s", text->str);

	StringDelete(text);
	ParserDelete(tokens);
}


char*
HttpResponseGetHeader(HttpResponse_t *this, char *name)
{
	HttpResponseVerify(this);

	HashEntry_t *e;
	if ( (e = HashFindString(this->headers, name)) != NULL )
		return ((String_t*)e->var)->str;
	return NULL;
}


void
HttpResponseClearContent(HttpResponse_t *this)
{
	
	HttpResponseVerify(this);
	FifoClear(this->content);
}


void
HttpResponsePutContent(HttpResponse_t *this, char *content)
{
	HttpResponseVerify(this);
	FifoWrite(this->content, content, strlen(content)+1);	// add plus the null terminator
}



BtNode_t *HttpNodeList = NULL;


Http_t*
HttpConstructor(Http_t *this, char *file, int lno, char *name, ...)
{
	
	{
		String_t *tmp = StringNew(32);
		va_list ap;
		va_start(ap, name);
		StringSprintfV(tmp, name, ap);
		this->name = strdup(tmp->str);
		StringDelete(tmp);
	}
	return this;
}


void
HttpDestructor(Http_t *this, char *file, int lno)
{
	free(this->name);
}


Json_t*
HttpSerialize(Http_t *this)
{
	HttpVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	return root;
}


char*
HttpToString(Http_t *this)
{
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);

	HttpVerify(this);
	StringSprintf(out, "name=%s", this->name);
	return out->str;
}


HttpRequest_t*
HttpParseRequest(Http_t *this, char *text)
{

	HttpVerify(this);

	HttpRequest_t *req = HttpRequestNew();

	req->isValid = true;		// assume the best
	int lno = 0;
	Parser_t *lines = ParserNew(strlen(text)+10);
	ParserSetInputData(lines, text, strlen(text));
	boolean inContent = false;
	for(char *line = NULL; req->isValid && strlen(line = FTrim(ParserGetNextToken(lines, "\n"), " \r\n")) > 0; )
	{
		++lno;

		SysLog(LogDebug, "l%d: '%s'", lno, line);

		switch ( lno )
		{
			case 1:		// header line
			{
				Parser_t *tokens = ParserNew(strlen(line)+10);
				ParserSetInputData(tokens, line, strlen(line));

				int tno = 0;	// parse each token
				for(char *token = NULL; req->isValid && strlen(token = FTrim(ParserGetNextToken(tokens, " \r\n"), " \r\n")) > 0; )
				{
					++tno;
					SysLog(LogDebug, "w%d: '%s'", tno, token);
					switch(tno)
					{
						default:
							StringSprintf(req->errorText, "line %d: '%s' contains too many tokens: %d", lno, line, tno);
							SysLog(LogError, req->errorText->str);
							req->isValid = false;
							break;
						case 1:
							if ( stricmp(token, "GET") == 0 )
								req->methodType = HttpMethodGet;
							else if ( stricmp(token, "POST") == 0 )
								req->methodType = HttpMethodPost;
							else
							{
								StringSprintf(req->errorText, "line %d: '%s' does not contain a valid method (GET/POST): %s", lno, line, token);
								SysLog(LogError, req->errorText->str);
								req->isValid = false;
								break;
							}
							StringCpy(req->methodText, token);
							break;
						case 2:
							StringCpy(req->url, token);
							if ( strchr(token, '?') != NULL )		// have args, build arg table
							{
								char *ptr = strchr(token, '?');
								*ptr++ = '\0';
								StringCpy(req->path, token);		// args stripped
								Parser_t *args = ParserNew(strlen(req->url->str)+10);
								ParserSetInputData(args, ptr, strlen(ptr));
								int ano = 0;
								for(char *arg = NULL; req->isValid && strlen(arg = ParserGetNextToken(args, "&\r\n")) > 0; )
								{
									++ano;
									SysLog(LogDebug, "a%d: '%s'", ano, arg);
									String_t *atmp = StringNewCpy(arg);
									char *name = alloca(strlen(arg));
									char *value = alloca(strlen(arg));
									StringSplitPair(atmp, name, value, false);
									HttpRequestPutArg(req, HttpStripMaliciousChars(name), HttpStripMaliciousChars(value));
									StringDelete(atmp);
								}
								ParserDelete(args);
							}
							else		// no args, path is complete string
							{
								StringCpy(req->path, token);
							}
							break;
						case 3:
							StringCpy(req->protocol, token);
							break;
					}
				}

				ParserDelete(tokens);
				break;
			}

			default:		// a header or content line
			{
				if ( inContent )
					HttpRequestPutContent(req, line);
				else
					HttpRequestPutHeader(req, HttpStripMaliciousChars(line));
				break;
			}
		}
	}
	
	char *contentLength = HttpRequestGetHeader(req, "Content-Length");

	if ( contentLength != NULL )			// pull in content
	{
		// TODO: boolean err;
		// TODO: int len = IntFromString(contentLength, &err);
	}

	ParserDelete(lines);

	return req;
}


HttpResponse_t*
HttpParseResponse(Http_t *this, char *text)
{

	HttpVerify(this);

	HttpResponse_t *resp = HttpResponseNew();

	resp->isValid = true;		// assume the best
	int lno = 0;
	Parser_t *lines = ParserNew(strlen(text)+10);
	ParserSetInputData(lines, text, strlen(text));
	boolean inContent = false;
	for(char *line = NULL; resp->isValid && strlen(line = FTrim(ParserGetNextToken(lines, "\n"), " \r\n")) > 0; )
	{
		++lno;

		SysLog(LogDebug, "l%d: '%s'", lno, line);

		switch ( lno )
		{
			case 1:		// header line
			{
				Parser_t *tokens = ParserNew(strlen(line)+10);
				ParserSetInputData(tokens, line, strlen(line));

				int tno = 0;	// parse each token
				for(char *token = NULL; resp->isValid && strlen(token = FTrim(ParserGetNextToken(tokens, " \r\n"), " \r\n")) > 0; )
				{
					++tno;
					SysLog(LogDebug, "w%d: '%s'", tno, token);
					switch(tno)
					{
						default:
							StringSprintf(resp->errorText, "line %d: '%s' contains too many tokens: %d", lno, line, tno);
							SysLog(LogError, resp->errorText->str);
							resp->isValid = false;
							break;
						case 1:
							if ( strnicmp(token, "HTTP/1", 6) == 0 )
								StringCpy(resp->protocol, token);
							else
							{
								StringSprintf(resp->errorText, "line %d: '%s' does not contain a valid HTTP version: %s", lno, line, token);
								SysLog(LogError, resp->errorText->str);
								resp->isValid = false;
								break;
							}
							break;
						case 2:
							StringCpy(resp->statusCode, token);
							break;
						case 3:
							StringCpy(resp->statusCodeText, token);
							break;
					}
				}

				ParserDelete(tokens);
				break;
			}

			default:		// a header or content line
			{
				if ( inContent )
					HttpResponsePutContent(resp, line);
				else
					HttpResponsePutHeader(resp, HttpStripMaliciousChars(line));
				break;
			}
		}
	}

	char *contentLength = HttpResponseGetHeader(resp, "Content-Length");

	if ( contentLength != NULL )			// pull in content
	{
		// TODO: boolean err;
		// TODO: int len = IntFromString(contentLength, &err);
	}

	ParserDelete(lines);

	return resp;
}


static Fifo_t*
HttpRecvFifo(EventFile_t *this, int msTimeout, int flags)
{

	EventFileVerify(this);

	Fifo_t *fifo = FifoNew(this->name);

	{
		time_t xtime = time(NULL);
		xtime += ((msTimeout+500)/1000);     // no more than timeout (ms)

		for(;;)
		{
			char bfr[MaxSockPacketLen+10];	// allocate a buffer
			int rlen;
			if ((rlen = SockRecvRaw(this, bfr, sizeof(bfr), 0)) <= 0)
			{
				int sockErr = errno;
				if (sockErr != EAGAIN && sockErr != EINPROGRESS)
				{
					SysLog(LogError, "SockRecvRaw error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
					errno = sockErr;
					FifoDelete(fifo);
					break;		// error
				}
				errno = sockErr;
				rlen = 0;			// nothing read
			}

			FifoWrite(fifo, bfr, rlen);
			break;
		}
	}
	return fifo;
}


HttpRequest_t*
HttpRecvRequest(Http_t *this, EventFile_t *client, int msTimeout)
{

	HttpVerify(this);
	EventFileVerify(client);

	Fifo_t *fifo;
	if ( (fifo = HttpRecvFifo(client, 0, msTimeout)) == NULL )
	{
		SysLog(LogError, "HttpRecvFifo failed");
		return NULL;
	}

	SysLog(LogDebug, "\n%s\n", fifo->data);

	HttpRequest_t *req;
	if ( (req = HttpParseRequest(this, fifo->data)) == 0 || ! req->isValid )
		SysLog(LogError, "HttpParseRequest failed: %s", req?req->errorText->str:"");

	FifoDelete(fifo);
	return req;
}



HttpResponse_t*
HttpRecvResponse(Http_t *this, EventFile_t *client, int msTimeout)
{

	HttpVerify(this);
	EventFileVerify(client);

	Fifo_t *fifo;
	if ( (fifo = HttpRecvFifo(client, 0, msTimeout)) < 0 )
	{
		SysLog(LogError, "HttpRecvFifo failed");
		return NULL;
	}

	SysLog(LogDebug, "\n%s\n", fifo->data);

	HttpResponse_t *resp;
	if ( (resp = HttpParseResponse(this, fifo->data)) == 0 || ! resp->isValid )
		SysLog(LogError, "HttpParseResponse failed: %s", resp?resp->errorText->str:"");

	FifoDelete(fifo);
	return resp;
}


#if 0
/**
* Restructure chunked body into a contiguous string
* Body is expected to come in as any number of chunks: <length>CRLF<data>CRLF
*  with the last chunk as "0\r\n", where length is a base-16 string and data is 'length' characters
*/
int
HttpParseChunkedMsg(Http_t *this) 
{
	string      body;
	const char  *cursor = this->body.c_str();
	long        chunk   = -1;
	int         size    = 0; // size of new msg
	int         length  = this->body.length(); // length remaining of original msg

	while ( chunk != 0 ) 
	{
		int         i;
		char        *endP;
		const char  *chunkLen = cursor; // beginning of current chunk which should contain a hex length

		// look for hex numbers followed by optional data then\r\n

		// locate the end of the line and verify it has a value that will terminate strtol()
		for ( i = 0; *cursor != '\n' && *cursor != '\0' && i < length; i++, cursor++ )
		{
			// skip over hex length and any optional data that may be present which has no meaning to HTTP
		}

		if ( *cursor != '\n' )
		{
			return ( SysLog (LogError, -1, "Expected a newline, got " << *cursor) );
		}

		cursor++; // now points to beginning of chunk data

		chunk = strtol(chunkLen, &endP, 16); // will stop on the first non numeric
		if (endP <= chunkLen) // nothing found
		{
			return ( SysLog (LogError, -1, "Expected a hex number, got end of string. Parsed up to(" << i << ")") );
		}

		if ( chunk == LONG_MAX || chunk == LONG_MIN ) // bad value from strtol()
		{
			return ( SysLog (LogError, -1, uvxml::string::formatString("Chunked length string %.20s is out of range", chunkLen)) );
		}

		if ( chunk > 0 )
		{
			int proc_len = (chunk + 4 + i);

			if ( proc_len < 0 ) 
			{
				return ( SysLog (LogError, -1, uvxml::string::formatString("Chunk length %.20s (%ld) is less than 0", chunkLen, chunk)) );
			}

			if ( proc_len > length ) 
			{
				return ( SysLog (LogError, -1,
					uvxml::string::formatString("Chunk length %.20s (%ld) longer than remaining string (length %d) (max length %d)", chunkLen, chunk, length, this->body.length())));
			}

            string tmp(cursor, chunk);

            body   += tmp;
			cursor += chunk;
			if ( cursor[0] != '\r' || cursor[1] != '\n' ) 
			{
				return ( SysLog (LogError, -1, uvxml::string::formatString("No CRLF at end of chunk length %.20s (%ld). Instead found: %c%c", chunkLen, chunk, *cursor, cursor[1])));
			}
			cursor += 2;
			size   += chunk;
			length -= proc_len;
		}
	}

	this->body = body;

	return 0;
}
#endif


#if 0
#include "csocket.hpp"

void CHttpMsg:: Test()
{
	string  text;

	this->Init(CHttpMsg::HttpMethodGet, "news.google.com", "");
	this->AddHeader("Host", "news.google.com");
	this->AddHeader("User-Agent", "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.13; Google-TR-5.5.709.30344-en) Gecko/20080311 Firefox/2.0.0.13");
	this->AddHeader("Accept", "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5");
	this->AddHeader("Accept-Language", "en-us,en;q=0.5");
	this->AddHeader("Accept-Charset", "ISO-8859-1,evf-8;q=0.7,*;q=0.7");
	this->AddHeader("Keep-Alive", "300");
	this->AddHeader("Connection", "keep-alive");

	this->BuildMsg(text, false);
	cout << text << "\n";

	CSocket sock;
	sock.create();

	sock.connect("news.google.com", "80");
	sock.send(text);

	string resp;
	sock.recv(resp);
	cout << resp << "\n";

	this->ParseMsg(resp, true);
}
#endif


HttpResponse_t*
HttpGet(Http_t *this, HttpRequest_t *req, char *url, ...)
{

	HttpVerify(this);
	HttpRequestVerify(req);

	req->isValid = false;

	HttpRequestSetMethodType(req, HttpMethodGet);

	String_t *uriString = StringNew(32);
	{
		va_list ap;
		va_start(ap, url);
		StringSprintfV(uriString, url, ap);
	}

// parse the URI result

	HttpUri_t *uri = HttpUriNew();
	if ( HttpUriParse(uri, uriString->str) != 0 )
	{
		StringSprintf(req->errorText, "HttpUriParse failed with %s", uriString->str);
		SysLog(LogError, "%s", req->errorText->str);
		StringDelete(uriString);
		HttpUriDelete(uri);
		return NULL;
	}

	SysLog(LogDebug, "uri=%s", HttpUriToString(uri));

	HttpRequestPutHeader(req, "Host: %s", HttpStripMaliciousChars(uri->host->str));


	HttpResponse_t *resp = NULL;

	EventFile_t *client = EventFileNew();
	struct sockaddr_in *inaddr = (struct sockaddr_in*)uri->inetaddr->ai_addr;
	char *ip = IpAddrToString((unsigned char*)&inaddr->sin_addr);
	if (SockConnect(client, ip, uri->port, AF_INET, SOCK_STREAM) < 0)
	{
		SysLog(LogError, "SockConnect failed: %s:%s", uri->host, ip);
		resp = NULL;		// failed
	}
	else
	{
		// Build the Get
		String_t *payload = StringNew(32);
		StringCpy(req->path, uri->path->str);
		StringSprintf(req->url, "%s%s", req->path->str, HttpRequestGetArgList(req));
		StringSprintf(payload, "GET %s %s\r\n", req->url->str, req->protocol->str);
		// add headers
		for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(req->headers, entry)) != NULL;)
			StringSprintfCat(payload, "%s: %s\r\n", entry->string, (char*)entry->var);
		StringSprintfCat(payload, "Content-Length: %d\r\n", strlen(req->content->data));
		StringSprintfCat(payload, "\r\n");		// end of headers
		StringSprintfCat(payload, req->content->data);
		StringSprintfCat(payload, "\r\n");		// finalize

		SockSendRaw(client, payload->str, payload->len, 0);

		if ( (resp = HttpRecvResponse(this, client, 0)) == NULL )
			SysLog(LogError, "HttpRecvResponse failed");
		else
			SysLog(LogDebug, "%s", HttpResponseToString(resp));

		StringDelete(payload);
		SockClose(client);
	}

// Link the request/response
	if ( req != NULL )
		req->response = resp;
	if ( resp != NULL )
		resp->request = req;

	EventFileDelete(client);
	HttpUriDelete(uri);
	StringDelete(uriString);
	req->isValid = true;
	return resp;
}


HttpResponse_t*
HttpPost(Http_t *this, HttpRequest_t *req, char *url, ...)
{

	HttpVerify(this);
	HttpRequestVerify(req);

	req->isValid = false;

	HttpRequestSetMethodType(req, HttpMethodPost);

	String_t *uriString = StringNew(32);
	{
		va_list ap;
		va_start(ap, url);
		StringSprintfV(uriString, url, ap);
	}

// parse the URI result

	HttpUri_t *uri = HttpUriNew();
	if ( HttpUriParse(uri, uriString->str) != 0 )
	{
		StringSprintf(req->errorText, "HttpUriParse failed with %s", uriString->str);
		SysLog(LogError, "%s", req->errorText->str);
		StringDelete(uriString);
		HttpUriDelete(uri);
		return NULL;
	}

	SysLog(LogDebug, "uri=%s", HttpUriToString(uri));

	HttpRequestPutHeader(req, "Host: %s", HttpStripMaliciousChars(uri->host->str));


	HttpResponse_t *resp = NULL;

	EventFile_t *client = EventFileNew();
	struct sockaddr_in *inaddr = (struct sockaddr_in*)uri->inetaddr->ai_addr;
	char *ip = IpAddrToString((unsigned char*)&inaddr->sin_addr);
	if (SockConnect(client, ip, uri->port, AF_INET, SOCK_STREAM) < 0)
	{
		SysLog(LogError, "SockConnect failed: %s:%s", uri->host, ip);
		resp = NULL;		// failed
	}
	else
	{
		// Build the Post
		String_t *payload = StringNew(32);
		StringSprintf(payload, "POST %s %s\r\n", uri->path->str, req->protocol->str);
		// add headers
		for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(req->headers, entry)) != NULL;)
			StringSprintfCat(payload, "%s: %s\r\n", entry->string, (char*)entry->var);
		StringSprintfCat(payload, "Content-Length: %d\r\n", strlen(req->content->data));
		StringSprintfCat(payload, "\r\n");		// end of headers
		StringSprintfCat(payload, req->content->data);
		StringSprintfCat(payload, "\r\n");		// finalize

		SockSendRaw(client, payload->str, payload->len, 0);

		if ( (resp = HttpRecvResponse(this, client, 0)) == NULL )
			SysLog(LogError, "HttpRecvResponse failed");
		else
			SysLog(LogDebug, "%s", HttpResponseToString(resp));

		StringDelete(payload);
		SockClose(client);
	}

// Link the request/response
	if ( req != NULL )
		req->response = resp;
	if ( resp != NULL )
		resp->request = req;

	EventFileDelete(client);
	HttpUriDelete(uri);
	StringDelete(uriString);
	req->isValid = true;
	return resp;
}



BtNode_t *HttpUriNodeList = NULL;


HttpUri_t*
HttpUriConstructor(HttpUri_t *this, char *file, int lno)
{
	this->host = StringNew(32);
	this->service = StringNew(32);
	this->path = StringNew(32);
	return this;
}


void
HttpUriDestructor(HttpUri_t *this, char *file, int lno)
{
	StringDelete(this->host);
	StringDelete(this->service);
	StringDelete(this->path);

	if ( this->inetaddr )
		freeaddrinfo(this->inetaddr);
}


Json_t*
HttpUriSerialize(HttpUri_t *this)
{
	HttpUriVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	return root;
}


char*
HttpUriToString(HttpUri_t *this)
{
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);

	HttpUriVerify(this);

	StringSprintf(out, "host=%s", this->host->str);
	StringSprintfCat(out, ", service=%s", this->service->str);
	StringSprintfCat(out, ", path=%s", this->path->str);
	StringSprintfCat(out, ", port=%d", this->port);

	struct sockaddr_in *inaddr = (struct sockaddr_in*)this->inetaddr->ai_addr;
	if ( inaddr != NULL )
		StringSprintfCat(out, ", ip=%s", IpAddrToString((unsigned char*)&inaddr->sin_addr));
	
	return out->str;
}


int
HttpUriParse(HttpUri_t *this, char *uri)
{

	char *tmp = strdup(uri);
	char *ptr;
	char *trimmed = FTrim(tmp, " ");

	char *protocol = "http://";
	if ( strncmp(trimmed, protocol, strlen(protocol)) == 0 )
		trimmed += strlen(protocol);

	char *host = trimmed;
	char *service = "80";		// default
	char *path = "";		// default

	if ( (ptr = strchr(host, '/')) != NULL )
	{
		*ptr = '\0';
		path = ++ptr;
	}

	if ( (ptr = strchr(host, ':')) != NULL )
	{
		*ptr = '\0';
		service = ++ptr;
	}

	if ( strlen(host) <= 0 )
	{
		SysLog(LogError, "No host name given in uri: %s", uri);
		free(tmp);
		return -1;
	}

	boolean err;
	this->port = IntFromString(service, &err);
	if ( strlen(service) <= 0 || this->port <= 0 || err )
	{
		SysLog(LogError, "Bad port number given in uri: %s", uri);
		free(tmp);
		return -1;
	}

	if ( getaddrinfo(host, service, NULL, &this->inetaddr) != 0 || this->inetaddr == NULL )
	{
		SysLog(LogError, "Bad host name given in uri: %s; %d", uri, errno);
		free(tmp);
		return -1;
	}

	StringCpy(this->host, host);
	StringCpy(this->service, service);
	StringSprintf(this->path, "/%s", path);

	free(tmp);
	return 0;
}
