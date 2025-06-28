#!/bin/bash

cd `dirname $0`

# PROTOC=`which protoc`
PROTOC=/usr/bin/protoc
if [ $? -ne 0 ]; then
    echo "Not found command protoc!"
    echo "Please install libprotobuf first!"
    exit 1
fi

CPP_OUT_DIR=../src/generated
if [ ! -d "${CPP_OUT_DIR}" ]; then
    mkdir -p ${CPP_OUT_DIR}
fi

set -x
${PROTOC} --cpp_out=${CPP_OUT_DIR} *.proto
