# Makefile for disk scheduler project

CXX = g++
CXXFLAGS = -Wall -std=c++11 -g -ldl
ifeq ($(shell uname), Darwin)
LIBS = thread-mac.o libinterrupt-mac.a
else
LIBS = thread.o libinterrupt.a
endif

disk: disk.cc
	$(CXX) $(CXXFLAGS) -o $@ disk.cc $(LIBS)

clean:
	rm -rf disk disk.dSYM

