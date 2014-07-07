TARGET = client
LIBS = -lpthread -lboost_system
CXX = g++
CFLAGS = -std=c++11 -Wall -Wextra -g

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.h)

%.o: %.cpp $(HEADERS)
	mkdir -p bin/ && $(CXX) $(CFLAGS) -c $< -o bin/$@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(SUNDOWN)
	cd bin/ && $(CXX) $(OBJECTS) -Wall $(LIBS) -o $@ && cd ..

clean:
	-rm -rf bin
	
