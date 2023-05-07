Fifo()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp} >>${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.* FifoRead: .* Removing / {dump=1}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


Pos()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.* PosRecv: .* len=/ {dump=1}" >>${tmp}
	echo "/${port}.* PiPosSend: .* len=/ {dump=1}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


Posin()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.* PosRecv: .* len=/ {dump=1}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


Posout()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.* PiPosSend: .* len=/ {dump=1}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


Proxy()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.*ProxySend: .*,ReqData=/ {dump=1}" >>${tmp}
	echo "/${port}.* ProxyResponseHandler: .*,ReqData=/ {dump=1}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


Proxyin()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.* ProxyResponseHandler: .*,ReqData=/ {dump=1}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


Proxyout()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.*ProxySend: .*,ReqData=/ {dump=1}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


Sess()
{
	port=""
	if [ $# -ge 2 ];then
		if [ "$1" = "-p" ];then
			shift;port="$1";shift
		fi
	fi
	tmp="/tmp/$$.awk"
	echo '' >${tmp}
	echo "BEGIN {dump=0}" >>${tmp}
	echo "/ Service_.*${port}.*SockAccept: accepted socket / {print \$0}" >>${tmp}
	echo "/${port}.* PosDisconnect: / {print \$0}" >>${tmp}
	echo "/${port}.* PosRecv: .* len=/ {dump=1}" >>${tmp}
	echo "/${port}.* FifoRead: .* Removing / {dump=1}" >>${tmp}
	echo "/${port}.*ProxySend: .*,ReqData=/ {dump=1}" >>${tmp}
	echo "/${port}.* PiPosSend: .* len=/ {dump=1}" >>${tmp}
	echo "/${port}.* ProxyResponseHandler: .*,ReqData=/ {dump=1}" >>${tmp}
	echo "/ ERR .*${port}/ {print \$0}" >>${tmp}
	echo "/ WRN .*${port}/ {print \$0}" >>${tmp}
	echo "{" >>${tmp}
		echo "if(dump) {print \$0}" >>${tmp}
		echo "if(length(\$0)<10) {dump=0}" >>${tmp}
	echo "}" >>${tmp}
	awk -f ${tmp} $*
	rm -f ${tmp}
}


cmd=$1; shift
case "${cmd}" in


	fifo)
		Fifo $*
		;;
	pos)
		Pos $*
		;;
	posin)
		Posin $*
		;;
	posout)
		Posout $*
		;;
	proxy)
		Proxy $*
		;;
	proxyin)
		Proxyin $*
		;;
	proxyout)
		Proxyout $*
		;;
	sess)
		Sess $*
		;;
	*)
  		echo "'${cmd}' is an unknown command"
  		echo "try:"
		echo "    fifo | pos | posin | posout | proxy | proxyin | proxyout | sess  [-p port]"
		exit 1
		;;
esac
