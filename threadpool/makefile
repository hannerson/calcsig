###########
###20190113
###########
include ../common.mk

CXX=g++
CXXFLAGS=-g -Wall -c
LDFLAGS=-L/usr/local/lib -L/usr/lib64/mysql/ -L/usr/local/lib64/
LIBS=-lpthread -lcrypto
INCLUDES=-I/usr/include/mysql/ -I/usr/local/include -I../json/include
STATICS=../jsoncpp/lib/libjsoncpp.a ../glog/lib/libglog.a

srcs:=$(wildcard *.cpp *.hpp)
objs:=$(patsubst %.cpp,%.o,$(patsubst %.hpp,%.o,$(srcs)))
libs:=$(patsubst %.cpp,lib%.a,$(patsubst %.hpp,lib%.a,$(srcs)))

target:=test

$(target):$(objs) $(libs)
	$(CXX) -o $@ $< $(STATICS) $(INCLUDES) $(LDFLAGS) $(LIBS)

$(libs):$(objs)
	echo $(libs)
	#rm -f $@ || echo
	ar crv $@ $(objs)
	ranlib $@

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(STATICS) $< -o $@
%.o:%.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(STATICS) $< -o $@

clean:
	rm -rf $(target) *.o *.a
