find . |xargs touch
find . |xargs chmod +rw
find . -name harvest.sig |xargs rm -f
echo 'Converting from DOS to Unix format'
dos2unix dos2unix.sh >/dev/null 2>&1
sh dos2unix.sh >/dev/null 2>&1
