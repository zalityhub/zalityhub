/*****************************************************************************

Filename:   include/piprofile.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:37 $
 * $Header: /home/hbray/cvsroot/fec/include/Attic/piprofile.h,v 1.1.2.2 2011/09/24 17:49:37 hbray Exp $
 *
 $Log: piprofile.h,v $
 Revision 1.1.2.2  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.1.2.1  2011/08/24 13:22:29  hbray
 Renamed


 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: piprofile.h,v 1.1.2.2 2011/09/24 17:49:37 hbray Exp $ "


#ifndef _PIPROFILE_H
#define _PIPROFILE_H

// API return results

typedef enum
{
	eFailure		= -3,		// Abnormal (FATAL) event
	eWarn			= -2,		// Abnormal (FATAL) event
	eDisconnect		= -1,		// begin disconnect processing
	eOk				= 0,
	eWaitForData,				// Wait for pos data
	eVirtual,
} PiApiResult_t;


typedef struct PiApi_t
{
	union
	{
		struct libsym_s			// Plug-in entry points by name
		{
			PiApiResult_t (*Load) ();
			PiApiResult_t (*Unload) ();
			PiApiResult_t (*BeginSession) (void *);
			PiApiResult_t (*EndSession) (void *);
			PiApiResult_t (*ReadRequest) (void *);
			PiApiResult_t (*SendResponse) (void *);
			char*		  (*ToString) (void *);
			struct Json_t* (*Serialize) (void *);
		} entry;

		void *entries[sizeof(struct libsym_s) / sizeof(int (*)())];
	} methods;
} PiApi_t;

typedef struct PiLibSpecs_t
{
	char	libname[32];			// library name
	char	*version;				// library version
	int		persistent;				// Persistent POS connection
} PiLibSpecs_t;

typedef struct PiProfile_t
{
	boolean			loaded;
	boolean			isVirtual;
	void			*libbase;				// library load point

	PiLibSpecs_t	libSpecs;
	PiApi_t			api;

	Counts_t		posCounts;
	Counts_t		hostCounts;
} PiProfile_t;

extern char *PiProfileToString(PiProfile_t *profile);
extern struct Json_t *PiProfileSerialize(PiProfile_t *profile);
extern char *PiApiResultToString(PiApiResult_t result);

#endif
