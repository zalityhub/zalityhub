/*****************************************************************************

Filename:   include/http.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/http.h,v 1.3.4.3 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: http.h,v $
 Revision 1.3.4.3  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: http.h,v 1.3.4.3 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _HTTP_H
#define _HTTP_H


extern char* HttpGetTime(time_t tm, char *gmt);


typedef enum { HttpMethodGet = 0, HttpMethodPost = 1 } HttpMethodType_t ;


typedef struct HttpRequest_t
{
	boolean					isValid;
	String_t				*errorText;		// why it's invalid...

	struct Fifo_t			*content;

	struct HttpResponse_t	*response;		// if available...

	String_t				*methodText;
	HttpMethodType_t		methodType;
	String_t				*url;			// has the args
	String_t				*path;			// args stripped
	String_t				*protocol;

	HashMap_t				*args;
	HashMap_t				*headers;		// hash map char* name,values
} HttpRequest_t ;


// Object Functions

#define HttpRequestNew() ObjectNew(HttpRequest)
#define HttpRequestVerify(var) ObjectVerify(HttpRequest, var)
#define HttpRequestDelete(var) ObjectDelete(HttpRequest, var)

extern HttpRequest_t* HttpRequestConstructor(HttpRequest_t *this, char *file, int lno);
extern void HttpRequestDestructor(HttpRequest_t *this, char *file, int lno);
extern BtNode_t* HttpRequestNodeList;
extern struct Json_t* HttpRequestSerialize(HttpRequest_t *this);
extern char* HttpRequestToString(HttpRequest_t *this);

extern char* HttpRequestGetHeader(HttpRequest_t *this, char *name);
extern void HttpRequestPutHeader(HttpRequest_t *this, char *hdr, ...);
extern char* HttpRequestGetArg(HttpRequest_t *this, char *name);
extern void HttpRequestPutArg(HttpRequest_t *this, char *name, char *value);
extern void HttpRequestClearContent(HttpRequest_t *this);
extern void HttpRequestPutContent(HttpRequest_t *this, char *content);


typedef struct HttpResponse_t
{
	boolean					isValid;
	String_t				*errorText;		// why it's invalid...

	struct Fifo_t			*content;
	
	struct HttpRequest_t	*request;		// if available...

	String_t				*statusCode;
	String_t				*statusCodeText;
	String_t				*protocol;

	HashMap_t				*headers;		// hash map char* name,values
} HttpResponse_t ;

// Object Functions

#define HttpResponseNew() ObjectNew(HttpResponse)
#define HttpResponseVerify(var) ObjectVerify(HttpResponse, var)
#define HttpResponseDelete(var) ObjectDelete(HttpResponse, var)

extern HttpResponse_t* HttpResponseConstructor(HttpResponse_t *this, char *file, int lno);
extern void HttpResponseDestructor(HttpResponse_t *this, char *file, int lno);
extern BtNode_t* HttpResponseNodeList;
extern struct Json_t* HttpResponseSerialize(HttpResponse_t *this);
extern char* HttpResponseToString(HttpResponse_t *this);

extern char* HttpResponseGetHeader(HttpResponse_t *this, char *name);
extern void HttpResponsePutHeader(HttpResponse_t *this, char *hdr, ...);
extern void HttpResponseClearContent(HttpResponse_t *this);
extern void HttpResponsePutContent(HttpResponse_t *this, char *content);


typedef struct Http_t
{
	char	*name;
	void	*context;		// user context... whatever...
} Http_t ;


// Object Functions

#define HttpNew(name,...) ObjectNew(Http, name, ##__VA_ARGS__)
#define HttpVerify(var) ObjectVerify(Http, var)
#define HttpDelete(var) ObjectDelete(Http, var)

extern Http_t* HttpConstructor(Http_t *this, char *file, int lno, char *name, ...);
extern void HttpDestructor(Http_t *this, char *file, int lno);
extern BtNode_t* HttpNodeList;
extern struct Json_t* HttpSerialize(Http_t *this);
extern char* HttpToString(Http_t *this);

extern HttpRequest_t *HttpRecvRequest(Http_t *this, EventFile_t *evf, int msTimeout);
extern HttpResponse_t *HttpRecvResponse(Http_t *this, EventFile_t *evf, int msTimeout);
extern HttpResponse_t* HttpGet(Http_t *this, HttpRequest_t *req, char *url, ...);
extern HttpResponse_t* HttpPost(Http_t *this, HttpRequest_t *req, char *url, ...);
extern HttpRequest_t* HttpParseRequest(Http_t *this, char *text);
extern HttpResponse_t* HttpParseResponse(Http_t *this, char *text);


typedef struct HttpUri_t
{
	struct addrinfo	*inetaddr;
	int				port;

	String_t		*host;
	String_t		*service;
	String_t		*path;
} HttpUri_t ;

#define HttpUriNew() ObjectNew(HttpUri)
#define HttpUriVerify(var) ObjectVerify(HttpUri, var)
#define HttpUriDelete(var) ObjectDelete(HttpUri, var)

extern HttpUri_t* HttpUriConstructor(HttpUri_t *this, char *file, int lno);
extern void HttpUriDestructor(HttpUri_t *this, char *file, int lno);
extern BtNode_t* HttpUriNodeList;
extern struct Json_t* HttpUriSerialize(HttpUri_t *this);
extern char* HttpUriToString(HttpUri_t *this);
extern int HttpUriParse(HttpUri_t *this, char *uriString);

#endif
