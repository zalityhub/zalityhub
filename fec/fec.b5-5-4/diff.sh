doit()
{
	d=`diff -b -B -w $1 $2/$1`
	if [ "$d" != "" ];then
		echo "DIFF: $1"
		diff -b -B -w $1 $2/$1 >$1.diff
	fi
}


rm -f `find . -name '*.diff'`

for f in `find . -type f|fgrep -v CVS/|fgrep -v harvest.sig`;do
	doit $f $1
done
