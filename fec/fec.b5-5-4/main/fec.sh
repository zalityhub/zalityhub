#! /bin/bash
#
# chkconfig: 2345 99 99
# description: FEC Daemon
#
# processname: fec
# pidf: ./fec.pid

# Source function library.
. /etc/init.d/functions
 
Statusof()
{
	local base pid pid_file=

	# Test syntax.
	if [ "$#" = 0 ] ; then
		echo $"Usage: statusof [-p pidfile] {program}"
		return 1
	fi
	if [ "$1" = "-p" ]; then
		pid_file=$2
		shift 2
	fi
	base=${1##*/}

	# try "pidof"
	__pids_var_run "$1" "$pid_file"
	RC=$?
	if [ -z "$pid_file" -a -z "$pid" ]; then
		pid="$(_pidof "$1")"
	fi
	if [ -n "$pid" ]; then
	        echo $"${base} (pid $pid) is running..."
	        return 0
	fi

	case "$RC" in
		0)
			echo $"${base} (pid $pid) is running..."
			return 0
			;;
		1)
	                echo $"${base} dead but pid file exists"
	                return 1
			;;
	esac

	echo $"${base} is stopped"
	return 3
}


getpidlist()
{
	nc=`shopt -u nocasematch`
	shopt -s nocasematch
	if [[ "$1" =~ "nxproc*" ]];then
		t=$1;shift
	else
		t="NxProc${1}";shift
	fi
	echo `ps -efl|fgrep -i ${t}|grep "... ${user} "|fgrep -v grep|tr -s ' '|cut -d' ' -f4`
	${nc}
	return 0
}


#
# Hidden commands; for testing only; do not use!!

#!/bin/sh
GetServiceSockets()
{
	cat /proc/net/tcp \
		| grep "^[ \t]*[1234567890][1234567890]*: 00000000:" \
		| cut -d: -f3 | cut -d\  -f1 \
		| while read HEX
	do
	{
		echo "ibase=16; $HEX"|bc
	} done
	return 0
}


PingTest()
{
	s=$1
	p=`echo $2|cut -d: -f1`
	./pping $s $p >/dev/null

    if [ $? -ne 0 ];then
        if [ $html -eq 1 ];then
            echo -n -e '<td><div style="color:#FF0000"><b>FAILED</b></div></td>'
		else
        	echo -n -e 'FAILED'
        	echo -n -e '    '
        fi
    else
        if [ $html -eq 1 ];then
            echo -n -e '<td><div style="color:#009900">Passed</div></td>'
		else
        	echo -n -e 'Passed'
        	echo -n -e '    '
        fi
    fi
	return 0
}


FecTest()
{
	export local=1
	export html=0

	if [ "$1" == "-remote" ];then
		export local=0;shift
	fi
	if [ "$1" == "-html" ];then
		export html=1;shift
	fi

# Check FEC status
	if [ $html -eq 1 ];then
		Statusof -p ${pidf} ${prog}
	else
		Statusof -p ${pidf} ${prog}
	fi

	if [ "$?" -ne 0 ];then
		return 1		# Not running
	fi

	export KnxFec1=10.164.222.200
	export KnxFec2=10.164.222.201
	export KnxFec3=10.164.222.202
	export AtlFec1=10.164.218.200
	export AtlFec2=10.164.218.201
	export AtlFec3=10.164.218.202
	export DevFec1=10.164.67.132
	export DevFec2=10.164.67.133
	export DevFec3=10.164.67.134
	export DevFec229=10.165.3.229
	export QaFec1=10.164.67.135
	export QaFec2=10.164.67.136
	export QaFec3=10.164.67.137
	export Uat=10.164.222.210

	export LocalHost=127.0.0.1

	ports=""
	inactivePorts="5002 5404 6557 6610"

	for p in `cat ${procfec}/fec.ini | dos2unix |awk 'BEGIN {ot="";} {if ($1=="fec.service.load") {printf ":%s\n",$3} if ($1=="fec.service.type") {printf ":%s",$3} if ($1=="fec.service.port") {printf "%s", $3;}}'|sort -n`;do
		t="`echo $p|cut -d: -f1`"
		if [ "`echo "${inactivePorts}"|fgrep -w $t`" == "" ];then
			ports+=" $p"
		fi
	done

	if [ "${local}" == "1" ];then
		export servers="\$LocalHost"
	else
		case "`uname -n|tr '[:lower:]' '[:upper:]'`" in
			SPP*FEC*)
				export servers="\$KnxFec1 \$KnxFec2 \$KnxFec3 \$AtlFec1 \$AtlFec2 \$AtlFec3"
				;;

			*)
				export servers="\$DevFec1 \$DevFec2 \$DevFec3 \$DevFec229 \$QaFec1 \$QaFec2 \$QaFec3"
			;;
		esac
	fi


	if [ $html -eq 1 ];then
		echo '<table border="1"><tr><td/>'
	else
		echo -n -e "   "
	fi

	for s in ${servers}; do
		if [ $html -eq 1 ];then
			echo -n -e "<td>"
			echo -n -e "`echo $s|cut -c2-`"
			echo -n -e "</a>"
			echo -n -e "</td>"
		else
			echo -n -e "     `echo $s|cut -c2-`"
		fi
	done

	if [ $html -eq 1 ];then
		echo
		echo '</tr>'
	else
		echo
	fi

# if Local, report number of errors in log and test for required processes
	if [ "${local}" == "1" ];then

		c="`Er|wc -l`"
		if [ $html -eq 1 ];then
			echo -n -e "<tr><td>Error Log Entries</td>"
		else
			echo -n -e "Error Log Entries    "
		fi

		if [ $c -gt 0 ];then
			if [ $html -eq 1 ];then
				echo "<td><div style='color:#FF0000'><a href='/html/check_log.html'/><b>$c</b></div></td>"
			else
				echo "$c"
			fi
		else
			if [ $html -eq 1 ];then
				echo "<td><div style='color:#009900'>None</div></td>"
			else
				echo 'None'
			fi
		fi

		if [ $html -eq 1 ];then
			echo
			echo '</tr>'
		else
			echo
		fi

		procs="Root AuthProxy EdcProxy Worker"
		for p in ${procs}; do
			if [ $html -eq 1 ];then
				echo -n -e "<tr><td>$p</td>"
			else
				echo -n -e "$p    "
			fi

			if [ "`Si $p`" == "" ];then
				if [ $html -eq 1 ];then
					echo '<td><div style="color:#FF0000"><b>NOT ACTIVE</b></div></td>'
				else
					echo 'NOT ACTIVE'
				fi
			else
				if [ $html -eq 1 ];then
					if [ "$p" == "Worker" ];then
						t="`Si worker|head -1|sed -e's/.*NxProc//g'`"
					else
						t=$p
					fi
					echo "<td><div style='color:#009900'><a href='/showproc?target=$t'/>Active</div></td>"
				else
					echo 'Active'
				fi
			fi

			if [ $html -eq 1 ];then
				echo
				echo '</tr>'
			else
				echo
			fi
		done
	fi

	for p in ${ports};do
		if [ $html -eq 1 ];then
			echo -n -e "<tr><td>$p</td>"
		else
			echo -n -e "$p    "
		fi

		for s in ${servers}; do
			PingTest `eval echo $s` "$p"
		done

		if [ $html -eq 1 ];then
			echo
			echo '</tr>'
		else
			echo
		fi
	done

	if [ $html -eq 1 ];then
		echo '</table>'
	fi
	return 0
}


Ci()
{
	FecTest $*
	return 0
}


Er()
{
	export html=0
	if [ "$1" == "-html" ];then
		export html=1;shift
	fi
	if [ "$1" != "" ];then
		f=$1;shift
	else
		f="fec.log"
	fi

	if [ $html -eq 1 ];then
		echo '<table border="1">'
		egrep -nw 'ERR|FTL' $f|fgrep -v '<td>'|grep -v 'WRN.*LoadFecPlugIn'|grep -v 'WRN.*FecServiceCommit'|awk '{print "<tr><td>",$0,"</td></tr>"}'
		echo '</table>'
	else
		egrep -nw 'ERR|FTL' $f|fgrep -v '<td>'|grep -v 'WRN.*LoadFecPlugIn'|grep -v 'WRN.*FecServiceCommit'|awk '{print $0}'
	fi
	return 0
}


Li()
{
	for list in `GetServiceSockets`
	do
	{
		echo Service on $list
	} done
	return 0
}


Ki()
{

	if [ "$1" == "-f" ];then
		sig="SIGKILL"
		opt="-f"
		shift
	else
		sig="SIGTERM"
		opt=""
	fi

	if [ $# -lt 1 ];then
		echo "Usage: Ki  name"
		return 1
	fi

	for f in $*;do
		if [ "$f" == "all" ];then
			pids=`getpidlist Root`
			if [ "${pids}" != "" ];then	# Kill Root first
				echo "kill -s ${sig} ${pids}"
				kill -s ${sig} ${pids}
			fi
			pids=`getpidlist`
		else
			pids=`getpidlist $f`
		fi
		if [ "${pids}" == "" ];then
			echo "Nothing to kill"
			return 1
		fi
		echo "kill -s ${sig} ${pids}"
		kill -s ${sig} ${pids}
	done

	return 0
}


Si()
{
	if [ $# -lt 1 ];then
		ps -efl|fgrep -i NxProc|grep "... ${user} "|fgrep -v grep
	else
		for f in $*;do
			if [ "$f" == "all" ];then
				ps -efl|fgrep -i NxProc|grep "... ${user} "|fgrep -v grep
			else
				ps -efl|grep -i "NxProc.*${f}"|grep "... ${user} "|fgrep -v grep
			fi
		done
	fi
	return 0
}


Gi()
{
	if [ "$1" == "-r" ];then
		retry=1
		shift
	else
		retry=0
	fi

	t=$1; shift
	while [ 1 ];do
		pids=`getpidlist $t`
		if [ "${pids}" != "" ];then
			gdb $* ./${prog} ${pids}
			retry=0
		fi
		if [ ${retry} == 0 ];then
			break
		fi
	done
	return 0
}


Rs()
{
	./ki -f all;sleep 2;rm -f core* fec.log;./fec;tail -f fec.log
}


#
# Standard commands

Start()
{
	echo -n $"Starting $prog"	

	if [ "`ps -efl|fgrep -i NxProc|grep "... ${user} "|fgrep -v grep`" != "" ];then
		Ambush
		sleep 1
	fi

	if [ -e ${pidf} ];then
		echo -n $" cannot start ${prog}: ${prog} is already running.";
		failure $" cannot start ${prog}: ${prog} already running.";
		echo
		return 1
	fi

	if [ ! -d ${basedir}/attic ];then
		mkdir ${basedir}/attic
	fi
	if [ "`echo ${basedir}/${prog}*.log`" != "${basedir}/${prog}*.log" ];then
		mv ${basedir}/${prog}*.log ${basedir}/attic
	fi
	if [ "`echo ${basedir}/${prog}_audit*.log`" != "${basedir}/${prog}_audit*.log" ];then
		mv ${basedir}/${prog}_audit*.log ${basedir}/attic
	fi
	if [ "`echo ${basedir}/${prog}*.sqlite`" != "${basedir}/${prog}*.sqlite" ];then
		mv ${basedir}/${prog}*.sqlite ${basedir}/attic
	fi
	ulimit -c unlimited

	__pids_var_run "$base" "$pidf"

	[ -n "$pid" -a -z "$force" ] && return

	# Echo daemon
    [ "${BOOTUP:-}" = "verbose" -a -z "${LSB:-}" ] && echo -n " $base"

	while [ 1 ];do
		# Start it up.
		cd ${basedir};${basedir}/${prog}
		RETVAL=$?
		if [ "${RETVAL}" -eq 0 ];then
			success $"$base startup"
			break
		else
			if [ "${RETVAL}" -eq 127 ];then		# Library error
				chcon -t texrel_shlib_t ${basedir}/lib/*
			else
				failure $"$base startup"
				break
			fi
		fi
	done

	echo
	return $RETVAL
}


Stop()
{
	echo -n $"Stopping $prog: "
    if [ -e ${pidf} ] && [ -e /proc/`cat ${pidf}` ]; then
		x=''	# nothing to see here; move along
	else
 		echo -n $"cannot stop ${prog}: ${prog} is not running."
 		failure $"cannot stop ${prog}: ${prog} is not running."
 		echo
 		return 1;
	fi

	# killproc
	Ki root
	return 0
}	


Ambush()
{
	echo -n $"Stopping $prog: "
    if [ -e ${pidf} ] && [ -e /proc/`cat ${pidf}` ]; then
		x=''	# nothing to see here; move along
	else
 		echo -n $"cannot stop ${prog}: ${prog} is not running."
 		failure $"cannot stop ${prog}: ${prog} is not running."
 		echo
 		return 1;
	fi

	# killproc
	Ki -f all
	sleep 2
	Ki -f all
	echo
	return 0
}	


Status()
{
	Statusof -p ${pidf} ${prog}
	return 0
}	


Conn()
{
	netstat -nl|fgrep -w $*;netstat -n|fgrep -w $*
	return 0
}	


TcpDump()
{
	arg=""
	for f in $*;do
		if [ "${arg}" == "" ];then
			arg=' \( '
		else
			arg+=" or "
		fi
		arg+=" tcp port ${f} "
	done
	if [ "${arg}" != "" ];then
		arg+=' \) '
	fi
    echo "sudo /usr/sbin/tcpdump -i bond0 -U -n -l -e -tttt -vvv -XX -s 37768 ${arg}"
}


Link()
{
	for f in Si Ki Gi Li;do
		rm -f $f
		ln -s fec.sh $f
	done
	return 0
}	


Restart()
{
  	Ambush
	sleep 1
	Start
	return 0
}	



# Start here
#

# get the current user id (the one executing the script)
# if the user is ‘root’ (which is true on initial boot; and not true when deploying via harvest/SCM)
# set basedir variable, the executing directory to the required value; (it’s the root directory ‘/’ during boot)
# 	note this variable is referenced in the following line “cd ${basedir};”…
# re-execute the entire script with the arg-list via the sub-user command explicitly naming ‘gssvc’ as the user (this will cause the script to re-run where the user 'root' check will be false)

umask 022
export prog="fec"
export user=`id -nu`
export RETVAL=0

if [ "${user}" == "root" ];then
	export basedir="/opt/sc/fec"
	su -c "(cd ${basedir};${basedir}/fec.sh $* )" - gssvc
	exit 1
elif [ "${user}" == "hbray" ];then
	export basedir="."
	export pidf="${HOME}/${prog}.pid"
elif [ "`uname -n`" == "rhlinux-5-devel" ];then
	export basedir="/opt/sc/fec"
	export pidf="${HOME}/${prog}.pid"
else
	export basedir="/opt/sc/fec"
	export pidf="/home/gssvc/${prog}.pid"
fi


export procfec="/tmp/.${user}/procfec"
export logname="fec.history"

if [ -e ${logname} ];then
	if [ ! -w ${logname} ];then
		export logname="/tmp/fec.history"
	fi
fi
if [ -e ${logname} ];then
	if [ ! -w ${logname} ];then
		export logname="/tmp/fec.$$.history"
	fi
fi
if [ -e ${logname} ];then
	if [ ! -w ${logname} ];then
		echo "Unable to record command history"
		export logname="/dev/null"
	fi
fi

echo "`id -un` EXEC:" `date` >>${logname} 2>&1
echo '$*='"'$*'" >>${logname} 2>&1
echo '$#='"'$#'" >>${logname} 2>&1
echo '$?='"'$?'" >>${logname} 2>&1
echo '$-='"'$-'" >>${logname} 2>&1
echo '$$='"'$$'" >>${logname} 2>&1
echo '$!='"'$!'" >>${logname} 2>&1
echo '$0='"'$0'" >>${logname} 2>&1
echo '$_='"'$_'" >>${logname} 2>&1
printenv >>${logname} 2>&1
echo "SNAP_BEGIN:" `date` >>${logname} 2>&1
who -a >>${logname} 2>&1
ps -efl >>${logname} 2>&1
netstat -pan >>${logname} 2>&1
/sbin/ifconfig >>${logname} 2>&1


case "$1" in
  start)
  	Start $*
	;;
  stop)
  	Stop $*
	;;
  ambush)
  	Ambush $*
	;;
  restart)
  	Restart $*
	;;
  status)
  	Status $*
	;;
  conn)
  	shift
  	Conn $*
	;;

# Hidden commands; for testing only; do not use!!

  tcpdump)
  	shift
	TcpDump $*
	;;

  link)
  	shift
  	Link $*
	;;

  ci)
  	shift
  	Ci $*
	;;
  *)
	if [ `basename $0` == "ki" ];then
		Ki $*
	elif [ `basename $0` == "si" ];then
		Si $*
	elif [ `basename $0` == "gi" ];then
		Gi $*
	elif [ `basename $0` == "rs" ];then
		Rs $*
	elif [ `basename $0` == "li" ];then
		Li $*
	elif [ `basename $0` == "er" ];then
		Er $*
	else
		echo $"Usage: $0 {start|stop|status|restart|ambush}"
		echo $"Usage: $0 {start|stop|status|restart|ambush}" >>${logname} 2>&1
	fi
	RETVAL=1
	echo "END:(${RETVAL})" `date` >>${logname} 2>&1
	exit ${RETVAL}
esac

RETVAL=$?
echo "SNAP_FINAL:" `date` >>${logname} 2>&1
who -a >>${logname} 2>&1
ps -efl >>${logname} 2>&1
netstat -pan >>${logname} 2>&1
/sbin/ifconfig >>${logname} 2>&1
echo "END:(${RETVAL})" `date` >>${logname} 2>&1
