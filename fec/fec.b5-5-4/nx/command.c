/*****************************************************************************

Filename:   lib/nx/command.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:55 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/command.c,v 1.3.4.3 2011/10/27 18:33:55 hbray Exp $
 *
 $Log: command.c,v $
 Revision 1.3.4.3  2011/10/27 18:33:55  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:43  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:44  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: command.c,v 1.3.4.3 2011/10/27 18:33:55 hbray Exp $ "


#include <sys/types.h>
#include <sys/stat.h>

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/command.h"



BtNode_t *CommandNodeList = NULL;


Command_t*
CommandConstructor(Command_t *this, char *file, int lno, NxClient_t *client, CommandDef_t *defs)
{
	NxClientVerify(client);
	this->client = client;
	this->defs = defs;
	return this;
}


void
CommandDestructor(Command_t *this, char *file, int lno)
{
}


Json_t*
CommandSerialize(Command_t *this)
{
	CommandVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	Json_t *sub = JsonPushObject(root, "Defs");
	for (CommandDef_t *cmd = this->defs; cmd != NULL && cmd->word != NULL; ++cmd)
		JsonAddString(sub, cmd->word, "");
	JsonAddItem(root, "Client", NxClientSerialize(this->client));
	return root;
}


char*
CommandToString(Command_t *this)
{
	Json_t *root = CommandSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


CommandResult_t
CommandExecute(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{

	CommandVerify(this);

	++NxCurrentProc->commandDepth;

	CommandResult_t ret = CommandUnknown;

	char *word = ParserGetNextToken(parser, " ");

	if (strlen(word) > 0)
	{
		ParserDownshift(parser, word);

		for (CommandDef_t *cmd = this->defs; cmd != NULL && cmd->word != NULL; ++cmd)
		{
			if (strcmp(word, cmd->word) == 0)
			{
				char *p = alloca(strlen(prior) + strlen(cmd->word) + 32);

				sprintf(p, "%s %s", prior, cmd->word);
				prior = p;

				if (cmd->func == NULL)
					SysLog(LogError, "I don't have a command processor for '%s'", prior);
				else
					ret = (*cmd->func) (this, parser, prior, response);

				break;			// done
			}
		}
	}

	--NxCurrentProc->commandDepth;

	if (NxCurrentProc->commandDepth > 0 && ret == CommandUnknown)
		ret = CommandBadOption;	// we know the command; but, not the option...

	if (ret < 0)
	{
		if (strlen(response->str) <= 0)	// no help provided
		{
			for (CommandDef_t *c = this->defs; c != NULL && c->word != NULL; ++c)
				StringSprintfCat(response, "%s %s\n", prior, c->word);
		}
	}

	return ret;
}


char *
CommandResultToString(int result)
{
	char *text;

	switch (result)
	{
		default:
			text = StringStaticSprintf("CommandResult_%d", (int)result);
			break;

		EnumToString(CommandOk);
		EnumToString(CommandSiezedConnection);
		EnumToString(CommandResponded);
		EnumToString(CommandUnknown);
		EnumToString(CommandFailed);
		EnumToString(CommandBadOption);
	}

	if (strncmp(text, "Command", 7) == 0)
		text += 7;				// skip prefix

	return text;
}
