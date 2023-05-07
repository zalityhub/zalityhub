/*****************************************************************************

Filename:   include/pisession.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/fecconfig.h,v 1.3.4.4 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: fecconfig.h,v $
 Revision 1.3.4.4  2011/10/27 18:33:52  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:34  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: fecconfig.h,v 1.3.4.4 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _FECCONFIG_H
#define _FECCONFIG_H


#define FecConfigLibSignature DefineSignature(0xbeaf)


typedef enum
{
	FecConfigEmpty, FecConfigConfiguring, FecConfigVerified, FecConfigUnavailable
} FecConfigStatus_t;


// POS Service Structure
//

typedef struct FecService_t
{
	// These are static properties loaded from the .ini file

	struct
	{
		int		serviceNumber;
		int		port;
		char	type[16];
		char	protocol[16];
		char	stratus1[32];
		char	stratus2[32];
		char	stratus3[32];
		char	additional[256];
	} properties;				// loaded from the fec.ini file

	PiProfile_t	plugin;			// Plug-in info

	// Dynamic Content; changed as needed
	FecConfigStatus_t	status;
} FecService_t;


// FEC Config Structure
//

typedef struct FecConfig_t
{
	// These are static properties loaded from the .ini file

	struct
	{
		int		diagnosticLevel;
		int		listenersPerService;
		int		minWorkerPool;
		int		maxWorkerPool;
		int		workerPoolIncrement;
		int		maxDedicatedConnects;
	} properties;

	// Dynamic Content; changed as needed
	FecConfigStatus_t	status;

	// Array of configured/active services
	int					numberServices;
	FecService_t		services[MAXSERVICES];
} FecConfig_t;


// External Functions

#define FecConfigNew() ObjectNew(FecConfig)
#define FecConfigNewShared(fname) ObjectNewShared(fname, FecConfig)
#define FecConfigMapShared(fname) ObjectMapShared(fname, FecConfig)
#define FecConfigVerify(var) ObjectVerify(FecConfig, var)
#define FecConfigDelete(var) ObjectDelete(FecConfig, var)
#define FecConfigSync(var) ObjectSync(FecConfig, var)

extern FecConfig_t* FecConfigConstructor(FecConfig_t *this, char *file, int lno);
extern void FecConfigDestructor(FecConfig_t *this, char *file, int lno);
extern BtNode_t* FecConfigNodeList;
extern struct Json_t* FecConfigSerialize(FecConfig_t *this);
extern char* FecConfigToString(FecConfig_t *this);

#define FecConfigGlobal ((FecConfig_t*)(NxGlobal->context))

extern int FecConfigCommit(FecConfig_t *this, FecConfig_t *from);
extern int FecConfigParseString(FecConfig_t *this, char *line);
extern void FecConfigClear(FecConfig_t *this);
extern FecService_t *FecConfigGetServiceContext(FecConfig_t *this, int port);

static inline void FecConfigLocalizeTODO()
{
	if ( ObjectIsShared(FecConfigGlobal) )		// if config is shared memory
		ObjectLock(FecConfigGlobal);				// lock it
	FecConfig_t *localConfig = FecConfigNew();
	memcpy(localConfig, FecConfigGlobal, sizeof(*localConfig));
	NxGlobal->context = localConfig;		// set FecConfigGlobal; TODO: fix LHS error
	if ( ObjectIsShared(FecConfigGlobal) )		// if config is shared memory
		ObjectUnlock(FecConfigGlobal);			// Unlock it
};

extern struct Json_t *FecConfigServiceSerialize(FecService_t *service);
extern char *FecConfigServiceToString(FecService_t *service);

#endif
