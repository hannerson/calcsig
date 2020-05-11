###########
###20190113
###########

include ../common.mk

CXX=g++
CXXFLAGS=-g -Wall -c -std=c++11
LDFLAGS=-L/usr/local/lib -L/usr/lib64/mysql/ -L/usr/local/lib64/
LIBS=-lpthread -lcrypto -lz -lrt -lcrypto -lssl
INCLUDES=-I/usr/local/include -I../ -I../libevent/include -I../curl/include -I../glog/include -I../gflags/include -I../jsoncpp/include
STATICS=../glog/lib/libglog.a ../gflags/lib/libgflags.a ../curl/lib/libcurl.a ../jsoncpp/lib/libjsoncpp.a ../threadpool/libthreadpool_98.a ../utils/libconfig.a ../utils/libutils.a ../libevent/libevent.a ../libevent/libevent_core.a ../libevent/libevent_extra.a ../libevent/libevent_openssl.a ../libevent/libevent_pthreads.a

srcs=$(wildcard *.cpp)
objs=$(patsubst %.cpp,%.o,$(srcs))

target=server

$(target):$(objs)
	$(CXX) -o $@ $^ $(STATICS) $(INCLUDES) $(LDFLAGS) $(LIBS)
$(objs):%.o:%.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(INCLUDES) $(STATICS)

clean:
	rm -rf $(target) *.o
