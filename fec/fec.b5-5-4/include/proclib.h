/*****************************************************************************

Filename:   include/proclib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/include/proclib.h,v 1.3.4.3 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: proclib.h,v $
 Revision 1.3.4.3  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/11 19:47:32  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: proclib.h,v 1.3.4.3 2011/10/27 18:33:54 hbray Exp $ "


#ifndef _PROCLIB_H
#define _PROCLIB_H



// These values are used to identify the various Proc implementations which are started by the root process
// They are small numbers
// If a Proc tag is larger than the largest of these, ProcTagMax; then the tag value represents
// a pointer to the service table (ie; the tag is tagging a service implementation)
typedef enum
{
	ProcTagRoot = 0,
	ProcTagT70Proxy,
	ProcTagAuthProxy,
	ProcTagEdcProxy,
	ProcTagWorker,
	ProcTagAud,
	ProcTagWeb,
	ProcTagAuthSim,
	ProcTagEdcSim,
	ProcTagPosSim,
	ProcTagT70Sim,
	ProcTagMax
} ProcTag_t;


typedef enum
{
	CommandIdle = 0,
	CommandExecuting,
	CommandInCommandHandler,
} CommandState_t;


struct ProcContext_t;
struct NxClient_t;

typedef int (*ProcCommandHandler_t) (struct NxClient_t *client, Parser_t *parser, String_t *response);
typedef void (*ProcSignalHandler_t) (NxSignal_t*);


typedef struct Proc_t
{
	char					name[MaxNameLen];

	NxTime_t				timeCreated;
	char					procDir[MaxPropertyLen];
	char					sessionDir[MaxPropertyLen];

	int						ppid;				// parent
	int						pid;

	boolean					isActive;
	boolean					memoryLeakDetect;
	boolean					isClone;			// a clone...

	ProcTag_t				tag;

	struct Proc_t			*parent;

	HashMap_t				*childMap;		// hash map of children Proc_t*

	ProcCommandHandler_t	commandHandler;

	char					*(*contextToString)(struct ProcContext_t *this);
	Json_t					*(*contextSerialize)(struct ProcContext_t *this);

	int						killTime;
	Timer_t					*killTimer;

	struct NxServer_t		*commandServer;

	CommandState_t			commandState;
	int						commandDepth;

	struct ProcContext_t	*context;

	ProcSignalHandler_t		signalHandlers[_NSIG];	// array of ProcSignalHandler_t, keyed by signo
} Proc_t;


// External Functions
//

#define ProcNew(parent, name, tag, ...) ObjectNew(Proc, parent, name, tag, ##__VA_ARGS__)
#define ProcVerify(var) ObjectVerify(Proc, var)
#define ProcDelete(var) ObjectDelete(Proc, var)


extern Proc_t* ProcConstructor(Proc_t *this, char *file, int lno, Proc_t *parent, char *name, ProcTag_t tag, ...);
extern void ProcDestructor(Proc_t *this, char *file, int lno);
extern BtNode_t* ProcNodeList;
extern struct Json_t* ProcSerialize(Proc_t *this);
extern char* ProcToString(Proc_t *this);

extern int ProcStartV(Proc_t *child, int (*prelude) (Proc_t *), int (*startupEntry) (Proc_t *, va_list ap), va_list ap);
extern int ProcStart(Proc_t *child, int (*prelude) (Proc_t *), int (*startupEntry) (Proc_t *, va_list ap), ...);
extern int ProcClone(Proc_t *this, int (*prelude)(Proc_t*));
extern int ProcStop(Proc_t *this, int status);
extern int ProcSignal(Proc_t *this, int signo);
extern int ProcCmdSendResponse(Proc_t *this, struct NxClient_t *client, int status, char *bfr, int bfrlen);
extern Proc_t *ProcFindChildByPid(Proc_t *this, int pid);
extern char* ProcGetProcDir(Proc_t *this);

extern char *ProcTagToString(ProcTag_t type);

extern char* ProcGetPropertyValue(Proc_t *this, char *propName, ...);
#define ProcGetPropertyIntValue(proc, propName, ...) CnvStringIntValue(ProcGetPropertyValue(proc, propName, ##__VA_ARGS__))
#define ProcGetPropertyBooleanValue(proc, propName, ...) CnvStringBooleanValue(ProcGetPropertyValue(proc, propName, ##__VA_ARGS__))

extern void _ProcHandleSignals(Proc_t *this);
extern ProcSignalHandler_t ProcSetSignalHandler(Proc_t *this, int signo, ProcSignalHandler_t sigHandler);

#endif
