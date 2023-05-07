/*****************************************************************************

Filename:	lib/svcha/data.c

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:22 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/data.c,v 1.4 2011/07/27 20:22:22 hbray Exp $
 *
 $Log: data.c,v $
 Revision 1.4  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.3.2.2  2011/07/27 20:19:54  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: data.c,v 1.4 2011/07/27 20:22:22 hbray Exp $ "

// Application plugin data/method header declarations
#define C_DATA
#include "data.h"

// Application plugin release version, and connect status
PiLibSpecs_t PluginVersion = {

// Application plugin release version, and connect status data
#ifndef PLUGINNAME
	"libsvcha.so",
#else
	PLUGINNAME,
#endif

#ifndef VERSION
	"4.0",						// Default version value
#else
	VERSION,					// Value defined in data.h
#endif

#ifndef PERSISTENT
	false,						// Default persistence value
#else
	PERSISTENT,					// Value defined in data.h
#endif
};

// The following is required by the plugin loader.  DO NOT ALTER!
// This array must match the positional ordere of the libsym_t definition
char *(PluginMethods[]) =
{								// plugin method textual name
	"Load",						// plugin app object initializer
		"Unload",				// plugin app object de-initializer
		"BeginSession",			// plugin app called at start of new session
		"EndSession",			// plugin app called at end of session
		"ReadRequest",			// plugin app read POS request
		"SendResponse",			// plugin app write POS response
		"ToString",				// plugin app ToString function
		0						// Array must be NULLP terminated
};

NameToValueMap SvchaTagToValue[] = {
	// The following are not part of the XML document, their values
	// are taking from the MICROS "packet" or XML tag attribute
	// Noted here for information only
	{"retransmit", 1, -1},
	{"Originator", 2, -1},
	{"Target", 3, -1},

	// The XML document tokens below will be sent to the Switch
	// for transaction processing
	{"RevenueCenter", 4, -1},
	{"CheckNumber", 5, -1},
	{"RequestCode", 6, -1},
	{"Reversal", 7, -1},
	{"TransactionEmployee", 8, -1},
	{"Track1", 9, -1},
	{"Track2", 10, -1},
	{"SVAN", 11, -1},
	{"AuthorizationCode", 12, -1},
	{"Amount", 13, -1},
	{"AccountCurrency", 14, -1},
	{"ExchangeRate", 15, -1},
	{"ItemNumber", 16, -1},
	{"ItemType", 17, -1},
	{"ResponseCode", 18, -1},
	{"DisplayMessage", 19, -1},
	{"AccountBalance", 20, -1},
	{"RedeemHasAuth", 21, -1},
	{"TraceID", 22, -1},

	// The XML document tokens below will be sent in FEC plug-in field 99
	// which is in fact all translated tokens below appended into one
	// string, with commas (,) changed to byte 0x254.
	{"AmountPrompt", 21, 99},
	{"BonusPointsIssued", 22, 99},
	{"BusinessDate", 23, 99},
	{"CheckSequence", 24, 99},
	{"CheckSummary", 25, 99},
	{"Configuration", 26, 99},
	{"DS", 27, 99},
	{"Extension", 28, 99},
	{"LocalBalance", 29, -1},
	{"LocalDate", 30, 99},
	{"LocalTime", 31, 99},
	{"MenuItems", 32, 99},
	{"MI", 33, 99},
	{"Options", 34, 99},
	{"Payment", 35, 99},
	{"Payments", 36, 99},
	{"PointsIssued", 37, 99},
	{"PosPlatform", 38, 99},
	{"PrintLine", 39, 99},
	{"PrintLines", 40, 99},
	{"ProgramCode", 41, 99},
	{"ProgramName", 42, 99},
	{"PromptForCoupon/", 43, 99},
	{"SalesItemizers", 44, 99},
	{"SC", 45, 99},
	{"SI", 46, 99},
	{"Surcharges", 47, 99},
	{"SVCMessage", 48, 0},
	{"TerminalID", 49, 99},
	{"TerminalType", 50, 99},
	{"TipAmount", 51, 99},
	{"Totals", 52, 99},
	{"Transaction", 53, 99},
	{"Site", 54, 99},
	{"Discounts", 55, 99},
	{NULL, 0, 99}
};
