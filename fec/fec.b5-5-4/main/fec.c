/*****************************************************************************

Filename:   main/fec.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:00 $
 * $Header: /home/hbray/cvsroot/fec/main/fec.c,v 1.3.4.4 2011/10/27 18:34:00 hbray Exp $
 *
 $Log: fec.c,v $
 Revision 1.3.4.4  2011/10/27 18:34:00  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:52  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/24 13:22:29  hbray
 Renamed

 Revision 1.3.4.1  2011/08/17 17:59:05  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:24  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:57  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: fec.c,v 1.3.4.4 2011/10/27 18:34:00 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/nx.h"
#include "include/file.h"
#include "include/fifo.h"
#include "include/hostio.h"
#include "machine/include/root.h"
#include "include/bt.h"

// Start Here
//
#define VERSION_STAMP(version) char *NxVersion = "NxVersion: " NX_VERSION
VERSION_STAMP(NX_VERSION);		// create a grep'able string


int
main(int argc, char *argv[])
{

// Verify the HostHeader (FEC to Stratus) is exactly '55' characters; as required.

	if ( sizeof(HostFrameHeader_t) != 55 )
		NxCrash("HostFrameHeader_t is %d bytes; It must be 55; Terminating", sizeof(HostFrameHeader_t));

	{
		//	char				*name;
		//	boolean				required;
		//	boolean				dynamic;
		//	PropertySuffix_t	suffixes[16];
		static PropertyMap_t propertyMap[] =
		{
			{"Root.ConfigFileName",				true,	false, {{0}}},
			{"AuthProxy.Enabled",				true,	false, {{0}}},
			{"AuthProxy.HostAddr",				true,	false, {{0}}},
			{"AuthProxy.HostPort",				true,	false, {{0}}},
			{"AuthProxy.InputQueue",			true,	false, {{0}}},
			{"EdcProxy.Enabled",				true,	false, {{0}}},
			{"EdcProxy.HostAddr",				true,	false, {{0}}},
			{"EdcProxy.HostPort",				true,	false, {{0}}},
			{"EdcProxy.InputQueue",				true,	false, {{0}}},
			{"Service.HostAddr",				true,	false, {{0}}},
			{"T70Proxy.InputQueue",				true,	false, {{0}}},
			{"Platform.sysname",				true,   false, {{0}}},
			{"Platform.sysid",					true,   false, {{0}}},
			{NULL, 0}
		};

		// This call might result in an
		// error while loading shared libraries: lib/libnx.so: cannot restore segment prot after reloc: Permission denied
		// if The process ends abnormally with an error 127; try the command
		//  chcon -t texrel_shlib_t ${basedir}/lib/*
		// I cannot find much documentation of why this is necessary for some machines and not for others.
		NxInit("fec", NX_VERSION, LogWarn, true, 9500, propertyMap, argc, argv);
	}

// assimilate the args
	char *machine = NULL;
	while ( --argc > 0 )
	{
		char *arg = *++argv;
		if ( strcmp(arg, "-m") == 0 )
		{
			machine = *++argv;
			--argc;
		}
		else
		{
			SysLog(LogFatal, "%s is an invalid argument", arg);
		}
	}

// if -m machinename is an option, then start only that machine...
	if ( machine != NULL )
	{
		void *addr = GetSymbolAddr(NULL, machine);
		if ( addr == NULL )
			SysLog(LogFatal, "Unable to locate entry point %s", machine);
		Proc_t *proc = ProcNew(NULL, machine, ProcTagRoot);
		if (proc == NULL)
			SysLog(LogFatal, "Creation of %s failed", machine);
		ProcStart(proc, NULL, addr);
	}
	else // Start the root state machine
	{
	// Start the root state machine
		Proc_t *proc = ProcNew(NULL, "Root", ProcTagRoot);
		if (proc == NULL)
			SysLog(LogFatal, "Creation of Root failed");
		ProcStart(proc, NULL, ProcRootStart);
	}

	SysLog(LogDebug, "exit(0)");
	return 0;
}
