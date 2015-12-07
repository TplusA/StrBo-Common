#! /bin/sh

if test "x$VALGRIND" = x
then
    exit 0
fi

RET=0

for t in $(find . -maxdepth 1 -executable -name 'test_*')
do
    echo "Valgrind $t"
    ${VALGRIND} \
        --xml=yes --xml-file="${t}.valgrind.xml" \
        --leak-check=full --show-reachable=yes --error-limit=no \
        --num-callers=25 \
        ${VALGRIND_OPTIONS} \
        $t
    test $? -eq 0 || RET=1
done

exit $RET
