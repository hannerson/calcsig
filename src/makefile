###########
###20190113
###########

CXX=/usr/local/gcc-4.8/bin/g++
CXXFLAGS=-g -Wall -c -std=c++11
LDFLAGS=-L/usr/local/lib -L/usr/lib64/mysql/ -L/usr/local/lib64/
LIBS=-lpthread -lglog -lcrypto -levent -ljsoncpp -lglog -lmysqlclient
INCLUDES=-I/usr/include/mysql/ -I/usr/local/include -I../ -I../hiredis-vip-master
STATICS=/usr/local/lib64/libjsoncpp.a ../mysqlpool/libmysqlconn.a ../threadpool/libthreadpool.a ../hiredis-vip-master/libhiredis_vip.a

srcs=$(wildcard *.cpp)
objs=$(patsubst %.cpp,%.o,$(srcs))

target=test

$(target):$(objs)
	$(CXX) -o $@ $^ $(STATICS) $(INCLUDES) $(LDFLAGS) $(LIBS)
$(objs):%.o:%.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(INCLUDES) $(STATICS)

clean:
	rm -rf $(target) *.o
