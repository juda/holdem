TARGET = client
LIBS = -lpthread -lboost_system
CXX = g++
CFLAGS = -std=c++11 -Wall -Wextra -g

HEADERS = client.h common.h IO.h player.h pot.h query.h
OBJECTS = common.o client.o example.o main.o

.PHONY: all clean
.PRECIOUS: $(TARGET) $(OBJECTS)

all: ${TARGET}

${TARGET}: ${OBJECTS}

client.o: ${HEADERS} client.cpp
	${CXX} ${CFLAGS} -c -o client.o client.cpp

example.o: ${HEADERS} example.cpp
	${CXX} ${CFLAGS} -c -o example.o example.cpp

main.o: client.h main.cpp
	${CXX} ${CFLAGS} -c -o main.o main.cpp

common.o: common.h common.cpp
	${CXX} ${CFLAGS} -c -o common.o common.cpp

${TARGET}: ${OBJECTS}
	${CXX} ${OBJECTS} -Wall ${LIBS} -o ${TARGET} 

clean:
	-rm *.o
	-rm ${TARGET}
