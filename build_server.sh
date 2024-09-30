#!/bin/bash

# 定义变量
PROTOC=protoc
CXX=g++
CXXFLAGS="-I./generated -I./handler -I./proto -I/usr/local/include -L/usr/local/lib -lhv -lprotobuf -lgpr -lpthread"

# 生成 protobuf 文件
$PROTOC --cpp_out=generated proto/*.proto

# 编译源文件
$CXX -c protorpc.c $CXXFLAGS -o protorpc.o
$CXX -c protorpc_server.cpp $CXXFLAGS -o protorpc_server.o

# 链接生成可执行文件
$CXX -o protorpc protorpc.o protorpc_server.o generated/*.pb.cc $CXXFLAGS

echo "编译完成"
