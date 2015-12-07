#! /bin/sh
#
# Wrapper for Meson
#
set -eu

test $# -eq 5 || exit 1

CUTTER="$1"
XSLTPROC="$2"
WORK_DIR="$3"
SOURCE_DIR="$4"
TESTSO="$5"

TEST_NAME="$(echo "${TESTSO}" | sed 's/.*lib\(test_.*\)\.so/\1/')"
XML_REPORT="${TEST_NAME}.xml"
JUNIT_REPORT="${TEST_NAME}_junit.xml"

TEMP_DIR="cutter_wrap.${TEST_NAME}"

cd "${WORK_DIR}"

if test -d "${TEMP_DIR}"
then
    rm -f "${TEMP_DIR}/lib${TEST_NAME}.so"
    rmdir "${TEMP_DIR}"
fi
mkdir "${TEMP_DIR}"
ln -s "${TESTSO}" "${TEMP_DIR}"

${CUTTER} --source-directory="${SOURCE_DIR}" --xml-report="${XML_REPORT}" "${TEMP_DIR}"
${XSLTPROC} "${SOURCE_DIR}/cutter2junit.xslt" "${XML_REPORT}" >"${JUNIT_REPORT}"

rm "${TEMP_DIR}/lib${TEST_NAME}.so"
rmdir "${TEMP_DIR}"

exit 0
