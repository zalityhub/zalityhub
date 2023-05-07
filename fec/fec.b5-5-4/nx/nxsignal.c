/*****************************************************************************

Filename:   lib/nx/nxsignal.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/nxsignal.c,v 1.3.4.2 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: nxsignal.c,v $
 Revision 1.3.4.2  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:46  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: nxsignal.c,v 1.3.4.2 2011/10/27 18:33:57 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/nxsignal.h"


static EnumToStringMap SignalStringMap[] =
{
	{SIGHUP, "SIGHUP: Hangup"},
	{SIGINT, "SIGINT: Interrupt"},
	{SIGQUIT, "SIGQUIT: Quit"},
	{SIGILL, "SIGILL: Illegal instruction"},
	{SIGTRAP, "SIGTRAP: Trace trap"},
	{SIGABRT, "SIGABRT: Abort"},
	{SIGIOT, "SIGIOT: IOT trap"},
	{SIGBUS, "SIGBUS: BUS error"},
	{SIGFPE, "SIGFPE: Floating-point exception"},
	{SIGKILL, "SIGKILL: Kill, unblockable"},
	{SIGUSR1, "SIGUSR1: User-defined signal 1"},
	{SIGSEGV, "SIGSEGV: Segmentation violation"},
	{SIGUSR2, "SIGUSR2: User-defined signal 2"},
	{SIGPIPE, "SIGPIPE: Broken pipe"},
	{SIGALRM, "SIGALRM: Alarm clock"},
	{SIGTERM, "SIGTERM: Termination"},
	{SIGSTKFLT, "SIGSTKFLT: Stack fault"},
	{SIGCHLD, "SIGCHLD: Child status has changed"},
	{SIGCONT, "SIGCONT: Continue"},
	{SIGSTOP, "SIGSTOP: Stop, unblockable"},
	{SIGTSTP, "SIGTSTP: Keyboard stop"},
	{SIGTTIN, "SIGTTIN: Background read from tty"},
	{SIGTTOU, "SIGTTOU: Background write to tty"},
	{SIGURG, "SIGURG: Urgent condition on socket"},
	{SIGXCPU, "SIGXCPU: CPU limit exceeded"},
	{SIGXFSZ, "SIGXFSZ: File size limit exceeded"},
	{SIGVTALRM, "SIGVTALRM: Virtual alarm clock"},
	{SIGPROF, "SIGPROF: Profiling alarm clock"},
	{SIGWINCH, "SIGWINCH: Window size change"},
	{SIGIO, "SIGIO: I/O now possible"},
	{SIGPWR, "SIGPWR: Power failure restart"},
	{SIGSYS, "SIGSYS: Bad system call"},
	{-1, NULL}
};


BtNode_t *NxSignalNodeList = NULL;


NxSignal_t*
NxSignalConstructor(NxSignal_t *this, char *file, int lno, siginfo_t *si)
{
	memcpy(&this->si, si, sizeof(this->si));
	return this;
}


void
NxSignalDestructor(NxSignal_t *this, char *file, int lno)
{
}


char*
NxSignalNbrToString(int signo)
{
	char *text;
	if ( (text = EnumMapValToString(SignalStringMap, signo)) == NULL )
	{
		static char tmp[16];
		sprintf(tmp, "InvalidSigNo(%d)", signo);
		text = tmp;
	}
	return text;
}


Json_t*
NxSignalSerialize(NxSignal_t *this)
{
	NxSignalVerify(this);
	Json_t *root = JsonNew(__FUNC__);

// Get cmdline of pid which 'killed' me
	char cmdline[1024];
	{
		memset(cmdline, 0, sizeof(cmdline));
		char name[1024];

		sprintf(name, "/proc/%d/cmdline", this->si.si_pid);
		FILE *f = fopen(name, "r");

		if (f != NULL)
		{
			if (read(fileno(f), cmdline, sizeof(cmdline) - 1) < 0)
				memset(cmdline, 0, sizeof(cmdline));
			fclose(f);
		}
	}

	JsonAddString(root, "Signal", "signo=%d:%s, errno=%d, code=%d, pid=%d, cmdline='%s', uid=%d, status=%d, value=%p, int=%d, ptr=%p, addr=%p, band=%ld, fd=%d",
				this->si.si_signo, NxSignalNbrToString(this->si.si_signo), this->si.si_errno, this->si.si_code, this->si.si_pid, cmdline, this->si.si_uid, this->si.si_status,
				this->si.si_value.sival_ptr, this->si.si_int, this->si.si_ptr, this->si.si_addr, this->si.si_band, this->si.si_fd);
	return root;
}


char*
NxSignalToString(NxSignal_t *this)
{
	Json_t *root = NxSignalSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}
