/*****************************************************************************

Filename:   lib/nx/fecconfig.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/fecconfig.c,v 1.3.4.6 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: fecconfig.c,v $
 Revision 1.3.4.6  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:43  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:45  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: fecconfig.c,v 1.3.4.6 2011/10/27 18:33:56 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#include <dlfcn.h>



// Local Functions
static int ParseConfigLine(FecConfig_t *this, char *line);
static int FecServiceValidate(FecService_t *this, FecConfig_t *config);
static int LoadFecPlugIn(FecConfig_t *this, int sn, FecService_t *service);


BtNode_t *FecConfigNodeList = NULL;


FecConfig_t*
FecConfigConstructor(FecConfig_t *this, char *file, int lno)
{
	FecConfigClear(this);
	return this;
}


void
FecConfigDestructor(FecConfig_t *this, char *file, int lno)
{
}


void
FecConfigClear(FecConfig_t *this)
{

	FecConfigVerify(this);

	memset(this, 0, sizeof(*this));
}


char *
FecConfigStatusToString(FecConfigStatus_t status)
{
	char *text;

	switch (status)
	{
		default:
			text = StringStaticSprintf("FecConfigStatus_t_%d", (int)status);
			break;

		EnumToString(FecConfigEmpty);
		EnumToString(FecConfigConfiguring);
		EnumToString(FecConfigVerified);
		EnumToString(FecConfigUnavailable);
	}

	return &text[strlen("FecConfig")];	// remove the prefix "FecConfig"
}


Json_t *
FecConfigServiceSerialize(FecService_t *this)
{
	Json_t *root = JsonNew(__FUNC__);
	JsonAddNumber(root, "ServiceNumber", this->properties.serviceNumber);
	JsonAddString(root, "Status", FecConfigStatusToString(this->status));
	JsonAddNumber(root, "Port", this->properties.port);
	JsonAddString(root, "Type", this->properties.type);
	JsonAddString(root, "Protocol", this->properties.protocol);
	JsonAddString(root, "Stratus1", this->properties.stratus1);
	JsonAddString(root, "Stratus2", this->properties.stratus2);
	JsonAddString(root, "Stratus3", this->properties.stratus3);
	JsonAddString(root, "Additional", this->properties.additional);
	JsonAddString(root, "Status", FecConfigStatusToString(this->status));
	return root;
}


char *
FecConfigServiceToString(FecService_t *this)
{
	Json_t *root = FecConfigServiceSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


Json_t *
FecConfigSerialize(FecConfig_t *this)
{
	FecConfigVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	Json_t *sub = JsonPushObject(root, "Properties");
	JsonAddString(sub, "Status", FecConfigStatusToString(this->status));
	JsonAddNumber(sub, "DiagnosticLevel", this->properties.diagnosticLevel);
	JsonAddNumber(sub, "ListenersPerService", this->properties.listenersPerService);
	JsonAddNumber(sub, "MinWorkerPool", this->properties.minWorkerPool);
	JsonAddNumber(sub, "MaxWorkerPool", this->properties.maxWorkerPool);
	JsonAddNumber(sub, "WorkerPoolIncrement", this->properties.workerPoolIncrement);
	JsonAddNumber(sub, "MaxDedicatedConnects", this->properties.maxDedicatedConnects);
	JsonAddNumber(sub, "NumberServices", this->numberServices);

	sub = JsonPushObject(root, "Services");
	for (int i = 0; i < MAXSERVICES; ++i)
	{
		if (this->services[i].status != FecConfigEmpty)
			JsonAddItem(sub, "Service", FecConfigServiceSerialize(&this->services[i]));
	}
	return root;
}


char *
FecConfigToString(FecConfig_t *this)
{
	Json_t *root = FecConfigSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


static int
FecServiceValidate(FecService_t *this, FecConfig_t *config)
{

	FecConfigVerify(config);
	
	SysLog(LogDebug, "Attempting to commit service %s.%d", this->properties.type, this->properties.serviceNumber);

	if (this->properties.serviceNumber > MAXSERVICES)
	{
		SysLog(LogError, "ServiceNumber %d is too large; maximum of %d is allowed", this->properties.serviceNumber, MAXSERVICES);
		SysLog(LogError, "Service: {%s}", FecConfigServiceToString(this));
		return -1;
	}

	int rr;

	if ((rr = LoadFecPlugIn(config, this->properties.serviceNumber, this)) < 0)
	{
		SysLog(LogWarn, "LoadFecPlugIn failed (%s)", FecConfigServiceToString(this));
		return -1;
	}

	if (rr > 0)					// failed to load
	{
		this->status = FecConfigUnavailable;
		SysLog(LogWarn, "Plugin is not available Service {%s}", FecConfigServiceToString(this));
		return 0;
	}

	this->status = FecConfigVerified;

	return 0;
}


static int
FecConfigValidate(FecConfig_t *this)
{
	FecConfigVerify(this);

// Adjust configured number of services/workers
	int workersNeeded = this->numberServices * this->properties.listenersPerService;

	// Round workersNeeded to a multiple of sixteen(16), a nice round number
	workersNeeded = (workersNeeded / 16 + 1) * 16;

	// Must have at least one worker for each LISTEN (accept) handler
	if (this->properties.minWorkerPool < workersNeeded)
		this->properties.minWorkerPool = workersNeeded;

	this->properties.minWorkerPool = (this->properties.minWorkerPool / 16 + 1) * 16;

	// Must increase the worker pool at handler multiples
	if (this->properties.workerPoolIncrement < this->properties.minWorkerPool)
		this->properties.workerPoolIncrement = this->properties.minWorkerPool;

	this->properties.workerPoolIncrement = (this->properties.workerPoolIncrement / 16 + 1) * 16;

	// Must have a larger maximum worker pool value
	if (this->properties.maxWorkerPool < this->properties.minWorkerPool)
		this->properties.maxWorkerPool = this->properties.minWorkerPool;

	// Maximum worker pool value must be a multiple of worker pool increment
	this->properties.maxWorkerPool = this->properties.workerPoolIncrement * (this->properties.maxWorkerPool / this->properties.workerPoolIncrement + 1);

// Validate each configured service
	for (int i = 0; i < MAXSERVICES; ++i)
	{
		if (this->services[i].status == FecConfigConfiguring)
		{
			if (FecServiceValidate(&this->services[i], this) != 0)
			{
				SysLog(LogError, "FecServiceValidate failed");
				return -1;
			}
		}
	}

	return 0;
}


int
FecConfigCommit(FecConfig_t *this, FecConfig_t *from)
{
	FecConfigVerify(this);
	FecConfigVerify(from);

	if ( FecConfigValidate(from) != 0 )
	{
		SysLog(LogError, "FecConfigValidate failed");
		return -1;
	}

// Ok, Looks good; copy it over
	if ( ObjectIsShared(this) )		// if this is shared memory
		ObjectLock(this);				// lock it

	memcpy(this, from, sizeof(*this));
	this->status = FecConfigVerified;

	if ( ObjectIsShared(this) )		// if this is shared memory
		ObjectUnlock(this);			// unlock it

	return 0;
}


static int
ParseConfigLine(FecConfig_t *this, char *line)
{
	char token[256];
	char value[2048];
	boolean err = false;
	boolean valueTooLong = false;

	FecConfigVerify(this);

// Strip off leading whitespace
	char *bp = NULL;
	for (bp = line; isspace(*bp); ++bp) ;

	if (strlen(bp) <= 0)
		return 0;				// just a bunch of white 'stuff'...

	if (!parseIni(bp, token, value))
	{
		SysLog(LogError, "parseIni failed parsing %s", line);
		return -1;
	}

	if (stricmp(token, "end.of.config") == 0)
	{
		SysLog(LogDebug, "Hit end of config");
		return 1;
	}

	else if (striprefix(token, "diagnostic.level") == 0)
	{
		this->properties.diagnosticLevel = IntFromString(value, &err);
	}
	else if (striprefix(token, "min.listeners.per.port") == 0)
	{
		this->properties.listenersPerService = IntFromString(value, &err);
	}
	else if (striprefix(token, "min.generic.worker.pool.size") == 0)
	{
		this->properties.minWorkerPool = IntFromString(value, &err);
	}
	else if (striprefix(token, "max.generic.worker.pool.size") == 0)
	{
		this->properties.maxWorkerPool = IntFromString(value, &err);
	}
	else if (striprefix(token, "additional.generic.worker.increment") == 0)
	{
		this->properties.workerPoolIncrement = IntFromString(value, &err);
	}
	else if (striprefix(token, "max.dedicated.worker.connections") == 0)
	{
		this->properties.maxDedicatedConnects = IntFromString(value, &err);
	}

	// Ok, not a common property; check for a service definition
	else if (striprefix(token, "fec.service.") == 0)
	{
		int serviceNumber = this->numberServices - 1;

		if (serviceNumber < 0)
			serviceNumber = 0;
		FecService_t *service = &this->services[serviceNumber];

		char *sptr = &token[strlen("fec.service.")];

		if (stricmp(sptr, "port") == 0)
		{
			if (++this->numberServices > MAXSERVICES)
			{
				SysLog(LogError, "ServiceNumber %d is too large; maximum of %d is allowed", this->numberServices, MAXSERVICES);
				SysLog(LogError, "Configuration statement: %s", line);
				return -1;
			}

			serviceNumber = this->numberServices - 1;
			if (serviceNumber < 0)
				serviceNumber = 0;
			service = &this->services[serviceNumber];

			service->status = FecConfigConfiguring;
			service->properties.serviceNumber = serviceNumber;

			int port = IntFromString(value, &err);

			// Make sure the port has not been previously configured
			for (int ss = 0; ss < MAXSERVICES; ++ss)
			{
				if (this->services[ss].properties.port == port)
				{
					SysLog(LogError, "Port %d of port service %d has been previously configured", port, serviceNumber);
					SysLog(LogError, "Configuration statement: %s", line);
					return -1;
				}
			}

			service->properties.port = port;
		}
		else if (stricmp(sptr, "type") == 0)
		{
			sprintf(service->properties.type, "%.*s", (int)sizeof(service->properties.type) - 1, value);
			err = valueTooLong = (strlen(service->properties.type) != strlen(value));
		}
		else if (stricmp(sptr, "load") == 0)
		{
			sprintf(service->properties.protocol, "%.*s", (int)sizeof(service->properties.protocol) - 1, value);
			err = valueTooLong = (strlen(service->properties.protocol) != strlen(value));
		}
		else if (striprefix(sptr, "stratus") == 0)
		{
			int n = IntFromString(&sptr[8], &err);

			err = (sptr[7] != '.');
			if (!err)
			{
				switch (n)
				{
				default:
					SysLog(LogError, "%d is not a valid Stratus number; must be 1, 2 or 3");
					err = 1;
					break;
				case 1:
					sprintf(service->properties.stratus1, "%.*s", (int)sizeof(service->properties.stratus1) - 1, value);
					err = valueTooLong = (strlen(service->properties.stratus1) != strlen(value));
					break;
				case 2:
					sprintf(service->properties.stratus2, "%.*s", (int)sizeof(service->properties.stratus2) - 1, value);
					err = valueTooLong = (strlen(service->properties.stratus2) != strlen(value));
					break;
				case 3:
					sprintf(service->properties.stratus3, "%.*s", (int)sizeof(service->properties.stratus3) - 1, value);
					err = valueTooLong = (strlen(service->properties.stratus3) != strlen(value));
					break;
				}
			}
		}
		else if (stricmp(sptr, "additional") == 0)
		{
			sprintf(service->properties.additional, "%.*s", (int)sizeof(service->properties.additional) - 1, value);
			err = valueTooLong = (strlen(service->properties.additional) != strlen(value));
		}
		else
		{
			SysLog(LogError, "Unrecognized configuration statement: %s", line);
			return -1;
		}
	}
	else
	{
		SysLog(LogError, "Unrecognized configuration statement: %s", line);
		return -1;
	}

	if (err)
	{
		if (valueTooLong)
			SysLog(LogError, "Value exceeds maximum length in: %s", line);
		else
			SysLog(LogError, "Invalid value in: %s", line);
		return -1;
	}

	return 0;
}


int
FecConfigParseString(FecConfig_t *this, char *text)
{
	int fin = 0;

	FecConfigVerify(this);

	// peel it out into lines
	for (;;)
	{
		char *line;
		for (line = text; *text && *text != '\n'; ++text) ;

		if (*text)
			*text++ = '\0';

		if ((text - line) <= 0)
			break;				// done

		fin = ParseConfigLine(this, line);
		if (fin != 0)
			break;
	}

	return fin;
}


#ifdef __CYGWIN__
#define RTLD_LOCAL 0
#endif


static int
LoadFecPlugIn(FecConfig_t *this, int sn, FecService_t *service)
{

	FecConfigVerify(this);

	if (sn < 0 || sn >= MAXSERVICES)
	{
		SysLog(LogError, "service number %d is out of range", sn);
		return -1;
	}

	// Default to library plug-in is not loaded
	service->plugin.loaded = false;

	// library plug-in already loaded, skip testing it again
	for (int jj = 0; jj < sn; ++jj)
	{
		if (this->services[jj].status != FecConfigVerified || (!(this->services[jj].plugin.loaded)))
			continue;

		if (!strcmp(this->services[jj].properties.protocol, service->properties.protocol))
		{
			// Copy the application plug-in library version
			memcpy(&service->plugin, &this->services[jj].plugin, sizeof(service->plugin));
			break;
		}
	}


	if (service->plugin.loaded)
		return 0;				// this one was previously loaded..

	// Load the application plugin library

	void *libbase = 0;
	char libname[128];
	PiLibSpecs_t *libSpecs = NULL;

	sprintf(libname, "lib%s.so", service->properties.protocol);

	{
		char *wd = getcwd(NULL, 0);
		char tmp[1024];

		sprintf(tmp, "%s/plugs/%s", wd, libname);
		SysLog(LogDebug, "Loading plugin %s from %s", libname, tmp);
		libbase = dlopen(tmp, RTLD_NOW | RTLD_LOCAL);
	}

	if (!(libbase))
	{
		SysLog(LogWarn, "Unable to load %s; error %s", libname, dlerror());
		return 1;
	}

	// Retrieve the version object
	libSpecs = (PiLibSpecs_t *)dlsym(libbase, "PluginVersion");
	if (!(libSpecs))
	{
		SysLog(LogError, "Did not find PluginVersion in plugin %s", libname);
		dlclose(libbase);
		return 1;
	}							// if (!(plugin))


	// Verify library has current release version or higher
	// and load its metod entry points
	// NOTE: The FEC was newly implemented at the current release, introducing
	// the use of event driven, multiple, concurrent, finite state machines
	// plug-ins prior to release 4.0 are no longer compatible.

	if (strcmp(libname, libSpecs->libname) != 0)
	{
		SysLog(LogError, "Libnames do not match: %s!=%s", libname, libSpecs->libname);
		dlclose(libbase);
		return 1;
	}

	char **methods = 0;
	PiApi_t api;

	memset(&api, 0, sizeof(api));

	// Load the plugin method names, DO NOT FREE
	// The methods memory is located in the plugin library
	methods = (char **)dlsym(libbase, "PluginMethods");

	// Cannot find the list of plugin method names to load
	// Can only ignore this connection attempt
	if (!(methods))
	{
		SysLog(LogError, "Did not find PluginMethods structure in plugin %s", libname);
		dlclose(libbase);
		return 1;
	}							// if (!(methods))

	// Load the plugin methods by name
	dlerror();
	for (int jj = 0; methods[jj]; jj++)
	{
		char *ep;

		// Skip undefined method entries
		if (!(methods[jj]) || !(*methods[jj]))
			continue;

		// Load the plugin method
		api.methods.entries[jj] = dlsym(libbase, methods[jj]);

		// Test for symbol load failure
		if ((ep = dlerror()))
			SysLog(LogError, "Unable to load method %s for plugin %s; error %s", methods[jj], libname, ep);
	}

	// Validate plugin methods
	// At a minimum, the following plugin methods must be defined
	if ((api.methods.entry.BeginSession == NULL) ||
		(api.methods.entry.EndSession == NULL) || (api.methods.entry.ReadRequest == NULL) || (api.methods.entry.SendResponse == NULL))
	{
		if ( api.methods.entry.BeginSession == NULL )
			SysLog(LogError, "Missing BeginSession method in plugin %s", libname);
		if ( api.methods.entry.EndSession == NULL )
			SysLog(LogError, "Missing EndSession method in plugin %s", libname);
		if ( api.methods.entry.ReadRequest == NULL )
			SysLog(LogError, "Missing ReadRequest method in plugin %s", libname);
		if ( api.methods.entry.SendResponse == NULL )
			SysLog(LogError, "Missing SendResponse method in plugin %s", libname);
		dlclose(libbase);
		return 1;
	}

	PiApiResult_t (*load) ();

	if ((load = dlsym(libbase, "Load")))
	{
		PiApiResult_t ret;
		if ( (ret = (*load)()) < 0 )				// call the plugin's loader
		{
			SysLog(LogError, "plugin %s Load() reports error %s", libname, PiApiResultToString(ret));
			return -1;
		}
		if ( ret == eVirtual )
			service->plugin.isVirtual = true;
	}

	// Copy the application plug-in library version
	memcpy(&service->plugin.libSpecs, libSpecs, sizeof(service->plugin.libSpecs));

	// Copy the application plug-in library method entry point(s)
	memcpy(&service->plugin.api, &api, sizeof(api));

	// Add the FEC Application Plug-in to the Service entry
	// and indicate it is loaded
	service->plugin.libbase = libbase;
	service->plugin.loaded = true;

	return 0;
}


FecService_t*
FecConfigGetServiceContext(FecConfig_t *this, int port)
{
	FecConfigVerify(this);

	if ( ObjectIsShared(this) )		// if this is shared memory
		ObjectLock(this);				// lock it

	for (int ii = 0; ii < MAXSERVICES; ++ii)
	{
		if (this->services[ii].properties.port == port)
		{
			FecService_t *svc = &this->services[ii];

			if (!svc->plugin.loaded || svc->plugin.api.methods.entry.BeginSession == NULL)
				break;			// not loaded...

			if ( ObjectIsShared(this) )		// if this is shared memory
			{
				ObjectUnlock(this);				// unlock it
				static FecService_t localSvc;		// make a local copy... this is a mapped copy
				memcpy(&localSvc, svc, sizeof(localSvc));
				svc = &localSvc;
			}
			return svc;
		}
	}

	if ( ObjectIsShared(this) )		// if this is shared memory
		ObjectUnlock(this);				// lock it
	SysLog(LogError, "No service for port %d", port);
	return NULL;
}
