CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=receiver.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=receiver

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
clean: 
	rm -rf *.o receiver *~



