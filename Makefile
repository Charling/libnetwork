CXX = g++ 
CXXFLAGS = -c -Wall -I/usr/include/ -I./ -I./include -I../tinyxml -I../network/include -I../base/include -c -g -fPIC -g -O2 -DPOLLER_KQUEUE -std=c++11 
LDFLAGS = -L/usr/lib64/ -L/usr/lib/ -L. -g -shared -o -lstdc++ -L`pwd` 
SRCS = $(wildcard ./src/*.cpp)

OBJS = $(SRCS:.cpp=.o) 
PROG = libnetwork.a

all: $(PROG) $(MODULE)

$(PROG): $(OBJS) 
	$(CXX) $(OBJS) $(LDFLAGS) -o $(PROG) 

.cpp.o:
	$(CXX) -Wno-deprecated $(CXXFLAGS) $< -o $@

.PHONY :  install
install :
	-cp libnetwork.a /usr/local/lib

.PHONY :  clean
clean :
	rm -rf $(OBJS) $(PROG) $(PROG).core
.PHONY : clean
clean :


