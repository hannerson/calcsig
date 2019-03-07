###########
###20190113
###########

CXX=/usr/local/gcc-4.8/bin/g++
CXXFLAGS=-g -Wall -c -std=c++11
LDFLAGS=-L/usr/local/lib -L/usr/lib64/mysql/ -L/usr/local/lib64/
LIBS=-lpthread -lglog -lcrypto -levent -ljsoncpp -lglog -lmysqlclient
INCLUDES=-I/usr/include/mysql/ -I/usr/local/include -I../ -I../hiredis-vip-master
STATICS=/usr/local/lib64/libjsoncpp.a ../hiredis-vip-master/libhiredis_vip.a

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
