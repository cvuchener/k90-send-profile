CXX=g++
CXXFLAGS=-Wall -std=c++11 -g -O0
CXXFLAGS+=$(shell pkg-config jsoncpp --cflags)
LDFLAGS=$(shell pkg-config jsoncpp --libs)
CXXFLAGS+=-I/home/clement/opt/libusb-git/include/libusb-1.0
LDFLAGS+=-L/home/clement/opt/libusb-git/lib -lusb-1.0

TARGET=k90-send-profile
SRC= \
	Profile.cpp \
	KeyUsage.cpp \
	main.cpp

all: $(TARGET)

$(TARGET): $(SRC:.cpp=.o) 
	$(CXX) $^ $(LDFLAGS) -o $@

%.deps: %.cpp
	$(CXX) -M $(CXXFLAGS) $< > $@

-include $(SRC:.cpp=.deps)

%.o: %.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@

clean:
	rm -f $(SRC:.cpp=.o) $(SRC:.cpp=.deps)

