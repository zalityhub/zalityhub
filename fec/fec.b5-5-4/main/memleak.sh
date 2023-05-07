args=""
# "$@" preserves spaces
for f in "$@"; do
	if [ ${f:0:1} != '-' ];then
		cmd=$f
		break;
	fi
	args+=" "
	args+=$f
done

# echo args=$args
# echo cmd=${cmd}

if [ "${cmd}" == "" ];then
	echo "Need a command to run"
	exit 1
fi

if [ "${cmd}" == "status" ];then
	lst=`ps -efl|fgrep valgrind|fgrep -v grep|tr -s ' '|cut -d' ' -f4`
	if [ "${lst}" == "" ];then
		echo "all stopped"
	else
		echo `ps -efl|fgrep valgrind|fgrep -v grep`
	fi
	exit 0
fi
if [ "${cmd}" == "stop" ];then
	lst=`ps -efl|fgrep valgrind|fgrep -v grep|tr -s ' '|cut -d' ' -f4`
	if [ "${lst}" == "" ];then
		echo "all stopped"
	else
		kill ${lst}
	fi
	exit 0
fi
if [ "${cmd}" == "kill" ];then
	lst=`ps -efl|fgrep valgrind|fgrep -v grep|tr -s ' '|cut -d' ' -f4`
	if [ "${lst}" == "" ];then
		echo "all stopped"
	else
		kill -9 ${lst}
	fi
	exit 0
fi

valgrind --trace-children=yes --tool=memcheck --leak-check=full --show-reachable=yes --leak-resolution=high --child-silent-after-fork=no --log-file=${cmd}.%p.valg ${args} ${cmd}
