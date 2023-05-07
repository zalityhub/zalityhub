/*****************************************************************************

Filename:   lib/nx/nxregex.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/Attic/nxregex.c,v 1.1.2.2 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: nxregex.c,v $
 Revision 1.1.2.2  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.1.2.1  2011/09/24 17:52:01  hbray
 New

 Revision 1.3.4.3  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.3.4.2  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.1  2011/08/15 19:12:31  hbray
 5.5 revisions

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:44  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: nxregex.c,v 1.1.2.2 2011/10/27 18:33:57 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/nxregex.h"
#include "include/parseobj.h"




BtNode_t *NxRegexNodeList = NULL;


NxRegex_t*
NxRegexConstructor(NxRegex_t *this, char *file, int lno, void *context)
{
	this->context = context;
	return this;
}


void
NxRegexDestructor(NxRegex_t *this, char *file, int lno)
{
	if (this->compiled)
		regfree(&this->regex);
	if(this->expression)
		StringDelete(this->expression);
}


Json_t*
NxRegexSerialize(NxRegex_t *this)
{
	NxRegexVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddBoolean(root, "Compiled", this->compiled);
	JsonAddBoolean(root, "IgnoreCase", this->ignoreCase);
	JsonAddString(root, "Expression", this->expression->str);
	return root;
}


char*
NxRegexToString(NxRegex_t *this)
{
	Json_t *root = NxRegexSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


int
NxRegexCompile(NxRegex_t *this, char *expression, String_t *response)
{
	NxRegexVerify(this);

	if(this->expression)
		StringDelete(this->expression);

	if (this->compiled)
		regfree(&this->regex);

	int rc = regcomp(&this->regex, expression, REG_EXTENDED | REG_NOSUB);	// Compile the regular expression 

	if (rc < 0)
	{
		char tmp[1024];
		regerror(rc, &this->regex, tmp, sizeof(tmp));
		StringSprintfCat(response, "regcomp failed: %s", tmp);
		SysLog(LogError, tmp);
		StringSprintfCat(response, "%s\n", tmp);
		regfree(&this->regex);
		return -1;
	}

	this->compiled = true;
	this->expression = StringNewCpy(expression);
	return 0;
}


int
NxRegexExecute(NxRegex_t *this, char *target)
{
	NxRegexVerify(this);
	int rc = regexec(&this->regex, target, 0, 0, 0);
	return rc;
}
