/*****************************************************************************

Filename:   main/htmltt.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/nx.h"
#include "include/fifo.h"
#include "include/file.h"

#define LIBXML_HTML_ENABLED 1
#include "libxml/HTMLparser.h"


#undef printf
#undef puts
#undef putchar


typedef struct HtmlSax_t
{
	int			tableDepth;
	int			trDepth;
	int			tdDepth;
	String_t	*out;
} HtmlSax_t ;


static void
SaxWarning(void *ctx, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    SysLogV(LogWarn, (char*)msg, args);
    va_end(args);
}


static void
SaxError(void *ctx, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    SysLogV(LogError, (char*)msg, args);
    va_end(args);
}


static void
SaxFatalError(void *ctx, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    SysLogV(LogFatal, (char*)msg, args);
    va_end(args);
}


static void
SaxStartElement(void *ctx, const xmlChar *name, const xmlChar **atts)
{
	// printf("%s\n", name);

	HtmlSax_t *html = (HtmlSax_t*)ctx;

	if ( strcmp((char*)name, "table") == 0 )
	{
		++html->tableDepth;
	}
	else if ( strcmp((char*)name, "tr") == 0 )
	{
		++html->trDepth;
	}
	else if ( strcmp((char*)name, "td") == 0 )
	{
		++html->tdDepth;
	}
}


static void
SaxEndElement(void *ctx, const xmlChar *name)
{
	// printf("%s\n", name);

	HtmlSax_t *html = (HtmlSax_t*)ctx;

	if ( strcmp((char*)name, "br") == 0 )
	{
		StringCat(html->out, "\n");
	}
	else if ( strcmp((char*)name, "table") == 0 )
	{
		--html->tableDepth;
	}
	else if ( strcmp((char*)name, "tr") == 0 )
	{
		--html->trDepth;
		StringCat(html->out, "\n");
	}
	else if ( strcmp((char*)name, "td") == 0 )
	{
		--html->tdDepth;
	}
}


static void
SaxTab(HtmlSax_t *html)
{

	for(int t = 0; t < html->tableDepth; ++t)
	{
		StringCat(html->out, "    ");
	}
	for(int d = 0; d < html->tdDepth; ++d)
	{
		StringCat(html->out, "    ");
	}
}


static void
SaxCharacters(void *ctx, const xmlChar *ch, int len)
{
	HtmlSax_t *html = (HtmlSax_t*)ctx;
	char *tmp = alloca(len+1);
	memcpy(tmp, FTrim((char*)ch," \t\r\n\f"), len);
	tmp[len] = '\0';

	if ( strlen(tmp) > 0 )
	{
		SaxTab(html);
		StringCat(html->out, tmp);
	}
}


static int
HtmlToText(char *html, HtmlSax_t *ctx, char *url)
{

	htmlParserCtxt *ctxt = htmlCreateMemoryParserCtxt((const char*)html, strlen(html));
	if (ctxt == NULL)
		SysLog(LogFatal, "htmlCreateMemoryParserCtxt failed");

	htmlSAXHandler sax;
	memset(&sax, 0, sizeof(sax));
	sax.warning = SaxWarning;
	sax.error = SaxError;
	sax.fatalError = SaxFatalError;
	sax.startElement = SaxStartElement;
	sax.endElement = SaxEndElement;
	sax.characters = SaxCharacters;
	ctxt->sax = &sax;
	ctxt->userData = ctx;

	htmlParseDocument(ctxt);

	int ret;
	if (ctxt->wellFormed)
	{
		ret = 0;
	}
	else
	{
		SysLog(LogError, "Failed to parse %s", url);
		ret = -1;
	}

	ctxt->sax = NULL;
	htmlFreeParserCtxt(ctxt);
	xmlCleanupParser();
	return ret;
}


int
main()
{

	Fifo_t *bfr = FileFdReadContents(fileno(stdin));
	if ( bfr == NULL )
		SysLog(LogFatal, "FileReadContents failed");

	HtmlSax_t *html = calloc(1, sizeof(HtmlSax_t));
	html->out = StringNew(32);
	int ret = HtmlToText(bfr->data, html, "stdin");
	FifoDelete(bfr);

	printf("%s", html->out->str);

	StringDelete(html->out);
	free(html);

	return ret;
}
