CXX = g++
CXXFLAGS = -std=c++11 -I./generated -I./handler -I./proto

LIBS = -lhv -lprotobuf -lpthread

OBJS = protorpc.o protorpc_client.o

TARGET = protorpc_client

GENERATED = $(wildcard generated/*.pb.cc)

# # # 生成文件规则
# generated/%.pb.cc: proto/%.proto
# 	mkdir -p generated
# 	/usr/bin/protoc --cpp_out=generated $<


$(TARGET): $(OBJS) $(GENERATED)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS) generated/*.pb.cc generated/*.pb.h
