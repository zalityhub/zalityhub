if [ "$*" != "" ];then
	find .. \( -name Makefile -o -name '*.c' -o -name '*.h' -o -name '*.sh' -o -name '*.xml' -o -name '*.html' \)|fgrep -v attic/|fgrep -v dist/|fgrep -v /xml2/|fgrep -v /libxml/|fgrep -v sqlite|fgrep -v /test/|xargs $*
else
	find .. \( -name Makefile -o -name '*.c' -o -name '*.h' -o -name '*.sh' -o -name '*.xml' -o -name '*.html' \)|fgrep -v attic/|fgrep -v dist/|fgrep -v /xml2/|fgrep -v /libxml/|fgrep -v sqlite|fgrep -v /test/
fi
