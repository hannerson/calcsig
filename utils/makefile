###########
###20190113
###########

CXX=/usr/local/gcc-4.8/bin/g++
CXXFLAGS=-g -Wall -c -std=c++11
LDFLAGS=-L/usr/local/lib -L/usr/lib64/mysql/ -L/usr/local/lib64/
LIBS=-lpthread -lglog -lcrypto -levent -ljsoncpp -lglog -lmysqlclient
INCLUDES=-I/usr/include/mysql/ -I/usr/local/include -I../
STATICS=/usr/local/lib64/libjsoncpp.a

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
