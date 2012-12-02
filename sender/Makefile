CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=sender.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=sender

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
clean: 
	rm -rf *.o sender *~



