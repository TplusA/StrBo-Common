#! /bin/sh

for t in $(find . -maxdepth 1 -executable -name 'test_*')
do
    $t --reporters=xml 2>${t}.junit.xml
done
