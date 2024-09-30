# 定义编译器和编译选项
CXX = g++
CXXFLAGS = -std=c++11 -I./generated -I./handler -I./proto

# 依赖库
LIBS = -lhv -lprotobuf -lpthread

# 目标文件
OBJS = protorpc.o protorpc_client.o

# 生成的可执行文件
TARGET = protorpc_client

# 生成的文件列表
GENERATED = $(wildcard generated/*.pb.cc)

# 生成文件规则
generated/%.pb.cc: proto/%.proto
	mkdir -p generated
	/usr/bin/protoc --cpp_out=generated $<

# 编译目标
$(TARGET): $(OBJS) $(GENERATED)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 编译源文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS) generated/*.pb.cc generated/*.pb.h
