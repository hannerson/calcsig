###########
###20190113
###########

include ../common.mk

CXX=g++
CXXFLAGS=-g -Wall -c
LDFLAGS=-L/usr/local/lib -L/usr/lib64/mysql/ -L/usr/local/lib64/
LIBS=-lpthread -lcrypto
INCLUDES=-I/usr/include/mysql/ -I/usr/local/include -I../glog/include -I../gflags/include
STATICS=../glog/lib/libglog.a ../gflags/lib/libgflags.a

srcs:=$(wildcard *.cpp)
objs:=$(patsubst %.cpp,%.o,$(srcs))
libs:=$(patsubst %.cpp,lib%.a,$(srcs))

target:=test

$(target):$(objs) $(libs)
	$(CXX) -o $@ $^ $(STATICS) $(INCLUDES) $(LDFLAGS) $(LIBS)

$(libs):$(objs)
	echo $(libs)
	#rm -f $@ || echo
	ar crv $@ $(objs)
	ranlib $@

$(objs):%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(STATICS) $< -o $@

clean:
	rm -rf $(target) *.o *.a
