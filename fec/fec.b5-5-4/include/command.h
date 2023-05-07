/*****************************************************************************

Filename:   include/command.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/command.h,v 1.3.4.3 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: command.h,v $
 Revision 1.3.4.3  2011/10/27 18:33:52  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:35  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/11 19:47:31  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:33  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: command.h,v 1.3.4.3 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _COMMAND_H
#define _COMMAND_H

typedef enum
{
	CommandOk					= 0,
	CommandSiezedConnection		= 1,
	CommandResponded			= 2,
	CommandUnknown				= -1,
	CommandFailed				= -2,
	CommandBadOption			= -3
} CommandResult_t;


// Command structure

typedef struct Command_t
{
	struct NxClient_t			*client;
	struct CommandDef_t			*defs;		// null terminated array
} Command_t;


typedef struct CommandDef_t
{
	char *word;
	CommandResult_t (*func) (Command_t *cmd, Parser_t *parser, char *prior, String_t *response);
} CommandDef_t;


#define CommandNew(client, defs) ObjectNew(Command, client, defs)
#define CommandVerify(var) ObjectVerify(Command, var)
#define CommandDelete(var) ObjectDelete(Command, var)


// External Functions
extern Command_t* CommandConstructor(Command_t *this, char *file, int lno, struct NxClient_t *client, CommandDef_t *defs);
extern void CommandDestructor(Command_t *this, char *file, int lno);
extern BtNode_t* CommandNodeList;
extern struct Json_t* CommandSerialize(Command_t *this);
extern char* CommandToString(Command_t *this);

extern CommandResult_t CommandExecute(Command_t *this, Parser_t *parser, char *prior, String_t *response);

extern char *CommandResultToString(int result);

#endif
