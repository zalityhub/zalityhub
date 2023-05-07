/*****************************************************************************

Filename:   include/nx.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/nx.h,v 1.3.4.7 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: nx.h,v $
 Revision 1.3.4.7  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.2  2011/08/11 19:47:32  hbray
 Many changes

 Revision 1.3.4.1  2011/08/01 16:11:28  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: nx.h,v 1.3.4.7 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _NXMAIN_H
#define _NXMAIN_H



typedef struct PropertySuffix_t
{
	char				suffix;
	int					val;
} PropertySuffix_t ;


typedef struct PropertyMap_t
{
	char				*name;
	boolean				required;
	boolean				dynamic;
	PropertySuffix_t	suffixes[16];
} PropertyMap_t;


typedef struct Nx_t
{

// This section must be first
// singletons
	boolean						dummy;

	boolean						steadyState;

	int							objLogPort;

	Stack_t						*signalStack;

	struct SysLog_t			*syslog;

	struct Audit_t				*audit;

	struct AuditCounts_t		*auditCounts;

	struct EventGlobal_t		*efg;

	struct Proc_t				*currentProc;

	void						*context;

	char						*sysname;
	char						*sysid;
	char						*nodename;
// End of order dependencies

	char						name[MaxNameLen];
	char						version[MaxNameLen];
	long						pagesize;

	int							argc;
	char						**argv;
	int							argvSize;

	int							maxFd;
	char						homeDir[MaxPropertyLen];
	char						nxDir[MaxPropertyLen];
	int							pid;
	char						pidFilePath[MaxPropertyLen];
	int							logMaxFileSize;
	char						auditExceptionFilePath[MaxPropertyLen];
	
	boolean						debugFatal;

	boolean						auditSessions;

	struct HashMap_t			*natList;		// A list of Pair_t*

	struct HashMap_t			*ignoreIpList;	// A hash of 4 byte Ip addresses

// This is a list of booleans, keyed by strings referring to trusted IP addresses
	struct HashMap_t			*trustedIpList;

	boolean						inAudit;

	struct NxSql_t				*sql;

	PropertyMap_t				*stdPropertyMap;
	PropertyMap_t				*propertyMap;

// This is a list of Properties_t*'s keyed by name
	struct HashMap_t			*propList;
} Nx_t;




extern Nx_t *NxGlobal;
#define NxCurrentProc (NxGlobal->currentProc)


// Utility functions

#define NxSetProp(name, value,...) _NxSetProp(NxGlobal, name, value, ##__VA_ARGS__)
extern int _NxSetProp(Nx_t *this, char *name, char *value, ...);

#define NxGetProp(name, ...) _NxGetPropFull(NxGlobal, false, name, ##__VA_ARGS__)
extern char* _NxGetPropFull(Nx_t *this, boolean resolveref, char *name, ...);

#define NxGetPropertyValue(name, ...) _NxGetPropertyValue(NxGlobal, name, ##__VA_ARGS__)
extern char* _NxGetPropertyValue(Nx_t *this, char *name, ...);
#define NxGetPropertyIntValue(name, ...) CnvStringIntValue(NxGetPropertyValue(name, ##__VA_ARGS__))
#define NxGetPropertyBooleanValue(name, ...) CnvStringBooleanValue(NxGetPropertyValue(name, ##__VA_ARGS__))

#define NxSetPropertyValue(name, value,...) NxSetProp(name, value, ##__VA_ARGS__)
#define NxSetPropertyIntValue(name, value) {char tmp[1024];NxSetPropertyValue(name, IntegerToString(value,tmp));}
#define NxSetPropertyBooleanValue(name, value) NxSetPropertyValue(name, BooleanToString(value))

#define NxGetPropertyListMatching(name, ...) _NxGetPropertyListMatching(NxGlobal, name, ##__VA_ARGS__)
extern ObjectList_t* _NxGetPropertyListMatching(Nx_t *this, char *name, ...);

#define NxSignalPush(nxSignal) _NxSignalPush(NxGlobal, nxSignal)
extern void _NxSignalPush(Nx_t *this, NxSignal_t *nxSignal);
extern void NxSetSignalHandlers(Nx_t *this);
extern struct sigaction NxSetSigAction(int signum, void (*sighandler) (int, siginfo_t *, void *));

extern int NxLogFileBackup(Nx_t *this, char *name);


// External Functions

extern BtNode_t* NxNodeList;
extern struct Json_t *NxSerialize(Nx_t *this);
extern char *NxToString(Nx_t *this);

extern void NxInit(char *pname, char *version, SysLogLevel level, boolean new, int objLogPort, PropertyMap_t *propertyMap, int argc, char *argv[]);

#define NxDoIdle() _NxDoIdle(NxGlobal)
extern void _NxDoIdle(Nx_t *this);

#define NxIgnoreThisIp(peerIp) _NxIgnoreThisIp(NxGlobal, peerIp)
extern int _NxIgnoreThisIp(Nx_t *this, unsigned char *peerIp);

#define NxTrustThisIp(peerIp) _NxTrustThisIp(NxGlobal, peerIp)
extern boolean _NxTrustThisIp(Nx_t *this, unsigned char *peerIp);

#define NxCrash(msg, ...) _NxCrash(NxGlobal, __FILE__, __LINE__, __FUNC__, msg, ##__VA_ARGS__)
extern void _NxCrash(Nx_t *this, char *file, int lno, const char *fnc, char *msg, ...);

extern void NxShutdown(Nx_t *this);

extern BtNode_t* CountsNodeList;
extern Json_t* CountsSerialize(Counts_t);
extern char* CountsToString(Counts_t);


static inline NxTime_t NxGetTime() { return GetUsTime(); }
static inline char *NxTimeToString(NxTime_t us, char *output) { return UsTimeToStringShort(us, output); }


static inline NxUid_t
NxUidNext()
{
	NxUid_t	uid;
	uuid_generate_random(uid.u);
	return uid;
}


static inline char*
NxUidToString(NxUid_t uid)
{
	StringArrayStatic(sa, 16, 33);
	char *out = StringArrayNext(sa)->str;
	char *op = out;
 	for (int i = 0; i < 16; ++i)
	{
 		int ch = ((uid.u[i] >> 4) & 0x0f) + '0';
 		if ( ch > '9' )
 			ch += 7 + ('a'-'A');
		*op++ = ch;
 		ch = ((uid.u[i]) & 0x0f) + '0';
 		if ( ch > '9' )
 			ch += 7 + ('a'-'A');
		*op++ = ch;
	}
	return out;
}


static inline NxUid_t
NxUidFromString(char *stg)
{
 	NxUid_t		uid;
	memset(&uid.u, 0, sizeof(uid.u));
	for (int i = 0; i < 16 && isxdigit(*stg); ++i)
	{
 		int ch;
		if ( (ch = (toupper(*stg) - '0')) > 9 )
 			ch -= 7;        // hex digit
 		uid.u[i] = (unsigned char)(ch << 4);
 		++stg;      // next nibble
		if ( (ch = (toupper(*stg) - '0')) > 9 )
 			ch -= 7;        // hex digit
 		uid.u[i] |= (unsigned char)(ch);
 		++stg;      // next nibble
	}

	return uid;
}


static inline unsigned long long
NxUidCompare(NxUid_t uid1, NxUid_t uid2)
{
	if ( uid1.ull[0] != uid2.ull[0] )
		return uid1.ull[0] - uid2.ull[0];
	return uid1.ull[1] - uid2.ull[1];
}

#endif
