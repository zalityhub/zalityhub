/*****************************************************************************

Filename:	lib/onq/beginsession.c

Purpose:	GCS Hilton OnQ Message Set

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library instantiation method.  Called directly after
			the POS connection established, but prior to any communication.

			Allocate working memory storage from heap and any other required
			resources needed to process the specific protocol/packet type.

			Initialize the communications channel, i.e. send ENQ for example.

			Return true upon success, or false upon failure/error condition.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:20 $
 * $Header: /home/hbray/cvsroot/fec/lib/onq/beginsession.c,v 1.3 2011/07/27 20:22:20 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.3  2011/07/27 20:22:20  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:49  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.3 2011/07/27 20:22:20 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
BeginSession(PiSession_t *sess)
{
	PiSessionVerify(sess);

	// Sanity check, verifying lib/onq/data.h is in sync with this
	// method.  Only six(6) OnQ Message Types are initialized here.
	if (6 != MAXONQ)
		return (eFailure);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Initialize the plugin data
	memcpy(context->packets, (onqmsg_t[])
		   {
		   {
		   0, 32},				// Heartbeat
		   {
		   1, 265},				// Authorization
		   {
		   2, 56},				// EDC Begin Batch
		   {
		   3, 57},				// EDC End Batch
		   {
		   4, 43},				// EDC Status, handled as AUX by Host
		   {
		   5, 32}},				// Who Am I
		   (MAXONQ * sizeof(onqmsg_t)));

	return (eOk);
}								// int instantiate(sess_t *sess)
