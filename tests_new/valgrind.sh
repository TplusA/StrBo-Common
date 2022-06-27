#! /bin/sh

if test "x$VALGRIND" = x
then
    exit 0
fi

RET=0

for supp in $(find ${srcdir} -name '*.suppressions')
do
    VALGRIND_OPTIONS="${VALGRIND_OPTIONS} --suppressions=${supp}"
done

for t in $(find . -maxdepth 1 -executable -name 'test_*')
do
    echo "Valgrind $t"
    G_DEBUG='gc-friendly,fatal-warnings,fatal-criticals' \
    G_SLICE='always-malloc' \
    ${VALGRIND} \
        --xml=yes --xml-file="${t}.valgrind.xml" \
        --leak-check=full --leak-resolution=high --show-reachable=yes \
        --error-limit=no --num-callers=25 \
        ${VALGRIND_OPTIONS} \
        $t
    test $? -eq 0 || RET=1
done

exit $RET
