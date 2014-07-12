TARGET = server
LIBS = -lpthread -lboost_system
CXX = g++
CFLAGS = -std=c++11 -Wall -Wextra -g

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.h)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(SUNDOWN)
	$(CXX) $(OBJECTS) -Wall $(LIBS) -o $@ 

clean:
	-rm *.o
	-rm ${TARGET}
