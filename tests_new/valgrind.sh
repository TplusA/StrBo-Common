#! /bin/sh

if test "x$VALGRIND" = x
then
    exit 0
fi

for t in $(find . -maxdepth 1 -executable -name 'test_*')
do
    echo "Valgrind $t"
    $VALGRIND \
        --xml=yes --xml-file=${t}.valgrind.xml \
        --leak-check=full --show-reachable=yes --error-limit=no \
        $t
done
