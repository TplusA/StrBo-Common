#! /bin/sh

RET=0

for t in $(find . -maxdepth 1 -executable -name 'test_*')
do
    $t --reporters=strboxml --out=${t}.junit.xml
    test $? -eq 0 || RET=1
done

exit $RET
