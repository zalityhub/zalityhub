/*****************************************************************************

Filename:	lib/nx/hostrequest.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/hostrequest.c,v 1.3.4.5 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: hostrequest.c,v $
 Revision 1.3.4.5  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/24 17:49:44  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/02 14:17:02  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/01 14:49:45  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: hostrequest.c,v 1.3.4.5 2011/10/27 18:33:57 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/hostrequest.h"
#include "include/proxy.h"




Json_t*
HostRequestHeaderSerialize(HostRequestHeader_t *this)
{
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "SvcType", HostSvcTypeToString(this->svcType));
	JsonAddBoolean(root, "More", this->more);
	JsonAddString(root, "RequestEcho", NxUidToString(this->requestEcho));

	Json_t *sub = JsonPushObject(root, "PeerInfo");
	JsonAddString(sub, "Uid", NxUidToString(this->peerUid));
	JsonAddString(sub, "Name", this->peerName);
	JsonAddString(sub, "IpAddr", IpAddrToString((unsigned char*)this->peerIpAddr));
	JsonAddCookedString(sub, "IpType", this->peerIpType, sizeof(this->peerIpType));
	JsonAddNumber(sub, "IpPort", this->peerIpPort);
	return root;
}


char*
HostRequestHeaderToString(HostRequestHeader_t *this)
{
	Json_t *root = HostRequestHeaderSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


Json_t *
HostRequestSerialize(HostRequest_t *this, OutputType_t outputType)
{
	Json_t *root = JsonNew(__FUNC__);
	JsonAddItem(root, "Hdr", HostRequestHeaderSerialize(&this->hdr));
	if ( outputType != NoOutput && this->len > 0 )
		JsonDataOut(root, "Data", this->data, this->len, outputType);
	return root;
}


char *
HostRequestToString(HostRequest_t *this, OutputType_t outputType)
{
	Json_t *root = HostRequestSerialize(this, outputType);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


char*
HostSvcTypeToString(HostSvcType_t type)
{
	char *text;
	switch (type)
	{
		default:
			text = StringStaticSprintf("HostSvcType_%d", type);
			break;
		EnumToString(eSvcConfig);
		EnumToString(eSvcAuth);
		EnumToString(eSvcEdc);
		EnumToString(eSvcEdcMulti);
		EnumToString(eSvcEdcCapms);
		EnumToString(eSvcT70);
	}
	return text+4;		// skip 'eSvc' prefix...
}
