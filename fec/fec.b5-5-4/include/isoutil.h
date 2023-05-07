/*****************************************************************************

Filename:	include/isoutil.h

Purpose:	Utility methods for working with ISO data packets.
			Complies to the following specifications, combining bits
			from both.

				TSYS ISO 8583 ENDPOINT SPECIFICATION, Version 2.7
				January 2008.

				Stored Value Systems, Inc. (SVS) ISO8583 Message
				Specifications, February 2001

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:11 $
 * $Header: /home/hbray/cvsroot/fec/include/isoutil.h,v 1.2 2011/07/27 20:22:11 hbray Exp $
 *
 $Log: isoutil.h,v $
 Revision 1.2  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.06.26 joseph dionne		Created release 3.4
*****************************************************************************/

#ident "@(#) $Id: isoutil.h,v 1.2 2011/07/27 20:22:11 hbray Exp $ "

#ifndef H_ISOUTIL
#define H_ISOUTIL

#include <errno.h>
#include <arpa/inet.h>

// Local scope variable declaration
#pragma pack(1)
#if 0
The following varible types define the ISO 64 - bit primary and secondary bit maps contained within the ISO 8583 data packet.Each
eight(8)
bit
	map field is managed as two unsigned longs broken out into single bit
	fields per the ISO, and SVS, bitmap definitions.
	The variable type isopribitmap_t contains the break out of the ISO8583
	primary bit map, while isosecbitmap_t contains the break out of the ISO
	8583 secondary bit map.The tertiary ISO8583 has not be created as it
	is still marked as "Reserved for future use" in the TSYS specification.Tye variable type isobitmap_t is a "generic" container for use when calling the
setIsoBitMap()
method to "load" the ISO 8583 bitmap types from the data from the ISO8583 data packet.
#endif
typedef union
{
	unsigned long dword;
	struct
	{
		int mercid:1;			// 32 Merchant ID 
		int isobit031:1;
		int isobit030:1;
		int isobit029:1;
		int isobit028:1;
		int isobit027:1;
		int isobit026:1;
		int isobit025:1;
		int isobit024:1;
		int isobit023:1;
		int entrymode:1;		// 22 POS Entry Mode Code
		int isobit021:1;
		int isobit020:1;
		int country:1;			// 19 POS Country Code
		int isobit018:1;
		int isobit017:1;
		int isobit016:1;
		int isobit015:1;
		int expdate:1;			// 14 Expiration Date
		int date:1;				// 13 Local Transaction Date
		int time:1;				// 12 Local Transaction Time
		int auditid:1;			// 11 Trace Audit Number
		int isobit010:1;
		int dccrate:1;			//  9 Conversion Rate
		int isobit008:1;
		int xmitdttm:1;			//  7 Transmission Date and Time
		int isobit006:1;
		int balance:1;			//  5 Balance
		int amount:1;			//  4 Transaction Amount
		int trancode:1;			//  3 Processing Code
		int acctno:1;			//  2 Primary Account Number
		int bitmap2:1;			//  1 Secondary bitmap present
	} bit;
} isohpribit_t;

typedef union
{
	unsigned long dword;
	struct
	{
		int isobit064:1;
		int authtmlimit:1;		// 63 Pre-auth Time Limit
		int custom:1;			// 62 Custom Payment Service
		int isobit061:1;
		int adtldata:1;			// 60 Additional POS Data
		int geodata:1;			// 59 POS Geographic Data
		int isobit058:1;
		int isobit057:1;
		int isobit056:1;
		int isobit055:1;
		int isobit054:1;
		int seccntl:1;			// 53 Security Related Control
		int pin:1;				// 52 Account PIN Data
		int acctcurrency:1;		// 51 Account Holder Currency Code
		int isobit050:1;
		int trancurrency:1;		// 49 Tranaction Currency Code
		int isobit048:1;
		int isobit047:1;
		int isobit046:1;
		int track1:1;			// 45 Track 1 Data
		int isobit044:1;
		int isobit043:1;
		int mercid:1;			// 42 Card Acceptor ID Code
		int termid:1;			// 41 Card Acceptor Terminal ID
		int isobit040:1;
		int response:1;			// 39 Authorization Id Response
		int approval:1;			// 38 Approval Code
		int refnum:1;			// 37 Retrieval Reference Number
		int isobit036:1;
		int track2:1;			// 35 Track 2 Data
		int isobit034:1;
		int isobit033:1;
	} bit;
} isolpribit_t;

typedef struct
{
	isolpribit_t lo;
	isohpribit_t hi;
} isopribitmap_t;

typedef union
{
	unsigned long dword;
	struct
	{
		int isobit096:1;
		int isobit095:1;
		int isobit094:1;
		int isobit093:1;
		int isobit092:1;
		int isobit091:1;
		int orgdataelem:1;		// 90 Original Data Elements
		int isobit089:1;
		int isobit088:1;
		int isobit087:1;
		int isobit086:1;
		int isobit085:1;
		int isobit084:1;
		int isobit083:1;
		int isobit082:1;
		int isobit081:1;
		int isobit080:1;
		int isobit079:1;
		int isobit078:1;
		int isobit077:1;
		int isobit076:1;
		int isobit075:1;
		int isobit074:1;
		int isobit073:1;
		int isobit072:1;
		int isobit071:1;
		int netmgmcode:1;		// 70 Network Mmanagement Code
		int isobit069:1;
		int isobit068:1;
		int isobit067:1;
		int isobit066:1;
		int bitmap3:1;			// 65 Tertiary bitmap present
	} bit;
} isohsecbit_t;

typedef union
{
	unsigned long dword;
	struct
	{
		int isobit128:1;
		int isobit127:1;
		int isobit126:1;
		int isobit125:1;
		int isobit124:1;
		int isobit123:1;
		int isobit122:1;
		int isobit121:1;
		int isobit120:1;
		int isobit119:1;
		int isobit118:1;
		int isobit117:1;
		int isobit116:1;
		int isobit115:1;
		int isobit114:1;
		int isobit113:1;
		int isobit112:1;
		int isobit111:1;
		int isobit110:1;
		int isobit109:1;
		int isobit108:1;
		int isobit107:1;
		int isobit106:1;
		int isobit105:1;
		int isobit104:1;
		int isobit103:1;
		int isobit102:1;
		int isobit101:1;
		int isobit100:1;
		int isobit099:1;
		int isobit098:1;
		int isobit097:1;
	} bit;
} isolsecbit_t;

typedef struct
{
	isolsecbit_t lo;
	isohsecbit_t hi;
} isosecbitmap_t;

typedef union
{
	isopribitmap_t map;
	isopribitmap_t pri;
	isosecbitmap_t sec;
} isobitmap_t;

#pragma pack()

int
setIsoBitMap(isobitmap_t *, char *);

#endif // H_ISOUTIL
