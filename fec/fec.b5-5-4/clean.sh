# !/usr/bin
set -x

rm -rf `find . | grep '\.shm$'`
rm -rf `find . | grep '\.sqlite$'`
rm -rf `find . | grep '\.dll$'`
rm -rf `find . | grep '\.log$'`
rm -rf `find . | grep '.*.history'`
rm -rf `find . | grep '.*.swp'`
rm -rf `find . | grep '.gxml.*db$'`
rm -rf `find . | grep 'a.out$'`
rm -rf `find . | grep 'core*'`

rm -rf `find . -name '*.diff'`
rm -rf `find . -name 'junk*'`
rm -rf `find . -name 'j'`
rm -rf `find . -name typescript`

pushd lib
make clean
popd

pushd nx
make clean
popd

pushd plugs
make clean
popd

pushd test
make clean
popd

pushd main
make clean
cd etc
make clean
popd

for f in logs dist build attic;do
	find . -type d | fgrep -w $f | xargs rm -rf
done

rm -rf `find . | grep '\.o$'`
rm -rf `find . | grep '\.o\.d$'`
rm -f main/*.log
