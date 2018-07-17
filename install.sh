#!/bin/bash
#
# Download pcre and build hyperscan with pcre

readonly PCRE_TARGET="pcre-8.41"
readonly PCRE_ZIP_NAME="${PCRE_TARGET}.tar.gz"
readonly PCRE_DOWNLOAD_LOCATION="https://ftp.pcre.org/pub/pcre/${PCRE_ZIP_NAME}"

cp -r hyperscan library_source

if ! [ -f ${PCRE_ZIP_NAME} ]; then
    curl -o ${PCRE_ZIP_NAME} "${PCRE_DOWNLOAD_LOCATION}"
fi

tar -zxvf ${PCRE_ZIP_NAME} -C library_source

mkdir -p ./library_build
cd ./library_build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel ../library_source
cmake --build .
cd ..
